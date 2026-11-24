// --capture: the renderer's screen to a PNG (stb_image_write's code is compiled here only)

#include "renderer/device.hpp"
#include "state.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <cstdio>
#include <stb/stb_image_write.h>

bool save_capture(Renderer& renderer, const std::string& path) {
    std::vector<std::uint8_t> rgba;
    if (!renderer_read(renderer, rgba)) {
        return false;
    }
    if (stbi_write_png(path.c_str(), renderer.w, renderer.h, 4, rgba.data(), renderer.w * 4) == 0) {
        std::fprintf(stderr, "blobview: could not write %s\n", path.c_str());
        return false;
    }
    std::printf("blobview: wrote %s (%dx%d)\n", path.c_str(), renderer.w, renderer.h);
    return true;
}
