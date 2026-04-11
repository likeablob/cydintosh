#include "mqtt_weather.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "stdlib.h"
#include "string.h"

static const char *TAG = "mqtt_weather";

static esp_mqtt_client_handle_t mqtt_client = NULL;
static WeatherData weather_data;

static uint8_t parse_icon(const char *str) {
    if (strstr(str, "sunny") || strstr(str, "clear"))
        return WEATHER_ICON_SUN;
    if (strstr(str, "partly"))
        return WEATHER_ICON_SUN_CLOUD;
    if (strstr(str, "cloud"))
        return WEATHER_ICON_CLOUD;
    if (strstr(str, "rain") || strstr(str, "shower") || strstr(str, "pouring"))
        return WEATHER_ICON_RAIN;
    if (strstr(str, "snow"))
        return WEATHER_ICON_SNOW;
    if (strstr(str, "fog") || strstr(str, "hail") || strstr(str, "lightning") ||
        strstr(str, "windy") || strstr(str, "exceptional"))
        return WEATHER_ICON_CLOUD;
    return WEATHER_ICON_SUN;
}

static int parse_topic_index(const char *topic, const char *prefix) {
    const char *p = strstr(topic, prefix);
    if (!p)
        return -1;
    p += strlen(prefix);
    while (*p >= '0' && *p <= '9') {
        int idx = atoi(p);
        if (idx >= 0 && idx < 10)
            return idx;
        p++;
    }
    return -1;
}

static void handle_current_topic(const char *topic, const char *data, int len) {
    if (strstr(topic, "/location")) {
        strncpy(weather_data.location, data, 31);
        weather_data.location[31] = '\0';
    } else if (strstr(topic, "/temp")) {
        weather_data.temp_current = atoi(data);
    } else if (strstr(topic, "/condition")) {
        weather_data.condition = parse_icon(data);
    } else if (strstr(topic, "/humidity")) {
        weather_data.humidity = atoi(data);
    } else if (strstr(topic, "/wind_speed")) {
        weather_data.wind_speed = atoi(data);
    }
}

static void handle_hourly_topic(const char *topic, const char *data, int len) {
    int idx = parse_topic_index(topic, "/hourly/");
    if (idx < 0 || idx >= 5)
        return;

    char *slash = strchr(topic + strlen(MQTT_TOPIC_BASE) + 9, '/');
    if (!slash)
        return;

    const char *field = slash + 1;
    if (strstr(field, "time")) {
        strncpy(weather_data.hourly[idx].time, data, 5);
        weather_data.hourly[idx].time[5] = '\0';
    } else if (strstr(field, "icon")) {
        weather_data.hourly[idx].icon = parse_icon(data);
    } else if (strstr(field, "temp")) {
        weather_data.hourly[idx].temp = atoi(data);
    }
}

static void handle_forecast_topic(const char *topic, const char *data, int len) {
    int idx = parse_topic_index(topic, "/forecast/");
    if (idx < 0 || idx >= 3)
        return;

    char *slash = strchr(topic + strlen(MQTT_TOPIC_BASE) + 10, '/');
    if (!slash)
        return;

    const char *field = slash + 1;
    if (strstr(field, "date")) {
        strncpy(weather_data.forecast[idx].date, data, 11);
        weather_data.forecast[idx].date[11] = '\0';
    } else if (strstr(field, "day")) {
        strncpy(weather_data.forecast[idx].day, data, 3);
        weather_data.forecast[idx].day[3] = '\0';
    } else if (strstr(field, "icon")) {
        weather_data.forecast[idx].icon = parse_icon(data);
    } else if (strstr(field, "hi")) {
        weather_data.forecast[idx].hi = atoi(data);
    } else if (strstr(field, "lo")) {
        weather_data.forecast[idx].lo = atoi(data);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id,
                               void *event_data) {
    esp_mqtt_event_handle_t event = event_data;

    switch (event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected, subscribing to %s", MQTT_TOPIC_SUBSCRIBE);
        esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC_SUBSCRIBE, MQTT_QOS);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "Topic: %.*s, Data: %.*s", event->topic_len, event->topic, event->data_len,
                 event->data);

        char topic[64];
        char data[64];
        int topic_len = event->topic_len < 63 ? event->topic_len : 63;
        int data_len = event->data_len < 63 ? event->data_len : 63;

        memcpy(topic, event->topic, topic_len);
        topic[topic_len] = '\0';
        memcpy(data, event->data, data_len);
        data[data_len] = '\0';

        if (strstr(topic, "/current/")) {
            handle_current_topic(topic, data, data_len);
        } else if (strstr(topic, "/hourly/")) {
            handle_hourly_topic(topic, data, data_len);
        } else if (strstr(topic, "/forecast/")) {
            handle_forecast_topic(topic, data, data_len);
        } else if (strstr(topic, "/updated")) {
            strncpy(weather_data.updated, data, 15);
            weather_data.updated[15] = '\0';
            weather_data.seq++;
            ESP_LOGI(TAG, "Weather data updated (seq=%d), ready for Mac poll", weather_data.seq);
        }
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT disconnected");
        break;

    default:
        break;
    }
}

void mqtt_weather_init(void) {
    memset(&weather_data, 0, sizeof(WeatherData));

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URL,
        .credentials.username = MQTT_USERNAME,
        .credentials.authentication.password = MQTT_PASSWORD,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
}

void mqtt_weather_start(void) {
    if (mqtt_client) {
        esp_mqtt_client_start(mqtt_client);
        ESP_LOGI(TAG, "MQTT client started");
    }
}

WeatherData *mqtt_weather_get_data(void) {
    return &weather_data;
}

uint8_t mqtt_weather_get_seq(void) {
    return weather_data.seq;
}