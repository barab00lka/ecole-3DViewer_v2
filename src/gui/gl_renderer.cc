#include <epoxy/gl.h>
#include <gui/gl_renderer.h>
#include <misc/logger.h>

#include <cstddef>
#include <memory>
#ifndef NDEBUG
#include <format>
#endif

namespace ec::view {

Vector<uint8_t> GLRenderer::Snapshot(int width, int height) {
  int x, y;
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  x = viewport[0], y = viewport[1];
  if (width == -1) {
    width = viewport[2];
  } else {
    assert(width >= viewport[2]);
  }
  if (height == -1) {
    height = viewport[3];
  } else {
    assert(height >= viewport[3]);
  }
  Vector<uint8_t> pixels(width * height * kFramebufferChannels);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
  LOG_INFO("Renderer",
           std::format("read px from framebuf; w:{} h:{}", width, height));
  return pixels;
}

void GLRenderer::RenderFrame(model::SceneSettings* const cur_scene) {
  const auto& style = cur_scene->style();
  glClearBufferfv(
      GL_COLOR, 0,
      reinterpret_cast<const GLfloat*>(std::addressof(style.col_bg)));
  const GLfloat d = 1.0f;
  glClearBufferfv(GL_DEPTH, 0, &d);
  glUseProgram(shader_.get_program_id());
  glBindVertexArray(VAO);

  shader_.set_u_mat4("uMVP", cur_scene->mvp_mat4());
  shader_.set_u_mat4("uModelMatrix", cur_scene->model_mat4());

  if (style.obj_shading == model::ShadingStyle::None) {  // wireframe mode
    const bool draw_dashed = style.style_lines == model::EdgeStyle::Dashed;
    shader_.set_u_int("isWireframe", true);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(style.line_size);
    shader_.set_u_int("uDashedEdges", draw_dashed);
    shader_.set_u_float3("uColor", style.col_edge.r, style.col_edge.g,
                         style.col_edge.b);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wireframe_EBO);
    glDrawElements(GL_LINES, wireframe_index_count_, GL_UNSIGNED_INT, 0);
    // glDrawElements(GL_LINES, ebo_index_count_, GL_UNSIGNED_INT, 0);
    if (draw_dashed) {
      shader_.set_u_int("uDashedEdges", false);
    }
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
  } else {  // shaded mode
    const auto& lightsource_pos = cur_scene->light_offset();
    shader_.set_u_int("isWireframe", false);
    shader_.set_u_float3("uLightPos", lightsource_pos.x, lightsource_pos.y,
                         lightsource_pos.z);
    shader_.set_u_float3("uLightColor", style.col_lighting.r,
                         style.col_lighting.g, style.col_lighting.b);

    shader_.set_u_mat4("uNormalMatrix", cur_scene->normal_mat4());
    const auto& cp = cur_scene->cam_pos();
    shader_.set_u_float3("uViewPos", cp.x, cp.y, cp.z);
    if (style.obj_shading == model::ShadingStyle::Flat) {
      shader_.set_u_int("uFlatShading", 1);
    } else {
      shader_.set_u_int("uFlatShading", 0);
    }

    // Texture: always tell the shader what to do this frame. uTex defaults
    // to unit 0; uHasTex is what actually gates sampling.
    shader_.set_u_int("uHasTex", tex_id_ ? 1 : 0);
    if (tex_id_) {
      glActiveTexture(GL_TEXTURE0);
      shader_.set_u_int("uTex", 0);
      glBindTexture(GL_TEXTURE_2D, tex_id_);
    } else {
      shader_.set_u_float3("uColor", style.col_obj.r, style.col_obj.g,
                           style.col_obj.b);
    }

    glEnable(GL_DEPTH_TEST);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glDrawElements(GL_TRIANGLES, ebo_index_count_, GL_UNSIGNED_INT, 0);
  }

  // verteces (points)
  switch (style.style_vt) {
    case model::VertexStyle::None:
      goto cleanup;
    case model::VertexStyle::Squares:
      shader_.set_u_int("uCircleVertices", false);
      break;
    case model::VertexStyle::Circles:
      shader_.set_u_int("uCircleVertices", true);
      break;
  }
  shader_.set_u_float3("uColor", style.col_vt.r, style.col_vt.g,
                       style.col_vt.b);
  shader_.set_u_float("uPointSize", style.vertex_size);
  glEnable(GL_PROGRAM_POINT_SIZE);
  glDrawArrays(GL_POINTS, 0, vbo_vertex_count_);

cleanup:
  shader_.set_u_int("uCircleVertices", false);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
  glUseProgram(0);
}

void GLRenderer::UploadModel(const model::Mesh& mesh) {
  if (mesh.vertex_count == 0 || mesh.polygon_count == 0) return;
  ebo_index_count_ = static_cast<GLsizei>(mesh.facets.size());
  vbo_vertex_count_ = static_cast<GLsizei>(mesh.vertex_count);

  DeleteBuffers();
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER,
               vbo_vertex_count_ * sizeof(model::VertexAttributes),
               &mesh.vertices[0], GL_STATIC_DRAW);

  glGenBuffers(1, &EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, ebo_index_count_ * sizeof(GLuint),
               &mesh.facets[0], GL_STATIC_DRAW);

  // Build wireframe index buffer: each triangle -> 3 lines (6 indices)
  Vector<GLuint> wireframe_indices;
  wireframe_indices.reserve(mesh.facets.size() * 2);
  for (size_t i = 0; i + 2 < mesh.facets.size(); i += 3) {
    GLuint a = mesh.facets[i];
    GLuint b = mesh.facets[i + 1];
    GLuint c = mesh.facets[i + 2];
    wireframe_indices.push_back(a);
    wireframe_indices.push_back(b);
    wireframe_indices.push_back(b);
    wireframe_indices.push_back(c);
    wireframe_indices.push_back(c);
    wireframe_indices.push_back(a);
  }
  glGenBuffers(1, &wireframe_EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wireframe_EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               wireframe_indices.size() * sizeof(GLuint),
               wireframe_indices.data(), GL_STATIC_DRAW);
  wireframe_index_count_ = static_cast<GLsizei>(wireframe_indices.size());

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                        sizeof(model::VertexAttributes),
                        reinterpret_cast<const void*>(
                            offsetof(model::VertexAttributes, Position)));

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(
      1, 3, GL_FLOAT, GL_FALSE, sizeof(model::VertexAttributes),
      reinterpret_cast<const void*>(offsetof(model::VertexAttributes, Normal)));

  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                        sizeof(model::VertexAttributes),
                        reinterpret_cast<const void*>(
                            offsetof(model::VertexAttributes, TexCoords)));

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GLRenderer::UploadTexture(const model::Texture& t) {
  if (t.pixels.empty() || t.width <= 0 || t.height <= 0) return;

  if (!tex_id_) glGenTextures(1, &tex_id_);
  glBindTexture(GL_TEXTURE_2D, tex_id_);

  // 3 bytes per pixel, rows tightly packed — no alignment padding.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, t.width, t.height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, t.pixels.data());

  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glBindTexture(GL_TEXTURE_2D, 0);
}

void GLRenderer::ClearTexture() {
  if (tex_id_) glDeleteTextures(1, &tex_id_);
  tex_id_ = 0;
}

}  // namespace ec::view
