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

# Trường cấu hình mở rộng (Custom Configuration Fields)

Ngoài các thông tin cấu hình được tích hợp sẵn (Wi-Fi, MQTT, Web Authentication...), thư viện còn cho phép ứng dụng đăng ký thêm các trường cấu hình riêng.

Các trường này sẽ tự động:

* hiển thị trên trang cấu hình Web;
* được đọc từ bộ nhớ sau khi khởi động;
* được lưu xuống bộ nhớ khi người dùng nhấn **Save**.

Nhờ đó, người dùng có thể mở rộng giao diện cấu hình mà không cần sửa đổi logic bên trong thư viện.

### Đăng ký một trường cấu hình

Các trường nên được đăng ký trước khi gọi `HomeIoT_begin()`.

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

Mỗi trường gồm các thành phần:

| Thành phần     | Ý nghĩa                                                      |
| -------------- | ------------------------------------------------------------ |
| `key`          | Khóa duy nhất dùng để lưu trong bộ nhớ.                      |
| `label`        | Tên hiển thị trên trang cấu hình Web.                        |
| `defaultValue` | Giá trị mặc định khi chưa có dữ liệu được lưu.               |
| `value`        | Giá trị hiện tại của trường (được thư viện quản lý tự động). |

---

### Đọc giá trị

```cpp
String mac = getCustomField("target_mac");
```

---

### Ghi giá trị

```cpp
setCustomField(
    "target_mac",
    "11:22:33:44:55:66"
);
```

Giá trị mới sẽ được lưu xuống bộ nhớ khi cấu hình được lưu.

---

### Lưu trữ

Các trường cấu hình mở rộng được lưu chung với cấu hình mặc định của thư viện:

* ESP32 → Preferences (NVS)
* ESP8266 → EEPROM (dự kiến hỗ trợ)

Người dùng không cần tự viết thêm mã để load hoặc save các trường này.

---

### Một số ứng dụng

Custom Configuration Fields phù hợp để lưu các thông tin đặc thù của từng dự án, ví dụ:

* Địa chỉ MAC của thiết bị Wake-on-LAN
* Broadcast IP
* Thông số IP tĩnh
* Giá trị hiệu chuẩn (Calibration)
* Offset cảm biến
* Vị trí lắp đặt thiết bị
* MQTT Topic riêng
* API Key
* Các tham số cấu hình đặc thù khác

Mọi thông tin cấu hình riêng của ứng dụng cần được lưu lâu dài đều có thể triển khai thông qua **Custom Configuration Fields**.

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

> [**! LƯU Ý QUAN TRỌNG**]
> ## Truy cập AP cấu hình sau lần thiết lập đầu tiên
>
> Để tăng tính bảo mật, Access Point (AP) của ESP **chỉ hiển thị trong lần cấu hình đầu tiên** hoặc sau khi **Factory Reset**.
>
> Sau khi cấu hình thành công, AP sẽ được chuyển sang chế độ **ẩn (Hidden)**.
>
> Nếu cần cấu hình lại thiết bị (ví dụ khi thay đổi mạng WiFi), bạn cần **kết nối thủ công tới mạng WiFi ẩn** với thông tin:
>
> - **SSID:** `<deviceName>`
> - **Mật khẩu:** `12345678`
>
> Do AP ở chế độ ẩn nên sẽ **không xuất hiện trong danh sách các mạng WiFi khả dụng**. Bạn cần sử dụng chức năng **Add Hidden Network** (hoặc tương đương) trên điện thoại hoặc máy tính để kết nối.
>
> AP sẽ chỉ hiển thị trở lại sau khi thực hiện **Factory Reset**.

# Factory Reset

Nếu quên thông tin WiFi, cấu hình MQTT hoặc muốn đưa thiết bị về trạng thái ban đầu, bạn có thể thực hiện **Factory Reset**.

Factory Reset sẽ xóa toàn bộ cấu hình đã lưu, bao gồm:

- Device Name
- WiFi SSID
- WiFi Password
- MQTT Server
- MQTT Port
- MQTT Username
- MQTT Password

Sau khi Factory Reset:

- Toàn bộ cấu hình sẽ bị xóa.
- Thiết bị sẽ tự khởi động lại.
- Access Point cấu hình sẽ **hiển thị trở lại**.
- Mật khẩu mặc định của AP là **12345678**.

## Cách thực hiện Factory Reset

1. Cấp nguồn cho ESP32.
2. Nối chân `D15` của ESP với `GND` trong 5 giây.
3. Chờ đến khi đèn trạng thái chớp tắt.
4. Ngắt kết nối `D15` với `GND`.
5. Thiết bị sẽ khởi động lại và chuyển về chế độ cấu hình.

Sau đó bạn có thể kết nối lại với Access Point của thiết bị để thực hiện cấu hình từ đầu.

# Giấy phép

MIT License.