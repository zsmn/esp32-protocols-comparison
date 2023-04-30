#include <Arduino.h>
#include <BluetoothSerial.h>
#include <esp_log.h>
#include <esp_now.h>
#include <esp_task_wdt.h>

// Receiver MAC Address
uint8_t buffer[250];
static const char *TAG = "ROBOT";
static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;

// Packet throughput management
uint32_t packets_sent = 0;
uint32_t packets_failed = 0;
uint32_t packets_received = 0;

BluetoothSerial SerialBT;

///////////////// CALLBACK METHODS /////////////////

///////////////// CALLBACK METHODS /////////////////

///////////////// SETUPS /////////////////
void setup_for_basestation() {
  if (!SerialBT.begin("ARMORIAL-BASESTATION-BT", false)) {
    Serial.println("An error occurred initializing Bluetooth");
    ESP.restart();
  } else {
    Serial.println("Bluetooth initialized");
  }
}

void setup_for_robot() {
  if (!SerialBT.begin("ARMORIAL-ROBOT-BT", true)) {
    Serial.println("An error occurred initializing Bluetooth");
    ESP.restart();
  }

  if (!SerialBT.connect("ARMORIAL-BASESTATION-BT")) {
    Serial.print("An error occurred pairing Bluetooth");
    ESP.restart();
  } else {
    Serial.println("Bluetooth paired!");
  }
}
///////////////// SETUPS /////////////////

///////////////// TASKS /////////////////
void basestation_send_messages(void *param) {
  esp_task_wdt_init(10, false);

  while (true) {
    String str = (char *)buffer;
    if (SerialBT.print(str) == str.length()) {
      packets_sent++;
    } else {
      packets_failed++;
    }
  }
}

void robot_read_messages(void *param) {
  esp_task_wdt_init(10, false);

  while (true) {
    if (SerialBT.available()) {
      String readedBytes = SerialBT.readStringUntil('\n');
      packets_received++;
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
    packets_sent = 0;
    packets_failed = 0;
  }
}

void loop_robot() {
  xTaskCreate(robot_read_messages, "read messages", 8192, NULL, 1, NULL);

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

  for (int i = 0; i < 249; i++) {
    buffer[i] = 'a';
  }
  buffer[249] = '\n';

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