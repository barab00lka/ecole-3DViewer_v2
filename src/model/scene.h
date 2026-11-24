#ifndef MODEL_SCENE_H_
#define MODEL_SCENE_H_

#include <model/appearance.h>
#include <model/camera.h>
#include <model/color.h>
#include <model/transform.h>

#include <iostream>
namespace ec::model {

struct SceneSettings {
  Camera& camera() {
    scene_changed_ = true;
    return camera_;
  }
  Appearance& style() { return style_; }
  const float* mvp_mat4() {
    // only recalculate MVP if transform or camera matrix
    // were potentially messed with
    if (scene_changed_) {
      // Multiplication order is important! ( P * V * M )
      curModel_ = transform_.model_matrix();
      curNormal_ = static_cast<MyMat4>(curModel_.InverseMatrix().Transposed());
      curMVP_ = camera_.projection_matrix() * camera_.view_matrix() * curModel_;
      scene_changed_ = false;
    }
    return curMVP_.data();
  }
  const float* model_mat4() const { return curModel_.data(); }
  const float* normal_mat4() const { return curNormal_.data(); }

  bool operator==(const SceneSettings& o) const {
    return style_.style_lines == o.style_.style_lines &&
           style_.style_vt == o.style_.style_vt &&
           style_.col_bg == o.style_.col_bg &&
           style_.col_edge == o.style_.col_edge &&
           style_.col_vt == o.style_.col_vt &&
           style_.col_obj == o.style_.col_obj &&
           style_.vertex_size == o.style_.vertex_size &&
           style_.line_size == o.style_.line_size &&
           lightsource_.pos == o.lightsource_.pos;
  }
  // Object affine transformation setters/getters
  void obj_pitch(float radians) {
    transform_.pitch = radians;
    scene_changed_ = true;
  }
  void obj_yaw(float radians) {
    transform_.yaw = radians;
    scene_changed_ = true;
  }
  void obj_roll(float radians) {
    transform_.roll = radians;
    scene_changed_ = true;
  }
  // in radians
  float obj_pitch() const { return transform_.pitch; }
  // in radians
  float obj_yaw() const { return transform_.yaw; }
  // in radians
  float obj_roll() const { return transform_.roll; }
  void obj_offset_x(float v) {
    transform_.offset.x = v;
    scene_changed_ = true;
  }
  void obj_offset_y(float v) {
    transform_.offset.y = v;
    scene_changed_ = true;
  }
  void obj_offset_z(float v) {
    transform_.offset.z = v;
    scene_changed_ = true;
  }
  model::Vec3D obj_offset() const { return transform_.offset; }
  float obj_offset_x() const { return transform_.offset.x; }
  float obj_offset_y() const { return transform_.offset.y; }
  float obj_offset_z() const { return transform_.offset.z; }
  void obj_scale(float v) {
    transform_.scale = v;
    scene_changed_ = true;
  }
  float obj_scale() const { return transform_.scale; }

  // Light source
  Vec3D light_offset() const { return lightsource_.pos; }
  void light_offset(model::Vec3D position) { lightsource_.pos = position; }

  // Camera
  Vec3D cam_target() const { return camera_.target_; }
  Vec3D cam_pos() const { return camera_.cam_pos(); }
  float radius() const { return camera_.scene_radius_; }
  bool is_perspective() const { return camera_.perspective_; }
  void set_perspective(bool v = true) {
    camera_.perspective_ = v;
    scene_changed_ = true;
  }
  void set_viewport(float w, float h) {
    camera_.view_w_ = (w);
    camera_.view_h_ = (h);
    scene_changed_ = true;
  }
  void set_pivot(Vec3D p) {
    transform_.pivot = p;
    scene_changed_ = true;
  }

 private:
  Appearance style_;
  Camera camera_;
  Transform transform_;
  struct {
    Vec3D pos = {};
    float intensity = 0.2;  // [0.0-1.0]
  } lightsource_;

  MyMat4 curMVP_{}, curModel_{}, curNormal_{};
  bool scene_changed_ = true;
};
}  // namespace ec::model
#endif
