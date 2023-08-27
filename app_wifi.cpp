// ESP32CameraDemo - https://nodematrix.cc
// Copyright (c) 2023 Node Matrix
// MIT License

#include <string.h>
#include <nvs.h>
#include <esp_wifi_types.h>
#include <WiFi.h>

#include <esp_log.h>
static const char* TAG = "app_wifi";

typedef struct settings_s {
    wifi_mode_t wifi_mode;
    char ssid[32];
    char pwd[32];
    uint8_t channel;
} settings_t;

static const char* SETTINGS_NVS_KEY = "settings";
static settings_t* settings = NULL;

IPAddress ip = {0U};

static esp_err_t settings_save_to_nvs() {
    if (settings == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t ret = nvs_open(TAG, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to open nvs name \"%s\": 0x%x", TAG, ret);
        return ret;
    }

    ret = nvs_set_blob(handle, SETTINGS_NVS_KEY, settings, sizeof(settings_t));
    if (ret == ESP_OK) {
        ret = nvs_commit(handle);
    }
    else {
        ESP_LOGE(TAG, "failed to set nvs blob \"%s\": 0x%x", SETTINGS_NVS_KEY, ret);
    }
    nvs_close(handle);
    return ret;
}

static esp_err_t settings_load_from_nvs() {
    if (settings) free(settings);
    settings = NULL;

    settings = (settings_t*)malloc(sizeof(settings_t));
    if (!settings) {
        ESP_LOGE(TAG, "failed to allocate settings");
        return ESP_ERR_NO_MEM;
    }

    // defaults
    memset(settings, 0, sizeof(settings_t));
    settings->wifi_mode = WIFI_MODE_AP;
    settings->channel = 1;

    nvs_handle_t handle;
    esp_err_t ret = nvs_open(TAG, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        return ret;
    }

    // load settings
    size_t len = 0;
    ret = nvs_get_blob(handle, SETTINGS_NVS_KEY, NULL, &len);  // get actual length
    if (ret == ESP_OK) {
        if (len == sizeof(settings_t)) {
            ret = nvs_get_blob(handle, SETTINGS_NVS_KEY, settings, &len);
        }
        else {
            ret = ESP_ERR_NVS_INVALID_LENGTH;
        }
    }
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "nvs blob \"%s\" reading failed: 0x%x", SETTINGS_NVS_KEY, ret);
    }

    nvs_close(handle);
    return ret;
}

void startWiFi() {
    if (!settings) {
        settings_load_from_nvs();
    }
    if (!settings) return;

    if (settings->wifi_mode == WIFI_MODE_STA && settings->ssid[0]) {
        WiFi.begin(settings->ssid, settings->pwd);
        WiFi.setSleep(false);
        for (int t = 10; t > 0; t--) {
            delay(500);
            if (WiFi.status() == WL_CONNECTED) {
                ip = WiFi.localIP();
                ets_printf("WiFi connected, IP address: %s\n", ip.toString());
                return;
            }
        }
        WiFi.disconnect();
        delay(500);
    }

    // wifi_mode_ap or sta connection failed
    char ssid[32] = {0};
    char pwd[32] = {0};
    uint8_t channel = settings->channel;
    if (channel > 13) channel = 1;

    if (settings->wifi_mode == WIFI_MODE_AP && settings->ssid[0]) {
        strlcpy(ssid, settings->ssid, sizeof(ssid));
        strlcpy(pwd, settings->pwd, sizeof(pwd));
        if (pwd[0] && strlen(pwd) < 8) {
            pwd[0] = '\0';
        }
    }
    else {
        uint8_t mac[6];
        esp_efuse_mac_get_default(mac);
        sprintf(ssid, "Node-%02X%02X%02X", mac[3], mac[4], mac[5]);
    }
    WiFi.softAP(ssid, pwd, channel);
    ip = WiFi.softAPIP();
    ets_printf("SoftAP started, IP address: %s\n", ip.toString());
}

IPAddress get_wifi_ip() {
    return ip;
}

/*
 * Print settings json string to buffer.
 * {
 *   "wifi_mode": 1,
 *   "ssid": "31 chars max",
 *   "pwd": "31 chars max",
 *   "channel": 13
 * }
 *
 * Positive return value is print length, negative indicates buffer insufficient.
 */
int print_wifi_settings(char* buf, size_t size) {
    buf[0] = '\0';

    if (!settings) {
        settings_load_from_nvs();
    }
    if (!settings) return 0;

    int n = snprintf(buf, size,
                     "{\"wifi_mode\":%d,\"ssid\":\"%s\",\"pwd\":\"%s\",\"channel\":%d}",
                     settings->wifi_mode, settings->ssid, settings->pwd, settings->channel);
    if (n < 0 || n >= size) {
        return -1;
    }
    return n;  // length not including the null terminator
}

esp_err_t save_wifi_settings(wifi_mode_t wifi_mode, const char* ssid, const char* pwd, uint8_t channel) {
    if (!settings) {
        settings_load_from_nvs();
    }
    if (!settings) return ESP_FAIL;

    settings->wifi_mode = wifi_mode;
    strlcpy(settings->ssid, ssid, sizeof(settings->ssid));
    strlcpy(settings->pwd, pwd, sizeof(settings->pwd));
    settings->channel = channel;

    return settings_save_to_nvs();
}