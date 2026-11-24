#ifndef CONTROLLER_SCENE_CONTROLLER_H_
#define CONTROLLER_SCENE_CONTROLLER_H_

#include <misc/logger.h>
#include <model/scene.h>
#include <model/texture.h>
#include <model/wireframe_mesh.h>

#include <functional>
#include <string>

namespace ec::ctl {

enum class Axis { X, Y, Z };

class SceneController {
 public:
  static constexpr float kScaleMin = 0.1f;
  static constexpr float kScaleBase = 1.0f;
  static constexpr float kScaleMax = 70.0f;
  static constexpr float kPanFactor = 0.01f;
  static constexpr float kRotFactor = 0.5f;
  static constexpr float kZoomIn = 1.1f;
  static constexpr float kZoomOut = 0.9f;

  // notifications
  using Callback = std::function<void()>;
  using MeshCallback = std::function<void(const model::Mesh&)>;
  using TextureCallback =
      std::function<void(const model::Texture*)>;  // nullptr == unloaded
  Callback on_scene_changed;
  Callback on_transform_changed;
  MeshCallback on_model_loaded;
  TextureCallback on_texture_changed;

  explicit SceneController(std::string config_path)
      : config_path_(std::move(config_path)) {}

  // ---- lifecycle / persistence
  void LoadSettings();
  bool SaveSettings();

  // ---- model
  // Throws invalid_argument exception on model load error
  void LoadModel(const std::string& path);
  std::string GetModelInfo(const model::Mesh& m) const;
  const model::Mesh& GetMesh() const { return mesh_; }
  void LoadTexture(const std::string& path);
  bool IsLoadedTexture() const noexcept { return texture_loaded_; }
  bool IsLoadedModel() const noexcept { return model_loaded_; }

  // ---- UV map export
  // Renders UV edges onto the loaded texture and saves to a BMP file.
  // Throws std::runtime_error if no model or texture is loaded.
  void SaveUVMap(const std::string& path);

  model::SceneSettings* scene() noexcept { return &scene_; }

  // ---- viewport / projection
  void SetViewport(float w, float h);
  bool IsPerspective();
  void SetPerspective(bool on);

  // ---- transform: absolute (sliders / spinbuttons). Rotation in degrees.
  void ApplyRotateDeg(model::Vec3D deg = {});
  void ApplyPan(model::Vec3D coords = {});
  void ApplyScale(float scroll_dy = 0.0);
  void SetRotationDeg(Axis a, double deg);
  void SetOffset(Axis a, float v);
  void SetScale(float s);
  double GetRotationDeg(Axis a);
  float GetOffset(Axis a);
  model::Vec3D GetObjPos();
  float GetScale();
  void ResetAffine();

  // ---- appearance
  model::Appearance& style() { return scene_.style(); }
  void SetEdgeStyle(model::EdgeStyle s);
  void SetVertexStyle(model::VertexStyle s);
  void SetLineSize(float v);
  void SetVertexSize(float v);
  void SetDisplayType(model::ShadingStyle e);
  model::ShadingStyle GetDisplayType() { return scene_.style().obj_shading; }
  enum class ColorTarget { Background, Vertex, Edge, Base, Light };
  void SetColor(ColorTarget t, model::ColorRGBA c);
  model::ColorRGBA GetColor(ColorTarget t);
  void SetLightOffset(Axis a, float v);
  model::Vec3D GetLightPos() const { return scene_.light_offset(); }

 private:
  void notifyScene() const {
    if (on_scene_changed) on_scene_changed();
  }
  void notifyTransform() const {
    if (on_transform_changed) on_transform_changed();
    notifyScene();
  }

 private:
  model::SceneSettings scene_, prev_;
  model::Mesh mesh_;
  model::Texture texture_;
  bool model_loaded_ = false;
  std::string config_path_;
  bool texture_loaded_ = false;
};

}  // namespace ec::ctl
#endif
