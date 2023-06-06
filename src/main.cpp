#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>

#define wifi_ssid "SSID"
#define wifi_password "password"

#define mqtt_server "localhost"
#define mqtt_user "user"
#define mqtt_password "password"

#define temperature_f_topic "pool/tempF"
#define temperature_c_topic "pool/tempC"

WiFiClient espClient;
PubSubClient client(espClient);

const int oneWireBus = 4;     
OneWire oneWire(oneWireBus);

DallasTemperature sensors(&oneWire);

void setup() {
  client.setServer(mqtt_server, 1883);
  Serial.begin(115200);
  sensors.begin();
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // print local ip
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//check if temp is the same or close enough as it was
bool checkBound(float newValue, float prevValue, float maxDiff) {
  return !isnan(newValue) &&
         (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}

long lastMsg = 0;
float tempC = 0.0;
float tempF = 0.0;
float hum = 0.0;
float diff = 0.2;

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    
    sensors.requestTemperatures(); 
    float newTempC = sensors.getTempCByIndex(0);
    float newTempF = sensors.getTempFByIndex(0);
    //print and publish temp C
    if (checkBound(newTempC, tempC, diff)) {
      tempC = newTempC;
      Serial.print(tempC);
      Serial.println("ºC");
      client.publish(temperature_c_topic, String(tempC).c_str(), true);
    }
    //print and publish temp F
    if (checkBound(newTempF, tempF, diff)) {
      tempF = newTempF;
      Serial.print(tempF);
      Serial.println("ºF");
      client.publish(temperature_f_topic, String(tempF).c_str(), true);
    }
  }
}