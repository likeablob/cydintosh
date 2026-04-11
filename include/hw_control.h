#ifndef HW_CONTROL_H
#define HW_CONTROL_H

#include <stdint.h>

typedef struct {
    uint8_t seq;
    uint8_t backlight;
    uint8_t led_r;
    uint8_t led_g;
    uint8_t led_b;
} HwState;

void hw_control_init(void);

void hw_set_backlight(uint8_t value);

void hw_set_led_rgb(uint8_t r, uint8_t g, uint8_t b);

HwState *hw_get_state(void);

uint8_t hw_get_seq(void);

#endif