#ifndef MODEL_VEC3D_H_
#define MODEL_VEC3D_H_
#include <cmath>
namespace ec::model {

struct Vec3D {
  Vec3D operator+(const Vec3D& o) const { return {x + o.x, y + o.y, z + o.z}; }
  Vec3D operator-(const Vec3D& o) const { return {x - o.x, y - o.y, z - o.z}; }
  Vec3D operator*(float s) const { return {x * s, y * s, z * s}; }
  Vec3D& operator+=(const Vec3D& o) {
    x += o.x;
    y += o.y;
    z += o.z;
    return *this;
  }
  Vec3D& operator-=(const Vec3D& o) {
    x -= o.x;
    y -= o.y;
    z -= o.z;
    return *this;
  }
  Vec3D& operator*=(const Vec3D& o) {
    x *= o.x;
    y *= o.y;
    z *= o.z;
    return *this;
  }
  bool operator==(const Vec3D& o) const {
    return x == o.x && y == o.y && z == o.z;
  }

  float dot(const Vec3D& o) const { return x * o.x + y * o.y + z * o.z; }
  Vec3D cross(const Vec3D& o) const {
    return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
  }
  float length() const { return std::sqrt(x * x + y * y + z * z); }

  float x = 0.0f, y = 0.0f, z = 0.0f;
};
}  // namespace ec::model
#endif
