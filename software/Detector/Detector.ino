#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// MQTT Topics, WiFi Config
//--------------------------------
const char* MQTT_TOPIC_ACTION = "/DoorAssistant/Briefkasten/Detected";
const char* MQTT_TOPIC_SETTINGS = "/DoorAssistant/Briefkasten/Settings/MinInterval";
const char* SSID = "BriefkastenDetector WiFi Setup";
const char* WIFI_PASSWORD = "123";
//--------------------------------

// MQTT-Broker
//--------------------------------
char* MQTT_SERVER = "";
char* MQTT_PORT = "1883";
char* MQTT_USER = "";
char* MQTT_PASSWORD = "";
//--------------------------------

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;

//TODO: Replace with WiFiManager implementation
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.begin(SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// callback method for settings changes
void settingsCallback(char* topic, byte* payload, unsigned int length) {
    // TODO (check if even necessary)
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Create a random client ID
    String clientId = "Client-";
    clientId += String(random(0xffff), HEX);
    
    // Attempt to connect
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      // Once connected, (re-)subscribe to settings
      client.subscribe(MQTT_TOPIC_SETTINGS);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();

  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(settingsCallback);

}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // TODO: Check if anythings detected...

  if (millis() - lastMsg > 5000) {//this time could be a setting, TODO
    lastMsg = millis();

    // TODO: ... if so, send any message to MQTT_TOPIC_DETECTOR

    /*
    // Publish int (auch hier muss die Zahl erst in einen String umgewandelt werden, damit wir daraus ein Array machen können)
    String val_str = String(val);
    char val_buff[val_str.length() + 1];
    val_str.toCharArray(val_buff, val_str.length() + 1);
    client.publish(MQTT_TOPIC_DETECTOR, val_buff);
    */
  }
}