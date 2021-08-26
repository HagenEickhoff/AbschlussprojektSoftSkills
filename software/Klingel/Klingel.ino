#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// MQTT Topics, WiFi Config
//--------------------------------
const char* MQTT_TOPIC_KLINGEL = "YYYYYYY";
const char* MQTT_TOPIC_SETTINGS = "YYYYYYYY";
const char* SSID = "XXXXXXXXXX";
const char* WIFI_PASSWORD = "XXXXXXXXXX";
//--------------------------------

// MQTT-Broker
//--------------------------------
const char* MQTT_SERVER = "localhost";
const int MQTT_PORT = 1883;
//const char* MQTT_USER = "XXX";
//const char* MQTT_PASSWORD = "XXX";
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
    // TODO
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

  if (millis() - lastMsg > 5000) {//fastest re-ring time -> 5 seconds
    lastMsg = millis();

    // TODO: Check if button pressed, if so: publish any message to MQTT_TOPIC_KLINGEL

    /* For button press:
    // Publish int (auch hier muss die Zahl erst in einen String umgewandelt werden, damit wir daraus ein Array machen können)
    String val_str = String(val);
    char val_buff[val_str.length() + 1];
    val_str.toCharArray(val_buff, val_str.length() + 1);
    client.publish(MQTT_TOPIC_KLINGEL, val_buff);
    */
  }
}