#include <model/camera.h>
#include <model/mat4.h>
#include <model/vec3.h>
namespace ec::model {
MyMat4 Camera::projection_matrix() const {
  const float aspect = view_h_ > 1e-6f ? view_w_ / view_h_ : 1.0f;
  if (perspective_) {
    const float fovY = 45.0f * M_PI / 180.0f;
    return MyMat4::Perspective(fovY, aspect, 0.1f, 10.0f * scene_radius_);
  } else {
    const float half = 1.5f * scene_radius_;
    return MyMat4::Ortho(-half * aspect, half * aspect, -half, half, 0.1f,
                         10.0f * scene_radius_);
  }
}

MyMat4 Camera::view_matrix() const {
  return MyMat4::Translation({-target_.x, -target_.y, -target_.z - distance_});
}
void Camera::fit_to_scene(const Vec3D& center, float radius) {
  target_ = center;
  scene_radius_ = (radius > 1e-6f) ? radius : 1.0f;
  distance_ = kDefaultDistanceFactor * scene_radius_;
}
}  // namespace ec::model
