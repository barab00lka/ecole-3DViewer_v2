#ifndef MODEL_SCENE_H_
#define MODEL_SCENE_H_

#include <model/appearance.h>
#include <model/camera.h>
#include <model/color.h>
#include <model/transform.h>

namespace ec::model {

struct SceneSettings {
  Transform& transform() {
    scene_changed_ = true;
    return transform_;
  }
  Camera& camera() {
    scene_changed_ = true;
    return camera_;
  }
  Appearance& style() { return style_; }
  const MyMat4& mvp() {
    if (scene_changed_) {
      // Multiplication order is important! ( P * V * M )
      curMVP_ = camera_.projection_matrix() * camera_.view_matrix() *
                transform_.model_matrix(camera_.get_target());
      scene_changed_ = false;
    }
    return curMVP_;
  }
  bool operator==(const SceneSettings& o) const {
    return style_.style_lines == o.style_.style_lines &&
           style_.style_vt == o.style_.style_vt &&
           style_.col_bg == o.style_.col_bg &&
           style_.col_edge == o.style_.col_edge &&
           style_.col_vt == o.style_.col_vt &&
           style_.vertex_size == o.style_.vertex_size &&
           style_.line_size == o.style_.line_size;
  }

 private:
  Transform transform_;
  Camera camera_;
  Appearance style_;

  MyMat4 curMVP_{};
  bool scene_changed_ = true;
};
}  // namespace ec::model
#endif
