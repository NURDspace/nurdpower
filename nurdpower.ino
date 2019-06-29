#include "Arduino.h"

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <TM1637Display.h>

#define MQTT_HOST   "arbiter.vm.nurd.space"
#define MQTT_PORT   1883
#define MQTT_TOPIC  "power/main/power"

#define PIN_CLK D3
#define PIN_DIO D4

static char esp_id[16];

static WiFiManager wifiManager;
static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);
static TM1637Display display(PIN_CLK, PIN_DIO);

static int brightness = 0;
static int power = 0;

static void mqtt_callback(const char *topic, uint8_t * payload, unsigned int length)
{
    payload[length] = 0;
    Serial.printf("%s = %s\n", topic, payload);

    power = atoi((char *) payload);
    brightness = 0;
}

static bool mqtt_connect(void)
{
    bool ok = mqttClient.connected();
    if (!ok) {
        ok = mqttClient.connect(esp_id);
        if (ok) {
            mqttClient.setCallback(mqtt_callback);
            ok = mqttClient.subscribe(MQTT_TOPIC);
        }
    }

    return ok;
}

void setup(void)
{
    Serial.begin(115200);
    Serial.printf("\nESP-NURDPOWER!\n");

    // get ESP id
    sprintf(esp_id, "%08X", ESP.getChipId());
    Serial.print("ESP ID: ");
    Serial.println(esp_id);

    // connect to wifi
    Serial.println("Starting WIFI manager ...");
    wifiManager.setConfigPortalTimeout(120);
    wifiManager.autoConnect("ESP-POWER");

    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setCallback(mqtt_callback);

    display.setBrightness(7, false);
    display.clear();
}

void loop(void)
{
    static int tick_prev = 0;

    // fade in a new number
    int tick = millis() / 50;
    if (tick != tick_prev) {
        tick_prev = tick;
        if (brightness < 8) {
            display.setBrightness(brightness, true);
            display.showNumberDec(power);
            brightness++;
        }
    }
    // keep connected to mqtt
    if (!mqtt_connect()) {
        Serial.println("Restarting...");
        ESP.restart();
    }
    mqttClient.loop();
}
