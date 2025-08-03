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
#include "randomKeyGenerator.h"
#include "version.h"

#define PIN_OPTO 3
#define PIN_BUTTON 1
#define CONFIG_V200_FILE "/config.dat"
#define CONFIG_FILE "/ext_config.dat"   // configuration file extended
#define RESET_CONFIRM_DELAY 10000
#define SCREEN_OFF_MESSAGE_DELAY 5000
#define SCREENSAVER_DELAY 60000
#define AP_NAME "TeleInfoKit"
// #define AP_PWD "givememydata"

#define REFRESH_DELAY 1000

// Existing previous versions
#define V200 "v2.0.0.312588"

// The structure that stores configuration of version 2.0.0
typedef struct
{
  bool mode_tic_standard;
  char mqtt_server[40];
  char mqtt_port[6];
  char mqtt_server_username[32];
  char mqtt_server_password[32];
  char data_transmission_period[10];
} ConfStruct_V200;

// The generic structure that stores configuration of versions > 2.0.0
typedef struct
{
  bool mode_tic_standard;
  char mqtt_server[40];
  char mqtt_port[6];
  char mqtt_server_username[32];
  char mqtt_server_password[32];
  char data_transmission_period[10];

  // new fields in V2.1
  bool mode_triphase;
  char version[20];

  // generic fields for future versions
  bool bool_conf[10];
  char string_conf[30][40];
  int int_conf[30];
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

bool test_mode = false;

Data *data;
Display *d;
ESPTeleInfo ti = ESPTeleInfo();
Button2 button = Button2(PIN_BUTTON);
RandomKeyGenerator *randKey;

time_t now;
String getParam(String name);
void handlerBtn(Button2 &btn);

void initButton()
{
  button = Button2(PIN_BUTTON);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  button.setClickHandler(handlerBtn);
  button.setDoubleClickHandler(handlerBtn);
  button.setLongClickHandler(handlerBtn);
}

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
// char mode_tic_std_char[1];
// char mode_triphase_char[1];
char mqtt_server[40];
char mqtt_port[6];
char mqtt_server_username[32];
char mqtt_server_password[32];
char data_transmission_period[10];
char UNIQUE_ID[30];

char _customHtml_checkbox_mode_tic[200] = "";
char _customHtml_checkbox_triphase[250] = "";

#define HTML_RADIO_TIC_STD "<label for='mode_tic'><b>Mode TIC</b></label><br /><input type='radio' name='mode_tic' value='H' /> Historique<br><input type='radio' name='mode_tic' value='S' checked /> Standard<br>"
#define HTML_RADIO_TIC_HIST "<label for='mode_tic'><b>Mode TIC</b></label><br /><input type='radio' name='mode_tic' value='H' checked /> Historique<br><input type='radio' name='mode_tic' value='S' /> Standard<br>"

#define HTML_RADIO_MONOPHASE "<br /><br /><label for='mode_triphase'><b>Type compteur</b></label><br /><input type='radio' name='mode_triphase' value='M' checked /> Simple phase<br><input type='radio' name='mode_triphase' value='T' /> Triphasé<br><br /><hr />"
#define HTML_RADIO_TRIPHASE "<br /><br /><label for='mode_triphase'><b>Type compteur</b></label><br /><input type='radio' name='mode_triphase' value='M' /> Simple phase<br><input type='radio' name='mode_triphase' value='T' checked /> Triphasé<br><br /><hr />"

WiFiManagerParameter *checkbox_mode_tic;
WiFiManagerParameter *checkbox_triphase;
WiFiManagerParameter *custom_html;
WiFiManagerParameter *custom_mqtt_server;
WiFiManagerParameter *custom_mqtt_port;
WiFiManagerParameter *custom_mqtt_username;
WiFiManagerParameter *custom_mqtt_password;
WiFiManagerParameter *custom_data_transmission_period;
WiFiManagerParameter *custom_link;
WiFiManagerParameter *custom_version;

// Network connection has been done through captive portal of hotspot or through config webportal
void saveConfigCallback()
{
  d->log("Configuration sauvée", 500);
}

// gets called when WiFiManager enters configuration mode
void configModeCallback(WiFiManager *myWiFiManager)
{
  d->log("Hotspot Wifi: " + myWiFiManager->getConfigPortalSSID() + "\nClé : " + String(randKey->apPwd));
}

File isVersion200OrLess()
{
  // get the config file if exists and get its size
  File configFile = LittleFS.open(CONFIG_V200_FILE, "r");
  if (configFile)
  {
    size_t fileSize = configFile.size();
    if (fileSize <= sizeof(ConfStruct_V200))
    {
      return configFile;
    }
  }
  return File();
}

File currentVersionExists()
{
  // get the config file if exists and get its size
  File configFile = LittleFS.open(CONFIG_FILE, "r");
  return configFile;
}

// Retreives configuration from filesystem
void readConfig()
{

  if (LittleFS.begin())
  {
    if (LittleFS.exists(CONFIG_FILE) || LittleFS.exists(CONFIG_V200_FILE))
    {
      // file exists, reading and loading
      d->logPercent("Lecture configuration", 10);

      File configFile = currentVersionExists();
      File configFileV200 = isVersion200OrLess();
      if (configFile)
      {
        configFile.read((byte *)&config, sizeof(config));
        configFile.close();
      }
      else if (configFileV200)
      {
        d->logPercent("Migration config v2.0", 12);
        delay(400);

        // assume the file is from version 2.0.0
        ConfStruct_V200 configV200;
        configFileV200.read((byte *)&configV200, sizeof(configV200));
        configFileV200.close();

        // copy the values from the old struct to the new struct
        config.mode_tic_standard = configV200.mode_tic_standard;
        strcpy(config.mqtt_server, configV200.mqtt_server);
        strcpy(config.mqtt_port, configV200.mqtt_port);
        strcpy(config.mqtt_server_username, configV200.mqtt_server_username);
        strcpy(config.mqtt_server_password, configV200.mqtt_server_password);
        strcpy(config.data_transmission_period, configV200.data_transmission_period);
        config.mode_triphase = false; // set the new field to false by default
        strcpy(config.version, V200); // set the version to the default v2.0.0
      }

      // mode standard
      if (config.mode_tic_standard)
      {
        strcpy(_customHtml_checkbox_mode_tic, HTML_RADIO_TIC_STD);
      }
      else
      {
        strcpy(_customHtml_checkbox_mode_tic, HTML_RADIO_TIC_HIST);
      }

      // mode triphasé
      if (config.mode_triphase)
      {
        strcpy(_customHtml_checkbox_triphase, HTML_RADIO_TRIPHASE);
      }
      else
      {
        strcpy(_customHtml_checkbox_triphase, HTML_RADIO_MONOPHASE);
      }

      strcpy(mqtt_server, config.mqtt_server);
      strcpy(mqtt_port, config.mqtt_port);
      strcpy(mqtt_server_username, config.mqtt_server_username);
      strcpy(mqtt_server_password, config.mqtt_server_password);
      strcpy(data_transmission_period, config.data_transmission_period);

      d->logPercent("Configuration chargée", 15);
    }
    else
    {
      {
        d->log("Aucun fichier de configuration");
      }
    }
  }
  else
  {
    d->log("Erreur montage filesystem", 1000);
  }
}

void saveParamCallback()
{
  d->log("Sauvegarde configuration", 200);
  shouldSaveConfig = true;

  // strcpy(mode_tic_std_char, checkbox_mode_tic->getValue());
  // strcpy(mode_triphase_char, checkbox_triphase->getValue());
  strcpy(mqtt_server, custom_mqtt_server->getValue());
  strcpy(mqtt_port, custom_mqtt_port->getValue());
  strcpy(mqtt_server_username, custom_mqtt_username->getValue());
  strcpy(mqtt_server_password, custom_mqtt_password->getValue());
  strcpy(data_transmission_period, custom_data_transmission_period->getValue());

  File configFile = LittleFS.open(CONFIG_FILE, "w");
  if (!configFile)
  {
    d->log("Erreur écriture config", 1000);
  }
  else
  {
    config.mode_triphase = getParam("mode_triphase") == "T";
    bool std = getParam("mode_tic") == "S";
    if (std != config.mode_tic_standard)
    {
      ti.init(config.mode_tic_standard ? TINFO_MODE_STANDARD : TINFO_MODE_HISTORIQUE, config.mode_triphase);
      initButton();
    }
    else{
      ti.triphase = config.mode_triphase;
    }
    config.mode_tic_standard = std;


    strcpy(config.mqtt_server, custom_mqtt_server->getValue());
    strcpy(config.mqtt_port, custom_mqtt_port->getValue());
    strcpy(config.mqtt_server_username, custom_mqtt_username->getValue());
    strcpy(config.mqtt_server_password, custom_mqtt_password->getValue());
    strcpy(config.data_transmission_period, custom_data_transmission_period->getValue());
    strcpy(config.version, VERSION);
    configFile.write((byte *)&config, sizeof(config));
    configFile.close();

    if (config.mode_tic_standard)
    {
      strcpy(_customHtml_checkbox_mode_tic, HTML_RADIO_TIC_STD);
    }
    else
    {
      strcpy(_customHtml_checkbox_mode_tic, HTML_RADIO_TIC_HIST);
    }
    //checkbox_mode_tic = new WiFiManagerParameter("mode_tic_std", "Mode TIC Standard", "T", 2, _customHtml_checkbox_mode_tic, WFM_LABEL_BEFORE);

    
    if (config.mode_triphase)
    {
      strcpy(_customHtml_checkbox_triphase, HTML_RADIO_TRIPHASE);
    }
    else
    {
      strcpy(_customHtml_checkbox_triphase, HTML_RADIO_MONOPHASE);
    }
    //checkbox_triphase = new WiFiManagerParameter("mode_triphase", "<br />Mode Triphasé", "T", 2, _customHtml_checkbox_triphase, WFM_LABEL_BEFORE);

    
    d->log("Configuration sauvée", 500);
    d->drawGraph(ti.papp, config.mode_tic_standard ? 'S' : 'H');
  }
  
  uint16_t port = 1883;
  if (config.mqtt_port[0] != '\0')
  {
    port = atoi(config.mqtt_port);
    delay(1000);
  }
  ti.init(config.mode_tic_standard ? TINFO_MODE_STANDARD : TINFO_MODE_HISTORIQUE, config.mode_triphase);
  ti.initMqtt(config.mqtt_server, port, config.mqtt_server_username, config.mqtt_server_password, atoi(config.data_transmission_period));
  ti.sendMqttDiscovery();
  initButton();
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
  if (test_mode)
  {
    switch (btn.getType())
    {
    case single_click:
    case double_click:
    case triple_click:
    case long_click:
      if (ti.ticMode == TINFO_MODE_HISTORIQUE)
      {
        // go mode standard
        config.mode_tic_standard = true;
        ti.init(TINFO_MODE_STANDARD, config.mode_triphase);
        initButton();
      }
      else
      {
        // go mode historique
        config.mode_tic_standard = false;
        ti.init(TINFO_MODE_HISTORIQUE, config.mode_triphase);
        initButton();
      }
      break;
    case empty:
      break;
    }
    resetTs = 0;
    offTs = millis();
    screensaver = false;
  }
  else
  { // normal operation, no test mode
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
}

void setup()
{
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // //-pinMode(PIN_OPTO, INPUT_PULLUP);
  data = new Data();
  randKey = new RandomKeyGenerator();
  d = new Display();
  data->init();
  d->init(data);

  snprintf(UNIQUE_ID, 30, "teleinfokit-%06X", ESP.getChipId());

  d->displayStartup(String(VERSION));

  unsigned long reset_start = millis();

  while (millis() - reset_start < 500)
  {
    // if button is pressed, display TIC
    if (!digitalRead(PIN_BUTTON))
    { // no use of click handler because not called yet in a loop
      d->displayReset(randKey->apPwd);

      test_mode = true;
      d->displayTestTic("START", "START", 'X');
    }
  }

  d->logPercent("Démarrage", 5);

  while (!test_mode && millis() - reset_start < 1500)
  {
    // if button is pressed, reset management
    if (!digitalRead(PIN_BUTTON))
    { // no use of click handler because not called yet in a loop
      d->displayReset(randKey->apPwd);
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
  ti.init(config.mode_tic_standard ? TINFO_MODE_STANDARD : TINFO_MODE_HISTORIQUE, config.mode_triphase);

  wm.setPreOtaUpdateCallback(handlePreOtaUpdateCallback);

  // #REGION WifiManager ==================================
  wm.setAPCallback(configModeCallback);
  wm.setWebServerCallback(bindServerCallback);
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setSaveParamsCallback(saveParamCallback);

  custom_html = new WiFiManagerParameter("<p style=\"color:#375c72;font-size:22px;font-weight:Bold;\">Configuration TeleInfoKit</p>"); // only custom html
  checkbox_mode_tic = new WiFiManagerParameter(_customHtml_checkbox_mode_tic);
  checkbox_triphase = new WiFiManagerParameter(_customHtml_checkbox_triphase);

  custom_mqtt_server = new WiFiManagerParameter("server", "<br /><br />Serveur MQTT (taille max 40)", mqtt_server, 40);
  custom_mqtt_port = new WiFiManagerParameter("port", "Port MQTT", mqtt_port, 6);
  custom_mqtt_username = new WiFiManagerParameter("username", "MQTT login (taille max 32)", mqtt_server_username, 32);
  custom_mqtt_password = new WiFiManagerParameter("password", "MQTT mot de passe (taille max 32)", mqtt_server_password, 32, "type=\"password\"");
  custom_data_transmission_period = new WiFiManagerParameter("data_transmission_period", "Délai entre envoi données (secondes) [Laisser vide pour temps réel]", data_transmission_period, 10);
  custom_link = new WiFiManagerParameter("<p><a href='https://342apps.net/teleinfokit'>Documentation Teleinfokit</a></p>");
  custom_version = new WiFiManagerParameter(VERSION);


  wm.addParameter(custom_html);
  wm.addParameter(checkbox_mode_tic);
  wm.addParameter(checkbox_triphase);
  wm.addParameter(custom_mqtt_server);
  wm.addParameter(custom_mqtt_port);
  wm.addParameter(custom_mqtt_username);
  wm.addParameter(custom_mqtt_password);
  wm.addParameter(custom_data_transmission_period);
  wm.addParameter(custom_link);
  wm.addParameter(custom_version);
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

  if (!test_mode)
  {
    d->logPercent("Connexion au réseau wifi...", 35);

    // fetches ssid and pass and tries to connect
    // if it does not connect it starts an access point with the specified name
    // and goes into a blocking loop awaiting configuration
    wm.setConnectTimeout(45);

    // the AP password is random and specific to each device, but will be always the same for a device
    if (!wm.autoConnect(AP_NAME, randKey->apPwd))
    {
      d->log("Connexion impossible\n Reset...");
      delay(1000);
      // reset and try again
      ESP.reset();
      delay(1000);
    }
    d->logPercent("Connecté à " + String(WiFi.SSID()), 40);

    // /#REGION WifiManager ==================================

  } // end if !test_mode

  // ================ OTA ================
  ArduinoOTA.setHostname(UNIQUE_ID);
  ArduinoOTA.setPassword(randKey->apPwd);
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

  initButton();

  if (!test_mode)
  {
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

  } // end if !test_mode
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

  if (!test_mode)
  {
    // for cancelling reset settings requests automatically
    if (resetTs != 0 && (millis() - resetTs > RESET_CONFIRM_DELAY))
    {
      resetTs = 0;
      d->displayReset(randKey->apPwd);
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
          d->drawGraph(ti.papp, config.mode_tic_standard ? 'S' : 'H');
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
            d->displayReset(randKey->apPwd);
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
  }
  else
  {
    // ==== test mode
    if (millis() - refreshTime > REFRESH_DELAY)
    {

      d->displayTestTic(String(ti.papp), String(ti.index), ti.ticMode == TINFO_MODE_STANDARD ? 'S' : 'H');
      refreshTime = millis();

      if (millis() - offTs > 60000)
      {
        // restart auto
        d->log("Sortie du mode TEST", 1000);
        wm.reboot();
      }
    }
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

String getParam(String name){
  //read parameter from server, for customhmtl input
  String value;
  if(wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  return value;
}