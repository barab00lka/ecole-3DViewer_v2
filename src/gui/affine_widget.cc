#include <gtkmm/spinbutton.h>
#include <gui/main_window.h>
namespace ec::view {

Gtk::Box* MainWindow::createAffineControls() {
  auto* ctls = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
  ctls->set_margin(8);
  ctls->set_spacing(6);

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

  // Reset affine transformations button
  {
    auto* btn_reset = Gtk::make_managed<Gtk::Button>("Reset");
    btn_reset->set_tooltip_text(
        "Reset affine transformations to initial parameters");
    btn_reset->signal_clicked().connect([this]() { ctl_.ResetAffine(); });
    ctls->append(*btn_reset);
  }

  // Rotate
  {
    auto* rot_frame = Gtk::make_managed<Gtk::Frame>("Rotate");
    rot_frame->set_label_align(Gtk::Align::CENTER);
    auto* rot_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
    rot_box->set_margin(6);
    rot_frame->set_child(*rot_box);
    for (int i = 0; i < kAxisCount; ++i) {
      rot_box->append(*createAxisSliderBox(static_cast<Axis_>(i)));
    }
    ctls->append(*rot_frame);
  }

  // Scale
  {
    auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    box->append(*Gtk::make_managed<Gtk::Label>("Scale "));
    box->set_spacing(10);
    box->set_halign(Gtk::Align::END);

    obj_scale_ = Gtk::Adjustment::create(
        ctl::SceneController::kScaleBase, ctl::SceneController::kScaleMin,
        ctl::SceneController::kScaleMax, 0.1, 1.0);
    obj_scale_->signal_value_changed().connect(sigc::bind(
        sigc::mem_fun(*this, &MainWindow::onScaleChange), obj_scale_));

    auto* spin = Gtk::make_managed<Gtk::SpinButton>(obj_scale_, 0.1, 3);
    spin->set_numeric(true);
    spin->set_wrap(false);
    spin->set_width_chars(-1);
    box->append(*spin);
    ctls->append(*box);
  }

  // Move object
  auto* mv_frame = Gtk::make_managed<Gtk::Frame>("Move");
  mv_frame->set_label_align(Gtk::Align::CENTER);
  auto* mv_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
  mv_box->set_margin(6);
  mv_frame->set_child(*mv_box);
  mv_box->append(*createMovementField(kAxisX, obj_X_));
  mv_box->append(*createMovementField(kAxisY, obj_Y_));
  mv_box->append(*createMovementField(kAxisZ, obj_Z_));
  ctls->append(*mv_frame);

  // Lightsource
  auto* l_frame = Gtk::make_managed<Gtk::Frame>("Lightsource");
  l_frame->set_label_align(Gtk::Align::CENTER);
  auto* l_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
  l_box->set_margin(6);
  l_frame->set_child(*l_box);
  const model::Vec3D c_pos = ctl_.GetLightPos();
  l_box->append(*createMovementField(kAxisX, light_X_, c_pos.x));
  l_box->append(*createMovementField(kAxisY, light_Y_, c_pos.y));
  l_box->append(*createMovementField(kAxisZ, light_Z_, c_pos.z));
  ctls->append(*l_frame);

  return ctls;
}
}  // namespace ec::view
