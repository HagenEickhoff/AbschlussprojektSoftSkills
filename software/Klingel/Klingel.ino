#include <FS.h> 
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

// MQTT Topics, WiFi Config
//--------------------------------
const char* MQTT_TOPIC_ACTION = "/DoorAssistant/Klingel/Press";
const char* MQTT_TOPIC_SETTINGS = "/DoorAssistant/Klingel/Settings/MinInterval";
const char* SSID = "Klingel WiFi Setup";
const char* WIFI_PASSWORD = "123";
//--------------------------------

// MQTT-Broker (data inserted via setup_wifi())
// no empty initiliazitions ( = "" ), otherwise they somehow overwrite data loaded from the config *after* said data was loaded...
//--------------------------------
char* MQTT_SERVER = "192.168."; 
char* MQTT_PORT = "1883"; //changed to char* for simplicity, consider returning to int
char* MQTT_USER = "user (optional)";
char* MQTT_PASSWORD = "user password (optional)";
//--------------------------------

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

long lastPressTime = 0;
long MIN_PRESS_INTERVAL = 5000;//setting updated via MQTT

void setup_wifi()
{
  delay(10);
  randomSeed(micros());
  loadMQTTParametersFromFS();

  WiFiManager wifiManager;

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
    //failed to connect; print warning & restart MC
    Serial.println("Failed to connect, retrying...");
    delay(5000);
  }

  //Succesfully connected, read custom parameters & apply them
  MQTT_SERVER = strdup(param_mqtt_server.getValue());
  MQTT_PORT = strdup(param_mqtt_port.getValue());
  MQTT_USER = strdup(param_mqtt_user.getValue());
  MQTT_PASSWORD = strdup(param_mqtt_password.getValue());
  
  saveMQTTParametersToFS();
  
  //DEBUG
  Serial.println("RECEIVED PARAMS:");
  Serial.print("SERVER:");
  Serial.println(MQTT_SERVER);
  Serial.print("PORT:");
  Serial.println(MQTT_PORT);
  Serial.print("USER:");
  Serial.println(MQTT_USER);
  Serial.print("PW:");
  Serial.println(MQTT_PASSWORD);

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// write MQTT configuration to filesystgem in .json
void saveMQTTParametersToFS()
{
  Serial.println("saving config");
  DynamicJsonDocument doc(256);
  doc["mqtt_server"] = MQTT_SERVER;
  doc["mqtt_port"] = MQTT_PORT;
  doc["mqtt_user"] = MQTT_USER;
  doc["mqtt_pass"] = MQTT_PASSWORD;
  
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile)
  {
    Serial.println("failed to open config file for writing");
  }else{
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
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json"))
    {
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile)
      {
        Serial.println("opened config file");
        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, configFile);
        if (error)
          return;

        Serial.println("\nparsed json!");
        strcpy(MQTT_SERVER, doc["mqtt_server"]);
        strcpy(MQTT_PORT, doc["mqtt_port"]);
        strcpy(MQTT_USER, doc["mqtt_user"]);
        strcpy(MQTT_PASSWORD, doc["mqtt_pass"]);
        
        const char* srvr = doc["mqtt_server"];
        Serial.print("doc server"); Serial.println(srvr);        
        Serial.print("local server"); Serial.println(MQTT_SERVER);
        serializeJson(doc, Serial);
      }
    }
  }
  else
  {
    Serial.println("failed to mount FS");
  }
}

// callback method for settings changes
void settingsCallback(char *topic, byte *payload, unsigned int length)
{
  if (strcmp(topic, MQTT_TOPIC_SETTINGS) == 0)
  {
    char receivedPayload[length];
    for (int i = 0; i < length; i++)
    {
      Serial.print((char)payload[i]);
      receivedPayload[i] = (char)payload[i];
    }
    Serial.println();
    MIN_PRESS_INTERVAL = atoi(receivedPayload) * 1000; // convert s to ms
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!mqtt_client.connected())
  {
    Serial.print("Attempting MQTT connection...");

    // Create a random client ID
    String clientId = "Client-";
    clientId += String(random(0xffff), HEX);

    // Attempt to connect
    if (mqtt_client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD))
    {
      Serial.println("connected");
      // Once connected, (re-)subscribe to settings
      mqtt_client.subscribe(MQTT_TOPIC_SETTINGS);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  setup_wifi();

  mqtt_client.setServer(MQTT_SERVER, atoi(MQTT_PORT));
  mqtt_client.setCallback(settingsCallback);
}

void loop()
{

  if (!mqtt_client.connected())
  {
    reconnect();
  }
  mqtt_client.loop();

  if (millis() - lastPressTime > MIN_PRESS_INTERVAL)
  {
    lastPressTime = millis();

    // TODO: Check if button pressed, if so: publish any message to MQTT_TOPIC_KLINGEL
    String val_str = "Pressed";
    char val_buff[val_str.length() + 1];
    val_str.toCharArray(val_buff, val_str.length() + 1);
    mqtt_client.publish(MQTT_TOPIC_ACTION, val_buff);
  }
}
