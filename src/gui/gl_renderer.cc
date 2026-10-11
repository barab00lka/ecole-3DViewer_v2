#include <gui/gl_renderer.h>

using namespace ec::view;

Vector<uint8_t> GLRenderer::Snapshot(int width, int height) {
  int x, y;
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  x = viewport[0], y = viewport[1];
  if (width == -1){
	  width = viewport[2];
  }
  else {
	  assert(width >= viewport[2]);
  }
  if (height == -1){
	height = viewport[3];
  }
  else {
 	assert(height >= viewport[3]);
  }
  Vector<uint8_t> pixels(width * height * 3);
  glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

  return pixels;
}

void GLRenderer::Render(const model::MyMat4& mvp,
                        const model::Appearance& style) {
  glClearBufferfv(GL_COLOR, 0, reinterpret_cast<const GLfloat*>(&style.col_bg));
  const GLfloat d = 1.0f;
  glClearBufferfv(GL_DEPTH, 0, &d);

  glUseProgram(shader_.get_program_id());
  shader_.set_u_mat4("uMVP", mvp);

  glBindVertexArray(VAO);

  shader_.set_u_float4("uColor", style.col_edge.r, style.col_edge.g,
                       style.col_edge.b, style.col_edge.a);

  shader_.set_u_int("uDrawEdgesDashed",  ///////////////////////////////
                    style.style_lines == model::EdgeStyle::Dashed ? 1 : 0);
  shader_.set_u_float("uPatternLength",
                      12.0f);  /////////////////////////////////

  //    glPolygonMode(GL_FRONT_AND_BACK, style.style_lines ==
  //    Model::EdgeStyle::Solid
  //                                         ? GL_LINE
  //                                         : GL_POINT);

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  glLineWidth(style.line_size);
  glDrawElements(GL_TRIANGLES, ebo_index_count_, GL_UNSIGNED_INT, 0);

  shader_.set_u_int("uDrawEdgesDashed", 0);  //////////////////////////////////

  shader_.set_u_float4("uColor", style.col_vt.r, style.col_vt.g, style.col_vt.b,
                       style.col_vt.a);
  glDisable(GL_DEPTH_TEST);

  glPointSize(style.vertex_size);

  switch (style.style_vt) {
    case model::VertexStyle::Squares:
      //			glDisable(GL_POINT_SMOOTH);
      shader_.set_u_int("uRound", 0);  /////////////////////////////
      glDrawArrays(GL_POINTS, 0,
                   vbo_vertex_count_);  //////////////////////////////
      break;
    case model::VertexStyle::Circles:
      //			glEnable(GL_POINT_SMOOTH);
      //            glUniform1i(uRound, style.style_vt ==
      //            Model::VertexStyle::Circles); /////////////////////////
      shader_.set_u_int("uRound", 1);
      glDrawArrays(GL_POINTS, 0, vbo_vertex_count_);  ////////////////////////
      break;
    default:
      break;
  }

  shader_.set_u_int("uRound", 0);  //////////////////////////

  glEnable(GL_DEPTH_TEST);
  glBindVertexArray(0);
  glUseProgram(0);
}
void GLRenderer::UploadModel(const model::Mesh& mesh) {
  if (mesh.vertex_count == 0 || mesh.facet_count == 0) return;
  // assert(mesh.facet_count == mesh.facets.size());
  ebo_index_count_ = static_cast<GLsizei>(mesh.facets.size());
  vbo_vertex_count_ = static_cast<GLsizei>(mesh.vertex_count);

  DeleteBuffers();
  glCreateVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);

  // A great thing about structs is that their memory layout is sequential for
  // all its items. The effect is that we can simply pass a pointer to the
  // struct and it translates perfectly to a glm::vec3/2 array which again
  // translates to 3/2 floats which translates to a byte array.
  glBufferData(GL_ARRAY_BUFFER,
               vbo_vertex_count_ * sizeof(model::VertexAttributes),
               &mesh.vertices[0], GL_STATIC_DRAW);

  glGenBuffers(1, &EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, ebo_index_count_ * sizeof(GLuint),
               &mesh.facets[0], GL_STATIC_DRAW);

  // set the vertex attribute pointers
  // vertex Positions
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                        sizeof(model::VertexAttributes),
                        reinterpret_cast<const void*>(0));
  // // vertex normals
  // glEnableVertexAttribArray(1);
  // glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAttributes),
  // reinterpret_cast<const void*>(offsetof(VertexAttributes, Normal));
  // // vertex texture coords
  // glEnableVertexAttribArray(2);
  // load data into vertex buffers
  // glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexAttributes),
  // (void*)offsetof(VertexAttributes, TexCoords));
  // // vertex tangent
  // glEnableVertexAttribArray(3);
  // glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAttributes),
  // (void*)offsetof(VertexAttributes, Tangent));
  // // vertex bitangent
  // glEnableVertexAttribArray(4);
  // glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAttributes),
  // (void*)offsetof(VertexAttributes, Bitangent));

  // ------State cleanup----------
  // note that this is allowed, the call to glVertexAttribPointer registered
  // VBO as the vertex attribute's bound vertex buffer object so afterwards we
  // can safely unbind
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // remember: do NOT unbind the EBO while a VAO is active as the bound
  // element buffer object IS stored in the VAO; keep the EBO bound.
  // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  // You can unbind the VAO afterwards so other VAO calls won't accidentally
  // modify this VAO, but this rarely happens. Modifying other VAOs requires a
  // call to glBindVertexArray anyways so we generally don't unbind VAOs (nor
  // VBOs) when it's not directly necessary.
  glBindVertexArray(0);

  // now when VAO is inactive, we can unbind the EBO
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
