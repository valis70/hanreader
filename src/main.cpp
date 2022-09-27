#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "config.h"

static const String VERSION("0.0.4");

// The HAN metrics that will be interpreted. Just add or remove at your likings...
static const struct SensorInfo
{
  const char *haName;
  const char *mqttName;
  const char *hanName;
  const char *unit;
  const char *sensorClass;
} sensors[]=
{
  {"", "EnergyBuy",  "1-0:1.8.0",  "kWh",  "energy"},
  {"", "EnergySell", "1-0:2.8.0",  "kWh",  "energy"},
  {"", "PowerBuy",   "1-0:1.7.0",  "kW",   "power"},
  {"", "PowerSell",  "1-0:2.7.0",  "kW",   "power"},
  {"", "VoltageL1",  "1-0:32.7.0", "V",    "voltage"},
  {"", "VoltageL2",  "1-0:52.7.0", "V",    "voltage"},
  {"", "VoltageL3",  "1-0:72.7.0", "V",    "voltage"},
  {"", "CurrentL1",  "1-0:31.7.0",  "A",  "current"},
  {"", "CurrentL2",  "1-0:51.7.0",  "A",  "current"},
  {"", "CurrentL3",  "1-0:71.7.0",  "A",  "current"},
  {"", "","","",""}
};

WiFiUDP UDP;

WiFiClient wifiClient;
PubSubClient client(wifiClient);


void udpLog(const char *str)
{
  if(UDP_ENABLED)
  {
    UDP.beginPacket(UDP_IP.c_str(), UDP_PORT);
    UDP.write(str);
    UDP.endPacket();
  }
}

void udpLog(const String &str)
{
  udpLog(str.c_str());
}

void sendMqttDiscoveryMsgs()
{
  int i=0;
  while (sensors[i].mqttName[0]!=0)
  {
    DynamicJsonDocument doc(1024);
    char buffer[256];

    // Construct the topic names
    String discoveryTopic = "homeassistant/sensor/"+MQTT_NAME+"/"+String(sensors[i].mqttName)+"/config";
    String stateTopic = MQTT_NAME+"/"+String(sensors[i].mqttName)+"/state";

    // Poipulate the json
    doc["name"] = sensors[i].mqttName;
    doc["stat_t"] = stateTopic;
    doc["unit_of_meas"] = sensors[i].unit;
    doc["dev_cla"] = sensors[i].sensorClass;
    doc["frc_upd"] = true;
    doc["unique_id"]= String(MQTT_NAME)+"_"+String(sensors[i].mqttName);

    // Serialize the json
    size_t n = serializeJson(doc, buffer);

    // Publish the discovery message
    if(!client.publish(discoveryTopic.c_str(), buffer, n))
    {
      // Just logging any problems. Except for first boot, the matrics should already be registered...
      udpLog("publish of discovery failed");
    }
    i++;
  }

}


void mqttTryReconnectIfNeeded()
{
  if(!client.connected())
  {
    udpLog("reconnecting mqtt");
    if (!client.connect(MQTT_NAME.c_str(), MQTT_USER.c_str(), MQTT_PASS.c_str()))
    {
      udpLog("connect failed");
    }
 }
}




void setup()
{

  // Set up serial for the HAN port
  Serial.begin(115200, SERIAL_8N1, SERIAL_FULL, 1, true);

  // Begin WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Loop continuously while WiFi is not connected
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
  }  

  // Just send something to the UDP logger to show we're alive...
  udpLog("-");
  delay(100);
  udpLog("Starting");

  // Enable OTA updates
  ArduinoOTA.begin();

  // Connect to the MQTT server
  client.setServer(MQTT_IP.c_str(), MQTT_PORT);
  while (!client.connected())
  {
    if (client.connect(MQTT_NAME.c_str(), MQTT_USER.c_str(), MQTT_PASS.c_str()))
    {
      udpLog("Connected to MQTT");
    }
    else
    {
      udpLog("Failed to connect to mqtt with state: "+String(client.state()));
      delay(2000);
    }
  }

  // Send the discovery messages for hazzle free HA config
  sendMqttDiscoveryMsgs();

  // Just some logging
  udpLog("Up and running. IP: "+WiFi.localIP().toString());
  udpLog("Using firmware " + VERSION);
}

void loop()
{
  // Handle OTA
  ArduinoOTA.handle();

  // Handle anything the meter is throwing at us
  if(Serial.available())
  {    
    // Read a line - we expect the meter to be able to deliver one line at the time.
    String s=Serial.readStringUntil('\n');

    // Handle the known metrics
    int i=0;
    while (sensors[i].mqttName[0]!=0)
    {
      if(s.startsWith(sensors[i].hanName))
      {
        // We have a match, let's get the value
        char str[50];
        strcpy(str,s.c_str());
        char *start =strchr(str,'(')+1;
        start+=strspn(start,"0");
        int len=strcspn(start,"*");
        start[len]=0;
        if(start[0]=='.')
          start--;
        
        // Log to udp
        udpLog(String(String(sensors[i].mqttName)+": "+start).c_str());

        // Construct the topic
        String stateTopic = MQTT_NAME+"/"+String(sensors[i].mqttName)+"/state";

        // If not connected to MQTT, try to reconnect. Dunno if this is the proper behavior...
        // The idea is to reconnect (if needed) before we try to publish the message to 
        // increase the chanses of a successful publish
        if(!client.connected())
        {
          mqttTryReconnectIfNeeded();
        }

        // Publish to MQTT. If failing, try to reconnect. Dunno if this is the proper behavior...
        // We ignore any problems because we failed - we will get a new reading in 10 secs... :)
        // Then we force disconnect to trigger a reconnect.
        if(!client.publish(stateTopic.c_str(), start))
        {
          udpLog("publish failed");
          client.disconnect();
          mqttTryReconnectIfNeeded();
        }
      }
      else
      {
        udpLog("Unhandled metric: "+s);
      }
      i++;
    }
  }
  else
  {
    // If nothing available on the Serial, log RSSI every intervalRssiMs (10 seconds)
    static long lastRssi=0;
    static const long intervalRssiMs=10000;
    long currentRssi=millis();
    if (currentRssi - lastRssi >= intervalRssiMs)
    {
      lastRssi=currentRssi;
      udpLog("RSSI: "+ String(WiFi.RSSI()));
    }
  }    
}
