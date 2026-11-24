#ifndef GUI_GLAREA_WIDGET_H_
#define GUI_GLAREA_WIDGET_H_

#include <containers/ec_vector.h>
#include <gtkmm/glarea.h>
#include <gtkmm/widget.h>
#include <gui/gl_renderer.h>
#include <model/scene.h>
#include <model/texture.h>
#include <model/wireframe_mesh.h>

#include <functional>
#include <memory>

namespace ec::view {
using ec::Vector;

class GLAreaWidget : public Gtk::GLArea {
 public:
  GLAreaWidget();
  ~GLAreaWidget() override = default;

  void SetScene(model::SceneSettings* scene);
  void UploadModel(const model::Mesh& m);
  Vector<uint8_t> CaptureFrame();

  // t = nullptr clears (unloads) current texure.
  void UploadTexture(const model::Texture* t) {
    if (!get_realized() || renderer_ == nullptr) return;
    // make_current();
    if (t) {
      renderer_->UploadTexture(*t);
    } else {
      renderer_->ClearTexture();
    }
    queue_render();
  }

  std::function<void(int, int)> on_resized;

 protected:
  void onRealize();
  void onUnrealize();
  bool onRender(const Glib::RefPtr<Gdk::GLContext>&);
  void onResize(int w, int h);

 private:
  std::unique_ptr<GLRenderer> renderer_;
  model::SceneSettings* scene_ = nullptr;
  bool model_uploaded_ = false;
};

}  // namespace ec::view

#endif
