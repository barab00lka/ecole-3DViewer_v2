#include <gui/gif_writer.h>
#include <gui/gl_renderer.h>
#include <thirdparty/cgif/inc/cgif.h>

#include <iostream>

using ec::List, ec::Vector;
using ec::view::GLRenderer;

void flip_vertically(Vector<uint8_t>& vec, int width, int height) {
  if (!width || !height) return;
  assert(static_cast<size_t>(width * height *
                             GLRenderer::kFramebufferChannels) <= vec.size());
  for (int y = 0; y < height / 2; ++y) {
    for (int x = 0; x < width; ++x) {
      int top = (x + width * y) * GLRenderer::kFramebufferChannels;
      int bottom =
          (x + width * (height - y - 1)) * GLRenderer::kFramebufferChannels;

      for (int i = 0; i < GLRenderer::kFramebufferChannels; i++) {
        uint8_t temp = vec[top + i];
        vec[top + i] = vec[bottom + i];
        vec[bottom + i] = temp;
      }
    }
  }
}

void save_gif(int w, int h, const std::string& path,
              List<Vector<uint8_t>>& frames, uint16_t interval_cs) {
  CGIFrgb_Config config = {};
  CGIFrgb* pGIF;

  config.path = path.c_str();
  config.width = w;
  config.height = h;
  pGIF = cgif_rgb_newgif(&config);
  if (pGIF == nullptr) {
    std::cerr << "Error: cannot create GIF.\n";
    return;
  }
  cgif_chan_fmt pixel_format = GLRenderer::kFramebufferChannels == 4
                                   ? CGIF_CHAN_FMT_RGBA
                                   : CGIF_CHAN_FMT_RGB;

  for (auto& f : frames) {
    flip_vertically(f, w, h);
    CGIFrgb_FrameConfig fconfig = {};
    fconfig.fmtChan = pixel_format;
    fconfig.delay = interval_cs;  // interval in centiseconds (0.01s)
    fconfig.attrFlags |= CGIF_ATTR_IS_ANIMATED;
    fconfig.pImageData = f.data();

    cgif_rgb_addframe(pGIF, &fconfig);
  }
  cgif_rgb_close(pGIF);
}
