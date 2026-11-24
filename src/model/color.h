#ifndef MODEL_COLOR_H_
#define MODEL_COLOR_H_
#include <string>
namespace ec::model {
struct ColorRGBA {
  ColorRGBA() = default;
  ColorRGBA(float r_, float g_, float b_, float a_ = 1.0f)
      : r(r_), g(g_), b(b_), a(a_) {}
  explicit ColorRGBA(const std::string& hex);

  std::string hexstring() const;
  ColorRGBA negative() const { return {1.0f - r, 1.0f - g, 1.0f - b, a}; }
  bool operator==(const ColorRGBA& o) const {
    return r == o.r && g == o.g && b == o.b && a == o.a;
  }

  float r = .0f, g = .0f, b = .0f, a = 1.0f;
};

}  // namespace ec::model
#endif
