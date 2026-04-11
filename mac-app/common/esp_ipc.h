#ifndef ESP_IPC_H
#define ESP_IPC_H

#include <stdint.h>

#define IPC_BASE_ADDR 0xF00000UL

// CMD assignments by application (for future extensibility)
// 0x0x: Common/System
// 0x1x: WiFi (WiFiApp)
// 0x2x: Weather (WeatherApp)
// 0x3x: Hardware Control (CydCtlApp)

#define CMD_PING 0x00

#define CMD_GET_WIFI_LIST   0x10
#define CMD_GET_WIFI_STATUS 0x11

#define CMD_GET_WEATHER_DATA 0x20

#define CMD_GET_HW_STATE  0x30
#define CMD_SET_BACKLIGHT 0x31
#define CMD_SET_LED_RGB   0x32

#define WEATHER_STATUS_UPDATED     0x00
#define WEATHER_STATUS_NOT_CHANGED 0x01

#define HW_STATUS_UPDATED     0x00
#define HW_STATUS_NOT_CHANGED 0x01

void ipc_write(uint8_t offset, uint8_t value);
uint8_t ipc_read(uint8_t offset);

void ipc_send_command(uint8_t cmd, uint8_t len, uint8_t *data);
uint8_t ipc_get_response(uint8_t *len, uint8_t *data, uint8_t max_len);

uint8_t ipc_ping(uint8_t test_byte);

#endif