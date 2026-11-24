#ifndef MODEL_TRANSFORM_H_
#define MODEL_TRANSFORM_H_
#include <model/mat4.h>
#include <model/vec3.h>

namespace ec::model {
struct Transform {
  MyMat4 model_matrix() const {
    return MyMat4::Translation(offset.x, offset.y, offset.z) *
           MyMat4::RotationX(pitch) * MyMat4::RotationY(yaw) *
           MyMat4::RotationZ(roll) * MyMat4::ScaleUniform(scale) *
           MyMat4::Translation(-pivot.x, -pivot.y, -pivot.z);
  }
  Vec3D pivot = {};   // local-space bbox center, set on model load
  Vec3D offset = {};  // WORLD position of the pivot
  float pitch = 0, yaw = 0, roll = 0, scale = 1;
};

}  // namespace ec::model
#endif
