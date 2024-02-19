#include <WiFiManager.h>
#include <time.h>
#include <TZ.h>
#include <stdio.h>

#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "LittleFS.h"

#include "Button2.h"

#include "espteleinfo.h"
#include "display.h"

#define PIN_OPTO 3
#define PIN_BUTTON 1
#define CONFIG_FILE "/config.dat"
#define RESET_CONFIRM_DELAY 10000
#define SCREEN_OFF_MESSAGE_DELAY 5000
#define SCREENSAVER_DELAY 60000
#define AP_NAME "TeleInfoKit"
#define AP_PWD "givememydata"

#define REFRESH_DELAY 1000

// The structure that stores configuration
typedef struct
{
  bool mode_tic_standard;
  char mqtt_server[40];
  char mqtt_port[6];
  char mqtt_server_username[32];
  char mqtt_server_password[32];
  char data_transmission_period[10];
} ConfStruct;

ConfStruct config;

// screen refresh delay
unsigned long refreshTime = 0;
unsigned long mtime = 0;

// all screens available
enum modes
{
  GRAPH,
  DATA1,
  DATA2,
  NETWORK,
  TIME,
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

// flag for saving network configuration
bool shouldSaveConfig = false;

// timestamp for message before screen off
unsigned long offTs = 0;
bool screensaver = false;

bool reset_possible = true;
bool reset_pending = false;

Data *data;
Display *d;
ESPTeleInfo ti = ESPTeleInfo();
Button2 button = Button2(PIN_BUTTON);

time_t now;

void getTime()
{
  now = time(nullptr);
  unsigned timeout = 5000; // try for timeout
  unsigned start = millis();

  configTime(TZ_Europe_Paris, "pool.ntp.org", "time.nist.gov");
  d->log("Waiting for NTP time sync: ");
  while (now < 8 * 3600 * 2)
  { // what is this ?
    delay(100);
    // Serial.print(".");
    now = time(nullptr);
    if ((millis() - start) > timeout)
    {
      d->log("[ERROR] Failed to get NTP time.");
      return;
    }
  }

  struct tm timeinfo;

  char buffer[80];
  localtime_r(&now, &timeinfo); // update the structure tm with the current time
  strftime(buffer, 80, "%a %d %b %Y %H:%M:%S ", &timeinfo);
  d->log(buffer, 1000);
}

// #REGION WifiManager ==================================
WiFiManager wm;

bool TEST_NET = true;      // do a network test after connect, (gets ntp time)
bool ALLOWONDEMAND = true; // enable on demand
bool WMISBLOCKING = true;  // use blocking or non blocking mode, non global params wont work in non blocking

// network configuration variables
char mode_tic_std_char[1];
char mqtt_server[40];
char mqtt_port[6];
char mqtt_server_username[32];
char mqtt_server_password[32];
char data_transmission_period[10];
char UNIQUE_ID[30];

char _customHtml_checkbox[] = "type=\"checkbox\"";
WiFiManagerParameter *custom_checkbox;
WiFiManagerParameter *custom_html;
WiFiManagerParameter *custom_mqtt_server;
WiFiManagerParameter *custom_mqtt_port;
WiFiManagerParameter *custom_mqtt_username;
WiFiManagerParameter *custom_mqtt_password;
WiFiManagerParameter *custom_data_transmission_period;

// Network connection has been done through captive portal of hotspot or through config webportal
void saveConfigCallback()
{
  d->log("Configuration WiFi sauvée", 500);
}

// gets called when WiFiManager enters configuration mode
void configModeCallback(WiFiManager *myWiFiManager)
{
  d->log("Hotspot Wifi: " + myWiFiManager->getConfigPortalSSID() + "\n" + WiFi.softAPIP().toString());
}

// Retreives configuration from filesystem
void readConfig()
{

  if (LittleFS.begin())
  {
    if (LittleFS.exists(CONFIG_FILE))
    {
      // file exists, reading and loading
      d->logPercent("Lecture configuration", 10);

      File configFile = LittleFS.open(CONFIG_FILE, "r");
      if (configFile)
      {
        configFile.read((byte *)&config, sizeof(config));

        if (config.mode_tic_standard)
        {
          strcat(_customHtml_checkbox, " checked");
        }
        else
        {
          strcpy(_customHtml_checkbox, "type=\"checkbox\"");
        }
        
        strcpy(mqtt_server, config.mqtt_server);
        strcpy(mqtt_port, config.mqtt_port);
        strcpy(mqtt_server_username, config.mqtt_server_username);
        strcpy(mqtt_server_password, config.mqtt_server_password);
        strcpy(data_transmission_period, config.data_transmission_period);
        configFile.close();

        d->logPercent("Configuration chargée", 15);
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

void saveParamCallback()
{
  d->logPercent("Sauvegarde configuration", 5);
  shouldSaveConfig = true;

  strcpy(mode_tic_std_char, custom_checkbox->getValue());
  strcpy(mqtt_server, custom_mqtt_server->getValue());
  strcpy(mqtt_port, custom_mqtt_port->getValue());
  strcpy(mqtt_server_username, custom_mqtt_username->getValue());
  strcpy(mqtt_server_password, custom_mqtt_password->getValue());
  strcpy(data_transmission_period, custom_data_transmission_period->getValue());

  File configFile = LittleFS.open(CONFIG_FILE, "w");
  if (!configFile)
  {
    d->log("Erreur écriture config");
  }
  else
  {
    bool std = strcmp(custom_checkbox->getValue(), "T") == 0;
    if (std != config.mode_tic_standard)
    {
      ti.init(config.mode_tic_standard ? TINFO_MODE_STANDARD : TINFO_MODE_HISTORIQUE);
    }
    config.mode_tic_standard = std;
    strcpy(config.mqtt_server, custom_mqtt_server->getValue());
    strcpy(config.mqtt_port, custom_mqtt_port->getValue());
    strcpy(config.mqtt_server_username, custom_mqtt_username->getValue());
    strcpy(config.mqtt_server_password, custom_mqtt_password->getValue());
    strcpy(config.data_transmission_period, custom_data_transmission_period->getValue());
    configFile.write((byte *)&config, sizeof(config));

    if (config.mode_tic_standard)
    {
      strcat(_customHtml_checkbox, " checked");
    }
    else
    {
      strcpy(_customHtml_checkbox, "type=\"checkbox\"");
    }
    custom_checkbox = new WiFiManagerParameter("mode_tic_std", "Mode TIC Standard", "T", 2, _customHtml_checkbox, WFM_LABEL_BEFORE);

    d->drawGraph(ti.papp, config.mode_tic_standard ? 'S' : 'H');

    for (uint8_t i = 10; i <= 100; i++)
    {
      d->logPercent("Sauvegarde configuration", i);
      delay(5);
    }

    d->logPercent("Configuration sauvée", 100);
    delay(500);
  }

  uint16_t port = 1883;
  if (config.mqtt_port[0] != '\0')
  {
    port = atoi(config.mqtt_port);
    delay(1000);
  }
  ti.init(config.mode_tic_standard ? TINFO_MODE_STANDARD : TINFO_MODE_HISTORIQUE);
  ti.initMqtt(config.mqtt_server, port, config.mqtt_server_username, config.mqtt_server_password, atoi(config.data_transmission_period));
  configFile.close();
}

void bindServerCallback()
{
}

void handlePreOtaUpdateCallback()
{
  Update.onProgress([](unsigned int progress, unsigned int total)
                    { d->log("OTA Progress: %u%%\r", (progress / (total / 100))); });
}

// /#REGION WifiManager ==================================


// Handles clicks on button
void handlerBtn(Button2 &btn)
{
  switch (btn.getType())
  {
  case single_click:

    // reset management at startup
    if (reset_possible)
    {
      reset_pending = true;
    }

    if (screensaver == false)
    {
      mode = (mode + 1) % 7; // cycle through 7 screens
    }
    resetTs = 0;
    offTs = millis();
    screensaver = false;
    break;
  case double_click:
    mode = GRAPH;
    resetTs = 0;
    offTs = millis();
    screensaver = false;
    break;
  case triple_click:
    break;
  case long_click:
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
      d->log("Réinitialisation en cours");
      // reset
      wm.resetSettings();
      // ESP.eraseConfig();
      LittleFS.remove(CONFIG_FILE);
      // display restart
      d->log("Redémarrage", 1000);
      // restart
      // ESP.reset();
      wm.reboot();
      delay(200);
    }
    break;
  case empty:
    break;
  }
}

void setup()
{
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // //-pinMode(PIN_OPTO, INPUT_PULLUP);
  data = new Data();
  d = new Display();
  data->init();
  d->init(data);

  snprintf(UNIQUE_ID, 30, "teleinfokit-%06X", ESP.getChipId());

  d->displayStartup(String(VERSION));
  d->logPercent("Démarrage", 5);

  unsigned long reset_start = millis();

  while (millis() - reset_start < 1000)
  {
    // if button is pressed, reset management
    if (!digitalRead(PIN_BUTTON))
    { // no use of click handler because not called yet in a loop
      d->displayReset();
      reset = RST_PAGE;
      unsigned long now = millis();
      while (millis() - now < RESET_CONFIRM_DELAY)
      {
        button.loop();
        delay(10);
      }
      delay(10);
    }
  }

  readConfig();
  ti.init(config.mode_tic_standard ? TINFO_MODE_STANDARD : TINFO_MODE_HISTORIQUE);

  // #REGION WifiManager ==================================

  wm.setAPCallback(configModeCallback);
  wm.setWebServerCallback(bindServerCallback);
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setSaveParamsCallback(saveParamCallback);
  wm.setPreOtaUpdateCallback(handlePreOtaUpdateCallback);

  custom_html = new WiFiManagerParameter("<p style=\"color:#375c72;font-size:22px;font-weight:Bold;\">Configuration TeleInfoKit</p>"); // only custom html
  custom_checkbox = new WiFiManagerParameter("mode_tic_std", "Mode TIC Standard", "T", 2, _customHtml_checkbox, WFM_LABEL_BEFORE);
  custom_mqtt_server = new WiFiManagerParameter("server", "<br />Serveur MQTT", mqtt_server, 40);
  custom_mqtt_port = new WiFiManagerParameter("port", "Port MQTT", mqtt_port, 6);
  custom_mqtt_username = new WiFiManagerParameter("username", "MQTT login", mqtt_server_username, 32);
  custom_mqtt_password = new WiFiManagerParameter("password", "MQTT mot de passe", mqtt_server_password, 32, "type=\"password\"");
  custom_data_transmission_period = new WiFiManagerParameter("data_transmission_period", "Délai entre envoi données (secondes) [Laisser vide pour temps réel]", data_transmission_period, 10);

  wm.addParameter(custom_html);
  wm.addParameter(custom_checkbox);
  wm.addParameter(custom_mqtt_server);
  wm.addParameter(custom_mqtt_port);
  wm.addParameter(custom_mqtt_username);
  wm.addParameter(custom_mqtt_password);
  wm.addParameter(custom_data_transmission_period);
  wm.setCustomHeadElement("<style>body{background-color: #F9FAFB; color: #375c72;} .msg.S {border-left-color: #477089;} button{color: #FFF; background-color: #81AECA;}</style>");
  wm.setTitle(F("Portail TeleInfoKit"));

  if (strcmp(config.mqtt_port, "") == 0)
  {
    custom_mqtt_port->setValue("1883", 4);
  }

  wm.setParamsPage(true);

  wm.setHostname(UNIQUE_ID);

  if (!WMISBLOCKING)
  {
    wm.setConfigPortalBlocking(false);
  }

  // show static ip fields
  // wm.setShowStaticFields(true);

  // This is sometimes necessary, it is still unknown when and why this is needed but it may solve some race condition or bug in esp SDK/lib
  wm.setCleanConnect(true); // disconnect before connect, clean connect

  wm.setBreakAfterConfig(true); // needed to use saveWifiCallback

  d->logPercent("Connexion au réseau wifi...", 35);

  // fetches ssid and pass and tries to connect
  // if it does not connect it starts an access point with the specified name
  // and goes into a blocking loop awaiting configuration
  wm.setConnectTimeout(45);
  if (!wm.autoConnect(AP_NAME, AP_PWD))
  {
    d->log("Connexion impossible\n Reset...");
    delay(1000);
    // reset and try again
    ESP.reset();
    delay(1000);
  }
  d->logPercent("Connecté à " + String(WiFi.SSID()), 40);

  // /#REGION WifiManager ==================================

// ================ OTA ================
  ArduinoOTA.setHostname(UNIQUE_ID);
  ArduinoOTA.setPassword("admin4tele9Info");
  d->logPercent("Démarrage OTA", 50);

  ArduinoOTA.onStart([]()
                     {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
    {
      type = "sketch";
    }
    else
    { // U_SPIFFS
      type = "filesystem";
    }

    d->log("Demarrage MAJ " + type); });
  ArduinoOTA.onEnd([]()
                   {
    d->log("Mise à jour complète");
    delay(500);
    d->log("Redémarrage en cours\nVeuillez patienter"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        {
    uint8_t percent = (progress / (total / 100));
    d->logPercent("Mise à jour en cours", percent); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
    d->log("OTA error " + (String) error);
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
    } });
  ArduinoOTA.begin();

  pinMode(PIN_BUTTON, INPUT_PULLUP);

  button.setClickHandler(handlerBtn);
  button.setDoubleClickHandler(handlerBtn);
  button.setLongClickHandler(handlerBtn);

  uint16_t port = 1883;
  if (config.mqtt_port[0] != '\0')
  {
    port = atoi(config.mqtt_port);
    delay(1000);
  }

  ti.initMqtt(config.mqtt_server, port, config.mqtt_server_username, config.mqtt_server_password, atoi(config.data_transmission_period));

  d->logPercent("Obtention de l'heure", 60);
  d->getTime();

  d->logPercent("Connexion MQTT", 70);
  if (!ti.LogStartup())
  {
    d->log("Erreur config MQTT \nRéinitialiser les réglages", 2000);
  }

  d->logPercent("Envoi MQTT Discovery", 80);

  ti.sendMqttDiscovery();

  d->logPercent("Activation portail config", 90);

  wm.startWebPortal();
  d->logPercent("Démarrage terminé", 100);

  offTs = millis();
  ti.loop();
}

void loop()
{

  ArduinoOTA.handle();

  button.loop();
  wm.process();

  // for cancelling reset settings requests automatically
  if (resetTs != 0 && (millis() - resetTs > RESET_CONFIRM_DELAY))
  {
    resetTs = 0;
    d->displayReset();
  }

  if (millis() - offTs > SCREENSAVER_DELAY)
  {
    screensaver = true;
    d->displayOff();
  }

  if ((millis() - refreshTime > REFRESH_DELAY) || (mode == TIME && millis() - refreshTime > 1000))
  {
    data->storeValueBase(ti.index);

    if (!screensaver)
    {
      switch (mode)
      {
      case GRAPH:
        reset = IDLE;
        // d->drawGraph(ti.papp);
        d->drawGraph(ti.papp, config.mode_tic_standard ? 'S' : 'H'); // to remove and use commented code
        break;
      case DATA1:
        reset = IDLE;
        d->displayData1(ti.papp, ti.iinst);
        break;
      case DATA2:
        reset = IDLE;
        d->displayData2(ti.index, ti.adresseCompteur);
        break;
      case NETWORK:
        reset = IDLE;
        d->displayNetwork();
        break;
      case TIME:
        reset = IDLE;
        d->getTime();
        d->displayTime();
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
        if (millis() - offTs > SCREEN_OFF_MESSAGE_DELAY)
        {
          d->displayOff();
        }
        else
        {
          d->log("Ecran OFF dans 5s.\nAppui court pour rallumer.", 0);
        }
        break;
      }
    }

    refreshTime = millis();
  }

  ti.loop();

  // update time every 5 minutes
  if (millis() - mtime > 5 * 60 * 1000)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      d->getTime();
    }
    mtime = millis();
  }
}
