#ifndef MODEL_APPEARANCE_H_
#define MODEL_APPEARANCE_H_
#include <model/color.h>

namespace ec::model {

using std::string;
enum class EdgeStyle {
  Solid,
  Dashed,
};
enum class VertexStyle {
  None,
  Circles,
  Squares,
};
enum class ShadingStyle {
  None,
  Flat,
  Smooth,
};
// Display method of edges { solid, dashed }
// Thickness of edges
// Size of verteces
// Display method of verteces { circles, squares, none }
struct Appearance {
  ColorRGBA col_bg = {0.2f, 0.3f, 0.3f, 1.0f};    // teal
  ColorRGBA col_vt = {1.0f, 0.2f, 0.2f, 1.0f};    // red
  ColorRGBA col_edge = {0.2f, 0.3f, 1.0f, 1.0f};  // blue
  ColorRGBA col_obj = {1.0f, 1.0f, 0.2f, 1.0f};
  ColorRGBA col_lighting = {1.0f, 1.0f, 1.0f, 1.0f};  // white

  float line_size = 1.0f;
  float vertex_size = 5.0f;

  EdgeStyle style_lines;
  VertexStyle style_vt;
  ShadingStyle obj_shading;
};
}  // namespace ec::model
#endif
