#include <gui/glarea_widget.h>

#include <iostream>
#include <memory>

using namespace ec::view;

GLAreaWidget::GLAreaWidget() {
  set_size_request(600, 600);
  set_can_focus(false);
  set_can_target(true);
  set_expand(false);

  set_required_version(3, 3);
  set_has_depth_buffer(true);
  set_auto_render(false);

  signal_realize().connect(sigc::mem_fun(*this, &GLAreaWidget::onRealize));
  signal_unrealize().connect(sigc::mem_fun(*this, &GLAreaWidget::onUnrealize),
                             false);
  signal_render().connect(sigc::mem_fun(*this, &GLAreaWidget::onRender), false);
  signal_resize().connect(sigc::mem_fun(*this, &GLAreaWidget::onResize));
}

void GLAreaWidget::SetScene(model::SceneSettings* scene) { scene_ = scene; }

void GLAreaWidget::UploadModel(const model::Mesh& m) {
  if (!get_realized() || renderer_ == nullptr) return;
  make_current();
  renderer_->UploadModel(m);
  model_uploaded_ = true;
  queue_render();
}

Vector<uint8_t> GLAreaWidget::CaptureFrame() {
  if (!get_realized() || renderer_ == nullptr) {
    std::cerr << "Framebuffer not available: GLArea not realized" << std::endl;
    return {};
  }
  make_current();
  return renderer_->Snapshot();
}

void GLAreaWidget::onRealize() {
  make_current();
  try {
    renderer_ = std::make_unique<view::GLRenderer>();
    throw_if_error();
  } catch (const Gdk::GLError& gle) {
    std::cerr << "GL error during realize: " << gle.domain() << "-"
              << gle.code() << "-" << gle.what() << std::endl;
  }
}

void GLAreaWidget::onUnrealize() {
  make_current();
  renderer_.reset();
}

bool GLAreaWidget::onRender(const Glib::RefPtr<Gdk::GLContext>&) {
  // Nothing loaded: no GL calls at all.
  if (!model_uploaded_ || renderer_ == nullptr || scene_ == nullptr)
    return true;
  renderer_->Render(scene_->mvp(), scene_->style());
  return true;
}

void GLAreaWidget::onResize(int w, int h) {
  if (w <= 0 || h <= 0) return;
  if (on_resized) on_resized(w, h);
  make_current();
  glViewport(0, 0, w, h);
  queue_render();
}
