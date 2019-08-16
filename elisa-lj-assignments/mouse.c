#include "gpio.h"
#include "gpioextra.h"
#include "interrupts.h"
#include "ringbuffer.h"
#include "mouse.h"

#define MOUSE_CLK GPIO_PIN25
#define MOUSE_DATA GPIO_PIN26

#define CMD_RESET 0xFF
#define CMD_ENABLE_DATA_REPORTING 0xF4

static rb_t *rb;

static void mouse_write(unsigned char data);

bool mouse_init(void)
{
  rb = rb_new();

  gpio_set_function(MOUSE_CLK, GPIO_FUNC_INPUT);
  gpio_set_pullup(MOUSE_CLK);
  gpio_set_function(MOUSE_DATA, GPIO_FUNC_INPUT);
  gpio_set_pullup(MOUSE_DATA);

  // FIXME: Initialize mouse.
}

mouse_event_t mouse_read_event(void)
{
  mouse_event_t evt;

  // FIXME: Read scancode(s) and fill in evt.

  return evt;
}

int mouse_read_scancode(void)
{
  // FIXME: Read from ring buffer.
  return 0;
}

static void mouse_write(unsigned char data)
{
  // FIXME: Send host->mouse packet.
}

static void mouse_handler(unsigned int pc)
{
  // FIXME: Handle event on mouse clock line
}
