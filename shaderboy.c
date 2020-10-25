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

struct Timer {
    struct timespec startTime;
    struct timespec endTime;
    clockid_t clockId;
    void (*startFn)(struct Timer*);
    void (*stopFn)(struct Timer*);
} timer;

static void timerStart(struct Timer* t)
{
    clock_gettime(t->clockId, &t->startTime);
}

static void timerStop(struct Timer* t)
{
    clock_gettime(t->clockId, &t->endTime);
}

struct Stats {
    uint64_t frameCount;
    uint64_t nsTotal;
    unsigned long nsDelta;
    uint32_t shortestFrame;
    uint32_t longestFrame;
} stats;

static void updateStats(const struct Timer* t, struct Stats* s)
{
    s->nsDelta  = (t->endTime.tv_sec * NS_PER_S + t->endTime.tv_nsec) - (t->startTime.tv_sec * NS_PER_S + t->startTime.tv_nsec);
    s->nsTotal += s->nsDelta;

    if (s->nsDelta > s->longestFrame) s->longestFrame = s->nsDelta;
    if (s->nsDelta < s->shortestFrame) s->shortestFrame = s->nsDelta;

    s->frameCount++;
}

static void sleepLoop(const struct Stats* s)
{
    struct timespec diffTime;
    diffTime.tv_nsec = NS_TARGET > s->nsDelta ? NS_TARGET - s->nsDelta : 0;
    diffTime.tv_sec  = 0;
    // we could use the second parameter to handle interrupts and signals
    nanosleep(&diffTime, NULL);
}

static void initTimer(void)
{
    memset(&timer, 0, sizeof(timer));
    timer.clockId = CLOCK_MONOTONIC;
    timer.startFn = timerStart;
    timer.stopFn  = timerStop;
}

static void initStats(void)
{
    memset(&stats, 0, sizeof(stats));
    stats.longestFrame = UINT32_MAX;
}

void shaderboy_Init(const char* shaderName)
{
    tanto_v_config.rayTraceEnabled = true;
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
    tanto_v_InitSwapchain(NULL);
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
    initTimer();
    initStats();

    parms.shouldRun = true;
    parms.renderNeedsUpdate = false;

    while( parms.shouldRun ) 
    {
        timer.startFn(&timer);

        tanto_i_GetEvents();
        tanto_i_ProcessEvents();

        //r_WaitOnQueueSubmit(); // possibly don't need this due to render pass

        g_Update();

        if (parms.renderNeedsUpdate || stats.frameCount == 0 ) 
        {
            for (int i = 0; i < TANTO_FRAME_COUNT; i++) 
            {
                if (parms.renderNeedsUpdate)
                    tanto_r_WaitOnQueueSubmit();
                tanto_r_RequestFrame();
                r_UpdateRenderCommands();
                tanto_r_PresentFrame();
            }
            parms.renderNeedsUpdate = false;
        }
        else
        {
            tanto_r_RequestFrame();
            tanto_r_PresentFrame();
        }

        timer.stopFn(&timer);

        updateStats(&timer, &stats);

        printf("Delta ns: %ld\n", stats.nsDelta);

        sleepLoop(&stats);
    }
}
