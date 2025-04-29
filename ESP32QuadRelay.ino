// === Full Flash-Ready Sketch: Quad Relay Controller with Dynamic Config for GPIOs + NTP + Home Assistant Discovery By Jim (M1Musik.com)===
// Used with board from https://amzn.eu/d/8gHN66b (but should work with any ESP32 Quad Relay)
// ESP32 4-Channel Relay Module Binghe ESP32-WROOM Relay Development Boards with WiFi Bluetooth BLE  (LC-Relay-ESP32-4R-A2)

#include <WiFiManager.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncMqttClient.h>
#include <Preferences.h>
#include <time.h>

Preferences preferences;
AsyncWebServer* server;
AsyncMqttClient mqttClient;

WiFiManagerParameter custom_mqtt_server("server", "MQTT Server", "0.0.0.0", 40);
WiFiManagerParameter custom_mqtt_user("user", "MQTT Username", "username", 32);
WiFiManagerParameter custom_mqtt_pass("pass", "MQTT Password", "password", 32);
WiFiManagerParameter custom_device_name("devname", "Device Name", "esp32_quad_relay", 32);
WiFiManagerParameter custom_relay1("relay1", "Relay 1 GPIO", "25", 4);
WiFiManagerParameter custom_relay2("relay2", "Relay 2 GPIO", "26", 4);
WiFiManagerParameter custom_relay3("relay3", "Relay 3 GPIO", "32", 4);
WiFiManagerParameter custom_relay4("relay4", "Relay 4 GPIO", "33", 4);
WiFiManagerParameter custom_ntp1("ntp1", "NTP Server 1", "0.de.pool.ntp.org", 32);
WiFiManagerParameter custom_ntp2("ntp2", "NTP Server 2", "1.de.pool.ntp.org", 32);
WiFiManagerParameter custom_timezone("timezone", "Timezone (e.g. CET-1CEST,M3.5.0,M10.5.0/3)", "CET-1CEST,M3.5.0,M10.5.0/3", 64);

String mqttServer;
String mqttUser;
String mqttPassword;
String deviceName;
int relayPins[4];
String ntpServer1;
String ntpServer2;
String timezone;

bool relayStates[4] = {false, false, false, false};
String currentTime = "Time Sync Pending";
bool discoverySent = false;

// Function to sanitize deviceName for MQTT/Home Assistant
String sanitizeDeviceName(String name) {
  name.trim();
  name.replace(" ", "_");  // Replace spaces with underscores
  name.replace("#", "");  // Remove invalid characters
  name.replace("+", "");  // Remove MQTT-disallowed characters
  return name;
}

void saveConfig() {
  preferences.begin("config", false);
  preferences.putString("mqttServer", mqttServer);
  preferences.putString("mqttUser", mqttUser);
  preferences.putString("mqttPass", mqttPassword);
  preferences.putString("deviceName", deviceName);
  for (int i = 0; i < 4; i++) preferences.putInt(("relay" + String(i+1)).c_str(), relayPins[i]);
  preferences.putString("ntp1", ntpServer1);
  preferences.putString("ntp2", ntpServer2);
  preferences.putString("timezone", timezone);
  preferences.end();
  setenv("TZ", timezone.c_str(), 1);
  tzset();
}

void loadConfig() {
  preferences.begin("config", true);
  mqttServer = preferences.getString("mqttServer", "0.0.0.0");
  mqttUser = preferences.getString("mqttUser", "username");
  mqttPassword = preferences.getString("mqttPass", "password");
  deviceName = sanitizeDeviceName(preferences.getString("deviceName", "esp32_relayboard"));
  for (int i = 0; i < 4; i++) {
    int defaultRelay = (i == 0) ? 25 : (i == 1) ? 26 : (i == 2) ? 32 : 33;
    relayPins[i] = preferences.getInt(("relay" + String(i+1)).c_str(), defaultRelay);
  }
  ntpServer1 = preferences.getString("ntp1", "0.de.pool.ntp.org");
  ntpServer2 = preferences.getString("ntp2", "1.de.pool.ntp.org");
  timezone = preferences.getString("timezone", "CET-1CEST,M3.5.0,M10.5.0/3");
  preferences.end();
  setenv("TZ", timezone.c_str(), 1);
  tzset();
}

void setRelay(int relayNumber, bool state) {
  if (relayNumber < 1 || relayNumber > 4) return;
  digitalWrite(relayPins[relayNumber - 1], state ? HIGH : LOW);
  relayStates[relayNumber - 1] = state;
}

void publishRelayState(int relayNumber) {
  String switchBase = "homeassistant/switch/" + deviceName;
  String topic = switchBase + "/relay" + relayNumber + "/state";
  mqttClient.publish(topic.c_str(), 1, true, relayStates[relayNumber - 1] ? "ON" : "OFF");
}

void setupTime() {
  configTzTime(timezone.c_str(), ntpServer1.c_str(), ntpServer2.c_str());
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) delay(500);
  currentTime = getFormattedTime();
}

String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "Time Sync Failed";
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}

// you can change this line in publishdiscovery (payload += "\"name\":\"" + deviceName + "\",\"model\":\"LC-Relay-ESP32-4R-A2\",\"manufacturer\":\"Binghe\"}}"; ) to chnge the model and manufacturer, device name is a sanitized version of name entered during setup

void publishDiscovery() {
  String switchBase = "homeassistant/switch/" + deviceName;

  for (int i = 1; i <= 4; i++) {
    String topic = "homeassistant/switch/" + deviceName + "_relay" + i + "/config";
    String payload = "{";
    payload += "\"name\":\"" + deviceName + " Relay " + String(i) + "\",";
    payload += "\"command_topic\":\"" + switchBase + "/relay" + i + "/set\",";
    payload += "\"state_topic\":\"" + switchBase + "/relay" + i + "/state\",";
    payload += "\"device_class\":\"outlet\",";
    payload += "\"unique_id\":\"" + deviceName + "_relay" + String(i) + "\",";
    payload += "\"payload_on\":\"ON\",\"payload_off\":\"OFF\",";
    payload += "\"device\":{\"identifiers\":[\"" + deviceName + "\"],";
    payload += "\"name\":\"" + deviceName + "\",\"model\":\"LC-Relay-ESP32-4R-A2\",\"manufacturer\":\"Binghe\"}}";

    mqttClient.publish(topic.c_str(), 1, true, payload.c_str());
  }

  discoverySent = true;
}

void onMqttConnect(bool sessionPresent) {
  String switchBase = "homeassistant/switch/" + deviceName;
  for (int i = 1; i <= 4; i++) {
    mqttClient.subscribe((switchBase + "/relay" + i + "/set").c_str(), 1);
  }
  if (!discoverySent) publishDiscovery();
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  String message = String(payload).substring(0, len);
  String switchBase = "homeassistant/switch/" + deviceName;
  for (int i = 1; i <= 4; i++) {
    String setTopic = switchBase + "/relay" + i + "/set";
    if (String(topic) == setTopic) {
      setRelay(i, message == "ON");
      publishRelayState(i);
    }
  }
}

void initWebServer() {
  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>ESP32 Quad Relay Controller</title><meta name='viewport' content='width=device-width, initial-scale=1'><style>body{font-family:sans-serif;text-align:center;margin:20px;}button{font-size:18px;margin:10px;padding:10px 20px;border:none;border-radius:8px;cursor:pointer;}.status{margin-top:20px;padding:10px;}.active{background-color:#4CAF50;color:white;}.inactive{background-color:#f44336;color:white;}</style></head><body><h1>ESP32 Quad Relay Controller</h1><div class='status'><h2>WiFi Info</h2><p>SSID: " + WiFi.SSID() + "</p><p>IP Address: " + WiFi.localIP().toString() + "</p><p>Current Time: " + currentTime + "</p></div><div class='status'><h2>Relay Control</h2><div id='relays'></div></div><button class='inactive' onclick='deepReset()'>Enter Config Mode</button><script>async function getStatus(){const res=await fetch('/status');const states=await res.json();const labels=['Relay 1','Relay 2','Relay 3','Relay 4'];const container=document.getElementById('relays');container.innerHTML='';states.forEach((state,i)=>{const btn=document.createElement('button');btn.innerText=labels[i]+': '+(state?'ON':'OFF');btn.className=(state?'active':'inactive');btn.onclick=async()=>{await fetch('/toggle?relay='+(i+1));getStatus();};container.appendChild(btn);});}async function deepReset(){if(confirm('Enter WiFi Config Mode?')){await fetch('/deep_reset',{method:'POST'});}}getStatus();setInterval(getStatus,5000);</script></body></html>";
    request->send(200, "text/html", html);
  });

  server->on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    String status = "[" + String(relayStates[0]) + "," + String(relayStates[1]) + "," + String(relayStates[2]) + "," + String(relayStates[3]) + "]";
    request->send(200, "application/json", status);
  });

  server->on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("relay")) {
      int relay = request->getParam("relay")->value().toInt();
      if (relay >= 1 && relay <= 4) {
        setRelay(relay, !relayStates[relay-1]);
        publishRelayState(relay);
      }
    }
    request->send(200, "text/plain", "OK");
  });

  server->on("/deep_reset", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Rebooting into config mode...");
    delay(1000);
    preferences.begin("config", false);
    preferences.putBool("forceConfig", true);
    preferences.end();
  setenv("TZ", timezone.c_str(), 1);
  tzset();
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    delay(1000);
    esp_restart();
  });
}

void connectToMqtt() {
  mqttClient.setServer(mqttServer.c_str(), 1883);
  mqttClient.setCredentials(mqttUser.c_str(), mqttPassword.c_str());
  mqttClient.connect();
}

void setup() {
  //change cpu speed as required, but 80 worked ok on the test board
  setCpuFrequencyMhz(80);
  Serial.begin(115200);

  loadConfig();

  preferences.begin("config", false);
  bool forceConfig = preferences.getBool("forceConfig", false);
  preferences.end();
  setenv("TZ", timezone.c_str(), 1);
  tzset();

  WiFiManager wm;

  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_user);
  wm.addParameter(&custom_mqtt_pass);
  wm.addParameter(&custom_device_name);
  wm.addParameter(&custom_relay1);
  wm.addParameter(&custom_relay2);
  wm.addParameter(&custom_relay3);
  wm.addParameter(&custom_relay4);
  wm.addParameter(&custom_ntp1);
  wm.addParameter(&custom_ntp2);
  wm.addParameter(&custom_timezone);

  if (forceConfig) {
    preferences.begin("config", false);
    preferences.putBool("forceConfig", false);
    preferences.end();
  setenv("TZ", timezone.c_str(), 1);
  tzset();

    wm.setConfigPortalTimeout(300);
    if (!wm.startConfigPortal("ESP32_Quad_Relay")) {
      delay(3000);
      esp_restart();
    }

    mqttServer = custom_mqtt_server.getValue();
    mqttUser = custom_mqtt_user.getValue();
    mqttPassword = custom_mqtt_pass.getValue();
    deviceName = custom_device_name.getValue();
  timezone = custom_timezone.getValue();
    relayPins[0] = atoi(custom_relay1.getValue());
    relayPins[1] = atoi(custom_relay2.getValue());
    relayPins[2] = atoi(custom_relay3.getValue());
    relayPins[3] = atoi(custom_relay4.getValue());
    ntpServer1 = custom_ntp1.getValue();
    ntpServer2 = custom_ntp2.getValue();

    saveConfig();

    delay(1000);
    esp_restart();
  } else {
    if (!wm.autoConnect("ESP32_Quad_Relay")) {
      delay(3000);
      esp_restart();
    }
  }

  Serial.println("WiFi connected!");
  Serial.println("MQTT Server: " + mqttServer);
  Serial.println("Device Name: " + deviceName);

  server = new AsyncWebServer(80);
  initWebServer();
  server->begin();

  for (int i = 0; i < 4; i++) {
    pinMode(relayPins[i], OUTPUT);
    setRelay(i + 1, false);
  }

  setupTime();

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onMessage(onMqttMessage);
  connectToMqtt();

  esp_wifi_set_ps(WIFI_PS_NONE);
}

void loop() {
  static unsigned long lastReconnectAttempt = 0;
  static unsigned long lastTimeUpdate = 0;

  if (millis() - lastTimeUpdate > 1000) {
    currentTime = getFormattedTime();
    lastTimeUpdate = millis();
  }

  if (!mqttClient.connected()) {
    if (millis() - lastReconnectAttempt > 5000) {
      connectToMqtt();
      lastReconnectAttempt = millis();
    }
  }
}
