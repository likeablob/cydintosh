#ifndef MQTT_DISCOVERY_H
#define MQTT_DISCOVERY_H

#include "user_config.h"
#include <stdint.h>

#define HA_BACKLIGHT_TOPIC_STATE  MQTT_TOPIC_BASE_DEVICE "/backlight/state"
#define HA_BACKLIGHT_TOPIC_SET    MQTT_TOPIC_BASE_DEVICE "/backlight/set"
#define HA_BACKLIGHT_CONFIG_TOPIC HA_DISCOVERY_PREFIX "/number/" HA_DEVICE_ID "/backlight/config"

#define HA_BACKLIGHT_UNIQUE_ID HA_DEVICE_ID "_backlight"
#define HA_BACKLIGHT_MIN       1
#define HA_BACKLIGHT_MAX       100
#define HA_BACKLIGHT_STEP      1

void mqtt_discovery_publish(void);
void mqtt_discovery_subscribe_commands(void);
void mqtt_discovery_handle_command(const char *topic, const char *data, int len);
void mqtt_discovery_publish_backlight_state(uint8_t value);

#endif