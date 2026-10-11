#ifndef GUI_SHADER_MANAGER_H_
#define GUI_SHADER_MANAGER_H_

#include <epoxy/gl.h>
#include <model/transform.h>

#include <unordered_map>
#include <string>
namespace ec::view {

using std::unordered_map;

class ShaderMngr {
 public:
  ShaderMngr() = default;
  explicit ShaderMngr(const char* vertShaderCode, const char* fragShaderCode);
  ~ShaderMngr() {
    if (ID_) glDeleteProgram(ID_);
  }
  ShaderMngr(const ShaderMngr&) = delete;
  ShaderMngr& operator=(const ShaderMngr&) = delete;

  ShaderMngr(ShaderMngr&& o) noexcept
      : ID_(o.ID_),
        uniform_locations_(std::move(o.uniform_locations_)),
        initialized_(o.initialized_) {
    o.ID_ = 0;
    o.initialized_ = false;
  }

  ShaderMngr& operator=(ShaderMngr&& o) noexcept {
    if (this != &o) {
      if (ID_) glDeleteProgram(ID_);
      ID_ = o.ID_;
      uniform_locations_ = std::move(o.uniform_locations_);
      initialized_ = o.initialized_;
      o.ID_ = 0;
      o.initialized_ = false;
    }
    return *this;
  }
  void checkCompileErrors(unsigned int shader, std::string type);
  bool has_initialized() const noexcept { return initialized_; }
  GLuint get_program_id() const noexcept { return ID_; }

  void set_u_mat4(const std::string& uniform_name, const model::MyMat4& v,
                  bool row_major = true) {
    auto it = uniform_locations_.find(uniform_name);
    if (it != uniform_locations_.end())
      glUniformMatrix4fv(it->second, 1, static_cast<GLboolean>(row_major),
                         v.data());
  }
  void set_u_float4(const std::string& uniform_name, const float v1,
                    const float v2, const float v3, const float v4) {
    auto it = uniform_locations_.find(uniform_name);
    if (it != uniform_locations_.end()) glUniform4f(it->second, v1, v2, v3, v4);
  }

  void set_u_float(const char* name,
                   float v) {  //////////////////////////////////
    glUniform1f(glGetUniformLocation(get_program_id(), name), v);
  }

  void set_u_int(const char* name, int v) {
    glUniform1i(glGetUniformLocation(get_program_id(), name), v);
  }  /////////////////////////

 private:
  GLuint ID_ = 0;
  unordered_map<std::string, GLint> uniform_locations_;
  bool initialized_ = false;
};
}  // namespace ec::view
#endif
