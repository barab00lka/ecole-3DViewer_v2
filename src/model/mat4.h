#ifndef MODEL_MAT4_H_
#define MODEL_MAT4_H_
#include <containers/my_matrix/my_matrix_oop.h>
#include <model/vec3.h>

#include <cassert>
#include <cmath>
namespace ec::model {
using BaseMatrix = ec::Matrix;

struct MyMat4 : public BaseMatrix {
  MyMat4() : BaseMatrix(4, 4) {
    // 1 0 0 0
    // 0 1 0 0
    // 0 0 1 0
    // 0 0 0 1
    // 'identity matrix' by default
    (*this)(0, 0) = (*this)(1, 1) = (*this)(2, 2) = (*this)(3, 3) = 1.0f;
  }
  explicit MyMat4(const BaseMatrix& o) : BaseMatrix(o) {
    assert(4 == o.get_cols());
    assert(4 == o.get_rows());
  }
  // MyMat4(const MyMat4& o) = default;
  // ~MyMat4() noexcept = default;    // destructor

  MyMat4 operator*(const MyMat4& o) const {
    return static_cast<MyMat4>((BaseMatrix::operator*(o)));
  }

  MyMat4& operator*=(const MyMat4& o) {
    BaseMatrix::operator*=(o);
    return *this;
  }
  static MyMat4 Translation(float x, float y, float z) noexcept {
    MyMat4 t;
    t(0, 3) = x;
    t(1, 3) = y;
    t(2, 3) = z;
    return t;
  }
  static MyMat4 Translation(const Vec3D& v) noexcept {
    return Translation(v.x, v.y, v.z);
  }
  static MyMat4 ScaleUniform(float s) noexcept {
    MyMat4 scale;
    scale(0, 0) = s;
    scale(1, 1) = s;
    scale(2, 2) = s;
    return scale;
  }
  static MyMat4 Scale(float sx, float sy, float sz) noexcept {
    MyMat4 scale;
    scale(0, 0) = sx;
    scale(1, 1) = sy;
    scale(2, 2) = sz;
    return scale;
  }
  static MyMat4 RotationX(float rad) noexcept {
    MyMat4 rotx;
    rotx(1, 1) = cos(rad);
    rotx(1, 2) = -sin(rad);
    rotx(2, 1) = sin(rad);
    rotx(2, 2) = cos(rad);

    return rotx;
  }
  static MyMat4 RotationY(float rad) noexcept {
    MyMat4 roty;
    roty(0, 0) = cos(rad);
    roty(0, 2) = sin(rad);
    roty(2, 0) = -sin(rad);
    roty(2, 2) = cos(rad);

    return roty;
  }
  static MyMat4 RotationZ(float rad) noexcept {
    MyMat4 rotz;
    rotz(0, 0) = cos(rad);
    rotz(0, 1) = -sin(rad);
    rotz(1, 0) = sin(rad);
    rotz(1, 1) = cos(rad);
    return rotz;
  }
  static MyMat4 RotationEuler(float rx, float ry, float rz) noexcept;

  static MyMat4 Perspective(float fovY, float aspect, float n,
                            float f) noexcept {
    MyMat4 m;
    float t = std::tan(fovY * 0.5f);

    m(0, 0) = 1.0f / (aspect * t);
    m(1, 1) = 1.0f / t;
    m(2, 2) = (f + n) / (n - f);
    m(2, 3) = (2.0f * f * n) / (n - f);
    m(3, 2) = -1.0f;
    m(3, 3) = 0.0f;

    return m;
  }
  static MyMat4 Ortho(float l, float r, float b, float t, float n,
                      float f) noexcept {
    MyMat4 m;

    m(0, 0) = 2.0f / (r - l);
    m(1, 1) = 2.0f / (t - b);
    m(2, 2) = -2.0f / (f - n);

    m(0, 3) = -(r + l) / (r - l);
    m(1, 3) = -(t + b) / (t - b);
    m(2, 3) = -(f + n) / (f - n);

    return m;
  }

  using BaseMatrix::data;
};
}  // namespace ec::model
#endif
