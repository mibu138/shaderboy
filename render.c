#include "render.h"
#include "tanto/v_image.h"
#include "tanto/v_memory.h"
#include <memory.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <tanto/r_render.h>
#include <tanto/v_video.h>
#include <tanto/t_def.h>
#include <tanto/t_utils.h>
#include <tanto/r_pipeline.h>
#include <tanto/r_raytrace.h>
#include <tanto/v_command.h>
#include <vulkan/vulkan_core.h>

#define SPVDIR "./shaders/spv"

static Tanto_V_Image colorAttachment;
static Tanto_V_Image depthAttachment;

static VkFramebuffer offscreenFramebuffer;
static VkFramebuffer swapchainFramebuffers[TANTO_FRAME_COUNT];

static Tanto_V_BufferRegion shaderParmsBufferRegion;

const char* SHADER_NAME;

typedef enum {
    R_PIPE_MAIN,
    R_PIPE_POST
} R_PipelineId;

typedef enum {
    R_PIPE_LAYOUT_MAIN,
    R_PIPE_LAYOUT_POST
} R_PipelineLayoutId;

typedef enum {
    R_DESC_SET_MAIN,
    R_DESC_SET_POST
} R_DescriptorSetId;

// TODO: we should implement a way to specify the offscreen renderpass format at initialization
static void initOffscreenAttachments(void)
{
    //initDepthAttachment();
    depthAttachment = tanto_v_CreateImage(
            TANTO_WINDOW_WIDTH, TANTO_WINDOW_HEIGHT,
            depthFormat,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            VK_SAMPLE_COUNT_1_BIT);

    colorAttachment = tanto_v_CreateImageAndSampler(
            TANTO_WINDOW_WIDTH, TANTO_WINDOW_HEIGHT, 
            offscreenColorFormat,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | 
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_SAMPLE_COUNT_1_BIT,
            VK_FILTER_NEAREST);

    tanto_v_TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, &colorAttachment);
}

static void initImageDescriptors(void)
{
    VkDescriptorImageInfo imageInfo = {
        .imageLayout = colorAttachment.layout,
        .imageView   = colorAttachment.view,
        .sampler     = colorAttachment.sampler
    };

    VkWriteDescriptorSet writes[] = {{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstArrayElement = 0,
        .dstSet = descriptorSets[R_DESC_SET_POST],
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &imageInfo
    }};

    vkUpdateDescriptorSets(device, TANTO_ARRAY_SIZE(writes), writes, 0, NULL);
}

static void initNonMeshDescriptors(void)
{
    shaderParmsBufferRegion = tanto_v_RequestBufferRegion(sizeof(struct ShaderParms), 
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, TANTO_V_MEMORY_HOST_GRAPHICS_TYPE);
    memset(shaderParmsBufferRegion.hostData, 0, sizeof(Parms));

    VkDescriptorBufferInfo parmsInfo = {
        .buffer = shaderParmsBufferRegion.buffer,
        .offset = shaderParmsBufferRegion.offset,
        .range  = shaderParmsBufferRegion.size
    };

    VkWriteDescriptorSet writes[] = {{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstArrayElement = 0,
        .dstSet = descriptorSets[R_DESC_SET_MAIN],
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &parmsInfo

    }};

    vkUpdateDescriptorSets(device, TANTO_ARRAY_SIZE(writes), writes, 0, NULL);
}

static void initDescriptorSets(void)
{
    const Tanto_R_DescriptorSet descriptorSets[] = {{
        .id = R_DESC_SET_MAIN,
        .bindingCount = 1,
        .bindings = {{
            .descriptorCount = 1,
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        }}
    },{
        .id = R_DESC_SET_POST,
        .bindingCount = 1,
        .bindings = {{
            .descriptorCount = 1,
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        }}
    }};

    tanto_r_InitDescriptorSets(descriptorSets, TANTO_ARRAY_SIZE(descriptorSets));
}

static void initPipelineLayouts(void)
{
    const Tanto_R_PipelineLayout pipelayouts[] = {{
        .id = R_PIPE_LAYOUT_MAIN, 
        .descriptorSetCount = 1, 
        .descriptorSetIds = {R_DESC_SET_MAIN},
        .pushConstantCount = 0,
        .pushConstantsRanges = {}
    },{
        .id = R_PIPE_LAYOUT_POST,
        .descriptorSetCount = 1,
        .descriptorSetIds = {R_DESC_SET_POST},
        .pushConstantCount = 0,
        .pushConstantsRanges = {}
    }};

    tanto_r_InitPipelineLayouts(pipelayouts, TANTO_ARRAY_SIZE(pipelayouts));
}

static void initPipelines(void)
{

    char shaderPath[255];
    sprintf(shaderPath, "%s/%s-frag.spv", SPVDIR, SHADER_NAME); 

    const Tanto_R_PipelineInfo pipeInfos[] = {{
        .id       = R_PIPE_MAIN,
        .type     = TANTO_R_PIPELINE_POSTPROC_TYPE,
        .layoutId = R_PIPE_LAYOUT_MAIN,
        .payload.rasterInfo = {
            .renderPass = offscreenRenderPass,
            .fragShader = shaderPath,
            .sampleCount = VK_SAMPLE_COUNT_1_BIT,
            .vertexDescription = tanto_r_GetVertexDescription3D_Simple()
        }
    },{
        .id       = R_PIPE_POST,
        .type     = TANTO_R_PIPELINE_POSTPROC_TYPE,
        .layoutId = R_PIPE_LAYOUT_POST,
        .payload.rasterInfo = {
            .renderPass = swapchainRenderPass, 
            .fragShader = SPVDIR"/post-frag.spv",
            .sampleCount = VK_SAMPLE_COUNT_1_BIT,
        }
    }};

    tanto_r_InitPipelines(pipeInfos, TANTO_ARRAY_SIZE(pipeInfos));
}

static void initFramebuffers(void)
{
    const VkImageView offscreenAttachments[] = {colorAttachment.view, depthAttachment.view};

    VkFramebufferCreateInfo framebufferInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .layers = 1,
        .height = TANTO_WINDOW_HEIGHT,
        .width  = TANTO_WINDOW_WIDTH,
        .renderPass = offscreenRenderPass,
        .attachmentCount = 2,
        .pAttachments = offscreenAttachments
    };

    V_ASSERT( vkCreateFramebuffer(device, &framebufferInfo, NULL, &offscreenFramebuffer) );

    for (int i = 0; i < TANTO_FRAME_COUNT; i++) 
    {
        framebufferInfo.renderPass = swapchainRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &frames[i].swapImage.view;

        V_ASSERT( vkCreateFramebuffer(device, &framebufferInfo, NULL, &swapchainFramebuffers[i]) );
    }
}

static void mainRender(const VkCommandBuffer* cmdBuf, const VkRenderPassBeginInfo* rpassInfo)
{
    vkCmdBindPipeline(*cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[R_PIPE_MAIN]);

    vkCmdBindDescriptorSets(
        *cmdBuf, 
        VK_PIPELINE_BIND_POINT_GRAPHICS, 
        pipelineLayouts[R_PIPE_LAYOUT_MAIN], 
        0, 1, &descriptorSets[R_DESC_SET_MAIN],
        0, NULL);

    vkCmdBeginRenderPass(*cmdBuf, rpassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdDraw(*cmdBuf, 3, 1, 0, 0);

    vkCmdEndRenderPass(*cmdBuf);
}

static void postProc(const VkCommandBuffer* cmdBuf, const VkRenderPassBeginInfo* rpassInfo)
{
    vkCmdBindPipeline(*cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[R_PIPE_POST]);

    vkCmdBindDescriptorSets(
        *cmdBuf, 
        VK_PIPELINE_BIND_POINT_GRAPHICS, 
        pipelineLayouts[R_PIPE_LAYOUT_POST], 
        0, 1, &descriptorSets[R_DESC_SET_POST],
        0, NULL);

    vkCmdBeginRenderPass(*cmdBuf, rpassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdDraw(*cmdBuf, 3, 1, 0, 0);

    vkCmdEndRenderPass(*cmdBuf);
}

void r_InitRenderer()
{
    assert(SHADER_NAME);

    initDescriptorSets();
    initPipelineLayouts();
    initPipelines();

    initOffscreenAttachments();

    initImageDescriptors();
    initNonMeshDescriptors();

    initFramebuffers();
}

void r_UpdateRenderCommands(void)
{
    Tanto_R_Frame* frame = &frames[curFrameIndex];
    vkResetCommandPool(device, frame->commandPool, 0);
    VkCommandBufferBeginInfo cbbi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    V_ASSERT( vkBeginCommandBuffer(frame->commandBuffer, &cbbi) );

    VkClearValue clearValueColor = {0.002f, 0.023f, 0.009f, 1.0f};
    VkClearValue clearValueDepth = {1.0, 0};

    VkClearValue clears[] = {clearValueColor, clearValueDepth};

    const VkRenderPassBeginInfo rpassOffscreen = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .clearValueCount = 2,
        .pClearValues = clears,
        .renderArea = {{0, 0}, {TANTO_WINDOW_WIDTH, TANTO_WINDOW_HEIGHT}},
        .renderPass =  offscreenRenderPass,
        .framebuffer = offscreenFramebuffer,
    };

    const VkRenderPassBeginInfo rpassSwap = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .clearValueCount = 1,
        .pClearValues = clears,
        .renderArea = {{0, 0}, {TANTO_WINDOW_WIDTH, TANTO_WINDOW_HEIGHT}},
        .renderPass =  swapchainRenderPass,
        .framebuffer = swapchainFramebuffers[curFrameIndex] 
    };

    mainRender(&frame->commandBuffer, &rpassOffscreen);
    postProc(&frame->commandBuffer, &rpassSwap);

    V_ASSERT( vkEndCommandBuffer(frame->commandBuffer) );
}

void r_RecreateSwapchain(void)
{
    vkDeviceWaitIdle(device);

    r_CleanUp();
    tanto_r_CleanUpJustPipelines();

    curFrameIndex = 0;
    frameCounter = 0;
    tanto_v_RecreateSwapchain();
    initOffscreenAttachments();
    initPipelines();
    initImageDescriptors();
    initFramebuffers();

    for (int i = 0; i < TANTO_FRAME_COUNT; i++) 
    {
        tanto_r_RequestFrame();
        r_UpdateRenderCommands();
        tanto_r_PresentFrame();
        vkDeviceWaitIdle(device);
    }
}

struct ShaderParms* r_GetParms(void)
{
    return (struct ShaderParms*)shaderParmsBufferRegion.hostData;
}

void r_CleanUp(void)
{
    vkDestroyFramebuffer(device, offscreenFramebuffer, NULL);
    for (int i = 0; i < TANTO_FRAME_COUNT; i++) 
    {
        vkDestroyFramebuffer(device, swapchainFramebuffers[i], NULL);
    }
    tanto_v_DestroyImage(colorAttachment);
    tanto_v_DestroyImage(depthAttachment);
}
