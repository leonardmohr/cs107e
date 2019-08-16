#include "fb.h"

typedef unsigned int color;

const color WHITE = 0xffffffff;
const color RED = 0xffff0000;
const color GREEN = 0xff00ff00;
const color AQUA = 0x8050f0d0;
const color YELLOW = 0xffffff00;
const color PINK = 0xffd0c0c0;
const unsigned WIDTH = 1024;
const unsigned HEIGHT = 768;
const unsigned DEPTH = 4;

void draw_pixel(int x, int y, color c)
{
    unsigned (*im)[WIDTH] = (unsigned (*)[WIDTH])fb_get_draw_buffer();
    im[y][x] = c;
}

void draw_hline(int y, color c)
{
    for( int x = 0; x < fb_get_width(); x++ )
        draw_pixel(x, y, c);
}

void draw_vline(int x, color c)
{
    for( int y = 0; y < fb_get_height(); y++ )
        draw_pixel(x, y, c);
}

void draw_square(int x, int y, color c) {
    for(int i = 0; i < 16; i++) {
        for(int j = 0; j < 16; j++) {
            draw_pixel(x+i,y+j, c);
        }
    }
}

void main(void) 
{
  fb_init(WIDTH, HEIGHT, DEPTH, FB_SINGLEBUFFER);

  for( int y = 0; y < HEIGHT; y += 16 )
      draw_hline(y, YELLOW);

  for( int x = 0; x < WIDTH; x += 16 )
      draw_vline(x, RED);

  for( int y = 0; y < HEIGHT; y += 16 ){
    for( int x = 0; x < WIDTH; x += 16 ){
        if((x/16) % 2 == 0 && (y/16) % 2 == 0)         draw_square(x,y,AQUA);
        else if((x/16) % 2 == 1 && (y/16) % 2 == 1)    draw_square(x,y,AQUA);
        else                                           draw_square(x,y,PINK);
    }
  }

}

