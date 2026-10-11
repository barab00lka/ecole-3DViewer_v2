#include <controller/scene_controller.h>
#include <gui/config.h>

namespace ec::ctl {

namespace {
inline double DegToRad(double deg) { return deg * M_PI / 180.0; }
inline double RadToDeg(double rad) { return rad * 180.0 / M_PI; }
inline float clamp(float number, float lower, float upper) {
  if (lower > upper) {
    float temp = upper;
    upper = lower, lower = temp;
  }
  if (number < lower)
    return lower;
  else if (number > upper)
    return upper;
  else
    return number;
}
}  // namespace

using std::string;

void SceneController::LoadSettings() {
  scene_ = view::SceneSettingsSerializer::cfg_read_from_file(config_path_);
  prev_ = {};
  NotifyTransform();
}

bool SceneController::SaveSettings() {
  if (prev_ == scene_) return false;

  prev_ = scene_;
  return view::SceneSettingsSerializer::cfg_write_to_file(scene_, config_path_);
}

void SceneController::LoadModel(const string& path) {
  model::Mesh m = model::Mesh::load_from_obj(path);  // throws
  scene_.camera().fit_to_scene(m.get_center(), m.get_radius());
  if (on_model_loaded) on_model_loaded(m);
  NotifyTransform();
}

void SceneController::SetViewport(float w, float h) {
  scene_.camera().set_viewport(w, h);
}

bool SceneController::IsPerspective() {
  return scene_.camera().get_perspective();
}

void SceneController::SetPerspective(bool on) {
  scene_.camera().set_perspective(on);
  NotifyScene();
}

void SceneController::RotateByDeg(Axis a, double deg) {
  auto& t = scene_.transform();
  const float rad = static_cast<float>(DegToRad(deg));
  switch (a) {
    case Axis::X:
      t.pitch += rad;
      break;
    case Axis::Y:
      t.yaw += rad;
      break;
    case Axis::Z:
      t.roll += rad;
      break;
  }
  NotifyScene();
}

void SceneController::SetRotationDeg(Axis a, double deg) {
  auto& t = scene_.transform();
  const float rad = static_cast<float>(DegToRad(deg));
  switch (a) {
    case Axis::X:
      t.pitch = rad;
      break;
    case Axis::Y:
      t.yaw = rad;
      break;
    case Axis::Z:
      t.roll = rad;
      break;
  }
  NotifyScene();
}

void SceneController::SetOffset(Axis a, float v) {
  auto& t = scene_.transform();
  switch (a) {
    case Axis::X:
      t.offset.x = v;
      break;
    case Axis::Y:
      t.offset.y = v;
      break;
    case Axis::Z:
      t.offset.z = v;
      break;
  }
  NotifyScene();
}

void SceneController::SetScale(float s) {
  scene_.transform().scale = clamp(s, kScaleMin, kScaleMax);
  NotifyScene();
}

double SceneController::GetRotationDeg(Axis a) {
  auto& t = scene_.transform();
  switch (a) {
    case Axis::X:
      return RadToDeg(t.pitch);
    case Axis::Y:
      return RadToDeg(t.yaw);
    case Axis::Z:
      return RadToDeg(t.roll);
  }
  return 0.0;
}

float SceneController::GetOffset(Axis a) {
  auto& t = scene_.transform();
  switch (a) {
    case Axis::X:
      return t.offset.x;
    case Axis::Y:
      return t.offset.y;
    case Axis::Z:
      return t.offset.z;
  }
  return 0.f;
}

float SceneController::GetScale() { return scene_.transform().scale; }

void SceneController::ResetAffine() {
  scene_.transform() = {};
  NotifyTransform();
}

void SceneController::BeginRotate() {
  drag_pitch_ = scene_.transform().pitch;
  drag_yaw_ = scene_.transform().yaw;
}

void SceneController::RotateDrag(double dx, double dy) {
  auto& t = scene_.transform();
  t.pitch = drag_pitch_ + static_cast<float>(dy) * kRotateSpeed;
  t.yaw = drag_yaw_ + static_cast<float>(dx) * kRotateSpeed;
  NotifyTransform();
}

void SceneController::BeginPan() { drag_offset_ = scene_.transform().offset; }

void SceneController::PanDrag(double dx, double dy) {
  const float k = kPanSpeed * scene_.camera().get_scene_radius();
  const model::Vec3D d = {static_cast<float>(dx) * k,
                          -static_cast<float>(dy) * k, 0.f};
  scene_.transform().offset = drag_offset_ + d;
  NotifyTransform();
}

void SceneController::Zoom(double scroll_dy) {
  const float f = scroll_dy > 0 ? kZoomIn : kZoomOut;
  scene_.transform().scale =
      clamp(scene_.transform().scale * f, kScaleMin, kScaleMax);
  NotifyTransform();
}

void SceneController::SetEdgeStyle(model::EdgeStyle s) {
  scene_.style().style_lines = s;
  NotifyScene();
}
void SceneController::SetVertexStyle(model::VertexStyle s) {
  scene_.style().style_vt = s;
  NotifyScene();
}
void SceneController::SetLineSize(float v) {
  scene_.style().line_size = v;
  NotifyScene();
}
void SceneController::SetVertexSize(float v) {
  scene_.style().vertex_size = v;
  NotifyScene();
}

void SceneController::SetColor(ColorTarget t, const model::ColorRGBA& c) {
  auto& s = scene_.style();
  switch (t) {
    case ColorTarget::Background:
      s.col_bg = c;
      break;
    case ColorTarget::Vertex:
      s.col_vt = c;
      break;
    case ColorTarget::Edge:
      s.col_edge = c;
      break;
  }
  NotifyScene();
}

model::ColorRGBA SceneController::GetColor(ColorTarget t) {
  auto& s = scene_.style();
  switch (t) {
    case ColorTarget::Background:
      return s.col_bg;
    case ColorTarget::Vertex:
      return s.col_vt;
    case ColorTarget::Edge:
      return s.col_edge;
  }
  return {};
}

}  // namespace ec::ctl
