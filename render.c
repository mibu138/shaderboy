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

#define SPVDIR "/home/michaelb/dev/shaderboy/shaders/spv"

static Tanto_R_FrameBuffer  offscreenFrameBuffer;

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
static void initOffscreenFrameBuffer(void)
{
    //initDepthAttachment();
    offscreenFrameBuffer.depthAttachment = tanto_v_CreateImage(
            TANTO_WINDOW_WIDTH, TANTO_WINDOW_HEIGHT,
            depthFormat,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_DEPTH_BIT);

    offscreenFrameBuffer.colorAttachment = tanto_v_CreateImageAndSampler(
            TANTO_WINDOW_WIDTH, TANTO_WINDOW_HEIGHT, 
            offscreenColorFormat,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | 
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_FILTER_NEAREST);
    //
    // seting render pass and depth attachment
    offscreenFrameBuffer.pRenderPass = &offscreenRenderPass;

    const VkImageView attachments[] = {offscreenFrameBuffer.colorAttachment.view, offscreenFrameBuffer.depthAttachment.view};

    VkFramebufferCreateInfo framebufferInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .layers = 1,
        .height = TANTO_WINDOW_HEIGHT,
        .width  = TANTO_WINDOW_WIDTH,
        .renderPass = *offscreenFrameBuffer.pRenderPass,
        .attachmentCount = 2,
        .pAttachments = attachments
    };

    V_ASSERT( vkCreateFramebuffer(device, &framebufferInfo, NULL, &offscreenFrameBuffer.handle) );

    tanto_v_TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, &offscreenFrameBuffer.colorAttachment);
}

static void initNonMeshDescriptors(void)
{
    shaderParmsBufferRegion = tanto_v_RequestBufferRegion(sizeof(struct ShaderParms), 
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, TANTO_V_MEMORY_HOST_GRAPHICS_TYPE);
    memset(shaderParmsBufferRegion.hostData, 0, sizeof(Parms));

    VkDescriptorImageInfo imageInfo = {
        .imageLayout = offscreenFrameBuffer.colorAttachment.layout,
        .imageView   = offscreenFrameBuffer.colorAttachment.view,
        .sampler     = offscreenFrameBuffer.colorAttachment.sampler
    };

    VkDescriptorBufferInfo parmsInfo = {
        .buffer = shaderParmsBufferRegion.buffer,
        .offset = shaderParmsBufferRegion.offset,
        .range  = shaderParmsBufferRegion.size
    };

    VkWriteDescriptorSet writes[] = {{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstArrayElement = 0,
        .dstSet = descriptorSets[R_DESC_SET_POST],
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &imageInfo
    },{
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

static void InitPipelines(void)
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

    char shaderPath[255];
    sprintf(shaderPath, "%s/%s-frag.spv", SPVDIR, SHADER_NAME); 

    const Tanto_R_PipelineInfo pipeInfos[] = {{
        .id       = R_PIPE_MAIN,
        .type     = TANTO_R_PIPELINE_POSTPROC_TYPE,
        .layoutId = R_PIPE_LAYOUT_MAIN,
        .payload.rasterInfo = {
            .renderPassType = TANTO_R_RENDER_PASS_OFFSCREEN_TYPE, 
            .fragShader = shaderPath
        }
    },{
        .id       = R_PIPE_POST,
        .type     = TANTO_R_PIPELINE_POSTPROC_TYPE,
        .layoutId = R_PIPE_LAYOUT_POST,
        .payload.rasterInfo = {
            .renderPassType = TANTO_R_RENDER_PASS_SWAPCHAIN_TYPE, 
            .fragShader = SPVDIR"/post-frag.spv"
        }
    }};

    tanto_r_InitDescriptorSets(descriptorSets, TANTO_ARRAY_SIZE(descriptorSets));
    tanto_r_InitPipelineLayouts(pipelayouts, TANTO_ARRAY_SIZE(pipelayouts));
    tanto_r_InitPipelines(pipeInfos, TANTO_ARRAY_SIZE(pipeInfos));
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

    InitPipelines();

    initOffscreenFrameBuffer();

    initNonMeshDescriptors();
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
        .renderPass =  *offscreenFrameBuffer.pRenderPass,
        .framebuffer = offscreenFrameBuffer.handle,
    };

    const VkRenderPassBeginInfo rpassSwap = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .clearValueCount = 1,
        .pClearValues = clears,
        .renderArea = {{0, 0}, {TANTO_WINDOW_WIDTH, TANTO_WINDOW_HEIGHT}},
        .renderPass =  *frame->renderPass,
        .framebuffer = frame->frameBuffer 
    };

    mainRender(&frame->commandBuffer, &rpassOffscreen);
    postProc(&frame->commandBuffer, &rpassSwap);

    V_ASSERT( vkEndCommandBuffer(frame->commandBuffer) );
}

struct ShaderParms* r_GetParms(void)
{
    return (struct ShaderParms*)shaderParmsBufferRegion.hostData;
}

void r_CleanUp(void)
{
    vkDestroyFramebuffer(device, offscreenFrameBuffer.handle, NULL);
    tanto_v_DestroyImage(offscreenFrameBuffer.colorAttachment);
    tanto_v_DestroyImage(offscreenFrameBuffer.depthAttachment);
}
