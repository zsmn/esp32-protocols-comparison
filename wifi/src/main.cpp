#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <esp_log.h>
#include <esp_task_wdt.h>

// Receiver MAC Address
static uint8_t robotAddress[] = {0xEC, 0x62, 0x60, 0x1E, 0xEC, 0xE4};
static uint8_t basestationAddress[] = {0xCC, 0xDB, 0xA7, 0x16, 0x8D, 0x68};
String buffer;
static const char *TAG = "BS";

// AP credentials
const char *ssid = "ARMORIAL-BASESTATION";
const char *password = "123456789";
const char *serverName = "http://192.168.4.1/control";

// Webserver
AsyncWebServer server(80);

// Packet throughput management
uint32_t packets_sent = 0;
uint32_t packets_failed = 0;
uint32_t packets_received = 0;
bool packet_sent = false;

///////////////// CALLBACK METHODS /////////////////

///////////////// CALLBACK METHODS /////////////////

///////////////// SETUPS /////////////////
void setup_for_basestation() {
  WiFi.softAP(ssid, password);
  Serial.println(WiFi.macAddress());

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/control", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", buffer.c_str());
  });

  // Start server
  server.begin();
}

void setup_for_robot() {
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}
///////////////// SETUPS /////////////////

///////////////// TASKS /////////////////
void robot_send_requests(void *param) {
  esp_task_wdt_init(10, false);

  while (true) {
    WiFiClient client;
    HTTPClient http;

    // Your Domain name with URL path or IP address with path
    http.begin(client, serverName);

    // Send HTTP POST request
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      packets_received++;
    }

    // Free resources
    http.end();
  }
}
///////////////// TASKS /////////////////

///////////////// LOOPS /////////////////
void loop_basestation() {}

void loop_robot() {
  xTaskCreate(robot_send_requests, "send requests", 8192, NULL, 1, NULL);

  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Received %u - %0.3fMbps", packets_received,
             (packets_received * 8.0f * sizeof(buffer)) / 1000000.0f);
    packets_received = 0;
  }
}
///////////////// LOOPS /////////////////

void setup() {
  Serial.begin(115200);

  // Setup buffer
  for (int i = 0; i < 250; i++) {
    buffer.concat("a");
  }

  // Setup
  if (strcmp(TAG, "ROBOT") == 0) {
    setup_for_robot();
  } else {
    setup_for_basestation();
  }
}

void loop() {
  if (strcmp(TAG, "ROBOT") == 0) {
    loop_robot();
  } else {
    loop_basestation();
  }
}