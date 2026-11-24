#ifndef MODEL_CAMERA_H_
#define MODEL_CAMERA_H_

#include <model/mat4.h>
#include <model/vec3.h>

namespace ec::model {

struct Camera {
  static constexpr float kDefaultDistanceFactor = 2.5f;

  static constexpr Vec3D kDefaultTarget = {0, -5, -5};

  MyMat4 view_matrix() const;
  MyMat4 projection_matrix() const;
  void fit_to_scene(const Vec3D& center, float radius);

  // Returns camera position in world space.
  Vec3D cam_pos() const {
    return {target_.x, target_.y, target_.z + distance_};
  }

  Vec3D target_ = kDefaultTarget;
  float distance_ = 0.0f;
  float scene_radius_ = 1.0f;
  float view_w_ = 1.0f;
  float view_h_ = 1.0f;
  bool perspective_ = true;
};

}  // namespace ec::model
#endif
