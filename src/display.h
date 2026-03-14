#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32

extern uint8_t gfx[DISPLAY_HEIGHT][DISPLAY_WIDTH];

void init_console();
void restore_console();
void init_display();
void draw_display();
void sleep_ms(int ms);

#endif