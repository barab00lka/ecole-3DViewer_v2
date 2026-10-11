#include <model/color.h>

#include <algorithm>
#include <cstdio>

using namespace ec::model;

ColorRGBA::ColorRGBA(const std::string& hex) {
  unsigned o_r = 0, o_g = 0, o_b = 0, o_a = 255;
  int parsed =
      std::sscanf(hex.c_str(), "#%02x%02x%02x%02x", &o_r, &o_g, &o_b, &o_a);
  if (parsed >= 3) {
    r = static_cast<float>(o_r) / 255.0f, g = static_cast<float>(o_g) / 255.0f,
    b = static_cast<float>(o_b) / 255.0f,
    a = (parsed == 4) ? static_cast<float>(o_a) / 255.0f : 1.0f;
  }
}

std::string ColorRGBA::hexstring() const {
  auto byte = [](float v) {
    v = std::clamp(v, 0.0f, 1.0f);
    return static_cast<unsigned>(v * 255.0f + 0.5f);
  };
  char tmp[10];
  std::snprintf(tmp, sizeof(tmp), "#%02x%02x%02x%02x", byte(r), byte(g),
                byte(b), byte(a));
  return std::string(tmp);
}
