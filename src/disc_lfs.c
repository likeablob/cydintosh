#include "disc.h"
#include "esp_littlefs.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

static const char *TAG = "disc_lfs";

static bool lfs_mounted = false;

int disc_lfs_init(void) {
    ESP_LOGI(TAG, "Initializing LittleFS");

    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/disk",
        .partition_label = "disk",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    esp_err_t err = esp_vfs_littlefs_register(&conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount LittleFS: %s", esp_err_to_name(err));
        return -1;
    }

    lfs_mounted = true;

    size_t total = 0, used = 0;
    esp_littlefs_info("disk", &total, &used);
    ESP_LOGI(TAG, "LittleFS mounted: %d KB total, %d KB used", total / 1024, used / 1024);
    return 0;
}

// Unused currently
void disc_lfs_deinit(void) {
    if (lfs_mounted) {
        esp_vfs_littlefs_unregister("disk");
        lfs_mounted = false;
    }
}

static int disc_lfs_read(void *ctx, uint8_t *data, unsigned int offset, unsigned int len) {
    FILE *f = (FILE *)ctx;
    if (!f) {
        ESP_LOGE(TAG, "read: file is NULL");
        return -1;
    }

    fseek(f, offset, SEEK_SET);
    size_t read = fread(data, 1, len, f);
    ESP_LOGD(TAG, "read: offset=%u len=%u read=%u", offset, len, (unsigned)read);
    return (read == len) ? 0 : -1;
}

static int disc_lfs_write(void *ctx, uint8_t *data, unsigned int offset, unsigned int len) {
    FILE *f = (FILE *)ctx;
    if (!f)
        return -1;

    fseek(f, offset, SEEK_SET);
    size_t written = fwrite(data, 1, len, f);
    fflush(f);
    return (written == len) ? 0 : -1;
}

int disc_lfs_open(disc_descr_t *disc, const char *filename, int read_only) {
    if (!lfs_mounted) {
        if (disc_lfs_init() != 0) {
            return -1;
        }
    }

    char path[64];
    snprintf(path, sizeof(path), "/disk/%s", filename);

    FILE *f = fopen(path, read_only ? "rb" : "r+b");
    if (!f) {
        if (!read_only) {
            f = fopen(path, "wb");
            if (!f) {
                ESP_LOGE(TAG, "Failed to open %s", path);
                return -1;
            }
            fclose(f);
            f = fopen(path, "r+b");
        }
        if (!f) {
            ESP_LOGE(TAG, "Failed to open %s", path);
            return -1;
        }
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    disc->op_ctx = f;
    disc->size = size;
    disc->read_only = read_only;
    disc->op_read = disc_lfs_read;
    disc->op_write = read_only ? NULL : disc_lfs_write;

    ESP_LOGI(TAG, "Opened %s, size=%d, ro=%d", path, (int)size, read_only);
    return 0;
}

// Unused currently
void disc_lfs_close(disc_descr_t *disc) {
    if (disc->op_ctx) {
        fclose((FILE *)disc->op_ctx);
        disc->op_ctx = NULL;
    }
}