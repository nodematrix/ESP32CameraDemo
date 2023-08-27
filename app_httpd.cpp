// ESP32CameraDemo - https://nodematrix.cc
// Copyright (c) 2023 Node Matrix
// MIT License

#include <esp_timer.h>
#include <esp_camera.h>
#include <esp_wifi_types.h>
#include <esp_http_server.h>
#include <IPAddress.h>
#include "index.h"

#include <esp_log.h>
static const char *TAG = "app_httpd";

#define PART_BOUNDARY "1234567890987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const size_t _BOUNDARY_LEN = strlen(_STREAM_BOUNDARY);
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t camera_httpd = NULL;
httpd_handle_t stream_httpd = NULL;

// app_wifi.cpp
IPAddress get_wifi_ip();
int print_wifi_settings(char *buf, size_t size);
esp_err_t save_wifi_settings(wifi_mode_t wifi_mode, const char *ssid, const char *pwd, uint8_t channel);

// fps filter
#define FPS_POOL_SIZE 8U
typedef struct fps_pool_s {
    uint32_t index;
    uint32_t sum;
    uint32_t values[FPS_POOL_SIZE];
} fps_pool_t;

static fps_pool_t fps_pool = {0, 0, {0}};

static uint32_t frame_filter(uint32_t value) {
    uint32_t i = fps_pool.index;
    fps_pool.sum -= fps_pool.values[i];
    fps_pool.values[i] = value;
    fps_pool.sum += value;
    fps_pool.index = (i + 1) % FPS_POOL_SIZE;
    return fps_pool.sum / FPS_POOL_SIZE;
}

static esp_err_t still_handler(httpd_req_t *req) {
    esp_err_t ret = ESP_OK;
    camera_fb_t *fb = NULL;

    fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "failed to get fb");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    ret = httpd_resp_send(req, (const char *)fb->buf, fb->len);

    esp_camera_fb_return(fb);
    return ret;
}

static esp_err_t stream_handler(httpd_req_t *req) {
    esp_err_t ret = ESP_OK;
    camera_fb_t *fb = NULL;
    char part_buf[64];
    size_t part_len = 0;
    size_t jpg_len = 0;
    int64_t t_start = 0, t_end = 0, t_last = 0;

    httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    t_start = t_last = esp_timer_get_time();

    while (1) {
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "failed to get fb");
            break;
        }
        jpg_len = fb->len;
        part_len = sprintf(part_buf, _STREAM_PART, jpg_len);
        ret = httpd_resp_send_chunk(req, part_buf, part_len);
        if (ret == ESP_OK) {
            ret = httpd_resp_send_chunk(req, (const char *)fb->buf, jpg_len);
        }
        if (ret == ESP_OK) {
            ret = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, _BOUNDARY_LEN);
        }
        esp_camera_fb_return(fb);
        fb = NULL;

        if (ret != ESP_OK) {
            break;
        }

        t_end = esp_timer_get_time();
        uint32_t frame = (t_end - t_last) / 1000U;
        uint32_t avg = frame_filter(frame);
        if (avg) {
            ets_printf("%u.%uFPS %u.%ukB %ums\n",
                       1000U / avg, ((1000U % avg) * 10U) / avg,   // FPS
                       jpg_len / 1024U, (jpg_len % 1024U) / 103U,  // kB
                       frame);                                     // time consuming
        }
        t_last = t_end;
        t_start = esp_timer_get_time();
    }
    return ret;
}

static esp_err_t settings_handler(httpd_req_t *req) {
    static char buf[128];

    int len = print_wifi_settings(buf, 1024);
    if (len < 0) {
        return httpd_resp_send_500(req);
    }
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, buf, len);
}

static esp_err_t save_handler(httpd_req_t *req) {
    char buf[128];
    char value[8];

    wifi_mode_t wifi_mode = WIFI_MODE_NULL;
    char ssid[32] = {0};
    char pwd[32] = {0};
    uint8_t channel = 0;

    size_t len = httpd_req_get_url_query_len(req) + 1;
    if (len < 1 || len > sizeof(buf)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
    }
    if (httpd_req_get_url_query_str(req, buf, len) != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
    }

    if (httpd_query_key_value(buf, "wifi_mode", value, sizeof(value)) == ESP_OK) {
        wifi_mode = (wifi_mode_t)atoi(value);
    }
    if (wifi_mode != WIFI_MODE_AP && wifi_mode != WIFI_MODE_STA) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid Wi-Fi Mode");
    }
    if (httpd_query_key_value(buf, "ssid", ssid, sizeof(ssid)) != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid SSID");
    }
    if (httpd_query_key_value(buf, "pwd", ssid, sizeof(pwd)) != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid Password");
    }
    if (httpd_query_key_value(buf, "channel", value, sizeof(value)) == ESP_OK) {
        int c = atoi(value);
        if (c > 0 && c <= 13) channel = (uint8_t)c;
    }
    if (channel == 0) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid Channel");
    }

    if (save_wifi_settings(wifi_mode, ssid, pwd, channel) != ESP_OK) {
        return httpd_resp_send_500(req);
    }
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    return httpd_resp_send(req, (const char *)index_gz, index_gz_len);
}

void startServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;

    httpd_uri_t index_uri = {.uri = "/", .method = HTTP_GET, .handler = index_handler, .user_ctx = NULL, .is_websocket = true, .handle_ws_control_frames = false, .supported_subprotocol = NULL};
    httpd_uri_t settings_uri = {.uri = "/settings", .method = HTTP_GET, .handler = settings_handler, .user_ctx = NULL, .is_websocket = true, .handle_ws_control_frames = false, .supported_subprotocol = NULL};
    httpd_uri_t save_uri = {.uri = "/save", .method = HTTP_GET, .handler = save_handler, .user_ctx = NULL, .is_websocket = true, .handle_ws_control_frames = false, .supported_subprotocol = NULL};
    httpd_uri_t still_uri = {.uri = "/still", .method = HTTP_GET, .handler = still_handler, .user_ctx = NULL, .is_websocket = true, .handle_ws_control_frames = false, .supported_subprotocol = NULL};
    httpd_uri_t stream_uri = {.uri = "/stream", .method = HTTP_GET, .handler = stream_handler, .user_ctx = NULL, .is_websocket = true, .handle_ws_control_frames = false, .supported_subprotocol = NULL};

    if (httpd_start(&camera_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(camera_httpd, &index_uri);
        httpd_register_uri_handler(camera_httpd, &settings_uri);
        httpd_register_uri_handler(camera_httpd, &save_uri);
        httpd_register_uri_handler(camera_httpd, &still_uri);

        ets_printf("http://%s:%d\n", get_wifi_ip().toString(), config.server_port);
    }

    config.server_port += 1;
    config.ctrl_port += 1;
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);

        ets_printf("http://%s:%d\n", get_wifi_ip().toString(), config.server_port);
    }
    ets_printf("Server startup in %dms\n", (uint32_t)(esp_timer_get_time() / 1000));
}