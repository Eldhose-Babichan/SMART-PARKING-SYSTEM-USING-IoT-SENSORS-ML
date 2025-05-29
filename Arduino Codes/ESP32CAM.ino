#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>

// Camera pin configuration for AI Thinker ESP32-CAM module
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22


const char* ssid = "mr.eldhose";
const char* password = "Eldhose@1077";
const char* serverName = "http://192.168.174.232:12345/upload-image";  // Flask server for classification
const char* esp32WroomIP = "192.168.174.24";  // ESP32-WROOM IP

const int irSensorPin = 13; // GPIO pin connected to the IR sensor

void setup() {
  Serial.begin(115200);
  pinMode(irSensorPin, INPUT);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // Configure camera
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}

void loop() {
  if (digitalRead(irSensorPin) == LOW) {
    Serial.println("Motion detected! Capturing photo...");
    capturePhotoAndSend();
    delay(1000); // Prevent multiple captures for one motion event
  }
  delay(100);
}

void capturePhotoAndSend() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "image/jpeg");

    int httpResponseCode = http.POST(fb->buf, fb->len);
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Server Response: " + response);
      String classification = parseClassification(response);
      Serial.println("Vehicle Detected: " + classification);
      sendClassificationToESP32WROOM(classification);
    } else {
      Serial.println("Error sending classification: " + String(httpResponseCode));
    }
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }

  esp_camera_fb_return(fb);
}

String parseClassification(String response) {
  int startIndex = response.indexOf("\"prediction\":\"");
  if (startIndex == -1) return "Unknown";

  startIndex += 14;
  int endIndex = response.indexOf("\"", startIndex);
  if (endIndex == -1) return "Unknown";

  return response.substring(startIndex, endIndex);
}

void sendClassificationToESP32WROOM(String classification) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://" + String(esp32WroomIP) + "/?vehicle=" + classification;
    Serial.println("Sending request to: " + url);
    http.begin(url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      Serial.println("Sent to ESP32-WROOM successfully.");
    } else {
      Serial.println("Failed to connect to ESP32-WROOM");
    }
    http.end();
  }
}
