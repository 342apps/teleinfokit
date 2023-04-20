#include "espteleinfo.h"

#define INTERVAL 3000 // 3 sec delay between publishing

#define IINST "IINST"
#define PAPP "PAPP"
#define HC "HCHC"
#define HP "HCHP"
#define ADCO "ADCO"
#define OPTARIF "OPTARIF"
#define ISOUSC "ISOUSC"
#define IMAX "IMAX"
#define PTEC "PTEC"
#define BASE "BASE"
// mode triphase
#define IINST1 "IINST1"
#define IINST2 "IINST2"
#define IINST3 "IINST3"

#define NBTRY 5

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

TeleInfo teleinfo(&Serial);

ESPTeleInfo::ESPTeleInfo()
{
    mqtt_user[0] = '\0';
    mqtt_pwd[0] = '\0';
}

void ESPTeleInfo::init()
{
    staticInfoSsent = false;
    previousMillis = millis();
    iinst = 0;
    iinst_old = 254;
    iinst1 = 0;
    iinst1_old = 254;
    iinst2 = 0;
    iinst2_old = 254;
    iinst3 = 0;
    iinst3_old = 254;
    papp = 0;
    papp_old = 1;
    hc = 0;
    hc_old = 1;
    hp = 0;
    hp_old = 1;
    base = 0;
    base_old = 1;
    imax = 0;
    imax_old = 1;
    isousc = 0;
    isousc_old = 1;
    adc0[0] = '\0';
    ptec[0] = '\0';
    ptec_old[0] = '_';
    modeBase = false;
    modeTriphase = false;

    for(int i=0; i< LINE_MAX_COUNT; i++){
        values_old[i][0] = '\0';
    }

    Serial.begin(1200, SERIAL_8N1);
    // Init teleinfo
    teleinfo.begin();


    sprintf(CHIP_ID, "%06X", ESP.getChipId());


}

void ESPTeleInfo::initMqtt(char *server, uint16_t port, char *username, char *password, int period_data_power, int period_data_index)
{
    strcpy(mqtt_user, username);
    strcpy(mqtt_pwd, password);

    delay_index = period_data_index * 1000;
    delay_power = period_data_power * 1000;

    // we use the power delay for generic data
    delay_generic = delay_power;

    mqttClient.setServer(server, port);
}

bool ESPTeleInfo::connectMqtt()
{
    if (mqtt_user[0] == '\0')
    {
        return mqttClient.connect(CHIP_ID);
    }
    else
    {
        return mqttClient.connect(CHIP_ID, mqtt_user, mqtt_pwd);
    }
}

void ESPTeleInfo::loop(void)
{
    teleinfo.process();
    
    if (teleinfo.available())
    {
        sendPower = sendPowerData();
        sendIndex = sendIndexData();
        sendGeneric = sendGenericData();
        
        iinst = teleinfo.getLongVal(IINST);
        iinst1 = teleinfo.getLongVal(IINST1);
        iinst2 = teleinfo.getLongVal(IINST2);
        iinst3 = teleinfo.getLongVal(IINST3);

        iinst = iinst == -1 ? 0 : iinst;
        iinst1 = iinst1 == -1 ? 0 : iinst1;
        iinst2 = iinst2 == -1 ? 0 : iinst2;
        iinst3 = iinst3 == -1 ? 0 : iinst3;

        papp = teleinfo.getLongVal(PAPP);

        hc = teleinfo.getLongVal(HC);
        hp = teleinfo.getLongVal(HP);
        base = teleinfo.getLongVal(BASE);

        if(base > 0){
            modeBase  = true;
        }
        getPhaseMode();

        imax = teleinfo.getLongVal(IMAX);
        strncpy(ptec, teleinfo.getStringVal(PTEC), 20);

        if (connectMqtt())
        {
            // send all data in the data topic
            if(sendGenericData()){
                for(uint8_t i= 0; i<teleinfo.dataCount; i++){
                    if(strncmp(values_old[i], teleinfo.values[i], DATA_MAX_SIZE + 1) != 0){
                        sprintf(strDataTopic, "ticrack/data/%s", teleinfo.labels[i]);
                        mqttClient.publish(strDataTopic, teleinfo.values[i], true);
                        snprintf(values_old[i], DATA_MAX_SIZE+1, "%s", teleinfo.values[i]);
                    }
                }
                ts_generic = millis();
            }

            // send specific data - backwards compatibility
            if (!modeTriphase && iinst != iinst_old && sendPower)
            {
                mqttClient.publish("ticrack/iinst", teleinfo.getStringVal(IINST));
            }

            if (modeTriphase && sendPower){
                // mode triphasé only : intensités des 3 phases
                if (iinst1 != iinst1_old)
                {
                    mqttClient.publish("ticrack/iinst1", teleinfo.getStringVal(IINST1));
                }
                if (iinst2 != iinst2_old)
                {
                    mqttClient.publish("ticrack/iinst2", teleinfo.getStringVal(IINST2));
                }
                if (iinst3 != iinst3_old)
                {
                    mqttClient.publish("ticrack/iinst3", teleinfo.getStringVal(IINST3));
                }
            }

            if (papp != papp_old && sendPower)
            {
                mqttClient.publish("ticrack/papp", teleinfo.getStringVal(PAPP));
            }
            if (hc != hc_old && hc != 0 && sendIndex)
            {
                mqttClient.publish("ticrack/hc", teleinfo.getStringVal(HC), true);
            }
            if (hp != hp_old && hp != 0 && sendIndex)
            {
                mqttClient.publish("ticrack/hp", teleinfo.getStringVal(HP), true);
            }
            if (base != base_old && base != 0 && sendIndex)
            {
                mqttClient.publish("ticrack/base", teleinfo.getStringVal(BASE), true);
            }
            if (imax != imax_old)
            {
                mqttClient.publish("ticrack/imax", teleinfo.getStringVal(IMAX));
            }
            if (strcmp(ptec, ptec_old) != 0)
            {
                mqttClient.publish("ticrack/ptec", teleinfo.getStringVal(PTEC));
            }
        }

        if(sendPower)
        {
            iinst_old = iinst;
            iinst1_old = iinst1;
            iinst2_old = iinst2;
            iinst3_old = iinst3;
            papp_old = papp;
            ts_power = millis();
        }

        if(sendIndex){
            hc_old = hc;
            hp_old = hp;
            base_old = base;
            ts_index = millis();
        }

        imax_old = imax;
        strncpy(ptec_old, ptec, 20);

        if (!staticInfoSsent && connectMqtt())
        {
            if (teleinfo.getStringVal(ADCO)[0] != '\n')
            {
                strncpy(adc0, teleinfo.getStringVal(ADCO), 20);
                mqttClient.publish("ticrack/adc0", teleinfo.getStringVal(ADCO), true);
            }
            if (teleinfo.getStringVal(ISOUSC)[0] != '\n')
            {
                isousc = teleinfo.getLongVal(ISOUSC);
                mqttClient.publish("ticrack/isousc", teleinfo.getStringVal(ISOUSC), true);
            }

            staticInfoSsent = true;
        }

        teleinfo.resetAvailable();
    }
}

void ESPTeleInfo::getPhaseMode(){
    modeTriphase = !(
        iinst > 0 && 
        iinst1 <= 0 && 
        iinst2 <= 0 && 
        iinst3 <= 0);
}

bool ESPTeleInfo::LogStartup()
{
    int8_t nbTry = 0;
    while (nbTry < NBTRY && !connectMqtt())
    {
        delay(250);
        nbTry++;
    }
    if (nbTry < NBTRY)
    {
        char str[80];
        mqttClient.publish("ticrack/log", "Startup");
        strcpy (str,"Version: ");
        strcat (str, VERSION);
        mqttClient.publish("ticrack/log", str);
    #ifdef _HW_VER
        sprintf(str, "HW Version: %d", _HW_VER);
        mqttClient.publish("ticrack/log", str);
    #endif
        strcpy (str,"IP: ");
        strcat (str, WiFi.localIP().toString().c_str());
        mqttClient.publish("ticrack/log", str);
        strcpy (str,"MAC: ");
        strcat (str, WiFi.macAddress().c_str());
        mqttClient.publish("ticrack/log", str);
        return true;
    }
    else
    {
        return false;
    }
}

bool ESPTeleInfo::sendPowerData()
{
    return (delay_power <= 0 ) || (millis() - ts_power > (delay_power));
}

bool ESPTeleInfo::sendIndexData()
{
    return (delay_index <= 0 ) || (millis() - ts_index > (delay_index));
}

bool ESPTeleInfo::sendGenericData()
{
    return (delay_generic <= 0 ) || (millis() - ts_generic > (delay_generic));
}

// 30 char max !
void ESPTeleInfo::Log(String s)
{
    int8_t nbTry = 0;
    while (nbTry < NBTRY && !connectMqtt())
    {
        delay(250);
        nbTry++;
    }
    if (nbTry < NBTRY)
    {
        s.toCharArray(buffer, 30);
        mqttClient.publish("ticrack/log", buffer);
    }
}
