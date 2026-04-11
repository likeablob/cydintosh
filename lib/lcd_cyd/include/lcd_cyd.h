#ifndef LCD_CYD_H
#define LCD_CYD_H

#include <inttypes.h>

void lcd_cyd_init(void);
void lcd_draw_bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data);
void lcd_wait_trans_complete(void);

#endif