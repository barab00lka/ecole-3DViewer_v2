#include <gui/shader_manager.h>

#include <cassert>
#include <iostream>

using namespace ec::view;
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

  // Cache uniforms for later use
  uniform_locations_["uMVP"] = glGetUniformLocation(ID_, "uMVP");
  uniform_locations_["uColor"] = glGetUniformLocation(ID_, "uColor");
  assert(uniform_locations_["uColor"] != -1);
  assert(uniform_locations_["uMVP"] != -1);
  initialized_ = true;

  // Delete the shaders as they're linked into our program now and no longer
  // necessary
  glDeleteShader(vertex);
  glDeleteShader(fragment);

  std::cerr << "Shaders compiled successfully" << std::endl;
}

void ShaderMngr::checkCompileErrors(unsigned int shader, std::string type) {
  int success;
  char infoLog[1024];
  if (type != "PROGRAM") {
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(shader, 1024, NULL, infoLog);
      std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                << infoLog << std::endl;
    }
  } else {
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(shader, 1024, NULL, infoLog);
      std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
                << infoLog << std::endl;
    }
  }
}
