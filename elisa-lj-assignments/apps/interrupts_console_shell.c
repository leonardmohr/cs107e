#include "keyboard.h"
#include "console.h"
#include "shell.h"
#include "interrupts.h"

#define NROWS 20
#define NCOLS 40

void main(void) 
{
    keyboard_init();
    console_init(NROWS, NCOLS);
    shell_init(console_printf);

    interrupts_global_enable(); // everything fully initialized, now turn on interrupts

    shell_run();
}
