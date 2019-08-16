#include "gpio.h"

// GPIO pins 0-9
unsigned int* FSEL0 = (unsigned int*) 0x20200000;
// GPIO pins 10-19
unsigned int* FSEL1 = (unsigned int*) 0x20200004;
// GPIO pins 20-29
unsigned int* FSEL2 = (unsigned int*) 0x20200008;
// GPIO pins 30-39
unsigned int* FSEL3 = (unsigned int*) 0x2020000C;
// GPIO pins 40-49
unsigned int* FSEL4 = (unsigned int*) 0x20200010;
// GPIO pins 50-53
unsigned int* FSEL5 = (unsigned int*) 0x20200014;

// SET for pins 0-31
unsigned int* SET0 = (unsigned int*) 0x2020001C;
// SET for pins 32-53
unsigned int* SET1 = (unsigned int*) 0x20200020;

// CLR for pins 0-31
unsigned int* CLR0 = (unsigned int*) 0x20200028;
// CLR for pins 32-53
unsigned int* CLR1 = (unsigned int*) 0x2020002C;

// LEV for pins 0-31
unsigned int* LEV0 = (unsigned int*) 0x20200034;
// LEV for pins 32-53
unsigned int* LEV1 = (unsigned int*) 0x20200038;

void gpio_init(void) {
}

// helper function for determining FSEL based on pin
unsigned int* get_function_select(unsigned int pin) {
    int fselCount = pin / 10;
    switch (fselCount) {
        case 0 :
            return FSEL0;
        case 1 :
            return FSEL1;
        case 2 :
            return FSEL2;
        case 3 :
            return FSEL3;
        case 4 :
            return FSEL4;
        case 5 :
            return FSEL5;
        default : 
            return 0;
    }
}

void gpio_set_function(unsigned int pin, unsigned int function) {
    // check for valid pin input
    if (pin < GPIO_PIN_FIRST || pin > GPIO_PIN_LAST) {
        return;
    }

    // check for valid function input according to gpio.h
    // function list per manual
    if (function < 0 || function > 7) {
        return;
    }

    volatile unsigned int* functionSelect = get_function_select(pin);

    // get the offset for the pin
    int offset = (pin % 10) * 3;

    // set binary 111 on the GPIO pin to configure
    unsigned int firstMask = 0x7 << offset;
    firstMask = ~(firstMask);
    // secondMask stores what FSEL had except for
    // the pin to configure, which is now 000
    unsigned int secondMask = *functionSelect & firstMask;
    unsigned int functionMask = function << offset;
    // stores the updated FSEL config by orr-ing the 
    // function into secondMask
    *functionSelect = secondMask | functionMask;

}

unsigned int gpio_get_function(unsigned int pin) {
    if (pin < GPIO_PIN_FIRST || pin > GPIO_PIN_LAST) {
        return GPIO_INVALID_REQUEST;
    }

    volatile unsigned int* functionSelect = get_function_select(pin);

    // get the offset for the pin
    int offset = (pin % 10) * 3;

    unsigned int firstMask = 0x7 << offset;
    // masks all bits to 0 but preserves the given pin
    unsigned int secondMask = *functionSelect & firstMask;
    unsigned int function = secondMask >> offset;

    return function;
}

void gpio_set_input(unsigned int pin) {
    gpio_set_function(pin, GPIO_FUNC_INPUT);
}

void gpio_set_output(unsigned int pin) {
    gpio_set_function(pin, GPIO_FUNC_OUTPUT);
}

void gpio_write(unsigned int pin, unsigned int value) {
    // check for valid pin input
    if (pin < GPIO_PIN_FIRST || pin > GPIO_PIN_LAST) {
        return;
    }

    // check for valid value input
    if (value < 0 || value > 1) {
        return;
    }

    // either SET or CLR, 0 or 1
    volatile unsigned int* regAddress;

    // get CLR
    if (value == 0) {
        if (pin < GPIO_PIN32) {
            regAddress = CLR0;
        } else {
            regAddress = CLR1;
        }
    // get SET
    } else {
        if (pin < GPIO_PIN32) {
            regAddress = SET0;
        } else {
            regAddress = SET1;
        }
    }

    int offset = pin % 32;
    *regAddress = 1 << offset;
}

unsigned int gpio_read(unsigned int pin) {
    if (pin < GPIO_PIN_FIRST || pin > GPIO_PIN_LAST) {
        return GPIO_INVALID_REQUEST;
    }

    // either LEV0 or LEV1
    volatile unsigned int* levAddress;

    if (pin < GPIO_PIN32) {
        levAddress = LEV0;
    } else {
        levAddress = LEV1;
    }

    int offset = pin % 32;

    unsigned int value = *levAddress >> offset;
    // we are only interested in one bit
    value = value & 1;

    return value;
}
