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

#endif