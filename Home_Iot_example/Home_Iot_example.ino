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

    addCustomField("custom1", "Custom Field 1");
    addCustomField("custom2", "Custom Field 2","Default value for custom2",TEXT);

    HomeIoT_begin();

    registerHandler("status",statusHandler);
    registerHandler("restart",restartHandler);
    registerHandler("led", ledHandler);
    registerHandler("led_state", ledStateHandler);
}

static void every_5sec(){
    static long lastTime = 0;
    if(millis() - lastTime > 5000){
        lastTime = millis();
        Serial.println("Custom 1 value: " + getCustomField("custom1"));
        Serial.println("Custom 2 value: " + getCustomField("custom2"));
    }
}

void loop()
{
    every_5sec();
    HomeIoT_loop();
}