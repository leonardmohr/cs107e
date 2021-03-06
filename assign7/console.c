#include "console.h"
#include "gl.h"
#include "printf.h"
#include "malloc.h"
#include "uart.h"
#include "strings.h"
#include <stdarg.h>

#define _WIDTH 640
#define _HEIGHT 512
#define MAX_OUTPUT_LEN 1024

unsigned int NROWS;
unsigned int NCOLS;
volatile unsigned int cursor_x;
volatile unsigned int cursor_y;
char *buf;

const color_t COLOR_TEXT = GL_GREEN;
color_t COLOR_BACKGROUND = 0;

// we don't call this since shell_run runs indefinitely
void console_free()
{
    free(buf);
}

void console_init(unsigned int nrows, unsigned int ncols)
{
    //gl_init(_WIDTH, _HEIGHT, GL_DOUBLEBUFFER);
    gl_init(ncols*gl_get_char_width(), nrows*gl_get_char_height(), GL_DOUBLEBUFFER);
    //COLOR_BACKGROUND = gl_read_pixel(0, 0);

    NROWS = nrows;
    NCOLS = ncols; 
    buf = malloc(ncols * nrows);

    // initialize cursor to point to top-left corner
    cursor_x = 0;
    cursor_y = 0;
}

void console_clear(void)
{
    buf[0] = '\0';
    cursor_x = 0;
    cursor_y = 0;
    gl_clear(COLOR_BACKGROUND);
    gl_swap_buffer();
    gl_clear(COLOR_BACKGROUND);
    gl_swap_buffer();
}

int find_char(int ch)
{
    for (int i = 0; i < strlen(buf); i++) {
        if (buf[i] == ch) return i;
    }
    return -1;
}

int console_printf(const char *format, ...)
{
    gl_clear(COLOR_BACKGROUND);
    cursor_x = 0;
    cursor_y = 0;
    char format_buf[MAX_OUTPUT_LEN];

    va_list args;
    va_start(args, format);

    int input_len = vsnprintf(format_buf, MAX_OUTPUT_LEN, format, args);
    va_end(args);

    int total_len = strlcat(buf, format_buf, NROWS*NCOLS);

    for (int i = 0; i < total_len; i++) {
        char ch = buf[i];
        if (ch == '\n') {
            cursor_x = 0;
            cursor_y += gl_get_char_height();
        } else if (ch == '\b') {
            // edge case for backspace at beginning
            if (i > 0) {
                if (cursor_x != 0) {
                    cursor_x -= gl_get_char_width();
                // edge case to wrap back if at start of line
                } else {
                    cursor_x = (NCOLS - 1) * gl_get_char_width();
                    cursor_y -= gl_get_char_height();
                }
                // draw black box to overwrite char
                gl_draw_rect(cursor_x, cursor_y, gl_get_char_width(), gl_get_char_height(), COLOR_BACKGROUND);
            }
        } else if (ch == '\f') {
            console_clear();
        } else {
            // must wrap text around
            if (cursor_x >= NCOLS * gl_get_char_width()) {
                cursor_x = 0;
                cursor_y += gl_get_char_height();
            }
            // scroll if at bottom of buffer
            if (cursor_y >= NROWS * gl_get_char_height()) {
                // find the end of the first row
                gl_clear(COLOR_BACKGROUND);
                int rest_of_buf_start = find_char('\n') + 1;
                if (rest_of_buf_start == -1 || rest_of_buf_start > NCOLS) {
                    rest_of_buf_start = NCOLS;
                }
                char rest_of_buf[NCOLS*NROWS];
                memcpy(rest_of_buf, buf + rest_of_buf_start, (strlen(buf) - rest_of_buf_start));
                // must null-terminate
                rest_of_buf[(strlen(buf) - rest_of_buf_start)] = '\0';
                memcpy(buf, rest_of_buf, strlen(rest_of_buf));
                total_len -= rest_of_buf_start;
                buf[total_len] = '\0';
                cursor_x = 0;
                cursor_y = 0;
                i = -1;
                continue;
            }
            // treat return char as space
            if (ch == '\r') {
                ch = ' ';
            }
            gl_draw_char(cursor_x, cursor_y, ch, COLOR_TEXT);
            cursor_x += gl_get_char_width();
        }
    }

    gl_swap_buffer();

    return input_len;
}