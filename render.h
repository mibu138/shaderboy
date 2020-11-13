#ifndef VIEWER_R_COMMANDS_H
#define VIEWER_R_COMMANDS_H

#include <tanto/r_geo.h>
#include "common.h"

typedef struct {
    Vec4 clearColor;
    Vec3 lightDir;
    float lightIntensity;
    int   lightType;
    uint32_t posOffset;
    uint32_t colorOffset;
    uint32_t normalOffset;
    uint32_t uvwOffset;
} RtPushConstants;

struct ShaderParms {
    float time;
};

extern const char* SHADER_NAME;

void  r_InitRenderer(void);
void  r_UpdateRenderCommands(const int8_t frameIndex);
int   r_GetSelectionPos(Vec3* v);
void  r_LoadMesh(Tanto_R_Mesh mesh);
void  r_ClearMesh(void);
void  r_ClearPaintImage(void);
void  r_SavePaintImage(void);
void  r_CleanUp(void);
void  r_RecreateSwapchain(void);
const Tanto_R_Mesh* r_GetMesh(void);
struct ShaderParms* r_GetParms(void);

#endif /* end of include guard: R_COMMANDS_H */
