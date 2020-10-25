#include "shaderboy.h"

int main(int argc, char *argv[])
{
    assert(argc == 2);
    const char* shaderName = argv[1];
    shaderboy_Init(shaderName);
    shaderboy_StartLoop();
    return 0;
}
