#include <gui/main_window.h>
namespace ec::view {

// Screencast dropdown menu:
// screenshot: bmp / jpeg;
// screencast: 5s at 10fps converted to gif
// screencast with model preview: 5s at 10fps converted to gif, model spins
// around Y axis
Gtk::PopoverMenu* MainWindow::createScreencastMenu() {
  auto* menu = Gtk::make_managed<Gtk::PopoverMenu>();
  auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
  box->set_margin(6);

  btn_screenshot_.signal_clicked().connect([this, menu]() {
    menu->popdown();
    onSaveScreenshot();
  });
  btn_screencast_.signal_clicked().connect([this, menu]() {
    menu->popdown();
    onSaveScreencast(false);
  });
  btn_screencast_preview_.signal_clicked().connect([this, menu]() {
    menu->popdown();
    onSaveScreencast(true);
  });
  box->append(btn_screenshot_);
  box->append(btn_screencast_);
  box->append(btn_screencast_preview_);

  menu->set_child(*box);

  return menu;
}
}  // namespace ec::view
