#include "gl.h"
#include "fb.h"
#include "font.h"
#include "strings.h"

// fb will be initialized to a 4-byte depth
const int FB_DEPTH = 4;

void gl_init(unsigned int width, unsigned int height, unsigned int mode)
{
    fb_init(width, height, FB_DEPTH, mode);
}

void gl_swap_buffer(void)
{
    fb_swap_buffer();
}

unsigned int gl_get_width(void) 
{
    return fb_get_width();
}

unsigned int gl_get_height(void)
{
    return fb_get_height();
}

color_t gl_color(unsigned char r, unsigned char g, unsigned char b)
{
    color_t color;
    color = (unsigned int) b;
    color = color | (unsigned int) g << 8;
    color = color | (unsigned int) r << 16;
    color = color | (unsigned int) 0xFF << 24;

    return color;
}

void gl_clear(color_t c)
{
    unsigned (*db)[fb_get_pitch()/4] = (unsigned (*)[fb_get_pitch()/4]) fb_get_draw_buffer();
    for (int y = 0; y < gl_get_height(); y++) {
        for (int x = 0; x < gl_get_width(); x++) {
            db[y][x] = c;
        }
    }
}

void gl_draw_pixel(int x, int y, color_t c)
{
    if (x < gl_get_width() && y < gl_get_height()) {
        unsigned (*db)[fb_get_pitch()/4] = (unsigned (*)[fb_get_pitch()/4]) fb_get_draw_buffer();
        db[y][x] = c;
    }
}

color_t gl_read_pixel(int x, int y)
{
    if (x < gl_get_width() && y < gl_get_height()) {
        unsigned (*db)[fb_get_pitch()/4] = (unsigned (*)[fb_get_pitch()/4]) fb_get_draw_buffer();
        return db[y][x];
    // out of bounds
    } else {
        return 0;
    }
}

void gl_draw_rect(int x, int y, int w, int h, color_t c)
{
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            if ((x+i < gl_get_width()) && (y+j < gl_get_height())) {
                gl_draw_pixel(x+i, y+j, c);
            }
        }
    }
}

void gl_draw_char(int x, int y, int ch, color_t c)
{
    int char_size = font_get_size();
    unsigned char inner_buf[char_size];
    unsigned char (*buf)[gl_get_char_width()] = (unsigned char (*) [gl_get_char_width()]) inner_buf;
    // check if valid ascii character
    if (font_get_char(ch, inner_buf, char_size)) {
        for (int i = 0; i < gl_get_char_width(); i++) {
            for (int j = 0; j < gl_get_char_height(); j++) {
                if ((x+i < gl_get_width()) && (y+j < gl_get_height())) {
                    if (buf[j][i] != 0) {
                        gl_draw_pixel(x+i, y+j, c);
                    }
                }
            }
        }
    }
}

void gl_draw_string(int x, int y, char* str, color_t c)
{
    int len = strlen(str);
    int char_width = gl_get_char_width(); 
    for (int i = 0; i < len; i++) {
        int ch = str[i];
        gl_draw_char(x + (char_width * i), y, ch, c);
    }
}

unsigned int gl_get_char_height(void)
{
    return font_get_height();
}

unsigned int gl_get_char_width(void)
{
    return font_get_width();
}

