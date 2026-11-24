#include <gtkmm/dropdown.h>
#include <gtkmm/object.h>
#include <gtkmm/popovermenu.h>
#include <gtkmm/scale.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/stringlist.h>
#include <gui/main_window.h>

using ec::view::MainWindow;

Gtk::Box* MainWindow::createStylingControls() {
  // LEFT: Styling controls widget (Revealer)
  auto* styling_box =
      Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
  styling_box->set_margin(8);
  styling_box->set_hexpand(false);

  // Colors
  styling_box->append(
      *createColorRow("Background", get_scene_color(kBgColor), kBgColor));

  // Vertices styling
  auto* vt_frame = Gtk::make_managed<Gtk::Frame>("Vertices");
  vt_frame->set_label_align(Gtk::Align::CENTER);
  auto* vt_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
  vt_frame->set_child(*vt_box);
  vt_box->set_margin(6);
  auto* vt_size_row =
      Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
  vt_size_row->set_halign(Gtk::Align::END);
  vt_size_row->append(*Gtk::make_managed<Gtk::Label>("Size"));

  auto vt_size_adj = Gtk::Adjustment::create(5.0, 1.0, 50.0, 1.0, 5.0, 0.0);
  vt_size_adj->set_value(ctl_.style().vertex_size);
  auto* vt_size_spin = Gtk::make_managed<Gtk::SpinButton>(vt_size_adj, 1.0, 0);
  vt_size_spin->set_numeric(true);
  vt_size_spin->set_wrap(false);
  vt_size_spin->set_width_chars(-1);
  vt_size_row->append(*vt_size_spin);

  vt_size_adj->signal_value_changed().connect([this, vt_size_adj]() {
    ctl_.SetVertexSize(static_cast<float>(vt_size_adj->get_value()));
  });

  vt_box->append(*vt_size_row);
  auto vt_box_c = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 4);
  auto* vt_style_d = Gtk::make_managed<Gtk::DropDown>(
      Gtk::StringList::create({"None", "Circles", "Squares"}));
  vt_style_d->set_selected(static_cast<guint>(ctl_.style().style_vt));
  vt_style_d->property_selected().signal_changed().connect(
      [this, vt_style_d]() {
        const guint idx = vt_style_d->get_selected();
        if (idx == GTK_INVALID_LIST_POSITION) return;
        ctl_.SetVertexStyle(static_cast<model::VertexStyle>(idx));
      });
  vt_box_c->append(*vt_style_d);
  vt_box_c->append(*createColorRow("", get_scene_color(kVtColor), kVtColor));
  vt_box->append(*vt_box_c);
  styling_box->append(*vt_frame);

  // Texture load / unload
  btn_tex_load_.signal_clicked().connect(
      sigc::mem_fun(*this, &MainWindow::onTextureBtnClicked));
  styling_box->append(btn_tex_load_);

  // Save UV map (visible only when a model is loaded)
  btn_uv_save_.set_visible(false);
  btn_uv_save_.signal_clicked().connect(
      sigc::mem_fun(*this, &MainWindow::onSaveUVMap));
  styling_box->append(btn_uv_save_);

  // Edges
  auto* edge_frame = Gtk::make_managed<Gtk::Frame>("Edges");
  edge_frame->set_label_align(Gtk::Align::CENTER);
  auto* edge_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
  edge_box->set_margin(6);
  edge_frame->set_child(*edge_box);
  auto* edge_width_row =
      Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
  edge_width_row->append(*Gtk::make_managed<Gtk::Label>("Width"));
  edge_width_row->set_halign(Gtk::Align::END);

  auto edge_width_adj = Gtk::Adjustment::create(1.0, 0.5, 20.0, 0.5, 2.0, 0.0);
  edge_width_adj->set_value(ctl_.style().line_size);

  auto* edge_width_spin =
      Gtk::make_managed<Gtk::SpinButton>(edge_width_adj, 0.5, 1);
  edge_width_spin->set_numeric(true);
  edge_width_spin->set_wrap(false);
  edge_width_spin->set_width_chars(-1);
  edge_width_row->append(*edge_width_spin);

  edge_width_adj->signal_value_changed().connect([this, edge_width_adj]() {
    ctl_.SetLineSize(static_cast<float>(edge_width_adj->get_value()));
  });

  edge_box->append(*edge_width_row);
  auto edge_box_c =
      Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 4);

  auto* edge_style_d = Gtk::make_managed<Gtk::DropDown>(
      Gtk::StringList::create({"Solid", "Dashed"}));
  edge_style_d->set_selected(static_cast<guint>(ctl_.style().style_lines));
  edge_style_d->property_selected().signal_changed().connect(
      [this, edge_style_d]() {
        const guint idx = edge_style_d->get_selected();
        if (idx == GTK_INVALID_LIST_POSITION) return;
        ctl_.SetEdgeStyle(static_cast<model::EdgeStyle>(idx));
      });
  edge_box_c->append(*edge_style_d);
  edge_box_c->append(
      *createColorRow("", get_scene_color(kEdgeColor), kEdgeColor));
  edge_box->append(*edge_box_c);

  styling_box->append(*edge_frame);

  // Shaded object styling (non-wireframe)
  auto* obj_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
  // Object
  obj_box->append(*createColorRow("Object color", get_scene_color(kObjectColor),
                                  kObjectColor));

  // Lightsource
  auto* l_frame = Gtk::make_managed<Gtk::Frame>("Lightsource");
  l_frame->set_label_align(Gtk::Align::CENTER);
  auto* l_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
  l_box->set_margin(6);
  l_frame->set_child(*l_box);
  // Light color R G B
  const auto l_cl = ctl_.GetColor(ctl::SceneController::ColorTarget::Light);

  auto* r_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 4);
  auto* label_ = Gtk::make_managed<Gtk::Label>("R");
  r_box->append(*label_);
  light_R_ = Gtk::Adjustment::create(l_cl.r, 0.0, 1.0, 0.1, 1.0);
  light_R_->signal_value_changed().connect([this]() {
    auto target = ctl::SceneController::ColorTarget::Light;
    auto o = ctl_.GetColor(target);
    auto r_val = static_cast<float>(light_R_->get_value());
    ctl_.SetColor(target, {r_val, o.g, o.b});
  });
  auto slider_r =
      Gtk::make_managed<Gtk::Scale>(light_R_, Gtk::Orientation::HORIZONTAL);
  slider_r->set_hexpand();
  r_box->append(*slider_r);
  l_box->append(*r_box);

  auto* b_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 4);
  auto* b_label_ = Gtk::make_managed<Gtk::Label>("B");
  b_box->append(*b_label_);
  light_B_ = Gtk::Adjustment::create(l_cl.b, 0.0, 1.0, 0.1, 1.0);
  light_B_->signal_value_changed().connect([this]() {
    auto target = ctl::SceneController::ColorTarget::Light;
    auto o = ctl_.GetColor(target);
    ctl_.SetColor(target,
                  {o.r, o.g, static_cast<float>(light_B_->get_value())});
  });
  auto slider_b =
      Gtk::make_managed<Gtk::Scale>(light_B_, Gtk::Orientation::HORIZONTAL);
  slider_b->set_hexpand();
  b_box->append(*slider_b);
  l_box->append(*b_box);

  auto* g_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 4);
  auto* g_label_ = Gtk::make_managed<Gtk::Label>("G");
  g_box->append(*g_label_);
  light_G_ = Gtk::Adjustment::create(l_cl.g, 0.0, 1.0, 0.1, 1.0);
  light_G_->signal_value_changed().connect([this]() {
    auto target = ctl::SceneController::ColorTarget::Light;
    auto o = ctl_.GetColor(target);
    ctl_.SetColor(target,
                  {o.r, static_cast<float>(light_G_->get_value()), o.b});
  });
  auto slider_g =
      Gtk::make_managed<Gtk::Scale>(light_G_, Gtk::Orientation::HORIZONTAL);
  slider_g->set_hexpand();
  g_box->append(*slider_g);
  l_box->append(*g_box);

  obj_box->append(*l_frame);

  styling_box->append(*obj_box);

  // Type of object display
  auto* frame = Gtk::make_managed<Gtk::Frame>("Object display mode");
  frame->set_label_align(Gtk::Align::CENTER);
  auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
  box->set_margin(6);
  auto rb1 = Gtk::make_managed<Gtk::CheckButton>("Wireframe mode"),
       rb2 = Gtk::make_managed<Gtk::CheckButton>("Flat shading"),
       rb3 = Gtk::make_managed<Gtk::CheckButton>("Smooth shading");
  rb1->signal_toggled().connect([this, rb1, edge_frame, obj_box]() {
    if (rb1->get_active()) ctl_.SetDisplayType(model::ShadingStyle::None);
    edge_frame->set_visible();
    obj_box->set_visible(false);
  });
  rb2->signal_toggled().connect([this, rb2, edge_frame, obj_box]() {
    if (rb2->get_active()) ctl_.SetDisplayType(model::ShadingStyle::Flat);
    obj_box->set_visible();
    edge_frame->set_visible(false);
  });
  rb3->signal_toggled().connect([this, rb3, edge_frame, obj_box]() {
    if (rb3->get_active()) ctl_.SetDisplayType(model::ShadingStyle::Smooth);
    obj_box->set_visible();
    edge_frame->set_visible(false);
  });

  switch (ctl_.GetDisplayType()) {
    case model::ShadingStyle::Flat:
      rb2->set_active();
      break;
    case model::ShadingStyle::Smooth:
      rb3->set_active();
      break;
    default:
      rb1->set_active();
  }
  rb2->set_group(*rb1), rb3->set_group(*rb1);
  box->append(*rb1), box->append(*rb2), box->append(*rb3);
  frame->set_child(*box);

  styling_box->append(*frame);
  styling_box->reorder_child_at_start(*frame);

  return styling_box;
}
