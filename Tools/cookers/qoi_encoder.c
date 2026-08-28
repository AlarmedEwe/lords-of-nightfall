#define QOI_IMPLEMENTATION
#include "../../Game/ThirdParty/qoi/qoi.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../../Game/ThirdParty/raylib/src/external/stb_image.h"
#include <stdio.h>
#include <stdlib.h>

int encode_image_to_lontex(const char* input_path, const char* output_path)
{
    int width = 0, height = 0, channels = 0;
    unsigned char* pixels = stbi_load(input_path, &width, &height, &channels, 4);
    if (!pixels)
    {
        return 0;
    }

    qoi_desc desc;
    desc.width = (unsigned int)width;
    desc.height = (unsigned int)height;
    desc.channels = 4;
    desc.colorspace = QOI_SRGB;

    int written = qoi_write(output_path, pixels, &desc);
    stbi_image_free(pixels);

    return written > 0 ? 1 : 0;
}
