#include <Home_IoT.h>

#define LED_PIN 2

/**
 * @brief Handler điều khiển LED.
 *
 * Payload:
 * {
 *   "msg_type":"led",
 *   "state":true
 * }
 */
void ledHandler(JsonDocument& doc)
{
    bool state = doc["state"] | false;

    digitalWrite(LED_PIN, state);
}

/**
 * @brief Handler trả về trạng thái LED.
 *
 * Payload yêu cầu:
 * {
 *   "msg_type":"led_state"
 * }
 *
 * ESP sẽ publish:
 * {
 *   "source":"esp32",
 *   "msg_type":"led_state",
 *   "state":true/false
 * }
 */
void ledStateHandler(JsonDocument& doc)
{
    JsonDocument response;

    response["state"] = digitalRead(LED_PIN);

    publishMQTT_JSON("led_state",response);
}

/**
 * @brief Xử lý yêu cầu gửi trạng thái thiết bị.
 */
void statusHandler(JsonDocument& doc)
{
    publishStatus();
}

/**
 * @brief Khởi động lại ESP.
 */
void restartHandler(JsonDocument& doc)
{
    ESP.restart();
}

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    Serial.begin(115200);

    HomeIoT_begin();

    registerHandler("status",statusHandler);
    registerHandler("restart",restartHandler);
    registerHandler("led", ledHandler);
    registerHandler("led_state", ledStateHandler);
}

void loop()
{
    HomeIoT_loop();
}