#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "secrets.h"

#define LED_PIN 27
#define BUZZER_PIN 25

typedef struct __attribute__((packed)) {
  uint8_t  nodeId;
  uint32_t bootCount;
  float    temperature;
  float    humidity;
  uint8_t  alert;
} Packet;

Adafruit_SSD1306 display(128, 64, &Wire, -1);
WiFiClientSecure net;
PubSubClient mqtt(net);

volatile bool newData = false;
Packet rxPacket;

void onReceive(const uint8_t *mac, const uint8_t *data, int len) {
  if (len == sizeof(Packet)) {
    memcpy((void*)&rxPacket, data, sizeof(Packet));
    newData = true;
  }
}

void mqttConnect() {
  while (!mqtt.connected()) {
    Serial.print("MQTT connecting...");
    // clientId, user, pass, willTopic, willQoS, willRetain, willMessage
    if (mqtt.connect("esp32-gateway", MQTT_USER, MQTT_PASS,
                     "iot/gateway/status", 1, true, "offline")) {
      Serial.println("OK");
      mqtt.publish("iot/gateway/status", "online", true);   // Birth message
    } else {
      Serial.printf("failed rc=%d\n", mqtt.state());
      delay(2000);
    }
  }
}

void showOLED(const Packet &p) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);  display.printf("Node %d  Boot #%u", p.nodeId, p.bootCount);
  display.setCursor(0, 16); display.printf("Temp: %.1f C", p.temperature);
  display.setCursor(0, 28); display.printf("Hum : %.1f %%", p.humidity);
  display.setCursor(0, 44); display.print(p.alert ? "!! ALERT !!" : "Status: OK");
  display.setCursor(0, 56); display.print(mqtt.connected() ? "MQTT: connected" : "MQTT: down");
  display.display();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Wire.begin(21, 22);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay(); display.setCursor(0,0);
  display.print("Connecting WiFi..."); display.display();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(300);
  Serial.printf("WiFi OK, channel %d, MAC %s\n", WiFi.channel(), WiFi.macAddress().c_str());

  esp_now_init();
  esp_now_register_recv_cb(onReceive);

  net.setInsecure();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqttConnect();
}

void loop() {
  if (!mqtt.connected()) mqttConnect();
  mqtt.loop();

  if (newData) {
    Packet p;
    memcpy(&p, (const void*)&rxPacket, sizeof(p));
    newData = false;

    Serial.printf("RX node=%d boot=%u T=%.1f H=%.1f alert=%d\n",
                  p.nodeId, p.bootCount, p.temperature, p.humidity, p.alert);

    showOLED(p);

    if (p.alert) {
      digitalWrite(LED_PIN, HIGH);
      tone(BUZZER_PIN, 2000, 500);
    } else {
      digitalWrite(LED_PIN, LOW);
    }


    char json[160];
    snprintf(json, sizeof(json),
      "{\"node\":%d,\"boot\":%u,\"temp\":%.1f,\"hum\":%.1f,\"alert\":%d}",
      p.nodeId, p.bootCount, p.temperature, p.humidity, p.alert);
    mqtt.publish("iot/node1/data", json);
  }
}