#include <gui/shader_manager.h>
#include <misc/logger.h>

#include <cassert>
#include <format>

#ifndef NDEBUG
#define ASSERT_EX(condition, statement) \
  do {                                  \
    if (!(condition)) {                 \
      statement;                        \
      assert(condition);                \
    }                                   \
  } while (false)
#else
#define ASSERT_EX(condition, statement) ((void)0)
#endif

namespace ec::view {
ShaderMngr::ShaderMngr(const char* vertShaderCode, const char* fragShaderCode) {
  assert(vertShaderCode && "Vertex shader code is NULL");
  assert(fragShaderCode && "Fragment shader code is NULL");
  unsigned int vertex, fragment;
  // Vertex shader
  vertex = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex, 1, &vertShaderCode, NULL);
  glCompileShader(vertex);
  checkCompileErrors(vertex, "VERTEX");
  // Fragment Shader
  fragment = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment, 1, &fragShaderCode, NULL);
  glCompileShader(fragment);
  checkCompileErrors(fragment, "FRAGMENT");
  // Shader Program
  ID_ = glCreateProgram();
  glAttachShader(ID_, vertex);
  glAttachShader(ID_, fragment);
  glLinkProgram(ID_);
  checkCompileErrors(ID_, "PROGRAM");
  initialized_ = true;

  // Delete the shaders as they're linked into our program now and no longer
  // necessary
  glDeleteShader(vertex);
  glDeleteShader(fragment);

  LOG_INFO("model", "Shaders compiled successfully");
}

void ShaderMngr::checkCompileErrors(unsigned int shader, std::string type) {
  int success;
  char infoLog[1024];
  if (type != "PROGRAM") {
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(shader, 1024, NULL, infoLog);
      LOG_INFO("model",
               std::format("ERROR::SHADER_COMPILATION_ERROR of type: {}\n {}",
                           type, infoLog));
    }
  } else {
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(shader, 1024, NULL, infoLog);
      LOG_INFO(
          "model",
          std::format("ERROR::SHADER_PROGRAM_LINKING_ERROR of type: {}\n {}",
                      type, infoLog));
    }
  }
}

GLint ShaderMngr::retrieveUniform(const std::string& uniform_name) {
  if (uniform_locations_.contains(uniform_name)) {
    return uniform_locations_[uniform_name];
  }
  GLint loc = glGetUniformLocation(ID_, uniform_name.c_str());
  ASSERT_EX(loc != -1, LOG_INFO("model", std::format("Uniform not found: {}",
                                                     uniform_name)));
  return loc;
}

void ShaderMngr::set_u_mat3(const std::string& uniform_name, const float* v) {
  glUniformMatrix3fv(retrieveUniform(uniform_name), 1, GL_FALSE, v);
}
void ShaderMngr::set_u_mat4(const std::string& uniform_name, const float* v,
                            bool row_major) {
  glUniformMatrix4fv(retrieveUniform(uniform_name), 1,
                     static_cast<GLboolean>(row_major), v);
}
void ShaderMngr::set_u_float3(const std::string& uniform_name, const float v1,
                              const float v2, const float v3) {
  glUniform3f(retrieveUniform(uniform_name), v1, v2, v3);
}

void ShaderMngr::set_u_float(const std::string& name, const float v) {
  glUniform1f(retrieveUniform(name), v);
}

void ShaderMngr::set_u_int(const std::string& name, int v) {
  glUniform1i(retrieveUniform(name), v);
}
}  // namespace ec::view
