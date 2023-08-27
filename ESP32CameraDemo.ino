// ESP32CameraDemo - https://nodematrix.cc
// Copyright (c) 2023 Node Matrix
// MIT License

#include "esp_camera.h"

// Node-Matrix Camera-1/M pin assignment
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  0
#define SIOD_GPIO_NUM  19
#define SIOC_GPIO_NUM  18
#define Y9_GPIO_NUM    38
#define Y8_GPIO_NUM    37
#define Y7_GPIO_NUM    36
#define Y6_GPIO_NUM    25
#define Y5_GPIO_NUM    34
#define Y4_GPIO_NUM    13
#define Y3_GPIO_NUM    12
#define Y2_GPIO_NUM    35
#define VSYNC_GPIO_NUM 5
#define HREF_GPIO_NUM  39
#define PCLK_GPIO_NUM  26

void startWiFi();
void startServer();

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();

    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_SVGA;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_LATEST;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 10;
    config.fb_count = 2;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
    }

    startWiFi();

    startServer();
}

void loop() {
    delay(10000);
}