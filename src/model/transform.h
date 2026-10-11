#ifndef MODEL_TRANSFORM_H_
#define MODEL_TRANSFORM_H_
#include <model/mat4.h>
#include <model/vec3.h>

namespace ec::model {
struct Transform {
  MyMat4 model_matrix(const Vec3D& center) const {
    return MyMat4::Translation(offset.x, offset.y, offset.z) *
           MyMat4::RotationX(pitch) * MyMat4::RotationY(yaw) *
           MyMat4::RotationZ(roll) * MyMat4::ScaleUniform(scale) *
           MyMat4::Translation(-center.x, -center.y, -center.z);
  }

  Vec3D offset = {0, 0, 0};
  float pitch = .0f;  // rotate around X_axis
  float yaw = .0f;    // rotate around Y_axis
  float roll = .0f;   // rotate around Z_axis
  float scale = 1.0f;
};

}  // namespace ec::model
#endif
