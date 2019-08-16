#include "gpio.h"
#include "gpioextra.h"
#include "keyboard.h"
#include "ps2.h"
#include "strings.h"
#include "printf.h"
#include "interrupts.h"
#include "ringbuffer.h"

const unsigned int CLK  = GPIO_PIN23;
const unsigned int DATA = GPIO_PIN24;

// variables for interrupt handling
static rb_t* rb;
static int cnt = 0;
static unsigned char int_scancode = 0;
static unsigned int ones_in_data = 0;

// global to save states of modifiers
static unsigned int modifiers = 0;

void wait_for_falling_clock_edge() {
    while (gpio_read(CLK) == 0) {}
    while (gpio_read(CLK) == 1) {}
}

void interrupt_handler(unsigned int pc)
{
    // interrupt called for each falling clock edge
    if (gpio_check_and_clear_event(CLK)) {
        // checking for start bit
        if (cnt == 0 && gpio_read(DATA) == 0) {
            cnt++;
            return;
        }
        // check parity
        if (cnt == 9) {
            unsigned int parity = gpio_read(DATA);
            // if even parity, reset count
            if ((parity + ones_in_data) % 2 == 0) {
                cnt = 0;
                ones_in_data = 0;
                int_scancode = 0;
                return;
            }
        }
        // only enqueue when full scancode has read
        if (cnt == 10) {
            // check that stop bit is high, otherwise reset
            if (gpio_read(DATA) == 1) {
                rb_enqueue(rb, int_scancode);
            }
            cnt = 0;
            ones_in_data = 0;
            int_scancode = 0;
            return;
        }
        // otherwise, read data
        int data_bit = gpio_read(DATA);
        int_scancode |= data_bit << (cnt - 1);
        ones_in_data += data_bit;
        cnt++;
    }
}

void setup_interrupts(void)
{
    gpio_enable_event_detection(CLK, GPIO_DETECT_FALLING_EDGE);
    interrupts_attach_handler(interrupt_handler);
    interrupts_enable_source(INTERRUPTS_GPIO3);
    interrupts_global_enable();
}

void keyboard_init(void) 
{
    gpio_set_input(CLK); 
    gpio_set_pullup(CLK); 
 
    gpio_set_input(DATA); 
    gpio_set_pullup(DATA); 

    setup_interrupts();
    rb = rb_new();
}

unsigned char keyboard_read_scancode(void) 
{
    // wait for rb to contain a scancode
    while (rb_empty(rb)) {}
    int scancode;
    rb_dequeue(rb, &scancode);
    return (unsigned char) scancode;
}

int keyboard_read_sequence(unsigned char seq[])
{
    int bytes_written = 0;
    unsigned char scancode = keyboard_read_scancode();
    // check for extended code
    if (scancode == PS2_CODE_EXTEND) {
        seq[bytes_written] = scancode;
        bytes_written++;
        scancode = keyboard_read_scancode();
    }
    // check for break code
    if (scancode == PS2_CODE_RELEASE) {
        seq[bytes_written] = scancode;
        bytes_written++;
        scancode = keyboard_read_scancode();
    }
    seq[bytes_written] = scancode;
    bytes_written++;
    return bytes_written;
}

key_event_t keyboard_read_event(void) 
{
    key_event_t event;
    unsigned char seq[3];

    int seq_len = keyboard_read_sequence(seq);
    memcpy(event.seq, seq, seq_len);
    event.seq_len = seq_len;

    unsigned char key_code = seq[seq_len - 1];
    event.key = ps2_keys[key_code];

    // check if key has been pressed or released
    if (seq_len > 1 && seq[seq_len - 2] == PS2_CODE_RELEASE) {
        event.action = KEYBOARD_ACTION_UP;
    } else {
        event.action = KEYBOARD_ACTION_DOWN;
    }

    if (event.key.ch == PS2_KEY_SCROLL_LOCK) {
        if (event.action == KEYBOARD_ACTION_DOWN) {
            modifiers ^= KEYBOARD_MOD_SCROLL_LOCK;
        }
    }

    if (event.key.ch == PS2_KEY_NUM_LOCK) {
        if (event.action == KEYBOARD_ACTION_DOWN) {
            modifiers ^= KEYBOARD_MOD_NUM_LOCK;
        }
    }

    if (event.key.ch == PS2_KEY_CAPS_LOCK) {
        if (event.action == KEYBOARD_ACTION_DOWN) {
            modifiers ^= KEYBOARD_MOD_CAPS_LOCK;
        }
    }

    if (event.key.ch == PS2_KEY_SHIFT) {
        if (event.action == KEYBOARD_ACTION_DOWN) {
            modifiers |= KEYBOARD_MOD_SHIFT;
        } else {
            modifiers &= ~KEYBOARD_MOD_SHIFT;
        }   
    }

    if (event.key.ch == PS2_KEY_ALT) {
        if (event.action == KEYBOARD_ACTION_DOWN) {
            modifiers |= KEYBOARD_MOD_ALT;
        } else {
            modifiers &= ~KEYBOARD_MOD_ALT;
        } 
    }

    if (event.key.ch == PS2_KEY_CTRL) {
        if (event.action == KEYBOARD_ACTION_DOWN) {
            modifiers |= KEYBOARD_MOD_CTRL;
        } else {
            modifiers &= ~KEYBOARD_MOD_CTRL;
        } 
    }

    event.modifiers = modifiers;

    return event;
}


unsigned char keyboard_read_next(void) 
{
    unsigned char char_read = '!';
    while (true) {
        int has_shift = 0;
        int has_caps_lock = 0;
        key_event_t event = keyboard_read_event();

        // do nothing on a key release
        if (event.action == KEYBOARD_ACTION_UP) continue;

        // don't return modifier codes or non-characters
        if (event.key.ch == PS2_KEY_SHIFT ||
            event.key.ch == PS2_KEY_SCROLL_LOCK ||
            event.key.ch == PS2_KEY_NUM_LOCK ||
            event.key.ch == PS2_KEY_CAPS_LOCK ||
            event.key.ch == PS2_KEY_ALT ||
            event.key.ch == PS2_KEY_CTRL) continue;

        if ((event.modifiers >> 2 & 1) == 1) {
            has_caps_lock = 1;
        }
        if ((event.modifiers >> 3 & 1) == 1) {
            has_shift = 1;
        }

        // either shifted or caps locked character
        if ((has_shift || has_caps_lock) && !(has_shift && has_caps_lock)) {
            // caps lock shouldn't change non-letters
            if (has_caps_lock && !has_shift && 
                (event.key.ch < 'a' || event.key.ch > 'z')) {
                char_read = event.key.ch;
                break;
            }
            // edge case check if only one value
            // associated with key
            if (event.key.other_ch == 0) {
                char_read = event.key.ch;
                break;
            }
            char_read = event.key.other_ch;
            break;
        } else {
            // caps lock shouldn't change non-letters
            if (has_caps_lock && has_shift && 
                (event.key.ch < 'a' || event.key.ch > 'z')) {
                // edge case check if only one value
                // associated with key
                if (event.key.other_ch == 0) {
                    char_read = event.key.ch;
                    break;
                }
                char_read = event.key.other_ch;
                break;
            }
            // no caps lock or shift
            char_read = event.key.ch;
            break;
        }

    }

    return char_read;
}
