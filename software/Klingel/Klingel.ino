#include <FS.h> 
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

#define SWITCH D5
// LCD: D1 SCL, D2 SDA (also used: 5V, GND) 

// MQTT Topics, WiFi Config
//--------------------------------
const char* MQTT_TOPIC_ACTION = "/MED/Klingel/Press";
const char* MQTT_TOPIC_SETTING_INTERVAL = "/MED/Klingel/Settings/MinInterval";
const char* MQTT_TOPIC_SETTING_DISPLAY_DURATION = "/MED/Klingel/Settings/DisplayDuration";
const char* MQTT_TOPIC_TEXT = "/MED/Klingel/Display/Text";
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

// LCD-Display Setup
//--------------------------------
const int LCD_COLUMNS = 16; // Alt: 20
const int LCD_ROWS = 2; // Alt: 4
LiquidCrystal_I2C lcd(0x27, LCD_COLUMNS, LCD_ROWS);
//--------------------------------

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

long lastPressTime = 0;
long MIN_PRESS_INTERVAL = 5000;//setting updated via MQTT
long lastDisplayEnableTime = 0;
long DISPLAY_ENABLE_DURATION = 20000;//setting updated via MQTT

void setup_wifi()
{
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
  Serial.println("saving config");
  DynamicJsonDocument doc(256);
  doc["mqtt_server"] = MQTT_SERVER;
  doc["mqtt_port"] = MQTT_PORT;
  doc["mqtt_user"] = MQTT_USER;
  doc["mqtt_pass"] = MQTT_PASSWORD;
  
  File configFile = SPIFFS.open("/config.json", "w");
  if (configFile)
  {
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

//print as much of given char[] to connected LCD 
void printTextToLCD(char receivedPayload[], int length){
  int charCounter = 0, rowCounter = 0;
  int i = 0;

  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0, rowCounter);

  //print chars while there are still rows available
  while(rowCounter < LCD_ROWS){
    lcd.print(receivedPayload[charCounter]);

    i++;
    charCounter++;
    if(charCounter >= length){
      break;// stop printing if we reached end of passed in char[]
    }
    
    if(i == LCD_COLUMNS){
      rowCounter++;// update row num if printed to all cells on current row
      i = 0;
      if(rowCounter == LCD_ROWS){
        break;// stop printing if last row finished
      }
      lcd.setCursor(0, rowCounter);
    }
  }
  lastDisplayEnableTime = millis();
}

// callback method for settings changes
void settingsCallback(char *topic, byte *payload, unsigned int length)
{
  char receivedPayload[length];
  for (int i = 0; i < length; i++)
  {
    receivedPayload[i] = (char)payload[i];
    delay(1);//remove this and everything burns (i.e. random symbols are appended to the end of receivedPayload, atoi() becomes inaccurate). why? I don't know.
  }
    
  // Update local settings or text on LCD depending on topic
  if(strcmp(topic, MQTT_TOPIC_SETTING_INTERVAL) == 0)
    MIN_PRESS_INTERVAL = atoi(receivedPayload) * 1000;// convert s to ms
  else if(strcmp(topic, MQTT_TOPIC_SETTING_DISPLAY_DURATION) == 0)
    DISPLAY_ENABLE_DURATION = atoi(receivedPayload) * 1000;// convert s to ms
  else if(strcmp(topic, MQTT_TOPIC_TEXT) == 0)
    printTextToLCD(receivedPayload, length);
}

//continually attempt to reconnect to MQTT broker until possible
void reconnect()
{
  // Loop until we're reconnected
  while (!mqtt_client.connected())
  {
    // Create a random client ID
    String clientId = "Client-";
    clientId += String(random(0xffff), HEX);

    // Attempt to connect
    if (mqtt_client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD))
    {
      // Once connected, (re-)subscribe to settings and text
      mqtt_client.subscribe(MQTT_TOPIC_SETTING_INTERVAL);
      mqtt_client.subscribe(MQTT_TOPIC_SETTING_DISPLAY_DURATION);
      mqtt_client.subscribe(MQTT_TOPIC_TEXT);
    }
    else
    {
      delay(5000); // Wait 5 seconds before retrying
    }
  }
}

//initial setup called on startup
void setup()
{
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  lcd.print("WiFi Setup...");
  setup_wifi();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MQTT Setup...");
  mqtt_client.setServer(MQTT_SERVER, atoi(MQTT_PORT));
  mqtt_client.setCallback(settingsCallback);

  lcd.clear();
  lcd.noBacklight();
  pinMode(SWITCH, INPUT); //enable Klingel-button as input
}

void loop()
{
  if (!mqtt_client.connected())
  {
    reconnect();
  }

  mqtt_client.loop();

  // Klingel-button update
  if (millis() - lastPressTime > MIN_PRESS_INTERVAL)
  {//only check press if interval has passed
    if(digitalRead(SWITCH) == 1){
      //pressed
      lastPressTime = millis();
      String val_str = "Pressed";// any value will do
      char val_buff[val_str.length() + 1];
      val_str.toCharArray(val_buff, val_str.length() + 1);
      mqtt_client.publish(MQTT_TOPIC_ACTION, val_buff);
    }
  }

  //LCD update
  if(lastDisplayEnableTime != 0 && (millis() - lastDisplayEnableTime > DISPLAY_ENABLE_DURATION)){
    lcd.clear();
    lcd.noBacklight();
    lastDisplayEnableTime = 0;
  }
}
