#include <giomm/liststore.h>
#include <glibmm/main.h>
#include <glibmm/timer.h>
#include <gtkmm/colordialog.h>
#include <gtkmm/colordialogbutton.h>
#include <gtkmm/dropdown.h>
#include <gtkmm/error.h>
#include <gtkmm/gesturedrag.h>
#include <gtkmm/gesturezoom.h>
#include <gtkmm/image.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/stringlist.h>
#include <gui/gif_writer.h>
#include <gui/main_window.h>
#include <thirdparty/tinyfiledialogs/tinyfiledialogs.h>

#include <cassert>
#include <climits>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>

namespace {
Gdk::RGBA negative(Gdk::RGBA color) {
  return Gdk::RGBA{1.0f - color.get_red(), 1.0f - color.get_green(),
                   1.0f - color.get_blue()};
}
}  // namespace
using ec::view::MainWindow;

MainWindow::MainWindow(int argc, char* argv[])
    : ctl_(config_path_from_args(argc, argv)) {
  maximize();  // requests to maximize window from the OS by default
  auto* vBox_main_ = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
  vBox_main_->set_margin(8);
  vBox_main_->set_spacing(6);
  set_child(*vBox_main_);

  ctl_.LoadSettings();  // Load settings from config first
  // Wire notification signals
  // controller notifies --> view changes accordingly
  {
    ctl_.on_scene_changed = [this] {
      overlay_.set_text_color(negative(get_scene_color(kBgColor)));
      gl_canvas_.queue_render();
    };
    ctl_.on_transform_changed = [this] { sync_sliders(); };
    ctl_.on_model_loaded = [this](const model::Mesh& m) {
      gl_canvas_.UploadModel(m);
      gl_canvas_.SetScene(ctl_.scene());
      overlay_.set_info_text(ctl_.GetModelInfo(m));
      btn_save_.set_visible(true);
    };
    ctl_.on_texture_changed = [this](const model::Texture* texture) {
      gl_canvas_.UploadTexture(texture);
      btn_tex_load_.set_label(texture == nullptr ? "Apply texture"
                                                 : "Unload texture");
      btn_uv_save_.set_visible(texture == nullptr ? false : true);
    };
    gl_canvas_.on_resized = [this](int w, int h) {
      ctl_.SetViewport(static_cast<float>(w), static_cast<float>(h));
      gl_canvas_.queue_render();
    };
  }

  // Settings auto save on close
  signal_close_request().connect(
      [this]() -> bool {
        ctl_.SaveSettings();
        return false;
      },
      false);
  // Settings auto save every 5 seconds for config persistence
  auto autosave_timer_ = std::make_unique<sigc::connection>();
  *autosave_timer_ = Glib::signal_timeout().connect(
      sigc::bind(sigc::mem_fun(*this, &MainWindow::on_autosave_timeout)),
      kAutoSaveIntervalMs);

  // Setup header bar
  auto bar = Gtk::make_managed<Gtk::HeaderBar>();
  {
    bar->set_title_widget(*Gtk::make_managed<Gtk::Label>("3DViewer"));
    set_titlebar(*bar);

    // Open file
    {
      btn_file_.set_tooltip_text("Load model from Wavefront .obj file");
      btn_file_.signal_clicked().connect(
          sigc::mem_fun(*this, &MainWindow::onOpenClicked));
      bar->pack_start(btn_file_);
    }

    // Setup screencasting menu
    {
      bar->pack_start(btn_save_);
      btn_save_.set_popover(*createScreencastMenu());
      btn_save_.set_label("Save");
      btn_save_.set_visible(false);  // becomes visible when model loaded
      btn_save_.set_icon_name("camera-photo-symbolic");
      btn_save_.set_tooltip_text("Capture screenshot or screencast");
      // Recording progress bar
      bar->pack_start(screencast_bar_);
      screencast_bar_.set_show_text(true);
      screencast_bar_.set_visible(false);
      screencast_bar_.set_margin(6);
    }

    bar->pack_start(btn_reveal_sidebar_);
  }

  // setup HUD and GLArea
  {
    setup_gesture_controls();  // sets up gesture controls in GLAreaWidget
    overlay_.set_expand();
    add_controller(
        overlay_.get_controller());  // GLAreaWidget is overlay_'s controller
    vBox_main_->append(overlay_);
  }

  // Sidebar: Stack(Styling | Affine) + StackSwitcher, one Revealer
  {
    auto* side = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
    side->set_hexpand(false);
    side->set_halign(Gtk::Align::START);  // дети не растягиваются

    auto* stack = Gtk::make_managed<Gtk::Stack>();
    stack->set_transition_type(Gtk::StackTransitionType::SLIDE_LEFT_RIGHT);
    stack->set_transition_duration(150);

    auto page_styling =
        stack->add(*createStylingControls(), "styling", "Styling");
    page_styling->set_icon_name("applications-graphics-symbolic");

    auto page_affine = stack->add(*createAffineControls(), "affine", "Affine");
    page_affine->set_icon_name("object-rotate-left-symbolic");

    stack->set_visible_child("styling");
    stack->set_hexpand(false);
    stack->set_halign(Gtk::Align::START);

    auto* switcher = Gtk::make_managed<Gtk::StackSwitcher>();
    switcher->set_stack(*stack);

    side->append(*switcher);
    side->append(*stack);

    auto* rv = Gtk::make_managed<Gtk::Revealer>();
    rv->set_transition_type(Gtk::RevealerTransitionType::SLIDE_RIGHT);
    rv->set_transition_duration(200);
    rv->set_reveal_child(true);
    rv->set_child(*side);
    rv->set_hexpand(false);

    auto* icon = Gtk::make_managed<Gtk::Image>();
    icon->set_from_icon_name("applications-system-symbolic");
    icon->set_pixel_size(16);
    btn_reveal_sidebar_.set_child(*icon);
    btn_reveal_sidebar_.set_tooltip_text("Styling / affine controls");
    btn_reveal_sidebar_.set_active(true);
    btn_reveal_sidebar_.signal_toggled().connect([this, rv]() {
      rv->set_reveal_child(btn_reveal_sidebar_.get_active());
    });

    vBox_main_->append(*rv);
  }

#ifndef NDEBUG
  gl_canvas_.signal_realize().connect([this]() {
    try {
      ctl_.LoadModel("sphere2.obj");
    } catch (const std::exception& err) {
      std::cerr << err.what() << '\n';
      return;
    }
    LOG_INFO("GUI", "Debug model loaded\n");
    ctl_.SetRotationDeg(static_cast<ctl::Axis>(kAxisY), 323);
    ctl_.SetRotationDeg(static_cast<ctl::Axis>(kAxisX), 10);
    ctl_.SetOffset(static_cast<ctl::Axis>(kAxisX), 2);
    ctl_.SetOffset(static_cast<ctl::Axis>(kAxisY), 1);
    ctl_.SetScale(0.7f);
  });
#endif
}

void MainWindow::onTextureBtnClicked() {
  if (ctl_.IsLoadedTexture()) {
    ctl_.LoadTexture("");
    return;
  }
  char const* filters[6] = {
      "*.bmp", "*.png", "*.jpg", "*.BMP", "*.PNG", "*.JPG",
  };
  const char* path =
      tinyfd_openFileDialog("Load texture image (max 1024x1024)",
                            "$HOME/Pictures/", 6, filters, "Images", 0);
  if (!path) {
    LOG_INFO("GUI", "No texture selected.");
    return;
  }
  try {
    ctl_.LoadTexture(path);
  } catch (const std::exception& e) {
    LOG_INFO("GUI", e.what());
  }
}

void MainWindow::onSaveUVMap() {
  std::cerr << "[DEBUG UV] onSaveUVMap called" << std::endl;
  if (!ctl_.IsLoadedModel()) {
    std::cerr << "[DEBUG UV] No model loaded." << std::endl;
    LOG_INFO("GUI", "No model loaded.");
    return;
  }
  if (!ctl_.IsLoadedTexture()) {
    std::cerr << "[DEBUG UV] No texture loaded." << std::endl;
    LOG_INFO("GUI", "No texture loaded.");
    return;
  }
  char const* FilterPatterns[1] = {"*.bmp"};
  const char* path =
      tinyfd_saveFileDialog("Save UV map",  // dialog title
                            "uv_map.bmp",   // default filename
                            1,              // number of filter patterns
                            FilterPatterns,
                            "Image files (BMP)");  // filter description

  if (!path) {
    LOG_INFO("GUI", "Saving UV map failed: no path given");
    return;
  }
  std::string sPath{path};

  auto has_ext = [](const std::string& p, const std::string& ext) {
    return p.size() >= ext.size() &&
           p.compare(p.size() - ext.size(), ext.size(), ext) == 0;
  };

  if (!has_ext(sPath, ".bmp") && !has_ext(sPath, ".BMP")) {
    sPath += ".bmp";
  }

  try {
    ctl_.SaveUVMap(sPath);
    LOG_INFO("GUI", "UV map saved to: ");
    std::cerr << sPath << '\n';
  } catch (const std::exception& e) {
    LOG_INFO("GUI", "Save UV map failed: ");
    std::cerr << e.what() << '\n';
  }
}

void MainWindow::onOpenClicked() {
  char const* lFilterPatterns[1] = {"*.obj"};
  const char* path = tinyfd_openFileDialog(
      "Open Wavefront geometry definition file",  // dialog title
      "$HOME/Documents/",                         // default path
      1,                                          // number of filter patterns
      lFilterPatterns,                            // filter patterns
      "Wavefront .obj",                           // filter description
      0);  // allow multiple selects (0 = no)

  if (!path) {
    // User cancelled (or an error occurred)
    LOG_INFO("GUI", "No file selected.");
    return;
  }

  try {
    // emits on_model_loaded and on_transform_changed notifications
    ctl_.LoadModel(path);
  } catch (const std::exception& err) {
    LOG_INFO("GUI", "Failed to load OBJ: ");
    std::cerr << err.what() << '\n';
    return;
  }
}

void MainWindow::onColorSet(ColorTarget target, const Gdk::RGBA& c) {
  if (target == kBgColor) {
    auto negative = Gdk::RGBA{1.0f - c.get_red(), 1.0f - c.get_green(),
                              1.0f - c.get_blue()};
    overlay_.set_text_color(negative);
  }
  set_scene_color(target, c);
}

void MainWindow::onScaleChange(const Glib::RefPtr<Gtk::Adjustment>& adj) {
  ctl_.SetScale(static_cast<float>(adj->get_value()));
}

void MainWindow::onRotateAround(Axis_ axis,
                                const Glib::RefPtr<Gtk::Adjustment>& adj) {
  ctl_.SetRotationDeg(static_cast<ctl::Axis>(axis), adj->get_value());
}

void MainWindow::onMove(Axis_ axis, const Glib::RefPtr<Gtk::Adjustment> adj) {
  const float v = static_cast<float>(adj->get_value());

  if (adj.get() == obj_X_.get() || adj.get() == obj_Y_.get() ||
      adj.get() == obj_Z_.get()) {
    ctl_.SetOffset(static_cast<ctl::Axis>(axis), v);
  } else {
    ctl_.SetLightOffset(static_cast<ctl::Axis>(axis), v);
  }
}

Gtk::Box* MainWindow::createAxisSliderBox(Axis_ axis) {
  auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 4);
  const char* text = axis == kAxisX   ? "Pitch "
                     : axis == kAxisY ? "Yaw "
                                      : "Roll ";

  auto* label = Gtk::make_managed<Gtk::Label>(text, Gtk::Align::CENTER);
  label->set_hexpand(true);
  box->append(*label);

  auto adj = Gtk::Adjustment::create(0.0, 0.0, 359.0, 1.0, 12.0, 0.0);

  switch (axis) {
    case kAxisX:
      obj_pitch_ = adj;
      break;
    case kAxisY:
      obj_yaw_ = adj;
      break;
    case kAxisZ:
      obj_roll_ = adj;
      break;
    default:
      break;
  }

  adj->signal_value_changed().connect(
      sigc::bind(sigc::mem_fun(*this, &MainWindow::onRotateAround), axis, adj));

  auto* spin = Gtk::make_managed<Gtk::SpinButton>(adj, 1.0, 0);
  spin->set_numeric(true);
  spin->set_wrap(true);
  spin->set_range(adj->get_lower() - 1, adj->get_upper() + 1);
  spin->set_width_chars(-1);
  box->append(*spin);
  return box;
}
Gtk::Box* MainWindow::createMovementField(Axis_ axis,
                                          Glib::RefPtr<Gtk::Adjustment>& p_adj,
                                          const float initial) {
  auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 4);
  const char* text = axis == kAxisX   ? "X axis "
                     : axis == kAxisY ? "Y axis "
                                      : "Z axis ";

  auto* label = Gtk::make_managed<Gtk::Label>(text, Gtk::Align::CENTER);
  label->set_hexpand(false);
  box->append(*label);

  p_adj = Gtk::Adjustment::create(initial, -1e6, 1e6, 1.0, 12.0, 0.0);
  p_adj->signal_value_changed().connect(
      sigc::bind(sigc::mem_fun(*this, &MainWindow::onMove), axis, p_adj));

  auto* spin = Gtk::make_managed<Gtk::SpinButton>(p_adj, 1.0, 0);
  spin->set_numeric(true);
  spin->set_wrap(true);
  spin->set_width_chars(-1);
  box->append(*spin);

  return box;
}

void MainWindow::onSaveScreenshot() {
  viewport_size_lock();
  const int scale = gl_canvas_.get_scale_factor();
  const int width = gl_canvas_.get_width() * scale;
  const int height = gl_canvas_.get_height() * scale;
  Vector<uint8_t> frame = gl_canvas_.CaptureFrame();
  if (frame.empty()) {
    LOG_INFO("GUI", "Unable to capture frame");
    viewport_size_unlock();
    return;
  }

  char const* FilterPatterns[3] = {"*.png", "*.jpg", "*.bmp"};
  const char* path =
      tinyfd_saveFileDialog("Save screenshot",  // dialog title
                            "screenshot.png",   // default filename
                            3,                  // number of filter patterns
                            FilterPatterns,     // filter patterns
                            "Image files (PNG, JPEG, BMP)");  // description

  if (!path) {
    LOG_INFO("GUI", "Saving screenshot failed: no path given");
    viewport_size_unlock();
    return;
  }
  std::string sPath{path};

  // tinyfd does NOT append an extension. Lowercase so "Screenshot.PNG" works.
  auto to_lower = [](std::string s) {
    for (auto& c : s)
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
  };
  auto has_ext = [&to_lower](const std::string& p, const std::string& ext) {
    return p.size() >= ext.size() &&
           to_lower(p.substr(p.size() - ext.size())) == ext;
  };

  const bool ends_jpg = has_ext(sPath, ".jpg") || has_ext(sPath, ".jpeg");
  const bool ends_png = has_ext(sPath, ".png");
  const bool ends_bmp = has_ext(sPath, ".bmp");
  if (!ends_jpg && !ends_png && !ends_bmp) {
    sPath += ".png";  // lossless + alpha-capable default
  }

  const std::string format = (has_ext(sPath, ".jpg") || has_ext(sPath, ".jpeg"))
                                 ? "jpeg"
                             : has_ext(sPath, ".png") ? "png"
                                                      : "bmp";

  // Per-pixel RGBA -> RGB converter (drops alpha). Only used for JPEG,
  // whose saver corrupts 4-channel input into a 1-component grayscale file.
  auto rgba_to_rgb = [](const uint8_t* rgba, uint8_t* rgb) {
    rgb[0] = rgba[0];
    rgb[1] = rgba[1];
    rgb[2] = rgba[2];
  };

  try {
    flip_vertically(frame, width, height);

    const std::size_t pixel_count =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);

    if (format == "jpeg") {
      // JPEG has no alpha channel — feed it 3-channel RGB.
      Vector<uint8_t> rgb(pixel_count * 3);
      for (std::size_t i = 0; i < pixel_count; ++i) {
        rgba_to_rgb(&frame[i * 4], &rgb[i * 3]);
      }
      Gdk::Pixbuf::create_from_data(rgb.data(), Gdk::Colorspace::RGB,
                                    /*has_alpha=*/false, 8, width, height,
                                    /*rowstride=*/width * 3)
          ->save(sPath, format);
    } else {
      // PNG and BMP both support real alpha — pass the RGBA buffer through.
      // gdk-pixbuf writes 4-channel PNG, and 32-bit BGRA BMP with alpha.
      Gdk::Pixbuf::create_from_data(frame.data(), Gdk::Colorspace::RGB,
                                    /*has_alpha=*/true, 8, width, height,
                                    /*rowstride=*/width * 4)
          ->save(sPath, format);
    }

    LOG_INFO("GUI", std::string("Saved to: ") + sPath);
  } catch (const std::exception& e) {
    LOG_INFO("GUI", std::string("Save failed: ") + e.what());
  }
  viewport_size_unlock();
}

void MainWindow::onSaveScreencast(bool rotate) {
  if (screencast_timer_.connected()) {
    LOG_INFO("GUI", "Error: Recording already active!\n");
    return;
  } else if (kGifFps <= 0 || kGifDurationSeconds <= 0) {
    LOG_INFO("GUI", "Invalid recording parameters\n");
    return;
  }
  viewport_size_lock();
  screencast_frames_.clear();
  screencast_frames_left_ = kGifTotalFrames;
  screencast_bar_.set_visible(true);
  if (rotate) ctl_.SetRotationDeg(ctl::Axis::Y, 0);
  screencast_timer_ = Glib::signal_timeout().connect(
      sigc::bind(sigc::mem_fun(*this, &MainWindow::on_framecapture_timeout),
                 rotate),
      kGifFrameIntervalMs);
}

Glib::RefPtr<Gtk::EventControllerKey> MainWindow::make_key_controller(
    std::pair<guint, std::function<void()>> binding) {
  auto ctl = Gtk::EventControllerKey::create();
  ctl->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);

  const guint keyval = binding.first;
  std::function<void()> action = std::move(binding.second);

  ctl->signal_key_pressed().connect(
      [keyval, action](guint kv, guint, Gdk::ModifierType) -> bool {
        if (kv != keyval) return false;
        action();
        return true;
      },
      false);

  return ctl;
}

Gtk::Box* MainWindow::createColorRow(const Glib::ustring& label,
                                     const Gdk::RGBA& initial,
                                     ColorTarget color_target,
                                     bool with_alpha) {
  auto* row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
  if (!label.empty()) row->append(*Gtk::make_managed<Gtk::Label>(label));
  row->set_homogeneous();
  row->set_hexpand();

  auto dialog = Gtk::ColorDialog::create();
  dialog->set_title(label);
  dialog->set_with_alpha(with_alpha);

  auto* btn = Gtk::make_managed<Gtk::ColorDialogButton>(dialog);
  btn->set_rgba(initial);

  btn->property_rgba().signal_changed().connect([this, btn, color_target]() {
    onColorSet(color_target, btn->get_rgba());
  });

  row->append(*btn);
  return row;
}

void MainWindow::sync_sliders() {
  using ctl::Axis;
  auto wrap360 = [](double d) {
    d = std::fmod(d, 360.0);
    return d < 0.0 ? d + 360.0 : d;
  };
  if (obj_pitch_) obj_pitch_->set_value(wrap360(ctl_.GetRotationDeg(Axis::X)));
  if (obj_yaw_) obj_yaw_->set_value(wrap360(ctl_.GetRotationDeg(Axis::Y)));
  if (obj_roll_) obj_roll_->set_value(wrap360(ctl_.GetRotationDeg(Axis::Z)));
  if (obj_scale_) obj_scale_->set_value(ctl_.GetScale());

  model::Vec3D obj_pos = ctl_.GetObjPos();
  if (obj_X_) obj_X_->set_value(obj_pos.x);
  if (obj_Y_) obj_Y_->set_value(obj_pos.y);
  if (obj_Z_) obj_Z_->set_value(obj_pos.z);

  model::Vec3D light_pos = ctl_.GetLightPos();
  if (light_X_) light_X_->set_value(light_pos.x);
  if (light_Y_) light_Y_->set_value(light_pos.y);
  if (light_Z_) light_Z_->set_value(light_pos.z);
}
void MainWindow::setup_gesture_controls() {
  // Pinch-to-zoom: scale
  auto zoom = Gtk::GestureZoom::create();
  auto base = std::make_shared<float>(ctl_.GetScale());

  zoom->signal_begin().connect(
      [this, base](Gdk::EventSequence*) { *base = obj_scale_->get_value(); });
  zoom->signal_scale_changed().connect(
      [this, base](double scale) { obj_scale_->set_value(*base * scale); });
  gl_canvas_.add_controller(zoom);

  // Scroll: Pan, Rotate and Scale
  auto scroll = Gtk::EventControllerScroll::create();
  scroll->set_flags(Gtk::EventControllerScroll::Flags::BOTH_AXES);
  scroll->signal_scroll().connect(
      [this, scroll](double dx, double dy) -> bool {
        auto state = scroll->get_current_event_state();
        bool ctrl =
            (state & Gdk::ModifierType::CONTROL_MASK) != Gdk::ModifierType{};
        bool shift =
            (state & Gdk::ModifierType::SHIFT_MASK) != Gdk::ModifierType{};

        if (ctrl) {
          ctl_.ApplyScale(-dy);
        } else if (shift) {
          ctl_.ApplyPan({static_cast<float>(-dx), static_cast<float>(dy), 0});
        } else {
          ctl_.ApplyRotateDeg(
              {static_cast<float>(-dy), static_cast<float>(-dx), 0});
        }
        return true;
      },
      false);
  gl_canvas_.add_controller(scroll);
}

void MainWindow::create_and_save_gif(const int px_w, const int px_h,
                                     const std::string& path) {
  if (path.empty() || path.length() > PATH_MAX) {
    LOG_INFO("GUI", "Saving gif failed: invalid path\n");
    viewport_size_unlock();
    return;
  }

  screencast_bar_.set_text("Saving GIF...");
  screencast_timer_ = Glib::signal_timeout().connect(
      [this]() {
        screencast_bar_.pulse();
        return true;
      },
      80);

  save_gif(px_w, px_h, std::string{path}, screencast_frames_,
           kGifFrameIntervalCs);
  LOG_INFO("[GUI:]", "\n [worker_ thread] GIF saved\n");
  screencast_timer_.disconnect();
  screencast_bar_.set_visible(false);
  viewport_size_unlock();
}

void MainWindow::set_scene_color(ColorTarget t, const Gdk::RGBA& c) {
  const model::ColorRGBA col{
      static_cast<float>(c.get_red()),
      static_cast<float>(c.get_green()),
      static_cast<float>(c.get_blue()),
      static_cast<float>(c.get_alpha()),
  };
  ctl_.SetColor(static_cast<ctl::SceneController::ColorTarget>(t), col);
}

Gdk::RGBA MainWindow::get_scene_color(ColorTarget t) {
  const auto c =
      ctl_.GetColor(static_cast<ctl::SceneController::ColorTarget>(t));
  Gdk::RGBA gcol;
  gcol.set_rgba(c.r, c.g, c.b, c.a);
  return gcol;
}

// Returns default config path if no path retrieved from cmdline arguments
std::string MainWindow::config_path_from_args(int argc, char** argv) {
  std::string value;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i] ? argv[i] : "";
    if (a == "-c" || a == "--config") {
      if (i + 1 >= argc) {
        std::cout << "[GUI]" << "argv: " << a << " requires a path"
                  << std::endl;
        break;
      }
      value = argv[++i];
    } else if (a.rfind("--config=", 0) == 0) {
      value = a.substr(9);
    } else if (a.rfind("-c", 0) == 0 && a.size() > 2 && a[2] != '-') {
      value = a.substr(2);
    } else {
      std::cout << "[GUI]" << "argv: ignored argument: " << a << std::endl;
      continue;
    }
  }
  if (value.empty()) {
    return Glib::get_user_config_dir() + '/' + kDefaultConfig;
  } else {
    return value;
  }
}

bool MainWindow::on_framecapture_timeout(bool rotate) {
  screencast_bar_.set_text(
      std::to_string(static_cast<int>(screencast_frames_left_) / kGifFps) +
      "s left");
  screencast_bar_.set_fraction(static_cast<double>(screencast_frames_left_) /
                               kGifTotalFrames);
  if (--screencast_frames_left_ == 0) {
    const int viewport_scale = gl_canvas_.get_scale_factor();
    char const* FilterPatterns[1] = {"*.gif"};
    const char* path =
        tinyfd_saveFileDialog("Save screencast",  // dialog title
                              "screencast.gif",   // default filename
                              1,                  // number of filter patterns
                              FilterPatterns,     // filter patterns
                              "Image files (GIF)");  // filter description

    if (!path) {
      // User cancelled — nothing to do
      LOG_INFO("GUI", "Saving screencast failed: no path given\n");
      screencast_bar_.set_visible(false);
    } else {
      worker_ =
          std::jthread{sigc::mem_fun(*this, &MainWindow::create_and_save_gif),
                       gl_canvas_.get_width() * viewport_scale,
                       gl_canvas_.get_height() * viewport_scale, path};
      LOG_INFO("GUI", "\n [worker_ thread] started GIF creation\n");
    }
    viewport_size_unlock();
    return false;
  }
  if (rotate) ctl_.ApplyRotateDeg({0, kPreviewYawDeg, 0});
  screencast_frames_.push_back(gl_canvas_.CaptureFrame());
  return true;
}

bool MainWindow::on_autosave_timeout() {
  // // if cfg_worker_ is already executing at this point, (i.e the thread is
  // // joinable) then a stop is requested and the thread joins. (cfg_worker_'s
  // // destructor is called here because of assignment operator.)
  // cfg_worker_ = std::jthread{[this]() {
  if (ctl_.SaveSettings()) {
    LOG_INFO("[GUI:]",
             "\n [cfg_worker_ thread] check autosave event: cfg updated\n");
  }
  // }};
  // by always returning true we can keep the timer
  // running for the whole lifetime of the main thread
  return true;
}

void MainWindow::viewport_size_lock() {
  btn_reveal_sidebar_.set_sensitive(false);
  set_size_request(get_size(Gtk::Orientation::HORIZONTAL),
                   get_size(Gtk::Orientation::VERTICAL));
  gl_canvas_.set_expand(false);
  overlay_.set_vexpand(false);
  set_resizable(false);
}
void MainWindow::viewport_size_unlock() {
  gl_canvas_.set_expand();
  overlay_.set_vexpand(false);
  btn_reveal_sidebar_.set_sensitive();
  set_resizable();
  set_size_request();
}
