#include "wifi_info.h"

#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include <string.h>

static const char *TAG = "wifi_info";

void wifi_get_ap_list(umac_ipc_response_t *resp) {
    wifi_scan_config_t config = {0};
    esp_wifi_scan_start(&config, true);

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);

    if (ap_count > 10)
        ap_count = 10;

    wifi_ap_record_t ap_records[10];
    esp_wifi_scan_get_ap_records(&ap_count, ap_records);

    resp->data[0] = (uint8_t)ap_count;
    int pos = 1;

    for (int i = 0; i < ap_count; i++) {
        uint8_t ssid_len = strlen((char *)ap_records[i].ssid);
        resp->data[pos++] = ssid_len;
        memcpy(&resp->data[pos], ap_records[i].ssid, ssid_len);
        pos += ssid_len;
        resp->data[pos++] = (uint8_t)ap_records[i].rssi;
        resp->data[pos++] = ap_records[i].primary;
    }

    resp->len = pos;
    resp->status = 0;
}

void wifi_get_status(umac_ipc_response_t *resp) {
    wifi_ap_record_t ap_info;
    esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);

    if (err == ESP_OK) {
        resp->data[0] = 2;
        uint8_t ssid_len = strlen((char *)ap_info.ssid);
        resp->data[1] = ssid_len;
        memcpy(&resp->data[2], ap_info.ssid, ssid_len);

        int ip_pos = 2 + ssid_len;
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        esp_netif_ip_info_t ip_info;
        if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
            resp->data[ip_pos] = esp_ip4_addr_get_byte(&ip_info.ip, 0);
            resp->data[ip_pos + 1] = esp_ip4_addr_get_byte(&ip_info.ip, 1);
            resp->data[ip_pos + 2] = esp_ip4_addr_get_byte(&ip_info.ip, 2);
            resp->data[ip_pos + 3] = esp_ip4_addr_get_byte(&ip_info.ip, 3);
        } else {
            memset(&resp->data[ip_pos], 0, 4);
        }
        resp->len = ip_pos + 4;
    } else {
        resp->data[0] = 0;
        resp->len = 1;
    }
    resp->status = 0;
}