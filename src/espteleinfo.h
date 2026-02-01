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

    void init(_Mode_e tic_mode, bool triphase);
    void initMqtt(char *server, uint16_t port, char *username, char *password, int period_data);
    void loop(void);

    // les données de consommation
    long iinst;               // HIST: IINST  STD IRMS1
    long papp;                // HIST: PAPP   STD SINSTS
    long index;               // énergie cumulée totale (tous tarifs)
    char adresseCompteur[20]; // HIST: ADCO   STD ADSC
    char strDataTopic[50];
    char strDiscoveryTopic[128];
    long ts_analyzeData;
    long ts_startup;
    char analyzeBuffer[20];

    TInfo tic;

    bool LogStartup();
    void SetData(char *name, char *val);
    void Log(String s);

    void sendMqttDiscovery();
    void AnalyzeTicForInternalData();
    _Mode_e ticMode;
    bool triphase;

private:
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

    /*** MODIF TEMPO : labels Tempo ***/
    char *_bbrhcjb_ = (char *)"BBRHCJB";
    char *_bbrhpjb_ = (char *)"BBRHPJB";
    char *_bbrhcjw_ = (char *)"BBRHCJW";
    char *_bbrhpjw_ = (char *)"BBRHPJW";
    char *_bbrhcjr_ = (char *)"BBRHCJR";
    char *_bbrhpjr_ = (char *)"BBRHPJR";

    // to store 3 for hist mode (BASE, HC, HP)
    long indexes[3];

    /*** MODIF TEMPO : stockage index Tempo ***/
    uint64_t idx_bbrhcjb = 0;
    uint64_t idx_bbrhpjb = 0;
    uint64_t idx_bbrhcjw = 0;
    uint64_t idx_bbrhpjw = 0;
    uint64_t idx_bbrhcjr = 0;
    uint64_t idx_bbrhpjr = 0;

    unsigned int delay_generic;
    bool sendGeneric;
    bool started;

    unsigned long ts_power;
    unsigned long ts_index;
    unsigned long ts_generic;

    char CHIP_ID[7] = {0};
    char UNIQUE_ID[30];
    char bufLabel[12];
    char bufLogTopic[35];
    char bufDataTopic[35];

    void SendAllData();
    void SendAllUnsentData();
    void SendData(char *label, char *value);
    bool sendGenericData();

    void clearAllDiscovery();
    void deleteMqttDiscovery(String label);
    void sendMqttDiscoveryIndex(String label, String friendlyName);
    void sendMqttDiscoveryText(String label, String friendlyName);
    void sendMqttDiscoveryForType(String label, String friendlyName, String deviceClass, String unit, String icon);

    char payloadDiscovery[500];

    bool connectMqtt();
    String sanitizeLabel(String input);

    String discoveryDevice;

    UnsentValueList *unsentList = nullptr;
    void freeList(UnsentValueList *&head);
    void addOrReplaceValueInList(UnsentValueList *&head, const char *name, const char *newValue);
};

#endif /* ESPTELEINFO_H */
