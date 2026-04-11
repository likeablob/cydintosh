#include "hw_control.h"

#include "esp_log.h"
#include "hw.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "driver/ledc.h"

#include <string.h>

static const char *TAG = "hw_control";

static HwState hw_state = {.seq = 0, .backlight = 100, .led_r = 0, .led_g = 0, .led_b = 0};

static nvs_handle_t hw_nvs_handle;

static void load_state(void) {
    esp_err_t err;

    err = nvs_open("hw_ctrl", NVS_READWRITE, &hw_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return;
    }

    err = nvs_get_u8(hw_nvs_handle, "backlight", &hw_state.backlight);
    if (err != ESP_OK) {
        hw_state.backlight = 100;
    }

    err = nvs_get_u8(hw_nvs_handle, "led_r", &hw_state.led_r);
    if (err != ESP_OK) {
        hw_state.led_r = 0;
    }

    err = nvs_get_u8(hw_nvs_handle, "led_g", &hw_state.led_g);
    if (err != ESP_OK) {
        hw_state.led_g = 0;
    }

    err = nvs_get_u8(hw_nvs_handle, "led_b", &hw_state.led_b);
    if (err != ESP_OK) {
        hw_state.led_b = 0;
    }

    if (hw_state.backlight == 0)
        hw_state.backlight = 1;

    ESP_LOGI(TAG, "Loaded state: BL=%d R=%d G=%d B=%d", hw_state.backlight, hw_state.led_r,
             hw_state.led_g, hw_state.led_b);
}

static void save_state(void) {
    nvs_set_u8(hw_nvs_handle, "backlight", hw_state.backlight);
    nvs_set_u8(hw_nvs_handle, "led_r", hw_state.led_r);
    nvs_set_u8(hw_nvs_handle, "led_g", hw_state.led_g);
    nvs_set_u8(hw_nvs_handle, "led_b", hw_state.led_b);
    nvs_commit(hw_nvs_handle);
}

static void ledc_init(void) {
    ledc_timer_config_t timer_conf = {.speed_mode = LEDC_MODE,
                                      .duty_resolution = LEDC_DUTY_RES,
                                      .timer_num = LEDC_TIMER,
                                      .freq_hz = LEDC_FREQUENCY,
                                      .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t ledc_ch = {.speed_mode = LEDC_MODE,
                                     .intr_type = LEDC_INTR_DISABLE,
                                     .timer_sel = LEDC_TIMER,
                                     .duty = 0,
                                     .hpoint = 0};

    ledc_ch.channel = LEDC_CHANNEL_R;
    ledc_ch.gpio_num = LED_R_PIN;
    ledc_channel_config(&ledc_ch);

    ledc_ch.channel = LEDC_CHANNEL_G;
    ledc_ch.gpio_num = LED_G_PIN;
    ledc_channel_config(&ledc_ch);

    ledc_ch.channel = LEDC_CHANNEL_B;
    ledc_ch.gpio_num = LED_B_PIN;
    ledc_channel_config(&ledc_ch);
}

static void ledc_init_backlight(void) {
    ledc_timer_config_t timer_conf = {.speed_mode = LEDC_MODE,
                                      .duty_resolution = LEDC_DUTY_RES,
                                      .timer_num = LEDC_TIMER_1,
                                      .freq_hz = LEDC_FREQUENCY,
                                      .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t ledc_ch = {.speed_mode = LEDC_MODE,
                                     .intr_type = LEDC_INTR_DISABLE,
                                     .timer_sel = LEDC_TIMER_1,
                                     .channel = LEDC_CHANNEL_3,
                                     .duty = 8191,
                                     .hpoint = 0,
                                     .gpio_num = TFT_BL};
    ledc_channel_config(&ledc_ch);
}

static void set_led_channel(ledc_channel_t channel, uint8_t value) {
    uint32_t duty = (8191 * value) / 255;
    ledc_set_duty(LEDC_MODE, channel, duty);
    ledc_update_duty(LEDC_MODE, channel);
}

void hw_control_init(void) {
    load_state();

    ledc_init();
    ledc_init_backlight();

    hw_set_backlight(hw_state.backlight);
    hw_set_led_rgb(hw_state.led_r, hw_state.led_g, hw_state.led_b);

    ESP_LOGI(TAG, "HW control initialized");
}

void hw_set_backlight(uint8_t value) {
    if (value > 100)
        value = 100;
    if (value == 0)
        value = 1;

    hw_state.backlight = value;
    hw_state.seq++;

    uint32_t duty = (8191 * value) / 100;
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_3, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_3);

    save_state();

    ESP_LOGI(TAG, "Backlight set to %d%% (duty=%lu)", value, duty);
}

void hw_set_led_rgb(uint8_t r, uint8_t g, uint8_t b) {
    hw_state.led_r = r;
    hw_state.led_g = g;
    hw_state.led_b = b;
    hw_state.seq++;

    set_led_channel(LEDC_CHANNEL_R, 255 - r);
    set_led_channel(LEDC_CHANNEL_G, 255 - g);
    set_led_channel(LEDC_CHANNEL_B, 255 - b);

    save_state();

    ESP_LOGI(TAG, "LED set to R:%d G:%d B:%d", r, g, b);
}

HwState *hw_get_state(void) {
    return &hw_state;
}

uint8_t hw_get_seq(void) {
    return hw_state.seq;
}