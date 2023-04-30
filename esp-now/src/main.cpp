#include <Arduino.h>
#include <WiFi.h>
#include <esp_log.h>
#include <esp_now.h>
#include <esp_task_wdt.h>

// Receiver MAC Address
static uint8_t robotAddress[] = {0xEC, 0x62, 0x60, 0x1E, 0xEC, 0xE4};
static uint8_t basestationAddress[] = {0xCC, 0xDB, 0xA7, 0x16, 0x8D, 0x68};
static uint8_t buffer[250];
static const char *TAG = "BS";
static const char *TAGSTS = "BS-STATUS";

// Packet throughput management
uint32_t packets_sent = 0;
uint32_t packets_failed = 0;
uint32_t packets_received = 0;
uint32_t packets_sent_2way = 0;
uint32_t packets_failed_2way = 0;
bool packet_sent = false;

esp_now_peer_info_t peerInfo;

///////////////// CALLBACK METHODS /////////////////
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  packet_sent = true;
  if (status == ESP_NOW_SEND_SUCCESS) {
    packets_sent_2way++;
  } else {
    packets_failed_2way++;
  }
}

void OnDataReceived(const uint8_t *mac_addr, const uint8_t *data,
                    int data_len) {
  packets_received++;
}
///////////////// CALLBACK METHODS /////////////////

///////////////// SETUPS /////////////////
void setup_for_basestation() {
  // Register peer
  memcpy(peerInfo.peer_addr, robotAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void setup_for_robot() {}
///////////////// SETUPS /////////////////

///////////////// TASKS /////////////////
void basestation_send_messages(void *param) {
  esp_task_wdt_init(10, false);

  while (true) {
    // Send message via ESP-NOW
    esp_err_t result =
        esp_now_send(robotAddress, (uint8_t *)&buffer, sizeof(buffer));

    if (result == ESP_OK) {
      while (!packet_sent) {
        vTaskDelay(1);
      }
      packet_sent = false;
      packets_sent++;
    } else {
      ESP_LOGE(TAG, "Error: %s", esp_err_to_name(result));
      packets_failed++;
    }
  }
}
///////////////// TASKS /////////////////

///////////////// LOOPS /////////////////
void loop_basestation() {
  xTaskCreate(basestation_send_messages, "send messages", 8192, NULL, 1, NULL);

  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Sent %u packets, failed to send %u packets - %0.3fMbps",
             packets_sent, packets_failed,
             (packets_sent * 8.0f * sizeof(buffer)) / 1000000.0f);
    ESP_LOGI(TAGSTS, "Sent %u packets, failed to send %u packets - %0.3fMbps",
             packets_sent_2way, packets_failed_2way,
             (packets_sent_2way * 8.0f * sizeof(buffer)) / 1000000.0f);
    packets_sent = 0;
    packets_sent_2way = 0;
    packets_failed = 0;
    packets_failed_2way = 0;
  }
}

void loop_robot() {
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
  WiFi.mode(WIFI_MODE_STA);
  Serial.println(WiFi.macAddress());

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callback method when send packet through esp-now
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataReceived);

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