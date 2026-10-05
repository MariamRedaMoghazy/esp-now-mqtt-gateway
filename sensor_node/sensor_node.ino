#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT11
#define WIFI_CHANNEL 6
#define SLEEP_SEC 30
#define TEMP_LIMIT 30.0
#define HUM_LIMIT 70.0

uint8_t gatewayMAC[] = {0x44, 0x1D, 0x64, 0xF3, 0x3B, 0x04};
typedef struct __attribute__((packed)) {
  uint8_t  nodeId;
  uint32_t bootCount;
  float    temperature;
  float    humidity;
  uint8_t  alert;
} Packet;

RTC_DATA_ATTR uint32_t bootCount = 0;
DHT dht(DHTPIN, DHTTYPE);
volatile bool sendDone = false;

void onSent(const uint8_t *mac, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivered" : "Failed");
  sendDone = true;
}

void setup() {
  Serial.begin(115200);
  bootCount++;
  Serial.printf("Boot #%u\n", bootCount);

  dht.begin();
  delay(2000);
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (!isnan(t) && !isnan(h)) {
    Packet p;
    p.nodeId = 1;
    p.bootCount = bootCount;
    p.temperature = t;
    p.humidity = h;
    p.alert = (t > TEMP_LIMIT || h > HUM_LIMIT) ? 1 : 0;

    WiFi.mode(WIFI_STA);
    esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() == ESP_OK) {
      esp_now_register_send_cb(onSent);
      esp_now_peer_info_t peer = {};
      memcpy(peer.peer_addr, gatewayMAC, 6);
      peer.channel = WIFI_CHANNEL;
      peer.encrypt = false;
      esp_now_add_peer(&peer);

      esp_now_send(gatewayMAC, (uint8_t*)&p, sizeof(p));
      unsigned long t0 = millis();
      while (!sendDone && millis() - t0 < 500) delay(5);
      Serial.printf("T=%.1f H=%.1f alert=%d\n", t, h, p.alert);
    }
  } else {
    Serial.println("DHT read failed");
  }

  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_SEC * 1000000ULL);
  Serial.println("Going to sleep...");
  Serial.flush();
  esp_deep_sleep_start();
}

void loop() {}