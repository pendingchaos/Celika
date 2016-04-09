#pragma once
#ifndef SHARED_H

#include <stdbool.h>
#include <stddef.h>

void gui_begin_frame(size_t w, size_t h);
void gui_end_frame();
bool gui_button(float posy, const char* label);

#endif
