#include "video.h"

#include "display.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

static const char *TAG = "video";
static uint32_t *video_framebuffer = NULL;

void video_init(uint32_t *framebuffer) {
    ESP_LOGI(TAG, "Video init");
    video_framebuffer = framebuffer;
    display_set_framebuffer((const uint8_t *)framebuffer);
}

void video_update(void) {
    if (video_framebuffer) {
        display_notify_update();
    }
}