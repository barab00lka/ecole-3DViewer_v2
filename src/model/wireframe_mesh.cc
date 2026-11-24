#include <fast_obj.h>  // for fastObjIndex, fastObjMesh, fast_obj_destroy, fast_obj_read
#include <misc/logger.h>
#include <model/wireframe_mesh.h>

#include <cmath>  // for INFINITY, sqrt
#include <format>
#include <set>        // for std::set (edge deduplication)
#include <stdexcept>  // for invalid_argument

#include "model/vec3.h"  // for Vec3D

namespace ec::model {
namespace {

using ObjPtr = std::unique_ptr<fastObjMesh, decltype(&fast_obj_destroy)>;

template <class Emit>
void for_each_triangle(const fastObjMesh& m, Emit emit) {
  const fastObjIndex* corner = m.indices;
  for (unsigned f = 0; f < m.face_count; ++f) {
    const unsigned n = m.face_vertices[f];
    for (unsigned k = 1; k + 1 < n; ++k) {
      const uint32_t a = corner[0].p;
      const uint32_t b = corner[k].p;
      const uint32_t c = corner[k + 1].p;
      if (a && b && c) emit(a - 1, b - 1, c - 1);
    }
    corner += n;
  }
}

Vector<uint32_t> first_normal_per_position(const fastObjMesh& m) {
  Vector<uint32_t> out(m.position_count);
  for (unsigned i = 0; i < m.index_count; ++i) {
    const auto& c = m.indices[i];
    if (c.p && c.n && out[c.p] == 0) out[c.p] = c.n;
  }
  return out;
}

Vector<uint32_t> first_texcoord_per_position(const fastObjMesh& m) {
  Vector<uint32_t> out(m.position_count);
  for (unsigned i = 0; i < m.index_count; ++i) {
    const auto& c = m.indices[i];
    if (c.p && c.t && out[c.p] == 0) out[c.p] = c.t;
  }
  return out;
}

}  // namespace

Mesh Mesh::load_from_obj(const std::string& filename) {
  if (filename.empty()) {
    throw std::invalid_argument("Couldn't load model: empty file name");
  }

  ObjPtr obj(fast_obj_read(filename.c_str()), fast_obj_destroy);
  if (!obj) {
    throw std::invalid_argument(
        std::format("Failed to load model: {}\n", filename));
  }
  const fastObjMesh& m = *obj;
  LOG_INFO(
      "model",
      std::format("fastobj: pos {}, polygons {}, normals {}, texcoords {}\n",
                  m.position_count - 1, m.face_count, m.normal_count - 1,
                  m.texcoord_count - 1));

  const unsigned n_positions =
      (m.position_count > 1) ? m.position_count - 1 : 0;

  const bool has_normals = m.normal_count > 1;
  const bool has_tex = m.texcoord_count > 1;
  const auto normal_of =
      has_normals ? first_normal_per_position(m) : Vector<uint32_t>{};
  const auto tex_of =
      has_tex ? first_texcoord_per_position(m) : Vector<uint32_t>{};

  Mesh out{.fname = filename,
           .polygon_count = m.face_count,
           .vertex_count = n_positions};

  out.vertices.reserve(n_positions);
  for (unsigned i = 1; i < m.position_count; ++i) {
    VertexAttributes v{};
    v.Position = {m.positions[3 * i + 0], m.positions[3 * i + 1],
                  m.positions[3 * i + 2]};

    if (has_normals && normal_of[i]) {
      const unsigned n = normal_of[i];
      v.Normal = {m.normals[3 * n + 0], m.normals[3 * n + 1],
                  m.normals[3 * n + 2]};
    }

    if (has_tex && tex_of[i]) {
      const unsigned t = tex_of[i];
      v.TexCoords = {m.texcoords[2 * t + 0], m.texcoords[2 * t + 1]};
    }

    out.vertices.push_back(v);
  }

  out.facets.reserve(m.index_count);
  for_each_triangle(m, [&](uint32_t a, uint32_t b, uint32_t c) {
    out.facets.insert_many_back(a, b, c);
  });

  // Compute vertex normals when the OBJ file does not provide them.
  if (!has_normals) {
    LOG_INFO("model", "Normal coordinates not found in OBJ. Calculating...");
    out.compute_vertex_normals();
  }
  return out;
}

void Mesh::compute_vertex_normals() {
  Vector<Vec3D> accumulators(vertices.size());

  // Accumulate face normals for every triangle that shares a vertex.
  for (size_t i = 0; i + 2 < facets.size(); i += 3) {
    const uint32_t a = facets[i];
    const uint32_t b = facets[i + 1];
    const uint32_t c = facets[i + 2];

    const Vec3D& pa = vertices[a].Position;
    const Vec3D& pb = vertices[b].Position;
    const Vec3D& pc = vertices[c].Position;

    const Vec3D edge1 = pb - pa;
    const Vec3D edge2 = pc - pa;
    const Vec3D face_normal = edge1.cross(edge2);

    accumulators[a] = accumulators[a] + face_normal;
    accumulators[b] = accumulators[b] + face_normal;
    accumulators[c] = accumulators[c] + face_normal;
  }

  // Normalize accumulated normals per vertex.
  for (size_t i = 0; i < vertices.size(); ++i) {
    const float len = accumulators[i].length();
    if (len > 1e-6f) {
      vertices[i].Normal = accumulators[i] * (1.0f / len);
    } else {
      vertices[i].Normal = {0.f, 0.f, 1.f};
    }
  }
}

std::size_t Mesh::count_uniq_edges() const {
  // Count unique (undirected) edges across all triangles.
  std::set<std::pair<uint32_t, uint32_t>> unique_edges;
  for (size_t i = 0; i + 2 < facets.size(); i += 3) {
    uint32_t a = facets[i];
    uint32_t b = facets[i + 1];
    uint32_t c = facets[i + 2];
    auto add_edge = [&](uint32_t x, uint32_t y) {
      if (x > y) {
        unique_edges.insert({y, x});
      } else {
        unique_edges.insert({x, y});
      }
    };
    add_edge(a, b);
    add_edge(b, c);
    add_edge(c, a);
  }
  return unique_edges.size();
}

std::pair<Vec3D, float> Mesh::calc_center_and_radius() const {
  if (vertices.empty()) {
    return {};
  }

  Vec3D min = {INFINITY, INFINITY, INFINITY};
  Vec3D max = {-INFINITY, -INFINITY, -INFINITY};

  for (auto i = size_t{}; i < vertices.size(); ++i) {
    if (vertices[i].Position.x < min.x) min.x = vertices[i].Position.x;
    if (vertices[i].Position.y < min.y) min.y = vertices[i].Position.y;
    if (vertices[i].Position.z < min.z) min.z = vertices[i].Position.z;

    if (vertices[i].Position.x > max.x) max.x = vertices[i].Position.x;
    if (vertices[i].Position.y > max.y) max.y = vertices[i].Position.y;
    if (vertices[i].Position.z > max.z) max.z = vertices[i].Position.z;
  }

  const float dx = max.x - min.x, dy = max.y - min.y, dz = max.z - min.z;

  Vec3D center = {(min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f,
                  (min.z + max.z) * 0.5f};
  float radius = 0.5f * std::sqrt(dx * dx + dy * dy + dz * dz);
  if (radius <= 0) radius = 1.0f;
  return {center, radius};
}
}  // namespace ec::model
