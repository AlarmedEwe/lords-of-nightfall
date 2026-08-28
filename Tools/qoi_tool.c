#define QOI_IMPLEMENTATION
#include "../Game/ThirdParty/qoi/qoi.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../Game/ThirdParty/raylib/src/external/stb_image.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: %s <input_png> <output_lontex>\n", argv[0]);
        return 1;
    }
    int w = 0, h = 0, channels = 0;
    unsigned char* pixels = stbi_load(argv[1], &w, &h, &channels, 4);
    if (!pixels) {
        fprintf(stderr, "Erro ao carregar imagem: %s\n", argv[1]);
        return 1;
    }
    qoi_desc desc = {
        .width = (unsigned int)w,
        .height = (unsigned int)h,
        .channels = 4,
        .colorspace = QOI_SRGB
    };
    int res = qoi_write(argv[2], pixels, &desc);
    stbi_image_free(pixels);
    if (!res) {
        fprintf(stderr, "Erro ao escrever QOI: %s\n", argv[2]);
        return 1;
    }
    return 0;
}
