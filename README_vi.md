# Home_IoT

Home_IoT là thư viện giao tiếp dành cho ESP32 trong hệ thống IoT.

Thư viện cung cấp:

- Cấu hình WiFi thông qua Access Point
- Trang Web cấu hình tích hợp
- Giao tiếp MQTT
- Trao đổi dữ liệu bằng JSON
- Đăng ký handler theo loại message
- Lưu cấu hình bằng Preferences

Mục tiêu của thư viện là giúp việc phát triển các thiết bị IoT trở nên đơn giản và dễ mở rộng.

---

# Tính năng

- Cấu hình WiFi
- Cấu hình Device Name
- Cấu hình MQTT
- Tự động kết nối lại
- Giao tiếp MQTT bằng JSON
- Đăng ký Message Handler
- Lưu cấu hình trong Flash
- Dễ tích hợp vào dự án Arduino

---

# Cài đặt

Copy thư viện vào thư mục Libraries của Arduino.

Sau đó include:

```cpp
#include <Home_IoT.h>
```

---

# Khởi động nhanh

Khởi tạo thư viện trong `setup()`.

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

# Cấu trúc gói MQTT

Mọi gói MQTT đều sử dụng JSON.

Thư viện sẽ tự động thêm:

- `source`
- `target`

Người dùng chỉ cần thêm:

- `msg_type`
- các trường dữ liệu khác

Ví dụ:

```json
{
    "source": "KitchenLight",
    "target": "Server",
    "msg_type": "status",
    "online": true
}
```

Ví dụ điều khiển LED:

```json
{
    "source": "Server",
    "target": "KitchenLight",
    "msg_type": "led",
    "state": true
}
```

---

# Gửi dữ liệu MQTT

Tạo một JsonDocument.

```cpp
JsonDocument doc;

doc["msg_type"] = "status";
doc["online"] = true;

publishMQTT_JSON(doc);
```

Thư viện sẽ tự động thêm:

```json
{
    "source": "...",
    "target": "..."
}
```

trước khi gửi.

---

# Đăng ký Message Handler

Ví dụ:

```cpp
registerHandler(
    "led",
    handleLed
);
```

Hàm xử lý:

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

Khi nhận được:

```json
{
    "msg_type":"led"
}
```

handler sẽ tự động được gọi.

---

# Chế độ cấu hình

Nếu:

- chưa cấu hình WiFi
- hoặc không kết nối được WiFi

ESP32 sẽ tự tạo Access Point.

Địa chỉ mặc định:

```
192.168.4.1
```

Trang web cho phép cấu hình:

- Device Name
- WiFi SSID
- WiFi Password
- MQTT Server
- MQTT Port
- MQTT Username
- MQTT Password

Toàn bộ cấu hình được lưu bằng Preferences.

---

# Lưu cấu hình

Thư viện lưu:

- Device Name
- WiFi
- MQTT

Các thông tin này vẫn được giữ sau khi khởi động lại.

---

# Tùy chỉnh giao diện Web

Trang cấu hình được tạo bởi hàm:

```cpp
makePage()
```

trong file:

```
Home_IoT.cpp
```

Nếu muốn thay đổi giao diện hoặc bố cục trang cấu hình, hãy chỉnh sửa nội dung của hàm `makePage()`.

---

# API

## Khởi tạo

```cpp
HomeIoT_begin();
```

Khởi tạo:

- Preferences
- WiFi
- Web Server
- MQTT
- Các dịch vụ nội bộ

---

## Vòng lặp

```cpp
HomeIoT_loop();
```

Phải được gọi liên tục trong `loop()`.

---

## Đăng ký Handler

```cpp
registerHandler(
    const String& msgType,
    MessageHandler handler
);
```

Đăng ký callback cho một `msg_type`.

---

## Gửi MQTT

```cpp
publishMQTT_JSON(doc);
```

Gửi một gói JSON.

Thư viện sẽ tự động thêm:

- source
- target

---

# Các Message Type được khuyến nghị

Thư viện **không tự động cài đặt sẵn** bất kỳ loại message nào.

Thông thường, bạn nên tự đăng ký các handler như:

- `status` (có thể sử dụng hàm `publishStatus()` được cung cấp bởi thư viện)
- `restart` (có thể sửu dụng `ESP.restart()`)

Ngoài ra, bạn có thể tự định nghĩa bất kỳ `msg_type` nào phù hợp với ứng dụng của mình.

---

# Cấu trúc thư viện

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

# Yêu cầu

- ESP32
- ArduinoJson
- PubSubClient

---

# Giấy phép

MIT License.