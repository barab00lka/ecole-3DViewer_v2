#include <gtest/gtest.h>
#include <model/camera.h>
#include <model/color.h>
#include <model/mat4.h>
#include <model/wireframe_mesh.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace ec::model {
namespace {

namespace fs = std::filesystem;

// RAII-хелпер: создаёт временный .obj файл, удаляет его в деструкторе.
class TempObj {
 public:
  explicit TempObj(const std::string& content,
                   const std::string& name = "test.obj") {
    path_ = fs::temp_directory_path() /
            (name + "_" + std::to_string(::getpid()) + ".obj");
    std::ofstream out(path_);
    out << content;
  }
  ~TempObj() {
    std::error_code ec;
    fs::remove(path_, ec);
  }
  TempObj(const TempObj&) = delete;
  TempObj& operator=(const TempObj&) = delete;

  std::string path() const { return path_.string(); }

 private:
  fs::path path_;
};

constexpr float kEps = 1e-4f;
bool Near(float a, float b) { return std::fabs(a - b) < kEps; }

// Tests for wireframe_mesh.cc
//
//  ============================================================
//   calc_center_and_radius
//  ============================================================

TEST(MeshCalcCenterRadius, EmptyVerticesReturnsZero) {
  Mesh m;
  auto [c, r] = m.calc_center_and_radius();
  EXPECT_TRUE(Near(c.x, 0.f));
  EXPECT_TRUE(Near(c.y, 0.f));
  EXPECT_TRUE(Near(c.z, 0.f));
  EXPECT_TRUE(Near(r, 0.f));
}

TEST(MeshCalcCenterRadius, SingleVertexRadiusIsZero) {
  Mesh m;
  m.vertices.push_back({{1.f, 2.f, 3.f}});

  auto [c, r] = m.calc_center_and_radius();
  EXPECT_TRUE(Near(c.x, 1.f));
  EXPECT_TRUE(Near(c.y, 2.f));
  EXPECT_TRUE(Near(c.z, 3.f));
  EXPECT_TRUE(Near(r, 0.f));
}

TEST(MeshCalcCenterRadius, SymmetricCube) {
  Mesh m;
  m.vertices = {
      {{-1.f, -1.f, -1.f}}, {{1.f, -1.f, -1.f}}, {{-1.f, 1.f, -1.f}},
      {{1.f, 1.f, -1.f}},   {{-1.f, -1.f, 1.f}}, {{1.f, -1.f, 1.f}},
      {{-1.f, 1.f, 1.f}},   {{1.f, 1.f, 1.f}},
  };

  auto [c, r] = m.calc_center_and_radius();
  EXPECT_TRUE(Near(c.x, 0.f));
  EXPECT_TRUE(Near(c.y, 0.f));
  EXPECT_TRUE(Near(c.z, 0.f));
  // Диагональ = 2*sqrt(3), радиус = sqrt(3)
  EXPECT_TRUE(Near(r, std::sqrt(3.f)));
}

TEST(MeshCalcCenterRadius, AsymmetricBox) {
  Mesh m;
  m.vertices = {
      {{0.f, 0.f, 0.f}},
      {{2.f, 0.f, 0.f}},
      {{0.f, 2.f, 0.f}},
      {{0.f, 0.f, 2.f}},
  };

  auto [c, r] = m.calc_center_and_radius();
  EXPECT_TRUE(Near(c.x, 1.f));
  EXPECT_TRUE(Near(c.y, 1.f));
  EXPECT_TRUE(Near(c.z, 1.f));
  // Диагональ = 2*sqrt(3), радиус = sqrt(3)
  EXPECT_TRUE(Near(r, std::sqrt(3.f)));
}

TEST(MeshCalcCenterRadius, AllNegativeCoordinates) {
  Mesh m;
  m.vertices = {
      {{-10.f, -10.f, -10.f}},
      {{-2.f, -2.f, -2.f}},
  };

  auto [c, r] = m.calc_center_and_radius();
  EXPECT_TRUE(Near(c.x, -6.f));
  EXPECT_TRUE(Near(c.y, -6.f));
  EXPECT_TRUE(Near(c.z, -6.f));
  // Диагональ = 8*sqrt(3), радиус = 4*sqrt(3)
  EXPECT_TRUE(Near(r, 4.f * std::sqrt(3.f)));
}

// ============================================================
//  load_from_obj
// ============================================================

TEST(MeshLoadFromObj, EmptyFilenameThrows) {
  EXPECT_THROW(Mesh::load_from_obj(""), std::invalid_argument);
}

TEST(MeshLoadFromObj, NonExistentFileThrows) {
  EXPECT_THROW(Mesh::load_from_obj("/nonexistent/path/xyz.obj"),
               std::invalid_argument);
}

TEST(MeshLoadFromObj, SimpleTriangle) {
  TempObj f(
      "v 0 0 0\n"
      "v 1 0 0\n"
      "v 0 1 0\n"
      "f 1 2 3\n");

  Mesh m = Mesh::load_from_obj(f.path());

  EXPECT_EQ(m.vertices.size(), 3u);
  EXPECT_EQ(m.facets.size(), 3u);
  EXPECT_EQ(m.facets[0], 0u);
  EXPECT_EQ(m.facets[1], 1u);
  EXPECT_EQ(m.facets[2], 2u);
}

TEST(MeshLoadFromObj, VertexCoordinatesAreCorrect) {
  TempObj f(
      "v 1.5 -2.25 3.75\n"
      "v -1 -1 -1\n"
      "v 0 0 0\n"
      "f 1 2 3\n");

  Mesh m = Mesh::load_from_obj(f.path());

  ASSERT_EQ(m.vertices.size(), 3u);
  EXPECT_TRUE(Near(m.vertices[0].Position.x, 1.5f));
  EXPECT_TRUE(Near(m.vertices[0].Position.y, -2.25f));
  EXPECT_TRUE(Near(m.vertices[0].Position.z, 3.75f));
  EXPECT_TRUE(Near(m.vertices[1].Position.x, -1.f));
}

TEST(MeshLoadFromObj, QuadIsTriangulated) {
  TempObj f(
      "v 0 0 0\n"
      "v 1 0 0\n"
      "v 1 1 0\n"
      "v 0 1 0\n"
      "f 1 2 3 4\n");

  Mesh m = Mesh::load_from_obj(f.path());

  EXPECT_EQ(m.vertices.size(), 4u);
  // Четырёхугольник → 2 треугольника → 6 индексов
  EXPECT_EQ(m.facets.size(), 6u);
  // Первый треугольник: 0 1 2
  EXPECT_EQ(m.facets[0], 0u);
  EXPECT_EQ(m.facets[1], 1u);
  EXPECT_EQ(m.facets[2], 2u);
  // Второй треугольник: 0 2 3 (fan triangulation)
  EXPECT_EQ(m.facets[3], 0u);
  EXPECT_EQ(m.facets[4], 2u);
  EXPECT_EQ(m.facets[5], 3u);
}

TEST(MeshLoadFromObj, SetsFilename) {
  TempObj f(
      "v 0 0 0\n"
      "v 1 0 0\n"
      "v 0 1 0\n"
      "f 1 2 3\n");

  Mesh m = Mesh::load_from_obj(f.path());
  EXPECT_EQ(m.fname, f.path());
}

TEST(MeshLoadFromObj, SizeKibMatchesFileSize) {
  const std::string content =
      "v 0 0 0\n"
      "v 1 0 0\n"
      "v 0 1 0\n"
      "f 1 2 3\n";

  TempObj f(content);
  const double expected = static_cast<double>(fs::file_size(f.path())) / 1024.0;

  Mesh m = Mesh::load_from_obj(f.path());
  EXPECT_NEAR(m.size_kib, expected, 1e-9);
}

TEST(MeshLoadFromObj, CubeCenterAndRadius) {
  TempObj f(
      "v -1 -1 -1\n"
      "v  1 -1 -1\n"
      "v  1  1 -1\n"
      "v -1  1 -1\n"
      "v -1 -1  1\n"
      "v  1 -1  1\n"
      "v  1  1  1\n"
      "v -1  1  1\n"
      "f 1 2 3 4\n"
      "f 5 6 7 8\n"
      "f 1 2 6 5\n"
      "f 2 3 7 6\n"
      "f 3 4 8 7\n"
      "f 4 1 5 8\n");

  Mesh m = Mesh::load_from_obj(f.path());

  EXPECT_TRUE(Near(m.center_.x, 0.f));
  EXPECT_TRUE(Near(m.center_.y, 0.f));
  EXPECT_TRUE(Near(m.center_.z, 0.f));
  EXPECT_TRUE(Near(m.radius_, std::sqrt(3.f)));
}

TEST(MeshLoadFromObj, RadiusFallbackForDegenerateModel) {
  // Все три вершины в одной точке → radius == 0 → fallback на 1.0
  TempObj f(
      "v 5 5 5\n"
      "v 5 5 5\n"
      "v 5 5 5\n"
      "f 1 2 3\n");

  Mesh m = Mesh::load_from_obj(f.path());
  EXPECT_TRUE(Near(m.radius_, 1.0f));
}

// ============================================================
//  Покрытие защитных веток в load_from_obj
// ============================================================

// Ветка `n < 3`: грань с 2 вершинами не должна триангулироваться
TEST(MeshLoadFromObj, FaceWithTwoVerticesIsSkipped) {
  TempObj f(
      "v 0 0 0\n"
      "v 1 0 0\n"
      "v 0 1 0\n"
      "f 1 2\n");  // ← всего 2 вершины

  Mesh m = Mesh::load_from_obj(f.path());

  EXPECT_EQ(m.vertices.size(), 3u);
  EXPECT_EQ(m.facets.size(), 0u);  // грань пропущена, треугольников нет
}

// Ветка `n < 3`: грань с 1 вершиной
TEST(MeshLoadFromObj, FaceWithOneVertexIsSkipped) {
  TempObj f(
      "v 0 0 0\n"
      "v 1 0 0\n"
      "v 0 1 0\n"
      "f 1\n");

  Mesh m = Mesh::load_from_obj(f.path());

  EXPECT_EQ(m.vertices.size(), 3u);
  EXPECT_EQ(m.facets.size(), 0u);
}

// Ветка `n < 3`: валидная грань, потом битая — обе должны обработаться
TEST(MeshLoadFromObj, MixedValidAndTwoVertexFaces) {
  TempObj f(
      "v 0 0 0\n"
      "v 1 0 0\n"
      "v 0 1 0\n"
      "v 1 1 0\n"
      "f 1 2 3\n"    // валидная
      "f 2 4\n"      // 2 вершины → пропуск
      "f 1 3 4\n");  // валидная

  Mesh m = Mesh::load_from_obj(f.path());

  EXPECT_EQ(m.vertices.size(), 4u);
  EXPECT_EQ(m.facets.size(), 6u);  // два треугольника по 3 индекса
}

// Ветка `p0 == 0`: грань, у которой первый индекс = 0
TEST(MeshLoadFromObj, FaceWithZeroFirstIndexIsSkipped) {
  TempObj f(
      "v 0 0 0\n"
      "v 1 0 0\n"
      "v 0 1 0\n"
      "f 0 1 2\n");  // ← p0 = 0

  Mesh m = Mesh::load_from_obj(f.path());

  EXPECT_EQ(m.vertices.size(), 3u);
  EXPECT_EQ(m.facets.size(), 0u);  // грань пропущена
}

// Ветка `p0 != 0`, но pa/pb == 0 (проверка `pa == 0 || pb == 0`)
TEST(MeshLoadFromObj, FaceWithZeroMiddleIndexSkipsTriangle) {
  TempObj f(
      "v 0 0 0\n"
      "v 1 0 0\n"
      "v 0 1 0\n"
      "v 1 1 0\n"
      "f 1 0 3 4\n");  // p0=1 (ок), pa=0 → continue, треугольник пропущен

  Mesh m = Mesh::load_from_obj(f.path());

  EXPECT_EQ(m.vertices.size(), 4u);
  // p0 != 0, но во внутреннем цикле pa==0 → оба треугольника пропущены
  EXPECT_EQ(m.facets.size(), 0u);
}

// ============================================================
//  Грани с < 3 вершинами → continue
// ============================================================

TEST(MeshLoadFromObj, TwoVertexFaceIsSkipped) {
  TempObj f(
      "v 0 0 0\n"
      "v 1 0 0\n"
      "v 0 1 0\n"
      "f 1 2\n");  // 2 вершины

  Mesh m = Mesh::load_from_obj(f.path());
  EXPECT_EQ(m.vertices.size(), 3u);
  EXPECT_EQ(m.facets.size(), 0u);
}

TEST(MeshLoadFromObj, OneVertexFaceIsSkipped) {
  TempObj f(
      "v 0 0 0\n"
      "v 1 0 0\n"
      "v 0 1 0\n"
      "f 1\n");

  Mesh m = Mesh::load_from_obj(f.path());
  EXPECT_EQ(m.facets.size(), 0u);
}

// ============================================================
//  Грани с p0 == 0 → continue
// ============================================================

TEST(MeshLoadFromObj, FaceStartingWithZeroIsSkipped) {
  TempObj f(
      "v 0 0 0\n"
      "v 1 0 0\n"
      "v 0 1 0\n"
      "f 0 1 2\n");  // p0 == 0

  Mesh m = Mesh::load_from_obj(f.path());
  EXPECT_EQ(m.vertices.size(), 3u);
  EXPECT_EQ(m.facets.size(), 0u);
}

// ============================================================
//  Грани со средним/последним индексом 0 → continue во внутреннем цикле
// ============================================================

TEST(MeshLoadFromObj, FaceWithZeroMiddleIndexProducesNoTriangles) {
  TempObj f(
      "v 0 0 0\n"
      "v 1 0 0\n"
      "v 0 1 0\n"
      "v 1 1 0\n"
      "f 1 0 3 4\n");  // p0=1 (ок), но pa=0

  Mesh m = Mesh::load_from_obj(f.path());
  // p0 != 0, но в fan-триангуляции pa==0 → все треугольники пропущены
  EXPECT_EQ(m.facets.size(), 0u);
}

TEST(MeshLoadFromObj, FaceWithZeroLastIndexIsRejected) {
  TempObj f(
      "v 0 0 0\n"
      "v 1 0 0\n"
      "v 0 1 0\n"
      "v 1 1 0\n"
      "f 1 2 3 0\n");

  Mesh m = Mesh::load_from_obj(f.path());

  // fast_obj отбрасывает грани с индексом 0 на этапе парсинга
  EXPECT_EQ(m.vertices.size(), 4u);
  EXPECT_EQ(m.facets.size(), 0u);  // ← было 6u, стало 0u
}

// ============================================================
//  Смешанный OBJ: валидные + мусорные грани
// ============================================================

TEST(MeshLoadFromObj, MixOfValidAndInvalidFaces) {
  TempObj f(
      "v 0 0 0\n"
      "v 1 0 0\n"
      "v 0 1 0\n"
      "v 1 1 0\n"
      "f 1 2 3\n"    // валидный треугольник
      "f 1 2\n"      // n < 3 → skip
      "f 0 3 4\n"    // p0 == 0 → skip
      "f 2 3 4\n");  // валидный треугольник

  Mesh m = Mesh::load_from_obj(f.path());
  EXPECT_EQ(m.vertices.size(), 4u);
  EXPECT_EQ(m.facets.size(), 6u);  // два треугольника по 3 индекса
}

// Tests for camera.cc
//
//  constexpr float kEps = 1e-4f;
//  bool Near(float a, float b, float eps = kEps) {
//    return std::fabs(a - b) < eps;
//  }

// ============================================================
//  Camera::fit_to_scene
// ============================================================

TEST(CameraFitToScene, SetsTargetRadiusAndDistance) {
  Camera cam;
  cam.fit_to_scene({1.f, 2.f, 3.f}, 5.f);

  EXPECT_TRUE(Near(cam.get_target().x, 1.f));
  EXPECT_TRUE(Near(cam.get_target().y, 2.f));
  EXPECT_TRUE(Near(cam.get_target().z, 3.f));
  EXPECT_TRUE(Near(cam.get_scene_radius(), 5.f));
  EXPECT_TRUE(Near(cam.get_distance(), 12.5f));  // 2.5 * 5
}

TEST(CameraFitToScene, ZeroRadiusFallsBackToOne) {
  Camera cam;
  cam.fit_to_scene({0.f, 0.f, 0.f}, 0.f);
  EXPECT_TRUE(Near(cam.get_scene_radius(), 1.f));
  EXPECT_TRUE(Near(cam.get_distance(), 2.5f));
}

TEST(CameraFitToScene, NegativeRadiusFallsBackToOne) {
  Camera cam;
  cam.fit_to_scene({0.f, 0.f, 0.f}, -3.f);
  EXPECT_TRUE(Near(cam.get_scene_radius(), 1.f));
  EXPECT_TRUE(Near(cam.get_distance(), 2.5f));
}

TEST(CameraFitToScene, TinyRadiusBelowThresholdFallsBack) {
  Camera cam;
  cam.fit_to_scene({0.f, 0.f, 0.f}, 1e-7f);
  EXPECT_TRUE(Near(cam.get_scene_radius(), 1.f));
}

TEST(CameraFitToScene, RadiusAboveThresholdIsPreserved) {
  Camera cam;
  cam.fit_to_scene({0.f, 0.f, 0.f}, 3.f);
  EXPECT_TRUE(Near(cam.get_scene_radius(), 3.f));
  EXPECT_TRUE(Near(cam.get_distance(), 7.5f));
}

TEST(CameraFitToScene, BoundaryRadiusExactlyThresholdFallsBack) {
  Camera cam;
  cam.fit_to_scene({0.f, 0.f, 0.f}, 1e-6f);  // условие `> 1e-6f` ложно
  EXPECT_TRUE(Near(cam.get_scene_radius(), 1.f));
}

TEST(CameraFitToScene, OverridesPreviousCall) {
  Camera cam;
  cam.fit_to_scene({10.f, 10.f, 10.f}, 5.f);
  cam.fit_to_scene({0.f, 0.f, 0.f}, 2.f);

  EXPECT_TRUE(Near(cam.get_target().x, 0.f));
  EXPECT_TRUE(Near(cam.get_scene_radius(), 2.f));
  EXPECT_TRUE(Near(cam.get_distance(), 5.f));
}

// ============================================================
//  Camera::view_matrix / projection_matrix
// ============================================================

TEST(CameraViewMatrix, TranslationMatchesTargetAndDistance) {
  Camera cam;
  cam.fit_to_scene({1.f, 2.f, 3.f}, 2.f);  // distance = 5

  MyMat4 v = cam.view_matrix();
  MyMat4 expected = MyMat4::Translation(-1.f, -2.f, -3.f - 5.f);
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) EXPECT_NEAR(v(i, j), expected(i, j), 1e-4f);
}

TEST(CameraViewMatrix, ZeroTargetHasOnlyNegativeDistanceZ) {
  Camera cam;
  cam.fit_to_scene({0.f, 0.f, 0.f}, 2.f);  // distance = 5

  MyMat4 v = cam.view_matrix();
  EXPECT_NEAR(v(0, 3), 0.f, 1e-4f);
  EXPECT_NEAR(v(1, 3), 0.f, 1e-4f);
  EXPECT_NEAR(v(2, 3), -5.f, 1e-4f);
}

TEST(CameraProjection, OrthoDiagonalScalesWithRadius) {
  Camera cam;
  cam.set_perspective(false);
  cam.set_viewport(400, 400);              // aspect = 1
  cam.fit_to_scene({0.f, 0.f, 0.f}, 2.f);  // half = 3, aspect = 1

  MyMat4 p = cam.projection_matrix();
  EXPECT_NEAR(p(0, 0), 1.f / 3.f, 1e-4f);
  EXPECT_NEAR(p(1, 1), 1.f / 3.f, 1e-4f);
}

// ============================================================
//  projection_matrix — perspective
// ============================================================

TEST(CameraProjection, PerspectiveDiagonalElements) {
  Camera cam;
  cam.set_perspective(true);
  cam.set_viewport(800, 400);
  cam.fit_to_scene({0.f, 0.f, 0.f}, 1.f);

  MyMat4 p = cam.projection_matrix();

  const float fovY = 45.0f * static_cast<float>(M_PI) / 180.0f;
  const float f = 1.0f / std::tan(fovY * 0.5f);
  const float aspect = 2.0f;

  EXPECT_NEAR(p(0, 0), f / aspect, 1e-4f);
  EXPECT_NEAR(p(1, 1), f, 1e-4f);
  EXPECT_NEAR(p(3, 2), -1.f, 1e-4f);
  EXPECT_NEAR(p(3, 3), 0.f, 1e-4f);
}

TEST(CameraProjection, PerspectiveAspectDependsOnViewport) {
  Camera cam;
  cam.set_perspective(true);
  cam.fit_to_scene({0.f, 0.f, 0.f}, 1.f);

  cam.set_viewport(1600, 400);  // aspect = 4
  MyMat4 p_wide = cam.projection_matrix();

  cam.set_viewport(400, 400);  // aspect = 1
  MyMat4 p_square = cam.projection_matrix();

  // (1,1) не зависит от aspect
  EXPECT_NEAR(p_wide(1, 1), p_square(1, 1), 1e-4f);

  // (0,0) обратно пропорционален aspect: p_square / p_wide = 4
  EXPECT_NEAR(p_square(0, 0) / p_wide(0, 0), 4.f, 1e-4f);
}

// ============================================================
//  projection_matrix — ortho
// ============================================================

// Для ортопроекции:
//   half = 1.5 * get_scene_radius
//   l = -half*aspect, r = half*aspect, b = -half, t = half
//   m[0][0] = 2 / (r - l) = 1 / (half * aspect)
//   m[1][1] = 2 / (t - b) = 1 / half
//   m[3][3] = 1

TEST(CameraProjection, OrthoScalesBySceneRadius) {
  Camera cam;
  cam.set_perspective(false);
  cam.set_viewport(400, 400);              // aspect = 1
  cam.fit_to_scene({0.f, 0.f, 0.f}, 2.f);  // half = 3

  MyMat4 p = cam.projection_matrix();

  // m[1][1] = 1 / half = 1/3
  EXPECT_TRUE(Near(p(1, 1), 1.f / 3.f));
}

TEST(CameraProjection, OrthoRadiusFallbackAffectsMatrix) {
  Camera cam;
  cam.set_perspective(false);
  cam.set_viewport(400, 400);

  cam.fit_to_scene({0.f, 0.f, 0.f}, 0.f);  // fallback → radius = 1 → half = 1.5
  MyMat4 p_fallback = cam.projection_matrix();

  cam.fit_to_scene({0.f, 0.f, 0.f}, 1.f);  // half = 1.5
  MyMat4 p_normal = cam.projection_matrix();

  // Оба должны быть равны
  EXPECT_TRUE(Near(p_fallback(1, 1), p_normal(1, 1)));
}

// Tests color.cc

TEST(ColorCtorFromHex, FullEightDigitLowercase) {
  ColorRGBA c("#ff800080");  // r=255, g=128, b=0, a=128
  EXPECT_TRUE(Near(c.r, 1.0f));
  EXPECT_TRUE(Near(c.g, 128.0f / 255.0f));
  EXPECT_TRUE(Near(c.b, 0.0f));
  EXPECT_TRUE(Near(c.a, 128.0f / 255.0f));
}

TEST(ColorCtorFromHex, SixDigitSetsAlphaToOne) {
  ColorRGBA c("#ff8000");  // без alpha
  EXPECT_TRUE(Near(c.r, 1.0f));
  EXPECT_TRUE(Near(c.g, 128.0f / 255.0f));
  EXPECT_TRUE(Near(c.b, 0.0f));
  EXPECT_TRUE(Near(c.a, 1.0f));
}

TEST(ColorCtorFromHex, UppercaseHex) {
  ColorRGBA c("#FF8000FF");
  EXPECT_TRUE(Near(c.r, 1.0f));
  EXPECT_TRUE(Near(c.g, 128.0f / 255.0f));
  EXPECT_TRUE(Near(c.b, 0.0f));
  EXPECT_TRUE(Near(c.a, 1.0f));
}

TEST(ColorCtorFromHex, MixedCaseHex) {
  ColorRGBA c("#Ff8000aA");
  EXPECT_TRUE(Near(c.r, 1.0f));
  EXPECT_TRUE(Near(c.g, 128.0f / 255.0f));
  EXPECT_TRUE(Near(c.b, 0.0f));
  EXPECT_TRUE(Near(c.a, 170.0f / 255.0f));
}

TEST(ColorCtorFromHex, Black) {
  ColorRGBA c("#00000000");
  EXPECT_TRUE(Near(c.r, 0.0f));
  EXPECT_TRUE(Near(c.g, 0.0f));
  EXPECT_TRUE(Near(c.b, 0.0f));
  EXPECT_TRUE(Near(c.a, 0.0f));
}

TEST(ColorCtorFromHex, White) {
  ColorRGBA c("#ffffffff");
  EXPECT_TRUE(Near(c.r, 1.0f));
  EXPECT_TRUE(Near(c.g, 1.0f));
  EXPECT_TRUE(Near(c.b, 1.0f));
  EXPECT_TRUE(Near(c.a, 1.0f));
}

TEST(ColorCtorFromHex, SixDigitBlackOpaque) {
  ColorRGBA c("#000000");
  EXPECT_TRUE(Near(c.r, 0.0f));
  EXPECT_TRUE(Near(c.g, 0.0f));
  EXPECT_TRUE(Near(c.b, 0.0f));
  EXPECT_TRUE(Near(c.a, 1.0f));  // важное поведение: 6-значный = непрозрачный
}

// ============================================================
//  ColorRGBA(const std::string&) — некорректные / крайние
// ============================================================

TEST(ColorCtorFromHex, EmptyStringDoesNotChangeFields) {
  ColorRGBA c("#ff8000ff");
  c = ColorRGBA("");
  EXPECT_TRUE(Near(c.r, 0.0f));
  EXPECT_TRUE(Near(c.g, 0.0f));
  EXPECT_TRUE(Near(c.b, 0.0f));
  EXPECT_TRUE(Near(c.a, 1.0f));
}

TEST(ColorCtorFromHex, NoHashParsesNothing) {
  ColorRGBA c("ff8000ff");
  EXPECT_TRUE(Near(c.r, 0.0f));
  EXPECT_TRUE(Near(c.g, 0.0f));
  EXPECT_TRUE(Near(c.b, 0.0f));
  EXPECT_TRUE(Near(c.a, 1.0f));
}

TEST(ColorCtorFromHex, OnlyHash) {
  ColorRGBA c("#");
  EXPECT_TRUE(Near(c.r, 0.0f));
  EXPECT_TRUE(Near(c.g, 0.0f));
  EXPECT_TRUE(Near(c.b, 0.0f));
  EXPECT_TRUE(Near(c.a, 1.0f));
}

TEST(ColorCtorFromHex, InvalidHexChars) {
  ColorRGBA c("#gggggg");
  EXPECT_TRUE(Near(c.r, 0.0f));
  EXPECT_TRUE(Near(c.g, 0.0f));
  EXPECT_TRUE(Near(c.b, 0.0f));
  EXPECT_TRUE(Near(c.a, 1.0f));
}

TEST(ColorCtorFromHex, TooShort) {
  ColorRGBA c("#ff");
  EXPECT_TRUE(Near(c.r, 0.0f));  // дефолты
}

TEST(ColorCtorFromHex, ExtraTrailingCharsIgnored) {
  ColorRGBA c("#ff800080zzzz");  // лишние символы после 8 hex
  // sscanf прочитает 4 значения, `parsed == 4`
  EXPECT_TRUE(Near(c.r, 1.0f));
  EXPECT_TRUE(Near(c.g, 128.0f / 255.0f));
  EXPECT_TRUE(Near(c.b, 0.0f));
  EXPECT_TRUE(Near(c.a, 128.0f / 255.0f));
}

// ============================================================
//  hexstring()
// ============================================================

TEST(ColorHexstring, White) {
  ColorRGBA c;
  c.r = c.g = c.b = c.a = 1.0f;
  EXPECT_EQ(c.hexstring(), "#ffffffff");
}

TEST(ColorHexstring, BlackOpaque) {
  ColorRGBA c;
  c.r = c.g = c.b = 0.0f;
  c.a = 1.0f;
  EXPECT_EQ(c.hexstring(), "#000000ff");
}

TEST(ColorHexstring, BlackTransparent) {
  ColorRGBA c;
  c.r = c.g = c.b = c.a = 0.0f;
  EXPECT_EQ(c.hexstring(), "#00000000");
}

TEST(ColorHexstring, HalfGray) {
  ColorRGBA c;
  c.r = c.g = c.b = 0.5f;
  c.a = 1.0f;
  // 0.5 * 255 + 0.5 = 128.0 → 128 = 0x80
  EXPECT_EQ(c.hexstring(), "#808080ff");
}

TEST(ColorHexstring, ClampsAboveOne) {
  ColorRGBA c;
  c.r = 2.0f;
  c.g = 1.5f;
  c.b = 1.0f;
  c.a = 1.5f;
  EXPECT_EQ(c.hexstring(), "#ffffffff");
}

TEST(ColorHexstring, ClampsBelowZero) {
  ColorRGBA c;
  c.r = -1.0f;
  c.g = -0.5f;
  c.b = 0.0f;
  c.a = -0.1f;
  EXPECT_EQ(c.hexstring(), "#00000000");
}

TEST(ColorHexstring, LowercaseLettersCorrect) {
  ColorRGBA c;
  c.r = static_cast<float>(0xAB) / 255.0f;
  c.g = static_cast<float>(0xCD) / 255.0f;
  c.b = static_cast<float>(0xEF) / 255.0f;
  c.a = 1.0f;
  EXPECT_EQ(c.hexstring(), "#abcdefff");
}

// ============================================================
//  Round-trip: hex → Color → hex
// ============================================================

TEST(ColorRoundTrip, FullOpaque) {
  const std::string src = "#123456ff";
  ColorRGBA c(src);
  EXPECT_EQ(c.hexstring(), src);
}

TEST(ColorRoundTrip, FullTransparent) {
  const std::string src = "#abcdef00";
  ColorRGBA c(src);
  EXPECT_EQ(c.hexstring(), src);
}

TEST(ColorRoundTrip, SixDigitBecomesEightDigit) {
  ColorRGBA c("#123456");  // alpha не задана → 1.0
  EXPECT_EQ(c.hexstring(), "#123456ff");
}

TEST(ColorRoundTrip, ZeroAlpha) {
  const std::string src = "#00000000";
  ColorRGBA c(src);
  EXPECT_EQ(c.hexstring(), src);
}

// ============================================================
//  Границы округления
// ============================================================

TEST(ColorHexstring, RoundingNearHalf) {
  ColorRGBA c;
  c.a = 1.0f;
  c.r = c.g = c.b = 127.0f / 255.0f;  // ≈ 0.4980
  // 0.4980 * 255 + 0.5 = 127.49 → 127 = 0x7f
  EXPECT_EQ(c.hexstring(), "#7f7f7fff");
}

TEST(ColorHexstring, RoundingAtZeroPointFive) {
  ColorRGBA c;
  c.a = 1.0f;
  c.r = c.g = c.b = 0.5f;
  // 0.5 * 255 + 0.5 = 128.0 → 128 = 0x80
  EXPECT_EQ(c.hexstring(), "#808080ff");
}

// Tests for mat4
//

constexpr float kPi = 3.14159265358979323846f;

// ============================================================
//  Хелперы
// ============================================================

void ExpectMat4Near(const MyMat4& m, const MyMat4& expected, float eps = kEps) {
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      EXPECT_NEAR(m(i, j), expected(i, j), eps)
          << "Mismatch at (" << i << "," << j << ")";
    }
  }
}

void ExpectIdentity(const MyMat4& m, float eps = kEps) {
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      const float want = (i == j) ? 1.0f : 0.0f;
      EXPECT_NEAR(m(i, j), want, eps)
          << "Mismatch at (" << i << "," << j << ")";
    }
  }
}

// Проверить, что элемент (i,j) близок к want, и что остальные — нули.
void ExpectSingleElement(const MyMat4& m, int ri, int rj, float want,
                         float eps = kEps) {
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      const float exp = (i == ri && j == rj) ? want : 0.0f;
      EXPECT_NEAR(m(i, j), exp, eps) << "Mismatch at (" << i << "," << j << ")";
    }
  }
}

// ============================================================
//  Конструкторы
// ============================================================

TEST(MyMat4Ctor, DefaultIsIdentity) {
  MyMat4 m;
  ExpectIdentity(m);
}

TEST(MyMat4Ctor, FromMatrix4x4) {
  Matrix src(4, 4);
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) src(i, j) = static_cast<float>(i * 4 + j);

  MyMat4 m(src);
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      EXPECT_NEAR(m(i, j), static_cast<float>(i * 4 + j), kEps);
}

// ============================================================
//  Translation
// ============================================================

TEST(MyMat4Translation, BasicValues) {
  constexpr float x = 1.5f;
  constexpr float y = -2.5f;
  constexpr float z = 3.5f;

  MyMat4 t = MyMat4::Translation(x, y, z);

  MyMat4 expected;
  expected(0, 3) = x;
  expected(1, 3) = y;
  expected(2, 3) = z;

  ExpectMat4Near(t, expected);
}

TEST(MyMat4Translation, ZeroIsIdentity) {
  MyMat4 t = MyMat4::Translation(0.f, 0.f, 0.f);
  ExpectIdentity(t);
}

TEST(MyMat4Translation, FromVec3D) {
  Vec3D v{1.f, 2.f, 3.f};
  MyMat4 t = MyMat4::Translation(v);
  MyMat4 expected = MyMat4::Translation(1.f, 2.f, 3.f);
  ExpectMat4Near(t, expected);
}

// ============================================================
//  Scale / ScaleUniform
// ============================================================

TEST(MyMat4ScaleUniform, SetsDiagonal) {
  MyMat4 s = MyMat4::ScaleUniform(2.5f);
  EXPECT_NEAR(s(0, 0), 2.5f, kEps);
  EXPECT_NEAR(s(1, 1), 2.5f, kEps);
  EXPECT_NEAR(s(2, 2), 2.5f, kEps);
  EXPECT_NEAR(s(3, 3), 1.0f, kEps);  // гомогенная координата не трогается
  EXPECT_NEAR(s(0, 1), 0.0f, kEps);
  EXPECT_NEAR(s(1, 0), 0.0f, kEps);
}

TEST(MyMat4ScaleUniform, OneIsIdentity) {
  ExpectIdentity(MyMat4::ScaleUniform(1.0f));
}

TEST(MyMat4Scale, SetsDifferentScales) {
  MyMat4 s = MyMat4::Scale(2.f, 3.f, 4.f);
  EXPECT_NEAR(s(0, 0), 2.f, kEps);
  EXPECT_NEAR(s(1, 1), 3.f, kEps);
  EXPECT_NEAR(s(2, 2), 4.f, kEps);
  EXPECT_NEAR(s(3, 3), 1.f, kEps);
  EXPECT_NEAR(s(0, 1), 0.f, kEps);
}

// ============================================================
//  RotationX
// ============================================================

TEST(MyMat4RotationX, ZeroIsIdentity) {
  ExpectIdentity(MyMat4::RotationX(0.f));
}

TEST(MyMat4RotationX, PiOverTwo) {
  MyMat4 r = MyMat4::RotationX(kPi * 0.5f);
  EXPECT_NEAR(r(1, 1), 0.f, kEps);
  EXPECT_NEAR(r(1, 2), -1.f, kEps);
  EXPECT_NEAR(r(2, 1), 1.f, kEps);
  EXPECT_NEAR(r(2, 2), 0.f, kEps);
  EXPECT_NEAR(r(0, 0), 1.f, kEps);
  EXPECT_NEAR(r(3, 3), 1.f, kEps);
}

TEST(MyMat4RotationX, PiFlipsYAndZ) {
  MyMat4 r = MyMat4::RotationX(kPi);
  EXPECT_NEAR(r(1, 1), -1.f, 1e-3f);  // cos(π) ≈ -1
  EXPECT_NEAR(r(2, 2), -1.f, 1e-3f);
  EXPECT_NEAR(r(1, 2), 0.f, 1e-3f);
  EXPECT_NEAR(r(2, 1), 0.f, 1e-3f);
}

TEST(MyMat4RotationX, TwoPiIsIdentity) {
  ExpectIdentity(MyMat4::RotationX(2.f * kPi), 1e-3f);
}

// ============================================================
//  RotationY
// ============================================================

TEST(MyMat4RotationY, ZeroIsIdentity) {
  ExpectIdentity(MyMat4::RotationY(0.f));
}

TEST(MyMat4RotationY, PiOverTwo) {
  MyMat4 r = MyMat4::RotationY(kPi * 0.5f);
  EXPECT_NEAR(r(0, 0), 0.f, kEps);
  EXPECT_NEAR(r(0, 2), 1.f, kEps);
  EXPECT_NEAR(r(2, 0), -1.f, kEps);
  EXPECT_NEAR(r(2, 2), 0.f, kEps);
  EXPECT_NEAR(r(1, 1), 1.f, kEps);
  EXPECT_NEAR(r(3, 3), 1.f, kEps);
}

TEST(MyMat4RotationY, PiFlipsXAndZ) {
  MyMat4 r = MyMat4::RotationY(kPi);
  EXPECT_NEAR(r(0, 0), -1.f, 1e-3f);
  EXPECT_NEAR(r(2, 2), -1.f, 1e-3f);
  EXPECT_NEAR(r(0, 2), 0.f, 1e-3f);
}

// ============================================================
//  RotationZ
// ============================================================

TEST(MyMat4RotationZ, ZeroIsIdentity) {
  ExpectIdentity(MyMat4::RotationZ(0.f));
}

TEST(MyMat4RotationZ, PiOverTwo) {
  MyMat4 r = MyMat4::RotationZ(kPi * 0.5f);
  EXPECT_NEAR(r(0, 0), 0.f, kEps);
  EXPECT_NEAR(r(0, 1), -1.f, kEps);
  EXPECT_NEAR(r(1, 0), 1.f, kEps);
  EXPECT_NEAR(r(1, 1), 0.f, kEps);
  EXPECT_NEAR(r(2, 2), 1.f, kEps);
  EXPECT_NEAR(r(3, 3), 1.f, kEps);
}

TEST(MyMat4RotationZ, PiFlipsXAndY) {
  MyMat4 r = MyMat4::RotationZ(kPi);
  EXPECT_NEAR(r(0, 0), -1.f, 1e-3f);
  EXPECT_NEAR(r(1, 1), -1.f, 1e-3f);
}

// ============================================================
//  Perspective
// ============================================================

TEST(MyMat4Perspective, KnownValues) {
  // fovY = π/2 → tan(π/4) = 1; aspect = 1; near = 1; far = 10
  MyMat4 p = MyMat4::Perspective(kPi * 0.5f, 1.f, 1.f, 10.f);

  EXPECT_NEAR(p(0, 0), 1.f, kEps);          // 1/(aspect*t)
  EXPECT_NEAR(p(1, 1), 1.f, kEps);          // 1/t
  EXPECT_NEAR(p(2, 2), 11.f / -9.f, kEps);  // (f+n)/(n-f)
  EXPECT_NEAR(p(2, 3), 20.f / -9.f, kEps);  // 2*f*n/(n-f)
  EXPECT_NEAR(p(3, 2), -1.f, kEps);
  EXPECT_NEAR(p(3, 3), 0.f, kEps);
}

TEST(MyMat4Perspective, AspectAffectsXOnly) {
  MyMat4 p1 = MyMat4::Perspective(kPi * 0.5f, 1.f, 1.f, 10.f);
  MyMat4 p2 = MyMat4::Perspective(kPi * 0.5f, 2.f, 1.f, 10.f);

  EXPECT_NEAR(p2(0, 0), p1(0, 0) / 2.f, kEps);
  EXPECT_NEAR(p2(1, 1), p1(1, 1), kEps);
  EXPECT_NEAR(p2(2, 2), p1(2, 2), kEps);
}

TEST(MyMat4Perspective, CornerIsZeroExcept3_2) {
  MyMat4 p = MyMat4::Perspective(kPi * 0.5f, 1.f, 1.f, 10.f);
  // Всё, кроме диагонали и (2,3),(3,2), должно быть нулём
  EXPECT_NEAR(p(0, 1), 0.f, kEps);
  EXPECT_NEAR(p(0, 2), 0.f, kEps);
  EXPECT_NEAR(p(0, 3), 0.f, kEps);
  EXPECT_NEAR(p(1, 0), 0.f, kEps);
  EXPECT_NEAR(p(1, 2), 0.f, kEps);
  EXPECT_NEAR(p(1, 3), 0.f, kEps);
  EXPECT_NEAR(p(3, 0), 0.f, kEps);
  EXPECT_NEAR(p(3, 1), 0.f, kEps);
}

// ============================================================
//  Ortho
// ============================================================

TEST(MyMat4Ortho, SymmetricBoxHasNoTranslation) {
  // l = -r, b = -t → m(0,3) = m(1,3) = 0
  MyMat4 o = MyMat4::Ortho(-1.f, 1.f, -1.f, 1.f, 1.f, 10.f);

  EXPECT_NEAR(o(0, 0), 1.f, kEps);         // 2/(r-l)
  EXPECT_NEAR(o(1, 1), 1.f, kEps);         // 2/(t-b)
  EXPECT_NEAR(o(2, 2), -2.f / 9.f, kEps);  // -2/(f-n)
  EXPECT_NEAR(o(0, 3), 0.f, kEps);
  EXPECT_NEAR(o(1, 3), 0.f, kEps);
  EXPECT_NEAR(o(2, 3), -11.f / 9.f, kEps);  // -(f+n)/(f-n)
  EXPECT_NEAR(o(3, 3), 1.f, kEps);
}

TEST(MyMat4Ortho, AsymmetricBoxHasTranslation) {
  MyMat4 o = MyMat4::Ortho(0.f, 2.f, 0.f, 4.f, 1.f, 10.f);

  EXPECT_NEAR(o(0, 0), 1.f, kEps);   // 2/(2-0)
  EXPECT_NEAR(o(1, 1), 0.5f, kEps);  // 2/(4-0)
  EXPECT_NEAR(o(2, 2), -2.f / 9.f, kEps);
  EXPECT_NEAR(o(0, 3), -1.f, kEps);  // -(2+0)/(2-0)
  EXPECT_NEAR(o(1, 3), -1.f, kEps);  // -(4+0)/(4-0)
  EXPECT_NEAR(o(2, 3), -11.f / 9.f, kEps);
  EXPECT_NEAR(o(3, 3), 1.f, kEps);
}

// ============================================================
//  operator*
// ============================================================

TEST(MyMat4OperatorMul, IdentityIsNeutralLeft) {
  MyMat4 i;
  MyMat4 t = MyMat4::Translation(1.f, 2.f, 3.f);

  ExpectMat4Near(i * t, t);
}

TEST(MyMat4OperatorMul, IdentityIsNeutralRight) {
  MyMat4 i;
  MyMat4 t = MyMat4::Translation(1.f, 2.f, 3.f);

  ExpectMat4Near(t * i, t);
}

TEST(MyMat4OperatorMul, TranslationsCompose) {
  MyMat4 a = MyMat4::Translation(1.f, 2.f, 3.f);
  MyMat4 b = MyMat4::Translation(4.f, 5.f, 6.f);

  MyMat4 expected = MyMat4::Translation(5.f, 7.f, 9.f);
  ExpectMat4Near(a * b, expected);
}

TEST(MyMat4OperatorMul, TranslationsComposeReverseOrder) {
  // T(a) * T(b) == T(b) * T(a) — трансляции коммутируют
  MyMat4 a = MyMat4::Translation(1.f, 2.f, 3.f);
  MyMat4 b = MyMat4::Translation(4.f, 5.f, 6.f);

  ExpectMat4Near(a * b, b * a);
}

TEST(MyMat4OperatorMul, ScaleUniformCompose) {
  MyMat4 s1 = MyMat4::ScaleUniform(2.f);
  MyMat4 s2 = MyMat4::ScaleUniform(3.f);

  MyMat4 expected = MyMat4::ScaleUniform(6.f);
  ExpectMat4Near(s1 * s2, expected);
}

TEST(MyMat4OperatorMul, RotationNinetyTwiceIsPi) {
  MyMat4 r90 = MyMat4::RotationZ(kPi * 0.5f);
  MyMat4 r180 = r90 * r90;

  // cos(π) = -1, sin(π) = 0
  EXPECT_NEAR(r180(0, 0), -1.f, 1e-3f);
  EXPECT_NEAR(r180(1, 1), -1.f, 1e-3f);
  EXPECT_NEAR(r180(0, 1), 0.f, 1e-3f);
  EXPECT_NEAR(r180(1, 0), 0.f, 1e-3f);
}

TEST(MyMat4OperatorMul, RotationByAngleThenNegatedIsIdentity) {
  const float a = 0.7f;
  MyMat4 r1 = MyMat4::RotationZ(a);
  MyMat4 r2 = MyMat4::RotationZ(-a);

  ExpectMat4Near(r1 * r2, MyMat4{}, 1e-4f);
}

// ============================================================
//  operator*=
// ============================================================

TEST(MyMat4OperatorMulAssign, MatchesOperatorMul) {
  MyMat4 a = MyMat4::Translation(1.f, 2.f, 3.f);
  MyMat4 b = MyMat4::Translation(4.f, 5.f, 6.f);

  MyMat4 copy = a;
  copy *= b;
  ExpectMat4Near(copy, a * b);
}

TEST(MyMat4OperatorMulAssign, ReturnsSelfReference) {
  MyMat4 a;
  MyMat4 b;
  MyMat4& ref = (a *= b);
  EXPECT_EQ(&ref, &a);
}

TEST(MyMat4OperatorMulAssign, IdentityDoesNotChange) {
  MyMat4 a = MyMat4::Translation(1.f, 2.f, 3.f);
  MyMat4 before = a;
  a *= MyMat4{};
  ExpectMat4Near(a, before);
}

// ============================================================
//  Композиция: T * R * S применяется к точке — проверяем через матрицы
// ============================================================

TEST(MyMat4Composition, TranslateAfterScale) {
  // T(10,0,0) * S(2) — сначала масштаб, потом перенос
  MyMat4 ts = MyMat4::Translation(10.f, 0.f, 0.f) * MyMat4::ScaleUniform(2.f);

  // Ожидаемая матрица: диагональ 2,2,2,1 и translation (10,0,0)
  EXPECT_NEAR(ts(0, 0), 2.f, kEps);
  EXPECT_NEAR(ts(1, 1), 2.f, kEps);
  EXPECT_NEAR(ts(2, 2), 2.f, kEps);
  EXPECT_NEAR(ts(3, 3), 1.f, kEps);
  EXPECT_NEAR(ts(0, 3), 10.f, kEps);
  EXPECT_NEAR(ts(1, 3), 0.f, kEps);
  EXPECT_NEAR(ts(2, 3), 0.f, kEps);
}

TEST(MyMat4Composition, ScaleAfterTranslate) {
  // S(2) * T(10,0,0) — сначала перенос, потом масштаб
  MyMat4 st = MyMat4::ScaleUniform(2.f) * MyMat4::Translation(10.f, 0.f, 0.f);

  // Масштаб умножает и компоненту переноса: translation = (20, 0, 0)
  EXPECT_NEAR(st(0, 0), 2.f, kEps);
  EXPECT_NEAR(st(1, 1), 2.f, kEps);
  EXPECT_NEAR(st(2, 2), 2.f, kEps);
  EXPECT_NEAR(st(0, 3), 20.f, kEps);
  EXPECT_NEAR(st(1, 3), 0.f, kEps);
  EXPECT_NEAR(st(2, 3), 0.f, kEps);
}

}  // namespace
}  // namespace ec::model
