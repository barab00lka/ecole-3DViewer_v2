#include <fast_obj.h>
#include <model/wireframe_mesh.h>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace ec::model {
Mesh Mesh::load_from_obj(const std::string& filename) {
  if (filename.empty())
    throw std::invalid_argument("Couldn't load model from file!");

  fastObjMesh* mesh = fast_obj_read(filename.c_str());
  if (!mesh) {
    throw std::invalid_argument("Failed to load ");
  }
  std::cout << "fastobj loaded: pos count=" << mesh->position_count - 1
            << ", face count=" << mesh->face_count
            << ", index count=" << mesh->index_count << '\n';

  Mesh model_mesh_{};
  model_mesh_.vertices.reserve(mesh->position_count);
  for (unsigned i = 1; i < mesh->position_count; ++i) {
    model_mesh_.vertices.push_back({mesh->positions[i * 3 + 0],
                                    mesh->positions[i * 3 + 1],
                                    mesh->positions[i * 3 + 2]});
  }
  model_mesh_.vertices.shrink_to_fit();

  size_t idx = 0;
  model_mesh_.facets.reserve(mesh->face_count * 3);
  for (unsigned f = 0; f < mesh->face_count; ++f) {
    unsigned n = mesh->face_vertices[f];

    if (n < 3) {
      idx += n;
      continue;
    }

    uint32_t p0 = mesh->indices[idx].p;
    if (p0 == 0) {
      idx += n;
      continue;
    }

    uint32_t v0 = p0 - 1;
    for (unsigned k = 1; k + 1 < n; ++k) {
      uint32_t pa = mesh->indices[idx + k].p;
      uint32_t pb = mesh->indices[idx + k + 1].p;
      if (pa == 0 || pb == 0) continue;

      model_mesh_.facets.push_back(v0);
      model_mesh_.facets.push_back(pa - 1);
      model_mesh_.facets.push_back(pb - 1);
    }
    idx += n;
  }
  model_mesh_.facets.shrink_to_fit();

  model_mesh_.fname = filename;
  model_mesh_.size_kib =
      static_cast<double>(std::filesystem::file_size(filename)) / (1024.0);
  ;
  model_mesh_.facet_count = mesh->face_count;
  model_mesh_.vertex_count = mesh->position_count - 1;
  auto [c, r] = model_mesh_.calc_center_and_radius();
  model_mesh_.center_ = c;
  model_mesh_.radius_ = (r > 1e-6f) ? r : 1.0f;

  fast_obj_destroy(mesh);
  return model_mesh_;
}

std::pair<Vec3D, float> Mesh::calc_center_and_radius() const {
  if (vertices.empty()) {
    return {{0, 0, 0}, 0.0f};
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

  Vec3D center{(min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f,
               (min.z + max.z) * 0.5f};

  float dx = max.x - min.x;
  float dy = max.y - min.y;
  float dz = max.z - min.z;
  float radius = 0.5f * std::sqrt(dx * dx + dy * dy + dz * dz);

  return {center, radius};
}
}  // namespace ec::model
