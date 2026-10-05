# ESP-NOW Gateway with MQTT over TLS

A hybrid local + cloud IoT system built with two ESP32 boards. A battery-powered
sensor node sends temperature and humidity over ESP-NOW to a gateway, which
displays the data, makes local decisions, and publishes to HiveMQ Cloud via MQTT/TLS.

## Architecture

```
[Sensor Node] --ESP-NOW--> [Gateway] --WiFi + MQTT/TLS--> [HiveMQ Cloud]
 DHT11 + Deep Sleep        OLED + LED + Buzzer
```

## Features
- ESP-NOW binary packets with deep sleep (30 s wake-up) and boot count in RTC memory
- Local threshold alert: LED + buzzer on the gateway
- OLED live display
- MQTT over TLS (port 8883) with Last Will and Testament + birth message

## Hardware
| Component | Qty | Pin |
|---|---|---|
| ESP32 DevKit | 2 | – |
| DHT11 | 1 | GPIO4 (Node) |
| SSD1306 OLED (0x3C) | 1 | SDA 21, SCL 22 (Gateway) |
| LED | 1 | GPIO27 (Gateway) |
| Passive buzzer | 1 | GPIO25 (Gateway) |

## Libraries
DHT sensor library, Adafruit Unified Sensor, Adafruit SSD1306, Adafruit GFX, PubSubClient.

## Setup
1. Upload a MAC/channel sketch to the gateway and note its MAC address and WiFi channel.
2. Put the gateway MAC and channel in `sensor_node/sensor_node.ino`.
3. Copy `gateway/secrets.h.example` to `gateway/secrets.h` and fill in your credentials.
4. Upload `gateway/gateway.ino` to Board B and `sensor_node/sensor_node.ino` to Board A.

## MQTT Topics
| Topic | Payload |
|---|---|
| `iot/node1/data` | `{"node":1,"boot":3,"temp":22.6,"hum":46.0,"alert":0}` |
| `iot/gateway/status` | `online` / `offline` (LWT) |

## Documentation
Full report: [docs/ESP-NOW_Gateway_MQTT_Documentation.docx](docs/ESP-NOW_Gateway_MQTT_Documentation.docx)

## Future Improvements
Certificate validation with `setCACert()`, ESP-NOW encryption, multiple nodes, dashboard.