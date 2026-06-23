/*
======================================================
ESP32 Home IoT Configuration Portal

Features:
    - WiFi Configuration
    - MQTT Configuration
    - Web Authentication
    - AP Fallback Mode
    - Factory Reset Button
    - Persistent Storage (Preferences)

Factory Reset:
    GPIO0 giữ LOW >= 5 giây

Default AP:
    SSID     : ESP_SETUP_xxxx
    Password : 12345678

Web Login:
    Username : home-iot
    Password : cấu hình lần đầu

Author:
    Nguyễn Ngọc Quang

======================================================
*/

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <Home_IoT.h>
#include <vector>

#define FACTORY_RESET_PIN 15
#define STATUS_LED_PIN 2
#define CONFIG_NAMESPACE "config"
#define DEVICE_TYPE "SMART_SWITCH" // Loại thiết bị mà edge đảm nhận vai trò

#define MSG_STATUS     "status"
#define MSG_RESTART    "restart"

Preferences prefs;
WebServer server(80);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

/*
    Toàn bộ cấu hình được lưu trong NVS Preferences.

    Sau này có thể mở rộng thêm:
    - OTA Server
    - Device Name
    - Sensor Calibration
    - SSL MQTT
    - Home Assistant Discovery
*/
struct Config
{
    String deviceName;

    String wifiSSID;
    String wifiPassword;

    String mqttServer;
    uint16_t mqttPort;

    String mqttTopic;

    /*
        MQTT Username/Password là tùy chọn.

        Nếu username rỗng:
            mqtt.connect(clientId)

        Nếu username có giá trị:
            mqtt.connect(clientId,
                        username,
                        password)
    */
    String mqttUsername;
    String mqttPassword;

    String webPassword;
    String webUsername;

    bool initialized;
};

Config config;

/*
    Trường cấu hình mở rộng do người dùng định nghĩa.

    Các trường này sẽ:

        - Tự động hiển thị trên Web Config.
        - Tự động Load từ Preferences.
        - Tự động Save xuống Preferences.

    Ví dụ:

        Target MAC
        API Key
        Broadcast Port
        Sensor Offset
        Device Location

    Sau này có thể mở rộng thêm:

        - Checkbox
        - Select
        - Number
        - ReadOnly
        - Placeholder
*/
struct CustomConfigField
{
    String key;
    String label;
    String value;
    String defaultValue;
    FieldType type;
};

/*
    Danh sách toàn bộ trường cấu hình mở rộng.
*/
std::vector<CustomConfigField> customFields;

unsigned long resetPressTime = 0;
bool factoryResetTriggered = false;


/*
    Tạo tên Device ID mặc định.

    Format:
        ESP_SETUP_xxxx

    xxxx:
        4 ký tự cuối của MAC Address.

    Mục đích:
        Tránh nhiều thiết bị trong cùng hệ thống

    Return:
        Device ID hoàn chỉnh.
*/
String getDeviceId()
{
    uint64_t chipid = ESP.getEfuseMac();

    char id[32];

    sprintf(
        id,
        "%s_%04X",
        DEVICE_TYPE,
        (uint16_t)(chipid & 0xFFFF)
    );

    return String(id);
}

/*
    Tạo tên Access Point dựa trên deviceName
*/
String getApName()
{
    return config.deviceName;
}

/*
    Đọc toàn bộ cấu hình từ NVS Preferences.

    Hàm được gọi:
        setup()

    Nếu key chưa tồn tại:
        sử dụng giá trị mặc định.

    Sau khi thực hiện:
        dữ liệu được nạp vào biến toàn cục config.
*/
void loadConfig()
{
    prefs.begin(CONFIG_NAMESPACE, false);

    config.initialized =
        prefs.getBool("init", false);

    config.deviceName = 
        prefs.getString("deviceName", getDeviceId());

    config.wifiSSID =
        prefs.getString("ssid", "");

    config.wifiPassword =
        prefs.getString("wifi_pw", "");

    config.mqttServer =
        prefs.getString("mqtt_srv", "");

    config.mqttPort =
        prefs.getUShort("mqtt_port", 1883);

    config.mqttTopic =
        prefs.getString("mqtt_topic", "");

    config.mqttUsername =
        prefs.getString("mqtt_user", "");

    config.mqttPassword =
        prefs.getString("mqtt_pw", "");

    config.webPassword =
        prefs.getString("web_pw", "");

    config.webUsername =
        prefs.getString("web_user", "home-iot");

    /*
    Load toàn bộ cấu hình mở rộng.
    */
    for(auto &field : customFields)
    {
        field.value =
            prefs.getString(
                field.key.c_str(),
                field.defaultValue
            );
    }
}

/*
    Lưu toàn bộ cấu hình hiện tại
    từ biến config xuống NVS Preferences.

    Hàm được gọi:
        handleSave()

    Lưu ý:
        Dữ liệu vẫn tồn tại sau:
            - Mất điện
            - Reset ESP
            - ESP.restart()
*/
void saveConfig()
{
    prefs.putBool(
        "init",
        config.initialized
    );

    prefs.putString(
        "deviceName",
        config.deviceName
    );

    prefs.putString(
        "ssid",
        config.wifiSSID
    );

    prefs.putString(
        "wifi_pw",
        config.wifiPassword
    );

    prefs.putString(
        "mqtt_srv",
        config.mqttServer
    );

    prefs.putUShort(
        "mqtt_port",
        config.mqttPort
    );

    prefs.putString(
        "mqtt_topic",
        config.mqttTopic
    );

    prefs.putString(
        "mqtt_user",
        config.mqttUsername
    );

    prefs.putString(
        "mqtt_pw",
        config.mqttPassword
    );

    prefs.putString(
        "web_pw",
        config.webPassword
    );

    prefs.putString(
        "web_user",
        config.webUsername
    );

    /*
    Lưu toàn bộ cấu hình mở rộng.
    */
    for(auto &field : customFields)
    {
        prefs.putString(
            field.key.c_str(),
            field.value
        );
    }
}

/*
    Đăng ký một trường cấu hình mở rộng.

    Hàm này chỉ nên được gọi trong setup()
    trước HomeIoT_begin().

    Nếu key đã tồn tại,
    hàm sẽ bỏ qua để tránh trùng lặp.

    Sau khi đăng ký,
    trường sẽ được:

        - Load tự động
        - Save tự động
        - Sinh HTML tự động
*/
void addCustomField(
    const String& key,
    const String& label,
    const String& defaultValue,
    FieldType type
)
{
    for(auto &field : customFields)
    {
        if(field.key == key)
        {
            return;
        }
    }

    CustomConfigField field;

    field.key = key;
    field.label = label;
    field.defaultValue = defaultValue;
    field.value = defaultValue;
    field.type = TEXT;

    customFields.push_back(field);
}

/*
    Đọc giá trị của một trường cấu hình mở rộng.

    Nếu không tìm thấy key,
    trả về chuỗi rỗng.
*/
String getCustomField(
    const String& key
)
{
    for(auto &field : customFields)
    {
        if(field.key == key)
        {
            return field.value;
        }
    }

    return "";
}

/*
    Cập nhật giá trị của một trường cấu hình mở rộng.

    Hàm chỉ cập nhật dữ liệu trong RAM.

    Để lưu xuống Flash cần:

        saveConfig()

    hoặc

        Save trên Web Config.
*/
void setCustomField(
    const String& key,
    const String& value
)
{
    for(auto &field : customFields)
    {
        if(field.key == key)
        {
            field.value = value;
            return;
        }
    }
}

/*
    Basic Authentication.

    Username:
        WEB_USER

    Password:
        config.webPassword

    Nếu sau này muốn bảo mật cao hơn
    có thể thay bằng:
        - Cookie Session
        - JWT
        - Access Token
*/
bool checkAuth()
{
    if(!config.initialized)
    {
        return true;
    }

    return server.authenticate(
        config.webUsername.c_str(),
        config.webPassword.c_str()
    );
}

/*
    AP Mode được bật khi:

    1. ESP chưa được cấu hình
    2. Kết nối WiFi thất bại

    Truy cập:
        192.168.4.1
*/
void startAP()
{
    String apName = getApName();

    // kiểm tra nếu đã cấu hình lần đầu hoặc đã kết nối wifi thì phải ẩn AP đi
    bool hidden = config.initialized && (WiFi.status() == WL_CONNECTED);

    WiFi.softAPdisconnect(true);
    delay(100);

    WiFi.mode(WIFI_AP_STA);
    delay(100);

    WiFi.softAP(
        apName.c_str(),
        "12345678",
        1,
        hidden
    );

    Serial.println();
    Serial.println("AP Started");

    Serial.print("SSID: ");
    Serial.println(apName);

    Serial.println("Password: 12345678");
    Serial.println("IP: 192.168.4.1");
    
    Serial.print("hidden: ");
    Serial.println(hidden);
}

/*
    Kết nối WiFi đã lưu.

    Timeout:
        15 giây

    Nếu thất bại:
        chuyển sang AP Mode

    Có thể bổ sung:
        WiFi.setAutoReconnect(true);
*/
bool connectWiFi()
{
    if(config.wifiSSID.isEmpty())
        return false;

    WiFi.mode(WIFI_AP_STA);

    WiFi.setHostname(
        config.deviceName.c_str()
    );

    WiFi.begin(
        config.wifiSSID.c_str(),
        config.wifiPassword.c_str()
    );

    Serial.print("Connecting WiFi");

    unsigned long start = millis();

    while(
        WiFi.status() != WL_CONNECTED &&
        millis() - start < 15000
    )
    {
        Serial.print(".");
        delay(500);
    }

    Serial.println();

    if(WiFi.status() == WL_CONNECTED)
    {
        Serial.println("WiFi Connected");
        Serial.println(
            WiFi.localIP()
        );

        return true;
    }

    Serial.println("WiFi Failed");

    return false;
}

/*
    Sinh nội dung HTML cho trang cấu hình.

    Nếu thiết bị chưa được khởi tạo:
        hiển thị thêm trường
        tạo mật khẩu quản trị.

    Nếu đã khởi tạo:
        không cho phép thay đổi
        mật khẩu quản trị.

    Return:
        Chuỗi HTML hoàn chỉnh.
*/
String makePage()
{
    String html;

    html += "<html>";
    html += "<head>";
    html += "<meta charset='utf-8'>";
    html += "<title>ESP Config</title>";
    html += "</head>";
    html += "<body>";

    html += "<h2>ESP Configuration</h2>";

    html += "<form method='POST' action='/save'>";

    
    if(!config.initialized)
    {
        html += "Username<br>";
        html += "<input name='web_username' value='home-iot'><br><br>";
        
        html += "Admin Password<br>";
        html +="<input type='password' name='web_pw'><br><br>";
    }

    html += "WiFi SSID<br>";
    html += "<input name='ssid' value='" + config.wifiSSID + "'><br><br>";

    html += "WiFi Password<br>";
    html += "<input type='password' name='wifi_pw' value='" + config.wifiPassword + "'><br><br>";

    html += "MQTT Server<br>";
    html += "<input name='mqtt_srv' value='" + config.mqttServer + "'><br><br>";

    html += "MQTT Port<br>";
    html += "<input name='mqtt_port' value='" + String(config.mqttPort) + "'><br><br>";

    html += "MQTT Topic<br>";
    html += "<input name='mqtt_topic' value='" + config.mqttTopic + "'><br><br>";

    html += "MQTT Username<br>";
    html += "<input name='mqtt_user' value='" + config.mqttUsername + "'><br><br>";

    html += "MQTT Password<br>";
    html += "<input type='password' name='mqtt_pw' value='" + config.mqttPassword + "'><br><br>";

    html += "Device name<br>";
    html += "<input name='deviceName' value='" + config.deviceName + "'><br><br>";

    /*
    Sinh giao diện cho toàn bộ
    cấu hình mở rộng do người dùng đăng ký.
    */
    if(!customFields.empty())
    {
        html += "<hr>";
        html += "<h3>Custom Configuration</h3>";

        for(auto &field : customFields)
        {
            html += field.label;
            html += "<br>";

            html += "<input ";

            html += "type='";
            
            switch(field.type)
            {
                case TEXT:
                    html += "text";
                    break;
                case PASSWORD:
                    html += "password";
                    break;
                case NUMBER:
                    html += "number";
                    break;
                // case CHECKBOX:               Not support yet
                //     html += "checkbox";
                //     break;
                default:
                    html += "text";
                    break;
            }

            html += "' name='";
            html += field.key;
            html += "' value='";
            html += field.value;
            html += "'>";

            html += "<br><br>";
        }
    }

    html += "<input type='submit' value='Save'>";

    html += "</form>";

    html += "</body>";
    html += "</html>";

    return html;
}

/*
    Xử lý request:

        GET /

    Nếu thiết bị đã được cấu hình:
        yêu cầu Basic Authentication.

    Nếu xác thực thành công:
        trả về trang cấu hình.

    Nếu thất bại:
        trình duyệt hiển thị hộp thoại login.
*/
void handleRoot()
{
    if(config.initialized)
    {
        if(!checkAuth())
        {
            return server.requestAuthentication();
        }
    }

    server.send(
        200,
        "text/html",
        makePage()
    );
}

/*
    Xử lý request:

        POST /save

    Chức năng:
        - Đọc dữ liệu từ form
        - Cập nhật biến config
        - Lưu xuống Preferences
        - Khởi động lại ESP

    Nếu đây là lần cấu hình đầu tiên:
        tạo mật khẩu quản trị.

    Sau khi lưu:
        ESP restart để áp dụng cấu hình mới.
*/
void handleSave()
{
    if(config.initialized)
    {
        if(!checkAuth())
        {
            return server.requestAuthentication();
        }
    }

    config.deviceName =
        server.arg("deviceName");

    config.wifiSSID =
        server.arg("ssid");

    config.wifiPassword =
        server.arg("wifi_pw");

    config.mqttServer =
        server.arg("mqtt_srv");

    int port =
    server.arg("mqtt_port").toInt();

    if(port <= 0)
    {
        port = 1883;
    }

    config.mqttPort = port;

    config.mqttTopic =
        server.arg("mqtt_topic");

    config.mqttUsername =
        server.arg("mqtt_user");

    config.mqttPassword =
        server.arg("mqtt_pw");

    if(!config.initialized)
    {
        String pw =
            server.arg("web_pw");
        
        String username =
            server.arg("web_username");

        if(pw.length() < 4)
        {
            server.send(
                400,
                "text/plain",
                "Admin password must be at least 4 characters"
            );

            return;
        }

        config.webPassword = pw;
        config.webUsername = username;

        config.initialized = true;
    }

    /*
    Cập nhật toàn bộ cấu hình mở rộng
    từ dữ liệu POST.
    */
    for(auto &field : customFields)
    {
        field.value =
            server.arg(
                field.key
            );
    }

    saveConfig();

    server.send(
        200,
        "text/html",
        "<h2>Saved</h2>"
        "<p>ESP will reboot in 3 seconds...</p>"
    );

    delay(3000);

    ESP.restart();
}

/*
    Đăng ký toàn bộ route HTTP
    và khởi động Web Server.

    Các route hiện tại:

        GET  /
        POST /save

    Sau này có thể bổ sung:

        GET  /status
        GET  /info
        GET  /mqtt
        POST /reboot
*/
void setupWebServer()
{
    server.on(
        "/",
        HTTP_GET,
        handleRoot
    );

    server.on(
        "/save",
        HTTP_POST,
        handleSave
    );

    server.begin();
}

/*
    Factory Reset

    GPIO:
        FACTORY_RESET_PIN

    Điều kiện:
        Giữ LOW liên tục >= 5 giây

    Hành động:
        prefs.clear()
        ESP.restart()

    Toàn bộ cấu hình sẽ bị xóa:
        - WiFi
        - MQTT
        - Web Password
*/
void checkFactoryReset()
{
    if(digitalRead(FACTORY_RESET_PIN) == LOW)
    {
        if(resetPressTime == 0)
        {
            resetPressTime = millis();
        }

        if(
            !factoryResetTriggered &&
            millis() - resetPressTime >= 5000
        )
        {
            factoryResetTriggered = true;

            Serial.println(
                "Factory Reset"
            );

            pinMode(STATUS_LED_PIN, OUTPUT);
            digitalWrite(STATUS_LED_PIN,false);
            delay(200);

            digitalWrite(STATUS_LED_PIN,true);
            prefs.clear();
            delay(100);
            digitalWrite(STATUS_LED_PIN,false);

            delay(3000);

            ESP.restart();
        }
    }
    else
    {
        resetPressTime = 0;
        factoryResetTriggered = false;
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length);

/*
    Kết nối MQTT Broker.

    Nếu MQTT Username rỗng:
        kết nối không dùng Authentication.

    Nếu có Username:
        kết nối bằng Username/Password.
*/
bool connectMQTT()
{
    if(config.mqttServer.isEmpty())
    {
        return false;
    }

    mqttClient.setServer(
        config.mqttServer.c_str(),
        config.mqttPort
    );

    mqttClient.setCallback(
        mqttCallback
    );

    String clientId =
        config.deviceName;

    bool connected = false;

    if(config.mqttUsername.isEmpty())
    {
        connected =
            mqttClient.connect(
                clientId.c_str()
            );
    }
    else
    {
        connected =
            mqttClient.connect(
                clientId.c_str(),
                config.mqttUsername.c_str(),
                config.mqttPassword.c_str()
            );
    }

    if(connected)
    {
        publishStatus();

        Serial.println(
            "MQTT Connected"
        );

        mqttClient.subscribe(
            config.mqttTopic.c_str()
        );

        Serial.print(
            "Subscribed: "
        );

        Serial.println(
            config.mqttTopic
        );
    }
    else
    {
        Serial.print(
            "MQTT Failed, rc="
        );

        Serial.println(
            mqttClient.state()
        );
    }

    return connected;
}

/*
    Tự động reconnect MQTT.

    Hàm được gọi liên tục trong loop().
*/
void mqttLoop()
{
    if(WiFi.status() != WL_CONNECTED)
    {
        return;
    }

    if(!mqttClient.connected())
    {
        static unsigned long lastRetry = 0;

        if(millis() - lastRetry > 5000)
        {
            lastRetry = millis();

            connectMQTT();
        }
    }

    mqttClient.loop();
}

/*
    Publish dữ liệu lên MQTT topic đã cấu hình.
*/
bool publishMQTT(const String& payload)
{
    if(!mqttClient.connected())
    {
        return false;
    }

    Serial.println(payload);

    return mqttClient.publish(
        config.mqttTopic.c_str(),
        payload.c_str()
    );
}

/*
  Chuyển gói json thành String sau đó publish
*/
bool publishMQTT_JSON(const String& msgType, JsonDocument& doc)
{
    //   Serial.printf(
    //     "Heap before Json: %u\n",
    //     ESP.getFreeHeap()
    // );

    // Bổ sung trường source (Bắt buộc)
    doc["source"] =
        config.deviceName;

    // Bổ sung trường target (Bắt buộc)
    doc["target"] = 
        config.mqttServer + ":" + config.mqttPort;

    doc["msg_type"] =
        msgType;

    String payload;

    // Serial.printf(
    //     "Heap before serialize: %u\n",
    //     ESP.getFreeHeap()
    // );

    serializeJson(
        doc,
        payload
    );

    // Serial.printf(
    //     "Heap after serialize: %u\n",
    //     ESP.getFreeHeap()
    // );

    return publishMQTT(payload);
}

// ===================== CUSTOMIZED MESSAGE HANDLER ======================
/*
  Các gói tin Publish phải đủ các trường:
  - source
  - target
  - msg_type
*/

/**
 * @brief Kiểu hàm xử lý cho từng loại MQTT message.
 *
 * Mỗi handler sẽ nhận JsonDocument đã parse từ payload MQTT.
 */
using MessageHandler = std::function<void(JsonDocument&)>;

/**
 * @brief Liên kết giữa msg_type và hàm xử lý tương ứng.
 */
struct MessageRoute
{
    String msgType;
    MessageHandler handler;
};

/**
 * @brief Router dùng để tìm và gọi handler theo msg_type.
 *
 * Ví dụ:
 * router.add("status", statusHandler);
 * router.add("restart", restartHandler);
 *
 * Khi dispatch(), router sẽ tự tìm handler phù hợp.
 */
class CommandRouter
{
public:

    /**
     * @brief Đăng ký một handler mới.
     *
     * @param msgType Giá trị của trường "msg_type".
     * @param handler Hàm sẽ được gọi khi nhận đúng msg_type.
     */
    void add(const String& msgType, MessageHandler handler)
    {
        routes.push_back({ msgType, handler });
    }

    /**
     * @brief Tìm handler theo msg_type và thực thi.
     *
     * @return true  Nếu tìm thấy handler.
     * @return false Nếu không có handler phù hợp.
     */
    bool dispatch(JsonDocument& doc)
    {
        String type = doc["msg_type"] | "";

        for (auto& route : routes)
        {
            if (route.msgType == type)
            {
                route.handler(doc);
                return true;
            }
        }

        return false;
    }

private:
    std::vector<MessageRoute> routes;
};

/**
 * @brief Router quản lý toàn bộ MQTT command.
 */
CommandRouter router;

/**
 * @brief Đăng ký handler mới.
 */
void registerHandler(const String& msgType, MessageHandler handler)
{
    router.add(msgType, handler);
}

/*
    Publish trạng thái hiện tại của thiết bị.

    Được gọi khi:
        - MQTT vừa kết nối
        - Nhận lệnh status
        - Home Server yêu cầu đồng bộ trạng thái

    Có thể mở rộng thêm:
        - uptime
        - firmware version
        - sensor values
        - free heap
*/
void publishStatus()
{

    //------------ Cấu trúc gói tin json ----------
    JsonDocument doc;

    doc["ip"] =
        WiFi.localIP().toString();

    doc["rssi"] =
        WiFi.RSSI();

    doc["free_heap"] =
        ESP.getFreeHeap();

    // gọi publish
    publishMQTT_JSON(MSG_STATUS,doc);
}

/*
    MQTT Message Callback

    Mọi message MQTT nhận được sẽ đi qua đây.

    Quy trình:

        MQTT Message
              |
              v
        Parse JSON
              |
              v
        Kiểm tra msg_type
              |
              +---- status
              |
              +---- restart
              |
              +---- relay_control
              |
              +---- sensor_read
              |
              +---- ...
*/
void mqttCallback(
    char* topic,
    byte* payload,
    unsigned int length
)
{
    JsonDocument doc;

    // Kiểm tra định dạng chuẩn JSON
    DeserializationError err =
        deserializeJson(
            doc,
            payload,
            length
        );

    if(err)
    {
        Serial.println(
            "[MQTT] Invalid JSON"
        );
        return;
    }

    // Không phải thiết bị này
    if (doc["target"] != config.deviceName)
        return;

    // Xuất debug
    Serial.println();
    Serial.println("========== MQTT ==========");

    Serial.print("Topic: ");
    Serial.println(topic);

    Serial.print("Source: ");
    Serial.println(doc["source"] | "");

    Serial.print("Type: ");
    Serial.println(doc["msg_type"] | "");

    // Chuyển message đến đúng handler
    if (!router.dispatch(doc))
    {
        Serial.println("Unknown msg_type");
    }
}

/*
    Startup Flow

    1. Load configuration
    2. Try connect WiFi
    3. If fail -> AP Mode
    4. Start Web Server

    Sau này có thể thêm:
        - MQTT
        - OTA
        - NTP
        - Sensor Init
*/
void HomeIoT_begin()
{
    pinMode(
        FACTORY_RESET_PIN,
        INPUT_PULLUP
    );

    loadConfig();

    if(config.initialized)
    {
        connectWiFi();
    }
    startAP();

    setupWebServer();

    if(WiFi.status() == WL_CONNECTED)
    {
        connectMQTT();
    }
}

/*
    Main Loop

    Nhiệm vụ:

        1. Xử lý Web Request
        2. Theo dõi Factory Reset

    Sau này có thể mở rộng:

        - MQTT loop
        - OTA update
        - Sensor polling
        - Relay control
        - Heartbeat publish
*/

static void every_5sec(){
    static long lastTime = 0;
    static String Wifi_status;
    if(millis() - lastTime > 5000){
        lastTime = millis();
        Wifi_status = (WiFi.status() == WL_CONNECTED) ? "true" : "false"; 
        Serial.println("Status: " + Wifi_status);
    }
}

void HomeIoT_loop()
{
    static bool wifi_state = false;
    if(wifi_state != (WiFi.status() == WL_CONNECTED)){
        startAP();
        wifi_state = (WiFi.status() == WL_CONNECTED);
    }

    every_5sec();

    server.handleClient();

    mqttLoop();

    checkFactoryReset();
}