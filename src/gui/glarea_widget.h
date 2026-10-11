#ifndef GUI_GLAREA_WIDGET_H_
#define GUI_GLAREA_WIDGET_H_

#include <containers/my_vector.h>
#include <gtkmm/glarea.h>
#include <gtkmm/widget.h>
#include <gui/gl_renderer.h>
#include <model/scene.h>
#include <model/wireframe_mesh.h>

#include <functional>
#include <memory>

namespace ec::view {
using ec::Vector;

// Dumb GL view. Holds no model logic.
// Reads scene (mvp, style) to draw. Never writes scene.
// Reports user-visible events (resize) via callback.
class GLAreaWidget : public Gtk::GLArea {
 public:
  GLAreaWidget();
  ~GLAreaWidget() override = default;

  // Controller is the owner of scene. Read-only use here.
  void SetScene(model::SceneSettings* scene);
  void UploadModel(const model::Mesh& m);
  Vector<uint8_t> CaptureFrame();

  // Set by MainWindow -> ctl::SetViewport.
  std::function<void(int, int)> on_resized;

 protected:
  void onRealize();
  void onUnrealize();
  bool onRender(const Glib::RefPtr<Gdk::GLContext>&);
  void onResize(int w, int h);

 private:
  std::unique_ptr<GLRenderer> renderer_;
  model::SceneSettings* scene_ = nullptr;  // non-owning
  bool model_uploaded_ = false;            // false at startup: draw nothing
};

}  // namespace ec::view

#endif
