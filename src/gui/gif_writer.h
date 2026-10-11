#ifndef GUI_GIF_WRITER_H_
#define GUI_GIF_WRITER_H_

#include <containers/my_list.h>
#include <containers/my_vector.h>

#include <cstdint>
#include <string>
void save_gif(int w, int h, const std::string& path,
              const ec::List<ec::Vector<uint8_t>>& frames,
              uint16_t interval_cs);
#endif
