#include <gui/gif_writer.h>
#include <thirdparty/cgif/inc/cgif.h>

#include <iostream>
void save_gif(int w, int h, const std::string& path,
              const ec::List<ec::Vector<uint8_t>>& frames,
              uint16_t interval_cs) {
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

  for (const auto& f : frames) {
    CGIFrgb_FrameConfig fconfig = {};
    fconfig.fmtChan = CGIF_CHAN_FMT_RGB;
    fconfig.delay = interval_cs;  // interval in centiseconds (0.01s)
    fconfig.attrFlags |= CGIF_ATTR_IS_ANIMATED;
    fconfig.pImageData = f.data();

    cgif_rgb_addframe(pGIF, &fconfig);
  }
  cgif_rgb_close(pGIF);
}
