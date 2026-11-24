#include <controller/scene_controller.h>
#include <gui/config.h>

#include <chrono>
#include <cstdio>
#include <filesystem>  // for file_size, path
#include <string>

namespace ec::ctl {

using std::string;
using std::to_string;

namespace {
inline double DegToRad(double deg) { return deg * M_PI / 180.0; }
inline double RadToDeg(double rad) { return rad * 180.0 / M_PI; }
inline float clamp(float number, float lower, float upper) {
  if (lower > upper) {
    float temp = upper;
    upper = lower, lower = temp;
  }
  if (number < lower) {
    return lower;
  } else if (number > upper) {
    return upper;
  } else {
    return number;
  }
}
}  // namespace

void SceneController::LoadSettings() {
  scene_ = view::SceneSettingsSerializer::cfg_read_from_file(config_path_);
  prev_ = {};
  // notifyTransform();
}

bool SceneController::SaveSettings() {
  if (prev_ == scene_) return false;

  prev_ = scene_;
  return view::SceneSettingsSerializer::cfg_write_to_file(scene_, config_path_);
}

void SceneController::LoadModel(const std::string& path) {
  auto start = std::chrono::high_resolution_clock::now();

  model::Mesh m = model::Mesh::load_from_obj(path);  // throws

  auto end = std::chrono::high_resolution_clock::now();
  auto d = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::printf("OBJ import of %s took %d ms\n", path.c_str(),
              static_cast<int>(d.count()));

  // Prepare model for display
  const auto [c, r] = m.calc_center_and_radius();
  m.center = c;
  m.radius = (r > 1e-6f) ? r : 1.0f;
  m.size_kib = static_cast<double>(std::filesystem::file_size(path)) / 1024.0;
  m.edge_count = m.count_uniq_edges();
  mesh_ = m;
  model_loaded_ = true;
  scene_.set_pivot(m.center);
  scene_.camera().fit_to_scene({0, 0, 0}, m.radius);
  notifyScene();
  if (on_model_loaded) on_model_loaded(m);
}

std::string SceneController::GetModelInfo(const model::Mesh& m) const {
  char filesize_rounded[25];
  std::string model_info_;
  std::snprintf(filesize_rounded, sizeof(filesize_rounded), "%.2lf",
                m.size_kib);
  model_info_ += "File:      " + m.fname + "\n";
  model_info_ += "Size:      " + string(filesize_rounded) + " KiB\n";
  model_info_ += "Vertices:  " + to_string(m.vertex_count) + "\n";
  model_info_ += "Edges:     " + to_string(m.edge_count) + "\n";
  model_info_ += "Polygons:    " + to_string(m.polygon_count);

  return model_info_;
}

void SceneController::LoadTexture(const std::string& path) {
  if (path.empty()) {  // unload
    texture_loaded_ = false;
    texture_ = {};
    if (on_texture_changed) on_texture_changed(nullptr);
    notifyScene();
    return;
  }
  model::Texture t =
      model::Texture::load_from_bmp(path);  // throws, state untouched
  texture_ = t;
  texture_loaded_ = true;
  if (on_texture_changed) {
    on_texture_changed(&texture_);
  }
  notifyScene();
}

void SceneController::SetViewport(float w, float h) {
  scene_.set_viewport(w, h);
}

bool SceneController::IsPerspective() { return scene_.is_perspective(); }

void SceneController::SetPerspective(bool enabled) {
  scene_.set_perspective(enabled);
  notifyTransform();
}

void SceneController::ApplyRotateDeg(model::Vec3D deg) {
  const float radX = static_cast<float>(DegToRad(deg.x)),
              radY = static_cast<float>(DegToRad(deg.y)),
              radZ = static_cast<float>(DegToRad(deg.z));

  scene_.obj_pitch(scene_.obj_pitch() + radX * kRotFactor);
  scene_.obj_yaw(scene_.obj_yaw() + radY * kRotFactor);
  scene_.obj_roll(scene_.obj_roll() + radZ * kRotFactor);
  notifyTransform();
}
void SceneController::ApplyPan(model::Vec3D coords) {
  scene_.obj_offset_x(scene_.obj_offset_x() + coords.x * kPanFactor);
  scene_.obj_offset_y(scene_.obj_offset_y() + coords.y * kPanFactor);
  scene_.obj_offset_z(scene_.obj_offset_z() + coords.z * kPanFactor);
  notifyTransform();
}

void SceneController::SetRotationDeg(Axis a, double deg) {
  const float rad = static_cast<float>(DegToRad(deg));
  switch (a) {
    case Axis::X:
      scene_.obj_pitch(rad);
      break;
    case Axis::Y:
      scene_.obj_yaw(rad);
      break;
    case Axis::Z:
      scene_.obj_roll(rad);
      break;
  }
  notifyTransform();
}

void SceneController::SetOffset(Axis a, float v) {
  switch (a) {
    case Axis::X:
      scene_.obj_offset_x(v);
      break;
    case Axis::Y:
      scene_.obj_offset_y(v);
      break;
    case Axis::Z:
      scene_.obj_offset_z(v);
      break;
  }
  notifyTransform();
}

void SceneController::SetScale(float s) {
  scene_.obj_scale(clamp(s, kScaleMin, kScaleMax));
  notifyTransform();
}

double SceneController::GetRotationDeg(Axis a) {
  switch (a) {
    case Axis::X:
      return RadToDeg(scene_.obj_pitch());
    case Axis::Y:
      return RadToDeg(scene_.obj_yaw());
    case Axis::Z:
      return RadToDeg(scene_.obj_roll());
  }
  return 0.0;
}

model::Vec3D SceneController::GetObjPos() {
  return {scene_.obj_offset_x(), scene_.obj_offset_y(), scene_.obj_offset_z()};
}
float SceneController::GetOffset(Axis a) {
  switch (a) {
    case Axis::X:
      return scene_.obj_offset_x();
    case Axis::Y:
      return scene_.obj_offset_y();
    case Axis::Z:
      return scene_.obj_offset_z();
  }
  return 0.0;
}

float SceneController::GetScale() { return scene_.obj_scale(); }

void SceneController::ResetAffine() {
  scene_.obj_scale(1.0);
  scene_.obj_offset_x({});
  scene_.obj_offset_y({});
  scene_.obj_offset_z({});
  scene_.obj_pitch({});
  scene_.obj_yaw({});
  scene_.obj_roll({});
  notifyTransform();
}

void SceneController::ApplyScale(float scroll_dy) {
  const float f = scroll_dy > 0 ? kZoomIn : kZoomOut;
  float old = scene_.obj_scale();
  scene_.obj_scale(clamp(old * f, kScaleMin, kScaleMax));
  notifyTransform();
}

void SceneController::SetEdgeStyle(model::EdgeStyle s) {
  scene_.style().style_lines = s;
  if (s == model::EdgeStyle::Dashed) {
    LOG_INFO("Controller", "Edges dashed");
  } else {
    LOG_INFO("Controller", "Edges solid");
  }
  notifyScene();
}
void SceneController::SetVertexStyle(model::VertexStyle s) {
  scene_.style().style_vt = s;
  notifyScene();
}
void SceneController::SetLineSize(float v) {
  scene_.style().line_size = v;
  notifyScene();
}
void SceneController::SetVertexSize(float v) {
  scene_.style().vertex_size = v;
  notifyScene();
}

void SceneController::SetDisplayType(model::ShadingStyle e) {
  scene_.style().obj_shading = e;
  if (e == model::ShadingStyle::Flat) {
    LOG_INFO("Controller", "Shading flat");
  } else if (e == model::ShadingStyle::Smooth) {
    LOG_INFO("Controller", "Shading smooth");
  } else {
    LOG_INFO("Controller", "Shading none (wireframe)");
  }
  notifyScene();
}

void SceneController::SetColor(ColorTarget t, model::ColorRGBA c) {
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
    case ColorTarget::Base:
      s.col_obj = c;
      break;
    case ColorTarget::Light:
      s.col_lighting = c;
      break;
  }
  notifyScene();
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
    case ColorTarget::Base:
      return s.col_obj;
    case ColorTarget::Light:
      return s.col_lighting;
  }
  return {};
}

void SceneController::SetLightOffset(Axis a, float v) {
  auto o = scene_.light_offset();
  switch (a) {
    case Axis::X:
      scene_.light_offset({v, o.y, o.z});
      break;
    case Axis::Y:
      scene_.light_offset({o.x, v, o.z});
      break;
    case Axis::Z:
      scene_.light_offset({o.x, o.y, v});
      break;
  }
  notifyScene();
};

void SceneController::SaveUVMap(const std::string& path) {
  std::cerr << "[DEBUG UV] SaveUVMap called with path: " << path << std::endl;
  if (!model_loaded_) {
    std::cerr << "[DEBUG UV] No model loaded" << std::endl;
    throw std::runtime_error("No model loaded");
  }
  if (!texture_loaded_) {
    std::cerr << "[DEBUG UV] No texture loaded" << std::endl;
    throw std::runtime_error("No texture loaded");
  }

  const auto& edge_color = scene_.style().col_edge;
  std::cerr << "[DEBUG UV] Drawing UV edges, color=(" << edge_color.r << ","
            << edge_color.g << "," << edge_color.b << ")" << std::endl;
  const model::Texture uv_tex =
      model::Texture::draw_uv_edges(texture_, mesh_, edge_color);
  if (!model::Texture::save_to_bmp(uv_tex, path)) {
    std::cerr << "[DEBUG UV] Failed to save BMP to: " << path << std::endl;
    throw std::runtime_error("Failed to save UV map to: " + path);
  }
  std::cerr << "[DEBUG UV] UV map saved successfully to: " << path << std::endl;
}
}  // namespace ec::ctl
