#ifndef MODEL_WIREFRAME_MESH_H_
#define MODEL_WIREFRAME_MESH_H_

#include <containers/ec_vector.h>
#include <model/vec3.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace ec::model {
using ec::Vector;
using std::string;

struct VertexAttributes {
  // position
  Vec3D Position;
  // normal
  Vec3D Normal;
  // texCoords
  struct Vec2D {
    float x, y;
  } TexCoords;
  // // tangent
  // Vec3D Tangent;
  // // bitangent (computed at runtime on GPU)
  // Vec3D Bitangent;
};

struct Mesh {
  static Mesh load_from_obj(const std::string& filename);
  std::size_t count_uniq_edges() const;
  std::pair<Vec3D, float> calc_center_and_radius() const;

  // Computes smooth vertex normals by averaging face normals of adjacent
  // triangles. Used when the OBJ file does not provide 'vn' normals.
  void compute_vertex_normals();

  // Data members
  std::string fname = {};
  double size_kib = 0;
  // mesh Data
  Vector<VertexAttributes> vertices = {};
  Vector<uint32_t> facets = {};
  std::size_t polygon_count = 0, vertex_count = 0, edge_count = 0;
  Vec3D center = {0, 0, 0};
  float radius = .0f;
};
}  // namespace ec::model
#endif
