#ifndef ESPTELEINFO_H
#define ESPTELEINFO_H

#define LINE_MAX_COUNT 50
#define DATA_MAX_SIZE 200
#define NBTRY 5

#include <ESP8266WiFi.h>
#include <LibTeleinfo.h>
#include <PubSubClient.h>
#include "version.h"

// Linked list structure containing all values received
typedef struct _UnsentValueList UnsentValueList;
struct _UnsentValueList
{
    UnsentValueList *next; // next element
    char *name;            // LABEL of value name
    char *value;           // value
};

class ESPTeleInfo
{

public:
    ESPTeleInfo();

    void init(_Mode_e tic_mode);
    void initMqtt(char *server, uint16_t port, char *username, char *password, int period_data);
    void loop(void);

    // les données de consommation
    long iinst;               // HIST: IINST  STD IRMS1
    long papp;                // HIST: PAPP   STD SINSTS
    long index;               // HIST: BASE + HCHC + HCHP   STD EAST
    char adresseCompteur[20]; // HIST: ADCO   STD ADSC
    char strDataTopic[50];
    char strDiscoveryTopic[75];
    long ts_analyzeData;
    long ts_startup;
    char analyzeBuffer[20];

    // _Mode_e tic_mode = TINFO_MODE_STANDARD;
    TInfo tic;

    bool LogStartup();
    void SetData(char *name, char *val);
    // 100 char max !
    void Log(String s);

    void sendMqttDiscovery();
    void AnalyzeTicForInternalData();
    _Mode_e ticMode;

private:
    long previousMillis;
    char logBuffer[100];
    char mqtt_user[32];
    char mqtt_pwd[32];

    char *_adc0_ = (char *)"ADCO";
    char *_adsc_ = (char *)"ADSC";
    char *_irms1_ = (char *)"IRMS1";
    char *_sinsts_ = (char *)"SINSTS";
    char *_east_ = (char *)"EAST";
    char *_iinst_ = (char *)"IINST";
    char *_papp_ = (char *)"PAPP";
    char *_base_ = (char *)"BASE";
    char *_hchc_ = (char *)"HCHC";
    char *_hchp_ = (char *)"HCHP";

    // to store 3 for hist mode (BASE, HP, HC)
    long indexes[3];

    unsigned int delay_generic;
    bool sendGeneric;
    bool started;

    // timestamp for the last power data send
    unsigned long ts_power;
    // timestamp for the last index data send
    unsigned long ts_index;
    // timestamp for the last generic data send
    unsigned long ts_generic;

    char CHIP_ID[7] = {0};
    char UNIQUE_ID[30];
    char bufLabel[10];
    char bufLogTopic[35];
    char bufDataTopic[35];

    void SendAllData();
    void SendAllUnsentData();
    void SendData(char *label, char *value);
    bool sendGenericData();

    void sendMqttDiscoveryIndex(String label, String friendlyName);
    void sendMqttDiscoveryText(String label, String friendlyName);
    void sendMqttDiscoveryForType(String label, String friendlyName, String deviceClass, String unit, String icon);

    char payloadDiscovery[500];

    bool connectMqtt();

    String discoveryDevice;

    UnsentValueList *unsentList = nullptr;
    void freeList(UnsentValueList *&head);
    void addOrReplaceValueInList(UnsentValueList *&head, const char *name, const char *newValue);
};

#endif /* ESPTELEINFO_H */