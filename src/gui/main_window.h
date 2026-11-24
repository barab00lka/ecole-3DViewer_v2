#ifndef GUI_MAIN_WINDOW_H_
#define GUI_MAIN_WINDOW_H_

#include <containers/ec_list.h>
#include <controller/scene_controller.h>
#include <gdkmm/rgba.h>
#include <glibmm/miscutils.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/eventcontrollerscroll.h>
#include <gtkmm/frame.h>
#include <gtkmm/headerbar.h>
#include <gtkmm/label.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/overlay.h>
#include <gtkmm/popovermenu.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/revealer.h>
#include <gtkmm/stack.h>
#include <gtkmm/stackswitcher.h>
#include <gtkmm/togglebutton.h>
#include <gui/glarea_widget.h>
#include <gui/hud_widget.h>
#include <misc/logger.h>

#include <thread>

namespace ec::view {

// The order of colors has to be the same as in SceneController::ColorTarget
enum ColorTarget {
  kBgColor,
  kVtColor,
  kEdgeColor,
  kObjectColor,
  kLightColor,
  kColorsCount
};
enum Axis_ { kAxisX, kAxisY, kAxisZ, kAxisCount };

using ec::List;

class MainWindow : public Gtk::ApplicationWindow {
  static constexpr guint kHideHudKey = GDK_KEY_i;
  static constexpr char kDefaultConfig[] = "3DViewer.ini";
  static constexpr int kGifFps = 10;
  static constexpr int kGifDurationSeconds = 5;
  static constexpr int kGifTotalFrames = kGifDurationSeconds * kGifFps;
  static constexpr int kGifFrameIntervalMs = 1000 / kGifFps;
  static constexpr int kGifFrameIntervalCs = 100 / kGifFps;
  static constexpr int kAutoSaveIntervalMs = 5000;
  static constexpr double kPreviewYawDeg =
      360.0 / (kGifFps * kGifDurationSeconds);

 public:
  explicit MainWindow(int argc, char* argv[]);
  ~MainWindow() = default;

 protected:
  void onRotateAround(Axis_ axis, const Glib::RefPtr<Gtk::Adjustment>& adj);
  void onMove(Axis_ axis, const Glib::RefPtr<Gtk::Adjustment> p_adj);
  void onScaleChange(const Glib::RefPtr<Gtk::Adjustment>& adj);
  void onOpenClicked();
  void onColorSet(ColorTarget target, const Gdk::RGBA& c);
  void onSaveScreenshot();
  void onSaveScreencast(bool rotate = false);
  void onTextureBtnClicked();
  void onSaveUVMap();

  Gtk::Box* createAxisSliderBox(Axis_ axis);
  Gtk::Box* createMovementField(Axis_ axis,
                                Glib::RefPtr<Gtk::Adjustment>& p_adj,
                                const float initial = 0.0f);

  Gtk::Box* createStylingControls();
  Gtk::Box* createAffineControls();
  Gtk::PopoverMenu* createScreencastMenu();
  Gtk::Box* createColorRow(const Glib::ustring& label, const Gdk::RGBA& initial,
                           ColorTarget color_target, bool with_alpha = false);
  static Glib::RefPtr<Gtk::EventControllerKey> make_key_controller(
      std::pair<guint, std::function<void()>> binding);

 private:
  void setup_gesture_controls();
  void sync_sliders();
  void create_and_save_gif(const int px_w, const int px_h,
                           const std::string& path);
  void set_scene_color(ColorTarget t, const Gdk::RGBA& c);
  Gdk::RGBA get_scene_color(ColorTarget t);
  std::string config_path_from_args(int argc, char** argv);
  bool on_framecapture_timeout(bool rotate = false);
  // TODO: aleciari - some details
  // add --autosave-timeout cmdline arg (in seconds).
  // 0=disable  autosaving completely, negative=error
  bool on_autosave_timeout();
  void on_toggle_wireframe_mode(bool enable = true);
  void viewport_size_lock();
  void viewport_size_unlock();

 private:
  // Controller owns SceneSettings + Mesh. View talks only to it.
  ctl::SceneController ctl_;

  // Presenter area
  GLAreaWidget gl_canvas_;
  HUDWidget overlay_{kHideHudKey, gl_canvas_};

  // Controls
  Gtk::Button btn_file_{"Open"}, btn_tex_load_{"Apply texture"},
      btn_uv_save_{"Save UV map"};
  Gtk::ToggleButton btn_reveal_sidebar_;

  // We save the Adjustments because
  // changes to Affine by GestureDrag are
  // reflected in the adjustments
  Glib::RefPtr<Gtk::Adjustment> obj_pitch_, obj_yaw_, obj_roll_, obj_X_, obj_Y_,
      obj_Z_, obj_scale_, light_X_, light_Y_, light_Z_, light_R_, light_G_,
      light_B_;

  // Screencast menu
  Gtk::MenuButton btn_save_;
  Gtk::Button btn_screencast_{"Screencast (GIF 5s)"},
      btn_screenshot_{"Screenshot (JPEG / BMP)"},
      btn_screencast_preview_{"Preview of the model rotating (GIF 5s)"};

  Gtk::ProgressBar screencast_bar_;
  List<Vector<uint8_t>> screencast_frames_;
  sigc::connection screencast_timer_{};
  int screencast_frames_left_ = 0;
  std::jthread worker_;
};

}  // namespace ec::view

#endif
