#ifndef GUI_HUD_WIDGET_H
#define GUI_HUD_WIDGET_H
#include <glibmm/markup.h>
#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/label.h>
#include <gtkmm/overlay.h>

#include <format>
#include <string>

namespace ec::view {

using std::string;
using std::to_string;
// Model info overlay
// displays:
// 1) filename
// 2) model size in Kilobytes
// 3) count of vertices
// 5) count of facets
// 4) count of edges
class HUDWidget : public Gtk::Overlay {
 public:
  explicit HUDWidget(const guint hide_key, Gtk::Widget& underlay)
      : hide_key_(hide_key) {
    set_child(underlay);
    content_.set_wrap(true);
    content_.set_visible(true);
    content_.set_can_focus(false);
    content_.set_can_target(false);
    content_.set_margin(5);

    hide_ctl_ = Gtk::EventControllerKey::create();
    hide_ctl_->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    hide_ctl_->signal_key_pressed().connect(
        sigc::mem_fun(*this, &HUDWidget::onHideKeyPressed), false);

    add_overlay(content_);
  }
  void set_info_text(const string text) {
    model_info_ = text;
    refresh_markup();
  }
  // automatically prepends with a newline
  void append_info_text(const string text) {
    model_info_ += '\n';
    model_info_ += text;
  }
  void set_text_color(const Gdk::RGBA& c) {
    char hex[8];
    std::snprintf(hex, sizeof(hex), "#%02x%02x%02x",
                  static_cast<int>(c.get_red() * 255),
                  static_cast<int>(c.get_green() * 255),
                  static_cast<int>(c.get_blue() * 255));
    text_col_hex_ = hex;
    refresh_markup();
  }
  Glib::RefPtr<Gtk::EventControllerKey> get_controller() const {
    return hide_ctl_;
  }

 protected:
  void refresh_markup() {
    content_.set_markup("<span foreground='" + text_col_hex_ + "'>" +
                        Glib::Markup::escape_text(model_info_) + "</span>");
  }
  bool onHideKeyPressed(guint kv, guint, Gdk::ModifierType) {
    if (kv != hide_key_) return false;
    content_.set_visible(!content_.get_visible());
    return true;
  }

 private:
  guint hide_key_;
  string model_info_ = "No model loaded.";
  string text_col_hex_ = "#ffffff";
  Gtk::Label content_{model_info_, Gtk::Align::START, Gtk::Align::START};
  Glib::RefPtr<Gtk::EventControllerKey> hide_ctl_;
};
}  // namespace ec::view
#endif
