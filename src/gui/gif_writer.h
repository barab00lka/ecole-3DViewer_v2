#ifndef GUI_GIF_WRITER_H_
#define GUI_GIF_WRITER_H_

#include <containers/ec_list.h>
#include <containers/ec_vector.h>

#include <cstdint>
#include <string>
void save_gif(int w, int h, const std::string& path,
              ec::List<ec::Vector<uint8_t>>& frames, uint16_t interval_cs);

void flip_vertically(ec::Vector<uint8_t>& vec, int width, int height);
#endif
