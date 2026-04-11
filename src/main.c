#include "disc_lfs.h"
#include "display.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "hw.h"
#include "hw_control.h"
#include "lcd_cyd.h"
#include "mqtt_weather.h"
#include "nvs_flash.h"
#include "rom.h"
#include "touch_cyd.h"
#include "umac.h"
#include "umac_ipc.h"
#include "user_config.h"
#include "video.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "cydintosh";

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                               void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "WiFi connected, starting MQTT weather client");
        mqtt_weather_init();
        mqtt_weather_start();
    }
}

// Benchmark macros
#define BENCH_START() int64_t _bench_t0 = esp_timer_get_time()
#define BENCH_END(label)                                      \
    do {                                                      \
        int64_t _bench_dt = esp_timer_get_time() - _bench_t0; \
        ESP_LOGI("BENCH", "%s: %lld us", label, _bench_dt);   \
    } while (0)

// Benchmark counters
static int64_t bench_umac_loop_total = 0;
static int64_t bench_video_update_total = 0;
static int bench_frame_count = 0;
static const int BENCH_REPORT_INTERVAL = 60; // Report every N frames

static uint8_t *umac_ram = NULL;
static uint8_t *umac_fb = NULL;        // Framebuffer for Mac
static const uint8_t *rom_mmap = NULL; // Flash mmap pointer

#define UMAC_ROM_SIZE 0x20000
#define FB_SIZE       (DISP_WIDTH * DISP_HEIGHT / 8) // Framebuffer size in bytes

static const uint8_t umac_disc_fallback[] = {0x4c, 0x4b};

static disc_descr_t discs[DISC_NUM_DRIVES] = {0};

static void disc_setup(void) {
    if (disc_lfs_open(&discs[0], "disk.img", 1) == 0) {
        ESP_LOGI(TAG, "Using LittleFS disk image (read-only)");
    } else {
        ESP_LOGW(TAG, "No disk image, using fallback");
        discs[0].base = (void *)umac_disc_fallback;
        discs[0].read_only = 1;
        discs[0].size = sizeof(umac_disc_fallback);
    }
}

static int rom_mmap_from_partition(const uint8_t **rom_ptr, size_t *rom_size) {
    ESP_LOGI(TAG, "Mapping ROM from partition...");

    // Find ROM partition (type 0x01, subtype 0x40)
    const esp_partition_t *rom_partition =
        esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x40, "rom");

    if (rom_partition == NULL) {
        ESP_LOGE(TAG, "ROM partition not found!");
        return -1;
    }

    ESP_LOGI(TAG, "Found ROM partition at offset 0x%x, size %u bytes",
             (unsigned)rom_partition->address, (unsigned)rom_partition->size);

    if (rom_partition->size < UMAC_ROM_SIZE) {
        ESP_LOGW(TAG, "ROM partition smaller than expected: %u < %u", (unsigned)rom_partition->size,
                 UMAC_ROM_SIZE);
    }

    // Map partition to memory (read-only, data)
    esp_partition_mmap_handle_t mmap_handle;
    esp_err_t err = esp_partition_mmap(rom_partition, 0, UMAC_ROM_SIZE, ESP_PARTITION_MMAP_DATA,
                                       (const void **)rom_ptr, &mmap_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mmap ROM partition: %s", esp_err_to_name(err));
        return -1;
    }

    *rom_size = UMAC_ROM_SIZE;

    ESP_LOGI(TAG, "ROM mapped successfully at %p (handle: %u)", (void *)*rom_ptr,
             (unsigned)mmap_handle);
    ESP_LOGI(TAG,
             "First 16 bytes: %02x %02x %02x %02x %02x %02x %02x %02x %02x "
             "%02x %02x %02x %02x %02x %02x %02x",
             (*rom_ptr)[0], (*rom_ptr)[1], (*rom_ptr)[2], (*rom_ptr)[3], (*rom_ptr)[4],
             (*rom_ptr)[5], (*rom_ptr)[6], (*rom_ptr)[7], (*rom_ptr)[8], (*rom_ptr)[9],
             (*rom_ptr)[10], (*rom_ptr)[11], (*rom_ptr)[12], (*rom_ptr)[13], (*rom_ptr)[14],
             (*rom_ptr)[15]);

    return 0;
}

// LEDC PWM configuration for RGB LED fade effect
static void led_pwm_init(void) {
    // Configure timer
    ledc_timer_config_t ledc_timer = {.speed_mode = LEDC_MODE,
                                      .timer_num = LEDC_TIMER,
                                      .duty_resolution = LEDC_DUTY_RES,
                                      .freq_hz = LEDC_FREQUENCY,
                                      .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Configure channels for R, G, B
    ledc_channel_config_t ledc_channel_r = {.gpio_num = LED_R_PIN,
                                            .speed_mode = LEDC_MODE,
                                            .channel = LEDC_CHANNEL_R,
                                            .timer_sel = LEDC_TIMER,
                                            .duty = 0, // OFF (active low: 0 = HIGH = OFF)
                                            .hpoint = 0};
    ledc_channel_config_t ledc_channel_g = {.gpio_num = LED_G_PIN,
                                            .speed_mode = LEDC_MODE,
                                            .channel = LEDC_CHANNEL_G,
                                            .timer_sel = LEDC_TIMER,
                                            .duty = 0, // OFF
                                            .hpoint = 0};
    ledc_channel_config_t ledc_channel_b = {.gpio_num = LED_B_PIN,
                                            .speed_mode = LEDC_MODE,
                                            .channel = LEDC_CHANNEL_B,
                                            .timer_sel = LEDC_TIMER,
                                            .duty = 0, // OFF
                                            .hpoint = 0};

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_r));
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_g));
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_b));
}

// Set RGB LED brightness (0-100%)
// Note: Active low - duty=0 is OFF, duty=max is ON
static void led_set_rgb(uint8_t r_percent, uint8_t g_percent, uint8_t b_percent) {
    const uint32_t max_duty = (1 << LEDC_DUTY_RES) - 1;

    uint32_t r_duty = max_duty - ((r_percent * max_duty) / 100);
    uint32_t g_duty = max_duty - ((g_percent * max_duty) / 100);
    uint32_t b_duty = max_duty - ((b_percent * max_duty) / 100);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_R, r_duty);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_G, g_duty);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_B, b_duty);

    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_R);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_G);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_B);
}

// Smooth fade animation: fade in then fade out
// duration_ms: total duration (fade in + fade out)
// max_brightness: peak brightness (0-100%)
static void led_fade_white(uint32_t duration_ms, uint8_t max_brightness) {
    const uint32_t fade_in_steps = 30;  // Number of steps for fade in
    const uint32_t fade_out_steps = 30; // Number of steps for fade out
    const uint32_t step_delay_ms = duration_ms / (fade_in_steps + fade_out_steps);

    ESP_LOGI(TAG, "LED fade start: %d%% brightness, %lu ms total", max_brightness, duration_ms);

    // Fade in: 0% -> max_brightness
    for (uint32_t i = 0; i <= fade_in_steps; i++) {
        uint8_t brightness = (max_brightness * i) / fade_in_steps;
        // Ease-in-out: apply smooth curve
        float t = (float)i / fade_in_steps;
        float eased = t * t * (3.0f - 2.0f * t); // Smoothstep
        uint8_t eased_brightness = (uint8_t)(max_brightness * eased);

        led_set_rgb(eased_brightness, eased_brightness, eased_brightness);
        vTaskDelay(pdMS_TO_TICKS(step_delay_ms));
    }

    // Fade out: max_brightness -> 0%
    for (uint32_t i = fade_out_steps; i > 0; i--) {
        float t = (float)i / fade_out_steps;
        float eased = t * t * (3.0f - 2.0f * t); // Smoothstep
        uint8_t eased_brightness = (uint8_t)(max_brightness * eased);

        led_set_rgb(eased_brightness, eased_brightness, eased_brightness);
        vTaskDelay(pdMS_TO_TICKS(step_delay_ms));
    }

    // Ensure LED is OFF
    led_set_rgb(0, 0, 0);
    ESP_LOGI(TAG, "LED fade complete");
}

static void umac_task(void *arg) {
    ESP_LOGI(TAG, "umac task started");

    // Map ROM from partition (no SRAM copy needed)
    size_t rom_mapped_size;
    if (rom_mmap_from_partition(&rom_mmap, &rom_mapped_size) != 0) {
        ESP_LOGE(TAG, "Failed to map ROM from partition!");
        return;
    }
    ESP_LOGI(TAG, "ROM mapped at %p, size: %u bytes", (void *)rom_mmap, rom_mapped_size);

    disc_setup();

    umac_init(umac_ram, (void *)rom_mmap, umac_fb, discs);

    // Video uses stable display buffer snapshot
    video_init((uint32_t *)umac_fb);

    ESP_LOGI(TAG, "Starting emulation");
    ESP_LOGI(TAG, "FB offset: 0x%x", umac_get_fb_offset());

    int emulated_cycles = 0;
    int frame_count = 0;
    int yield_counter = 0;
    const int cycles_per_frame = 133000;

    // Benchmark timing
    int64_t bench_last_report = esp_timer_get_time();

    while (1) {
        BENCH_START();
        umac_loop();
        bench_umac_loop_total += esp_timer_get_time() - _bench_t0;

        emulated_cycles += 40000;

        if (emulated_cycles >= cycles_per_frame) {
            // Measure video_update
            BENCH_START();

            umac_vsync_event();
            video_update();
            bench_video_update_total += esp_timer_get_time() - _bench_t0;

            emulated_cycles -= cycles_per_frame;
            frame_count++;
            yield_counter++;
            bench_frame_count++;

            // Report benchmark results
            if (bench_frame_count >= BENCH_REPORT_INTERVAL) {
                int64_t now = esp_timer_get_time();
                int64_t elapsed_ms = (now - bench_last_report) / 1000;
                ESP_LOGI("BENCH", "=== Report (last %d frames, %lld ms) ===", bench_frame_count,
                         elapsed_ms);
                ESP_LOGI("BENCH", "umac_loop avg: %lld us/frame",
                         bench_umac_loop_total / bench_frame_count);
                ESP_LOGI("BENCH", "video_update avg: %lld us/frame",
                         bench_video_update_total / bench_frame_count);
                ESP_LOGI("BENCH", "total frame time: %lld us",
                         elapsed_ms * 1000 / bench_frame_count);
                ESP_LOGI("BENCH", "estimated FPS: %d",
                         (int)(bench_frame_count * 1000 / elapsed_ms));

                // Reset counters
                bench_umac_loop_total = 0;
                bench_video_update_total = 0;
                bench_frame_count = 0;
                bench_last_report = now;
            }

            // 1Hz event every 60 frames
            if (frame_count >= 60) {
                umac_1hz_event();
                frame_count = 0;
            }

            // Check for mouse events from touch task (accumulate all pending
            // deltas)
            mouse_delta_t mouse;
            int32_t accum_dx = 0, accum_dy = 0;
            uint8_t touch_pressed = 0;
            uint8_t touch_event_count = 0;
            QueueHandle_t touch_mouse_queue = touch_get_mouse_queue();

            // Read all pending mouse deltas and accumulate
            while (xQueueReceive(touch_mouse_queue, &mouse, 0)) {
                touch_event_count++;
                accum_dx += mouse.dx;
                accum_dy += mouse.dy;
                // If ANY event has pressed=1, keep touch_pressed=1 (sticky)
                if (mouse.pressed)
                    touch_pressed = 1;
            }

            // Read BOOT button (GPIO0) state for click (coexist with
            // double-tap) GPIO0 is active LOW (pulled up, pressing connects to
            // GND)
            uint8_t button_pressed = (gpio_get_level(BOOT_BUTTON_PIN) == 0) ? 1 : 0;

            // Track button state changes for proper release detection
            static uint8_t last_button_state = 0;

            // Combine button and touch states (OR logic for coexistence)
            uint8_t final_button = button_pressed | touch_pressed;

            // Log mouse button events for debugging
            if (final_button && !last_button_state) {
                ESP_LOGI("MOUSE",
                         "CLICK DETECTED: touch_events=%d, touch_pressed=%d, "
                         "button_pressed=%d",
                         touch_event_count, touch_pressed, button_pressed);
            }

            // Apply accumulated movement with button state
            uint8_t final_button_changed = (final_button != last_button_state);
            if (accum_dx != 0 || accum_dy != 0 || final_button || final_button_changed) {
                umac_mouse((int16_t)accum_dx, (int16_t)accum_dy, final_button);
                last_button_state = final_button;
            }

            // Yield every 10 frames for watchdog
            if (yield_counter >= 10) {
                taskYIELD();
                yield_counter = 0;
            }
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Cydintosh starting...");

    // Initialize LCD FIRST (before any large allocations)
    lcd_cyd_init();
    display_init();

    // Initialize RGB LED
    led_pwm_init();

    // Boot fade effect: white, 50% brightness, 600ms total
    led_fade_white(600, 50);

    // Initialize BOOT button (GPIO0) as input with pull-up
    gpio_reset_pin(BOOT_BUTTON_PIN);
    gpio_set_direction(BOOT_BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BOOT_BUTTON_PIN, GPIO_PULLUP_ONLY);

    // Internal RAM - allocate Mac RAM BEFORE WiFi (WiFi consumes RAM)
    umac_ram = (uint8_t *)heap_caps_malloc(RAM_SIZE, MALLOC_CAP_INTERNAL);
    if (umac_ram == NULL) {
        ESP_LOGE(TAG, "Failed to allocate %d bytes from internal RAM!", RAM_SIZE);
        return;
    }
    ESP_LOGI(TAG, "Allocated %d bytes Mac RAM at %p (internal)", RAM_SIZE, umac_ram);
    memset(umac_ram, 0, RAM_SIZE);

    // Allocate separate framebuffer
    umac_fb = (uint8_t *)heap_caps_malloc(FB_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    if (umac_fb == NULL) {
        ESP_LOGE(TAG, "Failed to allocate %d bytes for framebuffer!", FB_SIZE);
        return;
    }
    ESP_LOGI(TAG, "Allocated %d bytes framebuffer at %p (internal DMA)", FB_SIZE, umac_fb);
    memset(umac_fb, 0, FB_SIZE);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_config_t wifi_config = {0};
    strcpy((char *)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char *)wifi_config.sta.password, WIFI_PASSWORD);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
    ESP_LOGI(TAG, "WiFi connecting to %s...", WIFI_SSID);

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED,
                                               &wifi_event_handler, NULL));

    hw_control_init();
    umac_ipc_init();

    ESP_LOGI(TAG, "WiFi and IPC initialized");

    size_t free_heap = esp_get_free_heap_size();
    size_t min_free = esp_get_minimum_free_heap_size();
    ESP_LOGI(TAG, "Free heap: %d bytes, min free: %d bytes", free_heap, min_free);

    // Initialize touch
    touch_init();

    // Create touch queue and start polling task on Core 0
    touch_create_queue();
    touch_start_task(0, 3);

    // Start display task on Core 0 (lower priority than touch)
    display_task_start(0, 2);

    ESP_LOGI(TAG, "Touch and display tasks started on Core 0");

    xTaskCreatePinnedToCore(umac_task, "umac", 32768, NULL, 5, NULL, 1);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}