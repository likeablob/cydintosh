#ifndef HW_H
#define HW_H

// ESP32-CYD (Cheap Yellow Display) Hardware Definitions

// LED - Use RGB LED on CYD (not TFT backlight)
// RGB LED pins are active low
#define LED_R_PIN    4
#define LED_G_PIN    16
#define LED_B_PIN    17
#define GPIO_LED_PIN LED_G_PIN // Use green LED for status

// TFT LCD (ILI9341) - SPI
#define TFT_SPI_MOSI 13
#define TFT_SPI_CLK  14
#define TFT_SPI_CS   15
#define TFT_SPI_MISO 12
#define TFT_DC       2
#define TFT_RESET    -1 // Not connected on CYD
#define TFT_BL       21

// Touch Screen (XPT2046) - separate SPI pins
#define TOUCH_MOSI 32
#define TOUCH_MISO 39
#define TOUCH_CLK  25
#define TOUCH_CS   33

// Touch calibration values (from CYD reference project)
#define TOUCH_MIN_X 200
#define TOUCH_MAX_X 3800
#define TOUCH_MIN_Y 250
#define TOUCH_MAX_Y 3850

// Screen dimensions (portrait mode)
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 320

// Boot button (GPIO0) - used as mouse click
#define BOOT_BUTTON_PIN 0

// LEDC PWM configuration for RGB LED
#define LEDC_TIMER     LEDC_TIMER_0
#define LEDC_MODE      LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL_R LEDC_CHANNEL_0
#define LEDC_CHANNEL_G LEDC_CHANNEL_1
#define LEDC_CHANNEL_B LEDC_CHANNEL_2
#define LEDC_DUTY_RES  LEDC_TIMER_13_BIT // 13-bit resolution (0-8191)
#define LEDC_FREQUENCY 5000              // 5kHz

#endif