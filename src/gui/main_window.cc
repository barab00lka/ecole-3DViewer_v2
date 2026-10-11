#include <giomm/liststore.h>
#include <glibmm/main.h>
#include <gtkmm/image.h>
#include <gtkmm/dropdown.h>
#include <gtkmm/colordialog.h>
#include <gtkmm/colordialogbutton.h>
#include <gtkmm/stringlist.h>
#include <glibmm/timer.h>
#include <gtkmm/error.h>
#include <gtkmm/gesturedrag.h>
#include <gtkmm/spinbutton.h>
#include <gui/gif_writer.h>
#include <gui/main_window.h>
#include <thirdparty/tinyfiledialogs/tinyfiledialogs.h>

#include <cassert>
#include <functional>
#include <iostream>
#include <memory>

namespace {
void flip_vertically(ec::Vector<uint8_t>& vec, int width, int height) {
  if (!width || !height) return;
  assert(static_cast<size_t>(width * height * 3) <= vec.size());
  for (int y = 0; y < height / 2; ++y) {
    for (int x = 0; x < width; ++x) {
      int top = (x + width * y) * 3;
      int bottom = (x + width * (height - y - 1)) * 3;

      for (int i = 0; i < 3; i++) {
        uint8_t temp = vec[top + i];
        vec[top + i] = vec[bottom + i];
        vec[bottom + i] = temp;
      }
    }
  }
}
Gdk::RGBA negative(Gdk::RGBA color) {
  return Gdk::RGBA{1.0f - color.get_red(), 1.0f - color.get_green(),
                   1.0f - color.get_blue()};
}
}  // namespace
using namespace ec::view;

MainWindow::MainWindow(int argc, char* argv[])
    : ctl_(config_path_from_args(argc, argv)) {
  // Main container
  auto* vBox_main_ = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
  vBox_main_->set_margin(8);
  vBox_main_->set_spacing(6);
  set_child(*vBox_main_);

  ctl_.LoadSettings();  // Load settings from config first
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
      overlay_.set_model_info(m);
      btn_save_.set_visible(true);
    };
    gl_canvas_.SetScene(ctl_.scene());
    gl_canvas_.on_resized = [this](int w, int h) {
      ctl_.SetViewport(static_cast<float>(w), static_cast<float>(h));
    };
  }

  // HEADER BAR
  {
    hb_.set_title_widget(*Gtk::make_managed<Gtk::Label>("3DViewer"));
    set_titlebar(hb_);

    // Open file
    {
      btn_file_.set_tooltip_text("Load model from Wavefront .obj file");
      btn_file_.signal_clicked().connect(
          sigc::mem_fun(*this, &MainWindow::onOpenClicked));
      hb_.pack_start(btn_file_);
    }
    // Save dropdown menu:
    // screenshot: bmp / jpeg;
    // screencast: 5s at 10fps converted to gif
    // screencast with model preview: 5s at 10fps converted to gif, model spins
    // around Y axis
    {
      btn_save_.set_label("Save");
      btn_save_.set_visible(false);  // becomes visible when model loaded
      btn_save_.set_icon_name("camera-photo-symbolic");
      btn_save_.set_tooltip_text("Capture screenshot or screencast");

      auto* popover = Gtk::make_managed<Gtk::Popover>();
      auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
      box->set_margin(6);

      btn_screenshot_.signal_clicked().connect([this, popover]() {
        popover->popdown();
        onSaveScreenshot();
      });
      btn_screencast_.signal_clicked().connect([this, popover]() {
        popover->popdown();
        onSaveScreencast(false);
      });
      btn_screencast_preview_.signal_clicked().connect([this, popover]() {
        popover->popdown();
        onSaveScreencast(true);
      });
      box->append(btn_screenshot_);
      box->append(btn_screencast_);
      box->append(btn_screencast_preview_);

      popover->set_child(*box);
      btn_save_.set_popover(*popover);
      hb_.pack_start(btn_save_);

      // Recording progress bar
      screencast_bar_.set_show_text(true);
      screencast_bar_.set_visible(false);
      screencast_bar_.set_expand(true);
      screencast_bar_.set_margin(6);
      hb_.pack_start(screencast_bar_);
    }

    // Transformations controls toggle 
    {
      btn_tsfm_ctls_.set_tooltip_text("Affine transformations controls");
      btn_tsfm_ctls_.set_active(true);

      auto* icon = Gtk::make_managed<Gtk::Image>();
      icon->set_from_icon_name("object-rotate-left-symbolic");
      icon->set_pixel_size(16);
      btn_tsfm_ctls_.set_child(*icon);

      btn_tsfm_ctls_.signal_toggled().connect([this]() {
        controls_revealer_.set_reveal_child(btn_tsfm_ctls_.get_active());
      });
      hb_.pack_end(btn_tsfm_ctls_);
    }

    // Styling controls toggle
    {
      btn_style_ctls_.set_tooltip_text("Styling controls");
      btn_style_ctls_.set_active(true);

      auto* icon = Gtk::make_managed<Gtk::Image>();
      icon->set_from_icon_name("applications-graphics-symbolic");
      icon->set_pixel_size(16);
      btn_style_ctls_.set_child(*icon);

      btn_style_ctls_.signal_toggled().connect([this]() {
        styling_revealer_.set_reveal_child(btn_style_ctls_.get_active());
      });
      hb_.pack_end(btn_style_ctls_);
    }
  }

  // LEFT: Styling (Revealer)
  {
    styling_revealer_.set_transition_type(
        Gtk::RevealerTransitionType::SLIDE_RIGHT);
    styling_revealer_.set_transition_duration(200);
    styling_revealer_.set_reveal_child(true);
    styling_revealer_.set_hexpand(false);

    auto* styling_frame = Gtk::make_managed<Gtk::Frame>("Styling");
    auto* styling_box =
        Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
    styling_box->set_margin(6);
    styling_frame->set_child(*styling_box);

    // Colors
    styling_box->append(*createColorsFrame());

    // Edges styling
    {
      auto* edge_frame = Gtk::make_managed<Gtk::Frame>("Edges");
      auto* edge_box =
          Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
      edge_box->set_margin(6);
      edge_frame->set_child(*edge_box);

      auto* edge_style_d = Gtk::make_managed<Gtk::DropDown>(
          Gtk::StringList::create({"Solid", "Dashed"}));
      edge_style_d->set_selected(static_cast<guint>(ctl_.style().style_lines));
      edge_style_d->property_selected().signal_changed().connect(
          [this, edge_style_d]() {
            const guint idx = edge_style_d->get_selected();
            if (idx == GTK_INVALID_LIST_POSITION) return;
            ctl_.SetEdgeStyle(static_cast<model::EdgeStyle>(idx));
          });
      edge_box->append(*edge_style_d);

      auto* edge_width_row =
          Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
      edge_width_row->append(*Gtk::make_managed<Gtk::Label>("Width"));

      auto edge_width_adj =
          Gtk::Adjustment::create(1.0, 0.5, 20.0, 0.5, 2.0, 0.0);
      edge_width_adj->set_value(ctl_.style().line_size);

      auto* edge_width_spin =
          Gtk::make_managed<Gtk::SpinButton>(edge_width_adj, 0.5, 1);
      edge_width_spin->set_hexpand(true);
      edge_width_spin->set_numeric(true);
      edge_width_spin->set_wrap(false);
      edge_width_spin->set_width_chars(6);
      edge_width_row->append(*edge_width_spin);

      edge_width_adj->signal_value_changed().connect([this, edge_width_adj]() {
        ctl_.SetLineSize(static_cast<float>(edge_width_adj->get_value()));
      });

      edge_box->append(*edge_width_row);
      styling_box->append(*edge_frame);
    }

    // Vertices styling
    {
      auto* vt_frame = Gtk::make_managed<Gtk::Frame>("Vertices");
      auto* vt_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
      vt_box->set_margin(6);
      vt_frame->set_child(*vt_box);

      auto* vt_style_d = Gtk::make_managed<Gtk::DropDown>(
          Gtk::StringList::create({"None", "Circles", "Squares"}));
      vt_style_d->set_selected(static_cast<guint>(ctl_.style().style_vt));
      vt_style_d->property_selected().signal_changed().connect(
          [this, vt_style_d]() {
            const guint idx = vt_style_d->get_selected();
            if (idx == GTK_INVALID_LIST_POSITION) return;
            ctl_.SetVertexStyle(static_cast<model::VertexStyle>(idx));
          });
      vt_box->append(*vt_style_d);

      auto* vt_size_row =
          Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
      vt_size_row->append(*Gtk::make_managed<Gtk::Label>("Size"));

      auto vt_size_adj = Gtk::Adjustment::create(5.0, 1.0, 50.0, 1.0, 5.0, 0.0);
      vt_size_adj->set_value(ctl_.style().vertex_size);
      auto* vt_size_spin =
          Gtk::make_managed<Gtk::SpinButton>(vt_size_adj, 1.0, 0);
      vt_size_spin->set_hexpand(true);
      vt_size_spin->set_numeric(true);
      vt_size_spin->set_wrap(false);
      vt_size_spin->set_width_chars(6);
      vt_size_row->append(*vt_size_spin);

      vt_size_adj->signal_value_changed().connect([this, vt_size_adj]() {
        ctl_.SetVertexSize(static_cast<float>(vt_size_adj->get_value()));
      });

      vt_box->append(*vt_size_row);
      styling_box->append(*vt_frame);
    }

    styling_revealer_.set_child(*styling_frame);
  }

  // CENTER: Overlay (GLArea + HUD)
  {
    overlay_.set_hexpand(true);
    overlay_.set_vexpand(true);
    add_controller(overlay_.get_controller());
  }

  // RIGHT: Affine (Revealer)
  {
    controls_revealer_.set_transition_type(
        Gtk::RevealerTransitionType::SLIDE_LEFT);
    controls_revealer_.set_transition_duration(200);
    controls_revealer_.set_reveal_child(true);
    controls_revealer_.set_hexpand(false);

    auto* ctls_frame = Gtk::make_managed<Gtk::Frame>("Affine");
    auto* ctls = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    ctls->set_hexpand(true);
    ctls->set_margin(8);
    ctls->set_spacing(6);
    ctls_frame->set_child(*ctls);

    // Reset affine transformations button
    {
      auto* btn_reset = Gtk::make_managed<Gtk::Button>("Reset");
      btn_reset->set_tooltip_text(
          "Reset affine transformations to initial parameters");
      btn_reset->signal_clicked().connect([this]() { ctl_.ResetAffine(); });
      ctls->append(*btn_reset);
    }

    // Projection toggle
    {
      auto* button_projection =
          Gtk::make_managed<Gtk::CheckButton>("Perspective");
      button_projection->set_active(ctl_.IsPerspective());
      button_projection->signal_toggled().connect([this, button_projection]() {
        ctl_.SetPerspective(button_projection->get_active());
      });
      ctls->append(*button_projection);
    }

    // Rotate
    {
      auto* rot_frame = Gtk::make_managed<Gtk::Frame>("Rotate");
      auto* rot_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
      rot_box->set_margin(6);
      rot_frame->set_child(*rot_box);
      for (int i = 0; i < kAxisCount; ++i) {
        rot_box->append(*createAxisSliderBox(static_cast<Axis_>(i)));
      }
      ctls->append(*rot_frame);
    }

    // Scale
    {
      auto* box =
          Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, false);
      box->append(*Gtk::make_managed<Gtk::Label>("Scale "));

      adj_scale_ = Gtk::Adjustment::create(
          ctl::SceneController::kScaleBase, ctl::SceneController::kScaleMin,
          ctl::SceneController::kScaleMax, 0.1, 1.0, 0.0);
      adj_scale_->signal_value_changed().connect(sigc::bind(
          sigc::mem_fun(*this, &MainWindow::onScaleChange), adj_scale_));

      auto* spin = Gtk::make_managed<Gtk::SpinButton>(adj_scale_, 0.1, 2);
      spin->set_numeric(true);
      spin->set_wrap(false);
      spin->set_width_chars(6);
      box->append(*spin);
      ctls->append(*box);
    }

    // Move
    {
      auto* mv_frame = Gtk::make_managed<Gtk::Frame>("Move");
      auto* mv_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
      mv_box->set_margin(6);
      mv_frame->set_child(*mv_box);
      for (int i = 0; i < kAxisCount; ++i) {
        mv_box->append(*createMovementField(static_cast<Axis_>(i)));
      }
      ctls->append(*mv_frame);
    }
    controls_revealer_.set_child(*ctls_frame);
  }

  vBox_main_->append(styling_revealer_);
  vBox_main_->append(overlay_);
  vBox_main_->append(controls_revealer_);

  // Controllers
  setup_gesture_controls();
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
    std::cerr << "No file selected." << std::endl;
    return;
  }

  try {
    ctl_.LoadModel(
        path);  // emits on_model_loaded and on_transform_changed notifications
  } catch (const std::exception& err) {
    std::cerr << "Failed to load OBJ: " << err.what() << std::endl;
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

Gtk::Box* MainWindow::createAxisSliderBox(Axis_ axis) {
  auto* box = Gtk::manage(new Gtk::Box{Gtk::Orientation::HORIZONTAL, false});

  const char* text = axis == kAxisX   ? "Pitch "
                     : axis == kAxisY ? "Yaw "
                                      : "Roll ";
  box->append(*Gtk::make_managed<Gtk::Label>(text));

  auto adj = Gtk::Adjustment::create(0.0, 0.0, 359.0, 1.0, 12.0, 0.0);

  switch (axis) {
    case kAxisX:
      adj_pitch_ = adj;
      break;
    case kAxisY:
      adj_yaw_ = adj;
      break;
    case kAxisZ:
      adj_roll_ = adj;
      break;
    default:
      break;
  }

  adj->signal_value_changed().connect(
      sigc::bind(sigc::mem_fun(*this, &MainWindow::onRotateAround), axis, adj));

  auto* spin = Gtk::make_managed<Gtk::SpinButton>(adj, 1.0, 0);
  spin->set_hexpand(true);
  spin->set_numeric(true);
  spin->set_wrap(true);
  spin->set_width_chars(6);
  box->append(*spin);
  return box;
}

void MainWindow::onMove(Axis_ axis, const Glib::RefPtr<Gtk::Adjustment>& adj) {
  const float v = static_cast<float>(adj->get_value());

  ctl_.SetOffset(static_cast<ctl::Axis>(axis), v);
}

Gtk::Box* MainWindow::createMovementField(Axis_ axis) {
  auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, false);
  const char* text = axis == kAxisX   ? "X axis "
                     : axis == kAxisY ? "Y axis "
                                      : "Z axis ";
  box->append(*Gtk::make_managed<Gtk::Label>(text));

  auto adj = Gtk::Adjustment::create(0.0, -1e6, 1e6, 1.0, 12.0, 0.0);

  switch (axis) {
    case kAxisX:
      adj_X_ = adj;
      break;
    case kAxisY:
      adj_Y_ = adj;
      break;
    case kAxisZ:
      adj_Z_ = adj;
      break;
    default:
      break;
  }

  adj->signal_value_changed().connect(
      sigc::bind(sigc::mem_fun(*this, &MainWindow::onMove), axis, adj));

  auto* spin = Gtk::make_managed<Gtk::SpinButton>(adj, 1.0, 0);
  spin->set_hexpand(true);
  spin->set_numeric(true);
  spin->set_wrap(true);
  spin->set_width_chars(6);
  box->append(*spin);
  return box;
}

void MainWindow::onSaveScreenshot() {
  viewport_size_lock();	
  const int scale = gl_canvas_.get_scale_factor();
  const int width = gl_canvas_.get_width() * scale;
  const int height = gl_canvas_.get_height() * scale;
  Vector<uint8_t> frame = gl_canvas_.CaptureFrame();
  flip_vertically(frame, width, height);
  if (frame.empty()) {
    std::cerr << "Unable to capture frame" << std::endl;
    return;
  }
  char const* FilterPatterns[2] = {"*.jpg", "*.bmp"};
  const char* path =
      tinyfd_saveFileDialog("Save screenshot",  // dialog title
                            "screenshot.jpg",   // default filename
                            2,                  // number of filter patterns
                            FilterPatterns,     // filter patterns
                            "Image files (JPEG, BMP)");  // filter description

  if (!path) { // User cancelled — nothing to do
	viewport_size_unlock();	
    return;
  }
  std::string sPath{path};

  // tinyfd does NOT append an extension automatically.
  // If the user typed a name without an extension, default to .jpg.
  auto has_ext = [](const std::string& p, const std::string& ext) {
    return p.size() >= ext.size() &&
           p.compare(p.size() - ext.size(), ext.size(), ext) == 0;
  };

  const bool ends_jpg = has_ext(sPath, ".jpg") || has_ext(sPath, ".jpeg");
  const bool ends_bmp = has_ext(sPath, ".bmp");
  if (!ends_jpg && !ends_bmp) {
    sPath += ".jpg";  // fall back to jpeg
  }
  const bool jpeg = has_ext(sPath, ".jpg") || has_ext(sPath, ".jpeg");

  const std::string format = jpeg ? "jpeg" : "bmp";
  try {
    Gdk::Pixbuf::create_from_data(frame.data(), Gdk::Colorspace::RGB, false, 8,
                                  width, height, width * 3)
        ->save(sPath, format);
    std::cout << "Saved to: " << sPath << '\n';
  } catch (const std::exception& e) {
    std::cerr << "Save failed: " << e.what() << '\n';
  }
}

void MainWindow::onSaveScreencast(bool rotate) {
  if (screencast_timer_.connected()) {
    std::cerr << "Error: Recording already active!\n";
    return;
  } else if (kGifFps <= 0 || kGifDurationSeconds <= 0) {
    std::cerr << "Invalid recording parameters\n";
    return;
  }
  screencast_frames_.clear();
  screencast_frames_left_ = kGifTotalFrames;
  screencast_bar_.set_visible(true);
  viewport_size_lock();	
  if (rotate) ctl_.SetRotationDeg(ctl::Axis::Y, 0);
  screencast_timer_ = Glib::signal_timeout().connect(
	sigc::bind(sigc::mem_fun(*this, &MainWindow::on_framecapture_timeout), rotate),
	kGifFrameIntervalMs
  );
}

Glib::RefPtr<Gtk::EventControllerKey> MainWindow::make_key_controller(
    std::pair<guint, std::function<void()>> binding) {
  auto ctl = Gtk::EventControllerKey::create();
  ctl->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);

  auto [keyval, action] = binding;
  ctl->signal_key_pressed().connect(
      [keyval, action = action](guint kv, guint, Gdk::ModifierType) -> bool {
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
  auto* row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
  row->append(*Gtk::make_managed<Gtk::Label>(label));

  auto dialog = Gtk::ColorDialog::create();
  dialog->set_with_alpha(with_alpha);

  auto* btn = Gtk::make_managed<Gtk::ColorDialogButton>(dialog);
  btn->set_hexpand(true);
  btn->set_rgba(initial);

  btn->property_rgba().signal_changed().connect([this, btn, color_target]() {
    onColorSet(color_target, btn->get_rgba());
  });

  row->append(*btn);
  return row;
}

Gtk::Frame* MainWindow::createColorsFrame() {
  auto* frame = Gtk::make_managed<Gtk::Frame>();
  auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
  box->set_margin(6);
  frame->set_child(*box);

  box->append(
      *createColorRow("Background", get_scene_color(kBgColor), kBgColor));
  box->append(*createColorRow("Vertex", get_scene_color(kVtColor), kVtColor));
  box->append(*createColorRow("Edge", get_scene_color(kEdgeColor), kEdgeColor));

  return frame;
}

// NEEDS TESTING
void MainWindow::sync_sliders() {
  using ctl::Axis;
  if (adj_pitch_) adj_pitch_->set_value(ctl_.GetRotationDeg(Axis::X));
  if (adj_yaw_) adj_yaw_->set_value(ctl_.GetRotationDeg(Axis::Y));
  if (adj_roll_) adj_roll_->set_value(ctl_.GetRotationDeg(Axis::Z));
  if (adj_scale_) adj_scale_->set_value(ctl_.GetScale());
  if (adj_X_) adj_X_->set_value(ctl_.GetOffset(Axis::X));
  if (adj_Y_) adj_Y_->set_value(ctl_.GetOffset(Axis::Y));
}

void MainWindow::setup_gesture_controls() {
  // ПКМ: вращение модели
  auto rotate = Gtk::GestureDrag::create();
  rotate->set_button(GDK_BUTTON_SECONDARY);
  rotate->signal_drag_begin().connect(
      [this](double, double) { ctl_.BeginRotate(); });
  rotate->signal_drag_update().connect(
      [this](double dx, double dy) { ctl_.RotateDrag(dx, dy); });
  gl_canvas_.add_controller(rotate);

  // ЛКМ: панорама (сдвиг модели в экранных координатах)
  auto pan = Gtk::GestureDrag::create();
  pan->set_button(GDK_BUTTON_PRIMARY);
  pan->signal_drag_begin().connect([this](double, double) { ctl_.BeginPan(); });
  pan->signal_drag_update().connect(
      [this](double dx, double dy) { ctl_.PanDrag(dx, dy); });
  gl_canvas_.add_controller(pan);

  // Scroll wheel -> scale
  auto scroll = Gtk::EventControllerScroll::create();
  scroll->set_flags(Gtk::EventControllerScroll::Flags::VERTICAL);
  scroll->signal_scroll().connect(
      [this](double, double dy) -> bool {
        ctl_.Zoom(dy);
        return true;
      },
      false);
  gl_canvas_.add_controller(scroll);
}

void MainWindow::create_and_save_gif(const int px_w, const int px_h) {
  char const* FilterPatterns[1] = {"*.gif"};
  const char* path =
      tinyfd_saveFileDialog("Save screencast",     // dialog title
                            "screencast.gif",      // default filename
                            1,                     // number of filter patterns
                            FilterPatterns,        // filter patterns
                            "Image files (GIF)");  // filter description

  if (!path) {
    // User cancelled — nothing to do
	viewport_size_unlock();
  	screencast_bar_.set_visible(false);
    return;
  }

  screencast_bar_.set_text("Saving GIF...");
  screencast_timer_= Glib::signal_timeout().connect(
      [this](){
	  	screencast_bar_.pulse();
		return true;
	  }, 80);

  for (auto& f : screencast_frames_) flip_vertically(f, px_w, px_h);
  save_gif(px_w, px_h, std::string{path}, screencast_frames_, kGifFrameIntervalCs);
  std::cerr << "[worker_ thread] GIF saved\n";
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
        std::cerr << "argv: " << a << " requires a path" << std::endl;
        break;
      }
      value = argv[++i];
    } else if (a.rfind("--config=", 0) == 0) {
      value = a.substr(9);
    } else if (a.rfind("-c", 0) == 0 && a.size() > 2 && a[2] != '-') {
      value = a.substr(2);
    } else {
      std::cerr << "argv: ignored argument: " << a << std::endl;
      continue;
    }
  }
  if (value.empty()) {
    return Glib::get_user_config_dir() + "/3DViewer.ini";
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
    worker_ = std::jthread{
		sigc::mem_fun(*this, &MainWindow::create_and_save_gif), gl_canvas_.get_width() * viewport_scale, gl_canvas_.get_height() * viewport_scale
	};
	std::cerr << "[worker_ thread] started GIF creation\n";
	viewport_size_unlock();
    return false;
  }
  if (rotate) ctl_.RotateByDeg(ctl::Axis::Y, kPreviewYawDeg);
  screencast_frames_.push_back(gl_canvas_.CaptureFrame());
  return true;
}

bool MainWindow::on_autosave_timeout() {
  // if worker_2_ is already executing at this point, (i.e the thread is joinable)
  // then a stop is requested and the thread joins. (worker_2_'s destructor is
  // called here because of assignment operator.)
  worker_2_ = std::jthread{
	[this]() {
	  if (ctl_.SaveSettings())
		  std::cerr << "[worker_2_ thread] check autosave event: cfg updated\n";
  	}};
  // by always returning true we can keep the timer
  // running for the whole lifetime of the main thread
  return true;
}

void MainWindow::viewport_size_lock(){
  btn_style_ctls_.set_sensitive(false), btn_tsfm_ctls_.set_sensitive(false);
  gl_canvas_.set_expand(false);
  set_resizable(false);
}
void MainWindow::viewport_size_unlock(){
  set_resizable();
  gl_canvas_.set_expand();
  btn_tsfm_ctls_.set_sensitive(), btn_style_ctls_.set_sensitive();
}
