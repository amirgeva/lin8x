#pragma once

#include <types.h>

typedef unsigned short Color;

bool screen_init();
void screen_shut();
void screen_print_info();
uint screen_get_rows();
uint screen_get_cols();
uint screen_get_width();
uint screen_get_height();
void screen_clear(Color color);
void screen_draw_pixel(uint x, uint y, Color color);
void screen_xor_rect(uint x, uint y, uint w, uint h, Color color);
void screen_fill_rect(uint x, uint y, uint w, uint h, Color color);
void screen_draw_rect(uint x, uint y, uint w, uint h, Color color);
void screen_draw_line(uint x1, uint y1, uint x2, uint y2, Color color);
void screen_scroll(uint dy, Color blank_color);
// x,y are coordinates in pixels
bool screen_draw_char(uint x, uint y, char c, Color fg, Color bg);
bool screen_draw_string(uint x, uint y, const char *str, Color fg, Color bg);
// x,y are coordinates in characters (0..width-1, 0..height-1)
bool text_draw_char(uint x, uint y, char c, Color fg, Color bg);
bool text_draw_string(uint x, uint y, const char *str, Color fg, Color bg);

Color RGB(uint r, uint g, uint b);

