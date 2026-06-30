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

# Custom Configuration Fields

Besides the built-in configuration items (Wi-Fi, MQTT, Web Authentication, etc.), the library also allows applications to register additional configuration fields that are automatically:

* displayed on the configuration webpage;
* loaded from persistent storage;
* saved back to flash memory.

This makes it possible to extend the configuration page without modifying the internal library logic.

### Registering a Custom Field

Register custom fields before calling `HomeIoT_begin()`.

```cpp
registerCustomField({
    "target_mac",
    "Target MAC Address",
    "AA:BB:CC:DD:EE:FF"
});

registerCustomField({
    "broadcast_ip",
    "Broadcast IP",
    "192.168.1.255"
});
```

Each field contains:

| Member         | Description                                                   |
| -------------- | ------------------------------------------------------------- |
| `key`          | Unique identifier used as the storage key.                    |
| `label`        | Display name shown on the web configuration page.             |
| `defaultValue` | Default value used when no saved configuration exists.        |
| `value`        | Current runtime value (managed automatically by the library). |

---

### Reading a Value

```cpp
String mac = getCustomField("target_mac");
```

---

### Updating a Value

```cpp
setCustomField(
    "target_mac",
    "11:22:33:44:55:66"
);
```

The updated value will be stored automatically the next time the configuration is saved.

---

### Storage

All custom fields are stored together with the built-in configuration in the same persistent storage:

* ESP32 → Preferences (NVS)
* ESP8266 → EEPROM (planned)

Applications do not need to manually implement loading or saving logic.

---

### Typical Use Cases

Examples of information that can be stored using Custom Fields:

* Wake-on-LAN target MAC address
* Broadcast IP
* Static IP parameters
* Calibration values
* Sensor offsets
* Device location
* User-defined MQTT topics
* API keys
* Device-specific parameters

Any application-specific configuration that should survive a reboot can be implemented as a Custom Field.

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

> [!IMPORTANT]
> ## Accessing Configuration AP After Initial Setup
>
> For security reasons, the ESP Access Point (AP) is **visible only during the first configuration** (or after a factory reset).
>
> Once the device has been configured successfully, the configuration AP will become **hidden**.
>
> If you need to configure the device again (for example, after changing the WiFi network), you must manually connect to the hidden WiFi network using:
>
> - **SSID:** `<deviceName>`
> - **Password:** `12345678`
>
> Since the AP is hidden, it will not appear in the list of available WiFi networks. You must manually add a hidden network from your operating system's WiFi settings.
>
> The AP becomes visible again only after performing a **factory reset**.

# Factory Reset

If you forget the WiFi credentials, MQTT settings, or need to restore the device to its initial state, you can perform a Factory Reset.

A Factory Reset will erase all stored configuration, including:

- Device Name
- WiFi SSID
- WiFi Password
- MQTT Server
- MQTT Port
- MQTT Username
- MQTT Password

After the reset:

- All saved settings are removed.
- The device restarts automatically.
- The configuration Access Point becomes **visible** again.
- The default AP password is restored to **12345678**.

## How to Perform a Factory Reset

1. Power on the ESP32.
2. Connect `D15` on ESP to `GND` for 5s.
3. Wait until the status LED flash.
4. Release `D15` and `GND`.
5. The device will reboot and enter configuration mode.

You can then reconnect to the configuration AP and set up the device again.

# License

MIT License.