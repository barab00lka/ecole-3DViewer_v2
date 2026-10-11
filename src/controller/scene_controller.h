#ifndef CONTROLLER_SCENE_CONTROLLER_H_
#define CONTROLLER_SCENE_CONTROLLER_H_

#include <model/scene.h>
#include <model/wireframe_mesh.h>

#include <functional>
#include <string>

namespace ec::ctl {

enum class Axis { X, Y, Z };

class SceneController {
 public:
  static constexpr float kScaleMin = 0.1f;   // wheel zoom floor
  static constexpr float kScaleBase = 1.0f;  // wheel zoom floor
  static constexpr float kScaleMax = 70.0f;
  static constexpr float kRotateSpeed = 0.01f;  // rad per pixel
  static constexpr float kPanSpeed = 0.001f;    // * scene radius per pixel
  static constexpr float kZoomIn = 1.1f;
  static constexpr float kZoomOut = 0.9f;

  // notifications
  using Callback = std::function<void()>;
  using MeshCallback = std::function<void(const model::Mesh&)>;
  Callback on_scene_changed;      // redraw needed
  Callback on_transform_changed;  // sliders need resync
  MeshCallback on_model_loaded;   // HUD, buttons, GL upload

  explicit SceneController(std::string config_path)
      : config_path_(std::move(config_path)) {}

  // ---- lifecycle / persistence
  void LoadSettings();
  bool SaveSettings();

  // ---- model
  // throws on bad file. Fits camera, hands mesh to view via on_model_loaded.
  // Mesh is not stored here: view/GPU own it after the callback.
  void LoadModel(const std::string& path);

  // GLArea render reads this (non-owning).
  model::SceneSettings* scene() noexcept { return &scene_; }

  // ---- viewport / projection
  void SetViewport(float w, float h);
  bool IsPerspective();
  void SetPerspective(bool on);

  // ---- transform: absolute (sliders / spinbuttons). Rotation in degrees.
  void RotateByDeg(Axis a, double deg);  // positive deg == clockwise , negative
                                         // deg == counter-clockwise
  void SetRotationDeg(Axis a, double deg);
  void SetOffset(Axis a, float v);
  void SetScale(float s);
  double GetRotationDeg(Axis a);
  float GetOffset(Axis a);
  float GetScale();
  void ResetAffine();

  // ---- transform: gestures (mouse). Deltas in pixels from drag start.
  void BeginRotate();
  void RotateDrag(double dx, double dy);
  void BeginPan();
  void PanDrag(double dx, double dy);
  void Zoom(double scroll_dy);

  // ---- appearance
  model::Appearance& style() { return scene_.style(); }
  void SetEdgeStyle(model::EdgeStyle s);
  void SetVertexStyle(model::VertexStyle s);
  void SetLineSize(float v);
  void SetVertexSize(float v);
  enum class ColorTarget { Background, Vertex, Edge };
  void SetColor(ColorTarget t, const model::ColorRGBA& c);
  model::ColorRGBA GetColor(ColorTarget t);

 private:
  void NotifyScene() const {
    if (on_scene_changed) on_scene_changed();
  }
  void NotifyTransform() const {
    if (on_transform_changed) on_transform_changed();
    NotifyScene();
  }

  model::SceneSettings scene_, prev_;
  std::string config_path_;

  // drag state lives in controller, not view
  float drag_pitch_ = 0.f, drag_yaw_ = 0.f;
  model::Vec3D drag_offset_{0, 0, 0};
};

}  // namespace ec::ctl
#endif
