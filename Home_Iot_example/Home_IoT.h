#ifndef HOMEIOT_H
#define HOMEIOT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>

/**
 * @brief Kiểu hàm callback xử lý MQTT message.
 */
using MessageHandler = std::function<void(JsonDocument&)>;

/**
 * @brief Khởi tạo toàn bộ HomeIoT.
 */
void HomeIoT_begin();

/**
 * @brief Hàm loop của HomeIoT.
 */
void HomeIoT_loop();

/**
 * @brief Đăng ký handler cho một msg_type.
 */
void registerHandler(const String& msgType, MessageHandler handler);

/**
 * @brief Publish một JSON lên MQTT.

    gói tin phải đủ 3 trường : source, target, msg_type.
 */
bool publishMQTT_JSON(const String& msgType, JsonDocument& doc);

/**
 * @brief Publish trạng thái thiết bị.
 */
void publishStatus();

/**
 * @brief Kiểu trường cấu hình.
 *
 * @param TEXT         Text input.
 * @param PASSWORD     Password input.
 * @param NUMBER       Number input.
 * @param CHECKBOX     Checkbox.
*/
enum FieldType
{
    TEXT,
    PASSWORD,
    NUMBER,
    CHECKBOX
};

/**
 * @brief Đăng ký một trường cấu hình mở rộng.
 *
 * Trường sẽ được:
 *  - Hiển thị trên Web Config.
 *  - Tự động Load từ Preferences.
 *  - Tự động Save xuống Preferences.
 *
 * Nếu key đã tồn tại thì sẽ không thêm mới.
 *
 * @param key           Tên key lưu trong Preferences.
 * @param label         Tiêu đề hiển thị trên Web.
 * @param defaultValue  Giá trị mặc định.
 * @param type          Kiểu trường.
 */
void addCustomField(
    const String& key,
    const String& label,
    const String& defaultValue = "",
    FieldType type = TEXT
);

/**
 * @brief Lấy giá trị của một trường cấu hình mở rộng.
 *
 * @param key Key đã đăng ký.
 *
 * @return Giá trị hiện tại.
 *         Trả về chuỗi rỗng nếu không tìm thấy.
 */
String getCustomField(
    const String& key
);

/**
 * @brief Cập nhật giá trị của một trường cấu hình mở rộng.
 *
 * Chỉ cập nhật trong RAM.
 *
 * Muốn lưu xuống Flash cần gọi saveConfig()
 * hoặc thực hiện Save từ Web Config.
 *
 * @param key Key đã đăng ký.
 * @param value Giá trị mới.
 */
void setCustomField(
    const String& key,
    const String& value
);



#endif