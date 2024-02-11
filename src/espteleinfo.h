#ifndef ESPTELEINFO_H
#define ESPTELEINFO_H

#define LINE_MAX_COUNT 50
#define DATA_MAX_SIZE 200


#include <ESP8266WiFi.h>
#include <LibTeleinfo.h>
#include <PubSubClient.h>
#include "version.h"

class ESPTeleInfo
{


public:

    ESPTeleInfo();

    void init();
    void initMqtt(char *server, uint16_t port, char *username, char *password, int period_data_power, int period_data_index);
    void loop(void);

    // les données de consommation
    long iinst, iinst_old;
    long iinst1, iinst1_old;
    long iinst2, iinst2_old;
    long iinst3, iinst3_old;
    long papp, papp_old;
    long hc, hc_old;
    long hp, hp_old;
    long base, base_old;
    long imax, imax_old;
    long isousc, isousc_old;
    char adc0[20];
    char ptec[20];
    char ptec_old[20];
    char strDataTopic[50];

    char values_old[LINE_MAX_COUNT][DATA_MAX_SIZE+1]; //+1 for '\0' ending

    bool modeBase;
    bool modeTriphase;

    _Mode_e tic_mode = TINFO_MODE_STANDARD;
    TInfo tic;
   


    bool LogStartup();
    // 30 char max !
    void Log(String s);
    void SendData(char* label, char* value);



private:
    long previousMillis;
    bool staticInfoSsent;
    char buffer[30];
    char mqtt_user[32];
    char mqtt_pwd[32];

    unsigned int delay_power;
    unsigned int delay_index;
    unsigned int delay_generic;
    bool sendPower;
    bool sendIndex;
    bool sendGeneric;

    // timestamp for the last power data send
    unsigned long ts_power;
    // timestamp for the last index data send
    unsigned long ts_index;
    // timestamp for the last generic data send
    unsigned long ts_generic;

    char CHIP_ID[7] = {0};
    char UNIQUE_ID [30];

    bool sendPowerData();
    bool sendIndexData();
    bool sendGenericData();

    // detects the communication mode (mono or triphase)
    void getPhaseMode();
    bool connectMqtt();
};

#endif /* ESPTELEINFO_H */