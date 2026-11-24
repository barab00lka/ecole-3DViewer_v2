#ifndef MODEL_TEXTURE_H_
#define MODEL_TEXTURE_H_

#include <containers/ec_vector.h>
#include <model/color.h>
#include <model/wireframe_mesh.h>

#include <cstdint>
#include <string>

namespace ec::model {
using ec::Vector;

struct Texture {
  static constexpr int kMaxSize = 1024;

  int width = 0;
  int height = 0;
  // RGB pixel data, exactly width * height * 3 bytes, tightly packed.
  Vector<uint8_t> pixels;

  // Loads a .bmp file and converts it to RGB. Throws std::invalid_argument
  // on any failure (unreadable file, unsupported format, or size > kMaxSize).
  static Texture load_from_bmp(const std::string& path);

  // Saves the texture pixels to a BMP file.
  static bool save_to_bmp(const Texture& t, const std::string& path);

  // Draws UV edges of the given mesh onto a copy of the texture pixels,
  // using the specified color. Returns the resulting texture.
  // UV coordinates are mapped to pixel space with (0,0) at the top-left.
  static Texture draw_uv_edges(const Texture& t, const Mesh& m,
                               const ColorRGBA& color);
};

}  // namespace ec::model
#endif
