#include <gtkmm.h>
#include <gui/main_window.h>

int main(int argc, char** argv) {
  auto app = Gtk::Application::create("com.aleciari.viewer");
  // Gio::Application::Flags::NON_UNIQUE makes it possible to run several
  // instances of this application simultaneously.
  int argc1 = 1;  // Don't give the command line arguments to Gtk::Application.
  return app->make_window_and_run<ec::view::MainWindow>(argc1, argv, argc,
                                                         argv);
}
