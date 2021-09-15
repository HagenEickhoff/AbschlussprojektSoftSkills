#include <FS.h> 
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

#define ShieldReciever D4
// MQTT Topics, WiFi Config
//--------------------------------
const char* MQTT_TOPIC_ACTION = "/MED/Briefkasten/Detected";
const char* MQTT_TOPIC_SETTING_INTERVAL = "/MED/Briefkasten/Settings/MinInterval";
const char* SSID = "BriefkastenDetektor WiFi Setup";
const char* WIFI_PASSWORD = "123";
//--------------------------------

// MQTT-Broker
// no empty initiliazitions ( = "" ), otherwise they somehow overwrite data loaded from the config *after* said data was loaded...
//--------------------------------
char* MQTT_SERVER = "192.168."; 
char* MQTT_PORT = "1883"; //changed to char* for simplicity, consider returning to int
char* MQTT_USER = "user (optional)";
char* MQTT_PASSWORD = "user password (optional)";
//--------------------------------

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

long lastDetectTime = 0;
long MIN_PRESS_INTERVAL = 5000;//setting updated via MQTT

void setup_wifi() {
  delay(10);
  randomSeed(micros());
  //attempt loading parameters from filesystem
  loadMQTTParametersFromFS();

  WiFiManager wifiManager;

  //create custom parameters for MQTT data (potentially filled with previously loaded data)
  WiFiManagerParameter param_mqtt_server("server", "MQTT Server", MQTT_SERVER, 15); //15 -> IPv4. Include IPv6/URI?
  WiFiManagerParameter param_mqtt_port("port", "MQTT Port", MQTT_PORT, 6);
  WiFiManagerParameter param_mqtt_user("user", "MQTT User (optional)", MQTT_USER, 32);
  WiFiManagerParameter param_mqtt_password("password", "MQTT Password (optional)", MQTT_PASSWORD, 64);

  wifiManager.addParameter(&param_mqtt_server);
  wifiManager.addParameter(&param_mqtt_port);
  wifiManager.addParameter(&param_mqtt_user);
  wifiManager.addParameter(&param_mqtt_password);

  while (!wifiManager.autoConnect(SSID, WIFI_PASSWORD))
  {
    //failed to connect, wait 5s and retry
    delay(5000);
  }

  //Succesfully connected, read custom parameters & apply them
  MQTT_SERVER = strdup(param_mqtt_server.getValue());
  MQTT_PORT = strdup(param_mqtt_port.getValue());
  MQTT_USER = strdup(param_mqtt_user.getValue());
  MQTT_PASSWORD = strdup(param_mqtt_password.getValue());
  
  saveMQTTParametersToFS();
}
// write MQTT configuration to filesystgem in .json file
void saveMQTTParametersToFS()
{
  DynamicJsonDocument doc(256);
  doc["mqtt_server"] = MQTT_SERVER;
  doc["mqtt_port"] = MQTT_PORT;
  doc["mqtt_user"] = MQTT_USER;
  doc["mqtt_pass"] = MQTT_PASSWORD;
  
  File configFile = SPIFFS.open("/config.json", "w");
  if (configFile)
  {
    serializeJson(doc, Serial);
    serializeJson(doc, configFile);
    configFile.flush();
    configFile.close();
  }
}

// read MQTT configuration from filesystem
void loadMQTTParametersFromFS()
{
  if (SPIFFS.begin())
  {
    if (SPIFFS.exists("/config.json"))
    {
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile)
      {
        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, configFile);
        if (error)
          return;

        //json parsed, store values
        strcpy(MQTT_SERVER, doc["mqtt_server"]);
        strcpy(MQTT_PORT, doc["mqtt_port"]);
        strcpy(MQTT_USER, doc["mqtt_user"]);
        strcpy(MQTT_PASSWORD, doc["mqtt_pass"]);
      }
    }
  }
}

// callback method for settings changes
void settingsCallback(char* topic, byte* payload, unsigned int length) {
  char receivedPayload[length];
  for (int i = 0; i < length; i++)
  {
    receivedPayload[i] = (char)payload[i];
    delay(1);//remove this and everything burns (i.e. random symbols are appended to the end of receivedPayload, atoi() becomes inaccurate). why? I don't know.
  }

  // Update local setting(s) depending on topic
  if(strcmp(topic, MQTT_TOPIC_SETTING_INTERVAL) == 0)
    MIN_PRESS_INTERVAL = atoi(receivedPayload) * 1000; // convert s to ms
}

//continually attempt to reconnect to MQTT broker until possible
void reconnect() {
  // Loop until we're reconnected
  while (!mqtt_client.connected()) {    
    // Create a random client ID
    String clientId = "Client-";
    clientId += String(random(0xffff), HEX);
    
    if (mqtt_client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      // Once connected, (re-)subscribe to settings
      mqtt_client.subscribe(MQTT_TOPIC_SETTING_INTERVAL);
    } else {
      delay(5000); // Wait 5 seconds before retrying
    }
  }
}

//initial setup called on startup
void setup() {
  Serial.begin(115200);
  pinMode(ShieldReciever, INPUT_PULLUP); // enable IR-Shield as input
  setup_wifi();

  mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt_client.setCallback(settingsCallback);
}

void loop() {

  if (!mqtt_client.connected()) {
    reconnect();
  }
  mqtt_client.loop();

  
  if (millis() - lastDetectTime > MIN_PRESS_INTERVAL) {//this time could be a setting, TODO
    if(digitalRead(ShieldReciever) == 0) {
      String val_str = "Detected"; //any value will do
      char val_buff[val_str.length() + 1];
      val_str.toCharArray(val_buff, val_str.length() + 1);
      client.publish(MQTT_TOPIC_DETECTOR, val_buff);
      lastDetectTime = millis();
    }
  }
}