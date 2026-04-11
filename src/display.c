#include "display.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "lcd_cyd.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

static const char *TAG = "display";

static uint16_t *rgb_buf = NULL; // Single RGB buffer for vertical line transfer
static uint8_t *fb_copy = NULL;  // Framebuffer copy for safe transfer

#define RGB_BUF_LINES 4

#ifndef DISP_WIDTH
#define DISP_WIDTH 240
#endif
#ifndef DISP_HEIGHT
#define DISP_HEIGHT 320
#endif

#define FB_COPY_SIZE (DISP_WIDTH * DISP_HEIGHT / 8)

static TaskHandle_t display_task_handle = NULL;
static const uint8_t *current_framebuffer = NULL;
static volatile bool frame_pending = false;

void display_init(void) {
    // Allocate RGB buffer for batch vertical line transfer
    rgb_buf = heap_caps_malloc(DISP_HEIGHT * RGB_BUF_LINES * 2, MALLOC_CAP_8BIT);
    if (rgb_buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate RGB buffer");
        return;
    }
    ESP_LOGI(TAG, "Allocated RGB buffer: %d bytes for %d lines", DISP_HEIGHT * RGB_BUF_LINES * 2,
             RGB_BUF_LINES);

    fb_copy = heap_caps_malloc(FB_COPY_SIZE, MALLOC_CAP_8BIT);
    if (fb_copy == NULL) {
        ESP_LOGE(TAG, "Failed to allocate framebuffer copy");
        return;
    }
    ESP_LOGI(TAG, "Allocated framebuffer copy: %d bytes", FB_COPY_SIZE);
}

void display_notify_update(void) {
    if (display_task_handle) {
        frame_pending = true;
        xTaskNotifyGive(display_task_handle);
    }
}

static void display_task(void *arg) {
    ESP_LOGI(TAG, "Display task started on Core %d", xPortGetCoreID());

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (frame_pending && current_framebuffer) {
            // Copy framebuffer at the start of transfer
            // This copy is held until the full LCD update completes
            memcpy(fb_copy, current_framebuffer, FB_COPY_SIZE);
            frame_pending = false;

            // Now transfer from fb_copy (umac can continue writing to original
            // fb)
            const int mac_bytes_per_row = DISP_WIDTH / 8;
            const int lcd_width = DISP_WIDTH;
            const int lcd_height = DISP_HEIGHT;

            // Batch vertical line transfer with synchronous completion
            for (int x = 0; x < lcd_width; x += RGB_BUF_LINES) {
                int lines = (x + RGB_BUF_LINES > lcd_width) ? (lcd_width - x) : RGB_BUF_LINES;

                // Collect RGB_BUF_LINES vertical lines into buffer
                for (int line = 0; line < lines; line++) {
                    int px = x + line;
                    int byte_offset_base = px / 8;
                    int bit_offset = 7 - (px % 8);

                    for (int y = 0; y < lcd_height; y++) {
                        int byte_offset = y * mac_bytes_per_row + byte_offset_base;
                        uint8_t pixel = (fb_copy[byte_offset] >> bit_offset) & 1;
                        rgb_buf[y * lines + line] = pixel ? 0xFFFF : 0x0000;
                    }
                }

                // Draw batch vertical lines and wait for completion
                lcd_draw_bitmap(x, 0, lines, lcd_height, (const uint8_t *)rgb_buf);
                lcd_wait_trans_complete();
            }
        }
    }
}

void display_task_start(int core, int priority) {
    if (display_task_handle != NULL) {
        ESP_LOGW(TAG, "Display task already running");
        return;
    }

    BaseType_t ret = xTaskCreatePinnedToCore(display_task, "display_task", 4096, NULL, priority,
                                             &display_task_handle, core);

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create display task");
    } else {
        ESP_LOGI(TAG, "Display task created on Core %d with priority %d", core, priority);
    }
}

void display_set_framebuffer(const uint8_t *fb) {
    current_framebuffer = fb;
}