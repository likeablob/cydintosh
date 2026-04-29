#include "umac_ipc.h"

#include "esp_log.h"
#include "hw_control.h"
#include "mqtt_cyd.h"
#include "string.h"
#include "wifi_info.h"

static const char *TAG = "umac_ipc";

static umac_ipc_request_t req;
static umac_ipc_response_t resp;

static void process_command(void) {
    resp.status = 0;
    resp.len = 0;

    switch (req.cmd) {
    case CMD_PING:
        resp.data[0] = req.data[0];
        resp.len = 1;
        break;

    case CMD_GET_WIFI_LIST:
        wifi_get_ap_list(&resp);
        break;

    case CMD_GET_WIFI_STATUS:
        wifi_get_status(&resp);
        break;

    case CMD_GET_WEATHER_DATA:
        if (req.len >= 1) {
            uint8_t last_seq = req.data[0];
            uint8_t current_seq = mqtt_weather_get_seq();

            if (current_seq != last_seq) {
                WeatherData *weather = mqtt_weather_get_data();
                size_t data_size = sizeof(WeatherData);
                if (data_size <= 254) {
                    memcpy(resp.data, weather, data_size);
                    resp.len = data_size;
                    resp.status = WEATHER_STATUS_UPDATED;
                    ESP_LOGI(TAG, "Weather data sent (seq %d -> %d)", last_seq, current_seq);
                } else {
                    resp.status = 0xFF;
                    ESP_LOGE(TAG, "WeatherData too large for IPC");
                }
            } else {
                resp.status = WEATHER_STATUS_NOT_CHANGED;
                ESP_LOGI(TAG, "Weather seq unchanged (%d)", current_seq);
            }
        } else {
            resp.status = 0xFF;
        }
        break;

    case CMD_GET_HW_STATE:
        if (req.len >= 1) {
            uint8_t last_seq = req.data[0];
            uint8_t current_seq = hw_get_seq();

            if (current_seq != last_seq) {
                HwState *state = hw_get_state();
                size_t data_size = sizeof(HwState);
                if (data_size <= 254) {
                    memcpy(resp.data, state, data_size);
                    resp.len = data_size;
                    resp.status = HW_STATUS_UPDATED;
                    ESP_LOGI(TAG, "HW state sent (seq %d -> %d)", last_seq, current_seq);
                } else {
                    resp.status = 0xFF;
                }
            } else {
                resp.status = HW_STATUS_NOT_CHANGED;
            }
        } else {
            resp.status = 0xFF;
        }
        break;

    case CMD_SET_BACKLIGHT:
        if (req.len >= 1) {
            hw_set_backlight(req.data[0]);
            resp.status = 0;
        } else {
            resp.status = 0xFF;
        }
        break;

    case CMD_SET_LED_RGB:
        if (req.len >= 3) {
            hw_set_led_rgb(req.data[0], req.data[1], req.data[2]);
            resp.status = 0;
        } else {
            resp.status = 0xFF;
        }
        break;

    default:
        resp.status = 0xFF;
        break;
    }
}

void umac_ipc_init(void) {
    memset(&req, 0, sizeof(req));
    memset(&resp, 0, sizeof(resp));
}

void umac_ipc_write(uint8_t offset, uint8_t value) {
    if (offset == 0) {
        req.cmd = value;
    } else if (offset == 1) {
        req.len = value;
        process_command();
    } else if (offset < UMAC_IPC_RANGE) {
        req.data[offset - 2] = value;
    }
}

uint8_t umac_ipc_read(uint8_t offset) {
    if (offset == 0) {
        return resp.status;
    } else if (offset == 1) {
        return resp.len;
    } else if (offset < UMAC_IPC_RANGE) {
        return resp.data[offset - 2];
    }
    return 0;
}