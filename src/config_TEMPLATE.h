#ifndef CONFIG_H
#define CONFIG_H

// Set WiFi credentials
static const String WIFI_SSID("Your SSID");
static const String WIFI_PASS("Your pwd");

static const String MQTT_IP("mqtt ip address");
static const int MQTT_PORT=1883;
static const String MQTT_NAME("hanreader");
static const String MQTT_USER("mqtt user");
static const String MQTT_PASS("mqtt pwd");

static const bool UDP_ENABLED=true;
static const String UDP_IP("udp server ip");
static const int UDP_PORT=6666;


#endif