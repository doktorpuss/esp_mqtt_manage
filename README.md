# Home_IoT

A lightweight Home IoT communication library for ESP32.

This library provides:

- WiFi configuration through a built-in Access Point
- Web configuration page
- MQTT communication
- JSON message routing
- Custom message handlers
- Automatic device configuration storage using Preferences

Designed to make ESP32 IoT devices easy to integrate into a Home IoT ecosystem.

---

# Features

- WiFi configuration portal
- Device name configuration
- MQTT configuration
- Automatic reconnect
- JSON-based MQTT communication
- Message handler registration
- Configuration stored in flash (Preferences)
- Easy integration into Arduino projects

---

# Installation

Copy the library into your Arduino libraries folder.

Include it:

```cpp
#include <Home_IoT.h>
```

---

# Quick Start

Initialize the library inside `setup()`.

```cpp
void setup()
{
    Serial.begin(115200);

    HomeIoT_begin();

    registerHandler(
        "led",
        handleLed
    );
}

void loop()
{
    HomeIoT_loop();
}
```

---

# MQTT Message Format

Every MQTT packet uses JSON.

The library automatically fills:

- `source`
- `target`

The user must provide:

- `msg_type`
- other custom fields

Example:

```json
{
    "source": "KitchenLight",
    "target": "Server",
    "msg_type": "status",
    "online": true
}
```

Example custom message:

```json
{
    "source": "Server",
    "target": "KitchenLight",
    "msg_type": "led",
    "state": true
}
```

---

# Publishing JSON

Create a JsonDocument.

```cpp
JsonDocument doc;

doc["msg_type"] = "status";
doc["online"] = true;

publishMQTT_JSON(doc);
```

The library automatically appends:

```json
{
    "source": "...",
    "target": "..."
}
```

before publishing.

---

# Registering Message Handlers

Register a handler for a message type.

```cpp
registerHandler(
    "led",
    handleLed
);
```

Handler function:

```cpp
void handleLed(JsonDocument& doc)
{
    bool state = doc["state"];

    digitalWrite(
        LED_BUILTIN,
        state
    );
}
```

Whenever a packet with

```json
{
    "msg_type":"led"
}
```

is received, the handler will be called automatically.

---

# Configuration Portal

When the device has no WiFi configuration or cannot connect to WiFi, it automatically starts an Access Point.

Default address:

```
192.168.4.1
```

The web page allows configuration of:

- Device Name
- WiFi SSID
- WiFi Password
- MQTT Server
- MQTT Port
- MQTT Username
- MQTT Password

All settings are stored using ESP32 Preferences.

---

# Configuration Storage

The library stores:

- Device Name
- WiFi credentials
- MQTT settings

These settings are preserved after reboot.

---

# Custom Web Interface

The configuration page is generated inside:

```cpp
makePage()
```

located in:

```
Home_IoT.cpp
```

If you want to customize the configuration webpage, edit the contents of the `makePage()` function.

---

# API

## Initialization

```cpp
HomeIoT_begin();
```

Initializes:

- Preferences
- WiFi
- Web Server
- MQTT
- Internal services

---

## Main Loop

```cpp
HomeIoT_loop();
```

Must be called continuously inside `loop()`.

---

## Register Handler

```cpp
registerHandler(
    const String& msgType,
    MessageHandler handler
);
```

Registers a callback for a specific `msg_type`.

---

## Publish MQTT

```cpp
publishMQTT_JSON(doc);
```

Publishes a JSON packet.

The library automatically inserts:

- source
- target

---

# Suggested Built-in Message Types

The library does **not** automatically implement any message type.

A common convention is to register handlers such as:

- `status` (can use `publishStatus()` built from library for simple status publish)
- `restart` (can use `ESP.restart()`)

You are free to define your own message types according to your application.

---

# Project Structure

```
Home_IoT
│
├── Home_IoT.h
├── Home_IoT.cpp
├── examples
│   └── BasicLED
└── README.md
```

---

# Requirements

- ESP32
- ArduinoJson
- PubSubClient

---

# License

MIT License.