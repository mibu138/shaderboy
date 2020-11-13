#include "shaderboy.h"
#include "game.h"
#include "common.h"
#include "render.h"
#include "tanto/r_geo.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <tanto/v_video.h>
#include <tanto/d_display.h>
#include <tanto/r_render.h>
#include <tanto/r_raytrace.h>
#include <tanto/m_math.h>
#include <tanto/t_utils.h>
#include <tanto/i_input.h>

#define NS_TARGET 16666666 // 1 / 60 seconds
#define NS_PER_S  1000000000

void shaderboy_Init(const char* shaderName)
{
    tanto_v_config.rayTraceEnabled = false;
#ifndef NDEBUG
    tanto_v_config.validationEnabled = true;
#else
    tanto_v_config.validationEnabled = false;
#endif
    SHADER_NAME = shaderName;
    tanto_d_Init();
    printf("Display initialized\n");
    tanto_v_Init();
    printf("Video initialized\n");
    tanto_v_InitSurfaceXcb(d_XcbWindow.connection, d_XcbWindow.window);
    printf("Swapchain initialized\n");
    tanto_r_Init();
    printf("Renderer initialized\n");
    tanto_i_Init();
    printf("Input initialized\n");
    tanto_i_Subscribe(g_Responder);
    r_InitRenderer();
    g_Init();
}

void shaderboy_StartLoop(void)
{
    Tanto_Timer     timer;
    Tanto_LoopStats stats;

    tanto_TimerInit(&timer);
    tanto_LoopStatsInit(&stats);

    parms.shouldRun = true;
    parms.renderNeedsUpdate = false;
    bool presentationSuccess = true;

    for (int i = 0; i < TANTO_FRAME_COUNT; i++) 
    {
        r_UpdateRenderCommands(i);
    }

    while( parms.shouldRun ) 
    {
        tanto_TimerStart(&timer);

        tanto_i_GetEvents();
        tanto_i_ProcessEvents();

        g_Update();

        if (parms.renderNeedsUpdate)
        {
            tanto_r_WaitOnQueueSubmit();
            for (int8_t i = 0; i < TANTO_FRAME_COUNT; i++) 
            {
                r_UpdateRenderCommands(i);
            }
            parms.renderNeedsUpdate = false;
        }
        else
        {
            int8_t frameIndex = tanto_r_RequestFrame();
            if (frameIndex >= 0) // success
                presentationSuccess = tanto_r_PresentFrame();
            else
            {
                presentationSuccess = false;
                printf("Failed to retrieve frame. Likely window resized\n");
            }
        }

        if (!presentationSuccess)
            r_RecreateSwapchain();

        tanto_TimerStop(&timer);

        tanto_LoopStatsUpdate(&timer, &stats);

        printf("Delta ns: %ld\n", stats.nsDelta);

        tanto_LoopSleep(&stats, NS_TARGET);
    }
}
