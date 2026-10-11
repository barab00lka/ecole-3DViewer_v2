#ifndef MODEL_WIREFRAME_MESH_H_
#define MODEL_WIREFRAME_MESH_H_

#include <containers/my_vector.h>
#include <model/vec3.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace ec::model {
using ec::Vector;
using std::string;

struct VertexAttributes {
  // position
  Vec3D Position;
  // // normal
  // Vec3D Normal;
  // // texCoords
  // glm::vec2 TexCoords;
  // // tangent
  // Vec3D Tangent;
  // // bitangent
  // Vec3D Bitangent;
};

struct Texture {
  uint32_t id;
  string type;
  string path;
};

struct Mesh {
  static Mesh load_from_obj(const std::string& filename);
  const Vec3D& get_center() const { return center_; }
  float get_radius() const { return radius_; }
  // TODO logic to count unique edges
  std::pair<Vec3D, float> calc_center_and_radius() const;

  // Data members
  std::string fname;
  double size_kib;
  // mesh Data
  Vector<VertexAttributes> vertices = {};
  Vector<uint32_t> facets = {};
  std::size_t facet_count = 0, vertex_count = 0;
  // Vector<Texture>      textures;
  Vec3D center_ = {0, 0, 0};
  float radius_ = .0f;
};
}  // namespace ec::model
#endif
