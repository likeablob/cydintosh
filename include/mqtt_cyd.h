#ifndef MQTT_CYD_H
#define MQTT_CYD_H

#include "user_config.h"

#include <stdint.h>

#define WEATHER_ICON_SUN       0
#define WEATHER_ICON_CLOUD     1
#define WEATHER_ICON_RAIN      2
#define WEATHER_ICON_SNOW      3
#define WEATHER_ICON_SUN_CLOUD 4

typedef struct {
    char time[6];
    uint8_t icon;
    int8_t temp;
} HourlyForecast;

typedef struct {
    char date[12];
    char day[4];
    uint8_t icon;
    int8_t hi;
    int8_t lo;
} ForecastDay;

typedef struct {
    uint8_t seq;
    char location[32];
    int8_t temp_current;
    uint8_t condition;
    uint8_t humidity;
    uint8_t wind_speed;
    uint8_t wind_dir;
    HourlyForecast hourly[5];
    ForecastDay forecast[3];
    char updated[16];
} WeatherData;

void mqtt_client_init(void);
void mqtt_client_start(void);
WeatherData *mqtt_weather_get_data(void);
uint8_t mqtt_weather_get_seq(void);

#endif