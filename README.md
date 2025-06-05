## ESP32 Camera Demo
A simplified ESP32 camera demo using Arduino. It provides still photo viewing, video streaming, and network configuration functionality.

Based on [arduino-esp32 version 2.0.11](https://github.com/espressif/arduino-esp32/releases/tag/2.0.11).

### Code
The camera sensor pins are defined in the `ESP32CameraDemo.ino` file.
```c++
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
```
Modify these definitions to match your specific development board.

### Usage Instructions
1. The firmware starts in Wi-Fi SoftAP mode by default. Use your phone to scan for Wi-Fi networks. Find the network name starting with `Node-` and connect to it (password: `nodenode`).
2. Open a web browser on your phone and go to `192.168.4.1`.
3. View the video stream or modify network settings on the webpage.

### Snapshot
![](snapshot.png)
