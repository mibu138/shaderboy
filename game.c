#include "game.h"
#include "render.h"
#include "common.h"
#include "tanto/m_math.h"
#include "tanto/t_utils.h"
#include <assert.h>
#include <string.h>
#include <tanto/t_def.h>
#include <tanto/i_input.h>


static Vec2  mousePos;
Parms parms; 
struct ShaderParms* pShaderParms;

static float t;

void g_Init(void)
{
    parms.shouldRun = true;
    pShaderParms = r_GetParms();
    t = 0.0;
}

void g_Responder(const Tanto_I_Event *event)
{
    switch (event->type) 
    {
        case TANTO_I_KEYDOWN: switch (event->data.keyCode)
        {
            case TANTO_KEY_ESC: parms.shouldRun = false; break;
            default: return;
        } break;
        case TANTO_I_KEYUP:   switch (event->data.keyCode)
        {
            default: return;
        } break;
        case TANTO_I_MOTION: 
        {
            mousePos.x = (float)event->data.mouseData.x / TANTO_WINDOW_WIDTH;
            mousePos.y = (float)event->data.mouseData.y / TANTO_WINDOW_HEIGHT;
        } break;
        case TANTO_I_MOUSEDOWN: 
        {
        } break;
        case TANTO_I_MOUSEUP:
        {
        } break;
        default: break;
    }
}

void g_Update(void)
{
    t += 0.016;
    pShaderParms->time = t;
}

