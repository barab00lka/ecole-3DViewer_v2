#ifndef MODEL_CAMERA_H_
#define MODEL_CAMERA_H_

#include <model/mat4.h>
#include <model/vec3.h>

namespace ec::model {

class Camera {
 public:
  MyMat4 view_matrix() const;
  MyMat4 projection_matrix() const;

  void set_viewport(int w, int h) {
    view_w_ = static_cast<float>(w);
    view_h_ = static_cast<float>(h);
  }
  void set_target(const Vec3D& t) { target_ = t; }
  void set_distance(float d) { distance_ = d; }
  void set_perspective(bool p) { perspective_ = p; }
  void set_scene_radius(float r) { scene_radius_ = r; }

  bool get_perspective() const { return perspective_; }
  float get_distance() const { return distance_; }
  const Vec3D& get_target() const { return target_; }
  const float& get_scene_radius() const { return scene_radius_; }
  // Fit camera to view a sphere (center, radius).
  // Resets target and distance to sensible defaults.
  void fit_to_scene(const Vec3D& center, float radius);

 private:
  Vec3D target_ = {0, 0, 0};
  float distance_ = 5.0f;
  float scene_radius_ = 1.0f;
  float view_w_ = 600.0f;
  float view_h_ = 600.0f;
  bool perspective_ = true;
};

}  // namespace ec::model
#endif
