#ifndef GUI_GL_RENDERER_H_
#define GUI_GL_RENDERER_H_
#include <containers/my_vector.h>
#include <epoxy/gl.h>
#include <gui/glsl-embed.h>
#include <gui/shader_manager.h>
#include <model/scene.h>
#include <model/wireframe_mesh.h>

namespace ec::view {
using ec::Vector;

// Данный класс инкапсулирует в себе всю работу с видеокартой через OpenGL
class GLRenderer {
 public:
  GLRenderer() : shader_(kVertexShaderSrc, kFragmentShaderSrc) {
    glUseProgram(shader_.get_program_id());
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  };
  GLRenderer(const GLRenderer& o) = delete;
  ~GLRenderer() { DeleteBuffers(); }

 public:
  // takes width and height of the whole frame by default
  Vector<uint8_t> Snapshot(int width = -1, int height = -1);
  void UploadModel(const model::Mesh& m);
  void Render(const model::MyMat4& mvp, const model::Appearance& style_attr);

 protected:
  void DeleteBuffers() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
  }

 private:
  ShaderMngr shader_;
  GLuint VAO = 0, VBO = 0, EBO = 0;
  GLsizei ebo_index_count_ = 0, vbo_vertex_count_ = 0;
};
}  // namespace ec::view
#endif
