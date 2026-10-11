#ifndef GUI_CONFIG_H_
#define GUI_CONFIG_H_
#include <model/scene.h>

#include <string>

namespace ec::view {
using std::string;
// Translates SceneSettings <-> GKeyFile.
// Knows both Model (SceneSettings) and GTK (Glib::KeyFile).
class SceneSettingsSerializer {
 public:
  static bool cfg_write_to_file(model::SceneSettings& cfg, const string& path);
  static model::SceneSettings cfg_read_from_file(const string& path);
};
}  // namespace ec::view
#endif
