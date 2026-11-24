#include <model/texture.h>

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#include <thirdparty/stb/stb_image.h>

namespace ec::model {

namespace {

void draw_line(Vector<uint8_t>& pixels, int width, int height, int x0, int y0,
               int x1, int y1, uint8_t r, uint8_t g, uint8_t b) {
  const bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
  if (steep) {
    std::swap(x0, y0);
    std::swap(x1, y1);
  }
  if (x0 > x1) {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }

  const int dx = x1 - x0;
  const int dy = std::abs(y1 - y0);
  int error = dx / 2;
  const int ystep = (y0 < y1) ? 1 : -1;
  int y = y0;

  for (int x = x0; x <= x1; ++x) {
    int px = steep ? y : x;
    int py = steep ? x : y;
    if (px >= 0 && px < width && py >= 0 && py < height) {
      const std::size_t idx = (static_cast<std::size_t>(py) * width + px) * 3;
      pixels[idx] = r;
      pixels[idx + 1] = g;
      pixels[idx + 2] = b;
    }
    error -= dy;
    if (error < 0) {
      y += ystep;
      error += dx;
    }
  }
}

bool write_bmp(const std::string& path, const Vector<uint8_t>& pixels,
               int width, int height) {
  const int row_bytes = width * 3;
  const int padded_row = (row_bytes + 3) & ~3;
  const int image_size = padded_row * height;
  const int file_size = 14 + 40 + image_size;

  std::ofstream f(path, std::ios::binary);
  if (!f) return false;

  uint8_t header[14] = {};
  header[0] = 'B';
  header[1] = 'M';
  header[2] = static_cast<uint8_t>(file_size & 0xFF);
  header[3] = static_cast<uint8_t>((file_size >> 8) & 0xFF);
  header[4] = static_cast<uint8_t>((file_size >> 16) & 0xFF);
  header[5] = static_cast<uint8_t>((file_size >> 24) & 0xFF);
  header[10] = 54;
  f.write(reinterpret_cast<const char*>(header), 14);

  uint8_t dib[40] = {};
  dib[0] = 40;
  dib[4] = static_cast<uint8_t>(width & 0xFF);
  dib[5] = static_cast<uint8_t>((width >> 8) & 0xFF);
  dib[6] = static_cast<uint8_t>((width >> 16) & 0xFF);
  dib[7] = static_cast<uint8_t>((width >> 24) & 0xFF);
  dib[8] = static_cast<uint8_t>(height & 0xFF);
  dib[9] = static_cast<uint8_t>((height >> 8) & 0xFF);
  dib[10] = static_cast<uint8_t>((height >> 16) & 0xFF);
  dib[11] = static_cast<uint8_t>((height >> 24) & 0xFF);
  dib[12] = 1;
  dib[14] = 24;
  f.write(reinterpret_cast<const char*>(dib), 40);

  Vector<uint8_t> pad_row(padded_row);
  for (int y = height - 1; y >= 0; --y) {
    const std::size_t row_start = static_cast<std::size_t>(y) * width * 3;
    f.write(reinterpret_cast<const char*>(pixels.data() + row_start),
            row_bytes);
    f.write(reinterpret_cast<const char*>(pad_row.data()),
            padded_row - row_bytes);
  }

  return f.good();
}

}  // namespace

Texture Texture::load_from_bmp(const std::string& path) {
  int width = 0;
  int height = 0;
  int channels_in_file = 0;

  stbi_set_flip_vertically_on_load(true);
  // Force exactly 3 output channels (RGB) so the upload path can assume a
  // fixed stride of 3 bytes per pixel regardless of the source format.
  unsigned char* raw =
      stbi_load(path.c_str(), &width, &height, &channels_in_file, 3);
  if (!raw) {
    throw std::invalid_argument("Failed to load texture: " + path);
  }

  if (width > kMaxSize || height > kMaxSize) {
    stbi_image_free(raw);
    throw std::invalid_argument(
        "Failed to load texture: size must not exceed 1024x1024");
  }
  if (width != height) {
    stbi_image_free(raw);
    throw std::invalid_argument(
        "Failed to load texture: texture must be square (width == height)");
  }

  Texture t;
  t.width = width;
  t.height = height;
  for (std::size_t i = 0; i < static_cast<std::size_t>(width) *
                                  static_cast<std::size_t>(height) * 3;
       i++) {
    t.pixels.push_back(raw[i]);
  }
  stbi_image_free(raw);
  return t;
}

bool Texture::save_to_bmp(const Texture& t, const std::string& path) {
  if (t.width <= 0 || t.height <= 0 || t.pixels.empty()) return false;
  return write_bmp(path, t.pixels, t.width, t.height);
}

Texture Texture::draw_uv_edges(const Texture& t, const Mesh& m,
                               const ColorRGBA& color) {
  Texture out = t;

  const uint8_t r = static_cast<uint8_t>(color.r * 255.0f);
  const uint8_t g = static_cast<uint8_t>(color.g * 255.0f);
  const uint8_t b = static_cast<uint8_t>(color.b * 255.0f);

  const auto width = t.width;
  const auto height = t.height;
  const auto& verts = m.vertices;
  const auto& facets = m.facets;

  for (size_t i = 0; i + 2 < facets.size(); i += 3) {
    const uint32_t a = facets[i];
    const uint32_t b_idx = facets[i + 1];
    const uint32_t c = facets[i + 2];

    const auto& ta = verts[a].TexCoords;
    const auto& tb = verts[b_idx].TexCoords;
    const auto& tc = verts[c].TexCoords;

    const int ax = static_cast<int>(ta.x * (width - 1) + 0.5f);
    const int ay = static_cast<int>((1.0f - ta.y) * (height - 1) + 0.5f);
    const int bx = static_cast<int>(tb.x * (width - 1) + 0.5f);
    const int by = static_cast<int>((1.0f - tb.y) * (height - 1) + 0.5f);
    const int cx = static_cast<int>(tc.x * (width - 1) + 0.5f);
    const int cy = static_cast<int>((1.0f - tc.y) * (height - 1) + 0.5f);

    draw_line(out.pixels, width, height, ax, ay, bx, by, r, g, b);
    draw_line(out.pixels, width, height, bx, by, cx, cy, r, g, b);
    draw_line(out.pixels, width, height, cx, cy, ax, ay, r, g, b);
  }

  return out;
}

}  // namespace ec::model
