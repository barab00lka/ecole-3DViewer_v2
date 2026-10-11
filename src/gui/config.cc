#include <glibmm/error.h>
#include <glibmm/keyfile.h>
#include <gui/config.h>

#include <iostream>

namespace ec::view {

namespace {
const char* to_str(model::EdgeStyle s) {
  switch (s) {
    case model::EdgeStyle::Solid:
      return "Solid";
    case model::EdgeStyle::Dashed:
      return "Dashed";
  }
  return "Solid";
}

const char* to_str(model::VertexStyle s) {
  switch (s) {
    case model::VertexStyle::None:
      return "None";
    case model::VertexStyle::Circles:
      return "Circles";
    case model::VertexStyle::Squares:
      return "Squares";
  }
  return "None";
}
}  // namespace

bool SceneSettingsSerializer::cfg_write_to_file(model::SceneSettings& cfg,
                                                const std::string& path) {
  auto kf = Glib::KeyFile::create();
  // Camera
  kf->set_boolean("Camera", "perspective", cfg.camera().get_perspective());
  kf->set_double("Camera", "distance", cfg.camera().get_distance());

  // Style
  kf->set_string("Style", "bg_color", cfg.style().col_bg.hexstring());
  kf->set_string("Style", "vt_color", cfg.style().col_vt.hexstring());
  kf->set_string("Style", "edge_color", cfg.style().col_edge.hexstring());
  kf->set_double("Style", "line_size", cfg.style().line_size);
  kf->set_double("Style", "vertex_size", cfg.style().vertex_size);
  kf->set_string("Style", "line_style", to_str(cfg.style().style_lines));
  kf->set_string("Style", "vertex_style", to_str(cfg.style().style_vt));

  try {
    kf->save_to_file(path);
    return true;
  } catch (const Glib::Error& e) {
    std::cerr << "save config failed: " << e.what() << "\n";
    return false;
  }
}

model::SceneSettings SceneSettingsSerializer::cfg_read_from_file(
    const std::string& path) {
  model::SceneSettings cfg;
  auto kf = Glib::KeyFile::create();

  try {
    kf->load_from_file(path);

    cfg.camera().set_perspective(kf->get_boolean("Camera", "perspective"));
    cfg.camera().set_distance(
        static_cast<float>(kf->get_double("Camera", "distance")));

    cfg.style().col_bg = model::ColorRGBA{kf->get_string("Style", "bg_color")};
    cfg.style().col_vt = model::ColorRGBA{kf->get_string("Style", "vt_color")};
    cfg.style().col_edge =
        model::ColorRGBA{kf->get_string("Style", "edge_color")};
    cfg.style().line_size = kf->get_double("Style", "line_size");
    cfg.style().vertex_size = kf->get_double("Style", "vertex_size");
    auto str = kf->get_string("Style", "line_style");
    if (str == "Dashed") {
      cfg.style().style_lines = model::EdgeStyle::Dashed;
    } else if (str == "Solid") {
      cfg.style().style_lines = model::EdgeStyle::Solid;
    }
    str = kf->get_string("Style", "vertex_style");
    if (str == "Circles") {
      cfg.style().style_vt = model::VertexStyle::Circles;
    } else if (str == "Squares") {
      cfg.style().style_vt = model::VertexStyle::Squares;
    } else {
      cfg.style().style_vt = model::VertexStyle::None;
    }
  } catch (const Glib::Error& e) {
    std::cerr << "load config failed: " << e.what() << "\n";
  }

  return cfg;
}

}  // namespace ec::view
