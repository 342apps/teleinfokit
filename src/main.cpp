//#include <FS.h>
#include "LittleFS.h"
#include <Arduino.h>
#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include "Button2.h"

#include "data.h"
#include "espteleinfo.h"
#include "display.h"
#include "webserver.h"

#define PIN_OPTO 3
#define PIN_BUTTON 1
#define CONFIG_FILE "/config.dat"
#define RESET_CONFIRM_DELAY 10000
#define AP_NAME "TeleInfoKit"
#define AP_PWD  "givememylinkydata"

#define REFRESH_DELAY 1000

// The structure that stores network configuration
typedef struct
{
  char mqtt_server[40];
  char mqtt_port[6];
  char mqtt_server_username[20];
  char mqtt_server_password[20];
} ConfStruct;

ConfStruct config;

// screen refresh delay
unsigned long refreshTime = 0;

// all screens available
enum modes
{
  GRAPH,
  DATA1,
  DATA2,
  DATA3,
  NETWORK,
  RESET,
  OFF
};

uint8_t mode = 0;

// network configuration reset states
enum reset_states
{
  IDLE,
  RST_PAGE,
  RST_REQ,
  RST_ACK
};

uint8_t reset = IDLE;

// timestamp for reset request auto-cancellation
unsigned long resetTs = 0;

Data *data;
Display *d;
ESPTeleInfo ti = ESPTeleInfo();
WebServer *web;
Button2 button = Button2(PIN_BUTTON);
WiFiManager wifiManager;

// network configuration variables
char mqtt_server[40];
char mqtt_port[6] = "1883";
char mqtt_server_username[20];
char mqtt_server_password[20];

// flag for saving network configuration
bool shouldSaveConfig = false;

// When hotspot is enabled to configure wifi connection
void configModeCallback(WiFiManager *myWiFiManager)
{
  d->log("Hotspot Wifi: " + myWiFiManager->getConfigPortalSSID() + "\n" + WiFi.softAPIP().toString());
}

// Network connection has been done through captive portal of hotspot
void saveConfigCallback()
{
  d->log("Configuration reseau OK");
  shouldSaveConfig = true;
}

// Retreives configuration from filesystem
void readConfig()
{

  if (LittleFS.begin())
  {
    if (LittleFS.exists(CONFIG_FILE))
    {
      //file exists, reading and loading
      d->logPercent("Lecture configuration", 10);
      delay(350); // just to see progress bar

      File configFile = LittleFS.open(CONFIG_FILE, "r");
      if (configFile)
      {
        configFile.read((byte *)&config, sizeof(config));

        strcpy(mqtt_server, config.mqtt_server);
        strcpy(mqtt_port, config.mqtt_port);
        strcpy(mqtt_server_username, config.mqtt_server_username);
        strcpy(mqtt_server_password, config.mqtt_server_password);
        configFile.close();

        d->logPercent("Configuration chargée", 30);
        delay(350); // just to see progress bar
      }
      else
      {
        {
          d->log("Aucun fichier de configuration");
        }
      }
    }
  }
  else
  {
    d->log("Erreur montage filesystem");
  }
}

// Handles clicks on button
void handlerBtn(Button2 &btn)
{
  switch (btn.getClickType())
  {
  case SINGLE_CLICK:
    mode = (mode + 1) % 7; // cycle through 7 screens
    resetTs = 0;
    break;
  case DOUBLE_CLICK:
    break;
  case TRIPLE_CLICK:
    break;
  case LONG_CLICK:
    // reset state machine mgmt
    if (reset == RST_PAGE)
    {
      reset = RST_REQ;
      // display reset confirmation
      d->log("Appui long pour confirmer\nAppui court pour annuler", 10);
      resetTs = millis();
    }
    else if (reset == RST_REQ)
    {
      reset = RST_ACK;
      // display reset requested
      d->log("Reinitialisation en cours");
      // reset
      wifiManager.resetSettings();
      // display restart
      d->log("Redemarrage", 2000);
      // restart
      ESP.restart();
    }
    break;
  }
}

void setup()
{
  data = new Data();
  d = new Display();
  data->init();
  ti.init();
  d->init(data);
  web = new WebServer();

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  //pinMode(PIN_OPTO, INPUT_PULLUP);
  button.setClickHandler(handlerBtn);
  button.setLongClickHandler(handlerBtn);

  d->logPercent("Demarrage", 5);
  delay(350); // just to see progress bar

  readConfig();
  uint16_t port = 1883;
  if(config.mqtt_port[0] != '\0')
  {
    port = atoi(config.mqtt_port);
    delay(1000);
  }
  ti.initMqtt(config.mqtt_server, port, config.mqtt_server_username, config.mqtt_server_password);

  d->logPercent("Connexion au reseau wifi...", 50);
  delay(350); // just to see progress bar

  // ========= WIFI MANAGER =========
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_username("username", "mqtt username", mqtt_server_username, 40);
  WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_server_password, 40);

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_username);
  wifiManager.addParameter(&custom_mqtt_password);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(AP_NAME, AP_PWD))
  {
    d->log("Could not connect");
    //reset and try again
    ESP.reset();
    delay(1000);
  }

  WiFi.hostname("TeleInfoKit_" + String(ESP.getChipId()));

  d->logPercent("Connecte a " + String(WiFi.SSID()), 80);
  delay(500); // just to see progress bar

// TODO issue on load here ??
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_server_username, custom_mqtt_username.getValue());
  strcpy(mqtt_server_password, custom_mqtt_password.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig)
  {
    d->logPercent("Sauvegarde configuration", 60);

    File configFile = LittleFS.open(CONFIG_FILE, "w");
    if (!configFile)
    {
      d->log("Erreur ecriture config");
    }
    else
    {
      strcpy(config.mqtt_server, custom_mqtt_server.getValue());
      strcpy(config.mqtt_port, custom_mqtt_port.getValue());
      strcpy(config.mqtt_server_username, custom_mqtt_username.getValue());
      strcpy(config.mqtt_server_password, custom_mqtt_password.getValue());
      configFile.write((byte *)&config, sizeof(config));
      d->logPercent("Configuration sauvee", 70);
    }

    ti.initMqtt(config.mqtt_server, port, config.mqtt_server_username, config.mqtt_server_password);
    configFile.close();
    //end save
  }

  // ================ OTA ================
  ArduinoOTA.setHostname("teleinfokit");
  ArduinoOTA.setPassword("admin4tele9Info");
  d->logPercent("Demarrage OTA", 90);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
    {
      type = "sketch";
    }
    else
    { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    d->log("Demarrage MAJ " + type);
  });
  ArduinoOTA.onEnd([]() {
    d->log("MAJ complete");
    delay(1000);
    d->log("Redemarrage en cours\nVeuillez patienter");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    uint8_t percent = (progress / (total / 100));
    d->logPercent("MAJ en cours", percent);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    d->log("OTA error " + error);
    if (error == OTA_AUTH_ERROR)
    {
      d->log("OTA Auth Failed");
    }
    else if (error == OTA_BEGIN_ERROR)
    {
      d->log("OTA Begin Failed");
    }
    else if (error == OTA_CONNECT_ERROR)
    {
      d->log("OTA Connect Failed");
    }
    else if (error == OTA_RECEIVE_ERROR)
    {
      d->log("OTA Receive Failed");
    }
    else if (error == OTA_END_ERROR)
    {
      d->log("OTA End Failed");
    }
  });
  ArduinoOTA.begin();

  web->init(&ti);

  if(!ti.LogStartup()){
    d->logPercent("Demarrage termine", 100);
    d->log("Erreur config MQTT \nReinitialiser les reglages",5000);
  }

  d->logPercent("Demarrage termine", 100);
  delay(300);
  d->displayReset();
}

void loop()
{
  ArduinoOTA.handle();
  button.loop();
  web->loop();

  // for cancelling reset settings requests automatically
  if (resetTs != 0 && (millis() - resetTs > RESET_CONFIRM_DELAY))
  {
    resetTs = 0;
    d->displayReset();
  }

  if (millis() - refreshTime > REFRESH_DELAY)
  {
    // data.storeValue(ti.hp, ti.hc);

    switch (mode)
    {
    case GRAPH:
      reset = IDLE;
      d->drawGraph();
      break;
    case DATA1:
      reset = IDLE;
      d->displayData1(ti.papp, ti.iinst_old);
      break;
    case DATA2:
      reset = IDLE;
      d->displayData2(ti.hp, ti.hc);
      break;
    case DATA3:
      reset = IDLE;
      d->displayData3(ti.adc0, ti.isousc, ti.ptec);
      break;
    case NETWORK:
      reset = IDLE;
      d->displayNetwork();
      break;
    case RESET:
      if (reset != RST_REQ && reset != RST_ACK)
      {
        reset = RST_PAGE;
        d->displayReset();
      }
      break;
    case OFF:
      reset = IDLE;
      d->displayOff();
      break;
    }

    refreshTime = millis();
  }

  ti.loop();
}
