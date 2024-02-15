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

TInfo teleinfo;

static ESPTeleInfo *instanceEsp;

static ESPTeleInfo *getESPTeleInfo() noexcept
{ // pour obtenir le singleton
    return instanceEsp;
}

ESPTeleInfo::ESPTeleInfo()
{
    mqtt_user[0] = '\0';
    mqtt_pwd[0] = '\0';
    ts_analyzeData = 0;
}

static void DataCallback(ValueList *me, uint8_t flags)
{

    //   if (flags & TINFO_FLAGS_ADDED) {
    //     getESPTeleInfo()->Log("New " + String(me->name) + " - " + String(me->value));
    //   }

    //   if (flags & TINFO_FLAGS_UPDATED){
    //     getESPTeleInfo()->Log("Maj " + String(me->name) + " - " + String(me->value));
    //   }

    getESPTeleInfo()->SetData(me->name, me->value);
    // while(me->next){
    //     me = me->next;
    // }

    // Display values

    //   Serial.print(me->name);
    //   Serial.print("=");
    //   Serial.println(me->value);
}

void ESPTeleInfo::init(_Mode_e tic_mode)
{
    instanceEsp = this;
    ticMode = tic_mode;
    previousMillis = millis();
    iinst = 0;
    papp = 0;
    index = 0;
    compteur[0] = '\0';

    for (int i = 0; i < LINE_MAX_COUNT; i++)
    {
        values_old[i][0] = '\0';
    }

    // sprintf(CHIP_ID, "%06X", ESP.getChipId());
    snprintf(UNIQUE_ID, 30, "teleinfokit-%06X", ESP.getChipId());

    Serial.flush();
    Serial.end();

    Serial.begin(tic_mode == TINFO_MODE_HISTORIQUE ? 1200 : 9600, SERIAL_7E1);
    // Init teleinfo
    teleinfo.init(tic_mode);
    teleinfo.attachData(DataCallback);

    if (Serial.available())
    {
        teleinfo.process(Serial.read());
    }
}

void ESPTeleInfo::initMqtt(char *server, uint16_t port, char *username, char *password, int period_data)
{
    strcpy(mqtt_user, username);
    strcpy(mqtt_pwd, password);

    // we use the power delay for generic data
    delay_generic = period_data;

    mqttClient.setServer(server, port);
}

bool ESPTeleInfo::connectMqtt()
{
    if (mqtt_user[0] == '\0')
    {
        return mqttClient.connect(UNIQUE_ID);
    }
    else
    {
        return mqttClient.connect(UNIQUE_ID, mqtt_user, mqtt_pwd);
    }
}

void ESPTeleInfo::AnalyzeData()
{
    if (ticMode == TINFO_MODE_HISTORIQUE)
    {
        analyzeBuffer[0] = '\0';
        teleinfo.valueGet(_iinst_, analyzeBuffer);
        iinst = atol(analyzeBuffer);

        analyzeBuffer[0] = '\0';
        teleinfo.valueGet(_papp_, analyzeBuffer);
        papp = atol(analyzeBuffer);

        analyzeBuffer[0] = '\0';
        teleinfo.valueGet(_base_, analyzeBuffer);
        indexes[0] = atol(analyzeBuffer);
        index = indexes[0] + indexes[1] + indexes[2];

        analyzeBuffer[0] = '\0';
        teleinfo.valueGet(_hchc_, analyzeBuffer);
        indexes[1] = atol(analyzeBuffer);
        index = indexes[0] + indexes[1] + indexes[2];

        analyzeBuffer[0] = '\0';
        teleinfo.valueGet(_hchp_, analyzeBuffer);
        indexes[2] = atol(analyzeBuffer);
        index = indexes[0] + indexes[1] + indexes[2];
    }
    else
    {
        // store intensity value for HIST or STD modes
        analyzeBuffer[0] = '\0';
        teleinfo.valueGet(_irms1_, analyzeBuffer);
        iinst = atol(analyzeBuffer);
        analyzeBuffer[0] = '\0';
        teleinfo.valueGet(_sinsts_, analyzeBuffer);
        papp = atol(analyzeBuffer);
        analyzeBuffer[0] = '\0';
        teleinfo.valueGet(_east_, analyzeBuffer);
        index = atol(analyzeBuffer);
    }
}

void ESPTeleInfo::SetData(char *label, char *value)
{
    if (delay_generic <= 0)
    {
        SendData(label, value);
    }
    else if (sendGenericData())
    {
        SendAllData();
        ts_generic = millis();
    }
}

void ESPTeleInfo::SendAllData()
{

    ValueList *item = teleinfo.getList();

    if (item)
    {
        while (item->next)
        {
            SendData(item->name, item->value);
            item = item->next;
        }
    }
}

void ESPTeleInfo::SendData(char *label, char *value)
{
    // send all data in the data topic
    if (connectMqtt())
    {
        sprintf(strDataTopic, "teleinfokit_dev/data/%s", label);
        mqttClient.publish(strDataTopic, value, true);
    }
}

void ESPTeleInfo::loop(void)
{
    if (Serial.available())
    {
        teleinfo.process(Serial.read());

        if (millis() - ts_analyzeData > 1000)
        {
            AnalyzeData();
            // ValueList *item = teleinfo.getList();

            // if (item)
            // {
            //     while (item->next)
            //     {
            //         if (item->flags & TINFO_FLAGS_UPDATED)
            //         {
            //         }
            //         item = item->next;
            //     }
            // }
            ts_analyzeData = millis();
        }

        if (compteur[0] == '\0')
        {

            if (ticMode == TINFO_MODE_HISTORIQUE)
            {
                teleinfo.valueGet(_adc0_, compteur);
            }
            else
            {
                teleinfo.valueGet(_adsc_, compteur);
            }
        }
        // teleinfo.getList().)

        // sendGeneric = sendGenericData();

        // iinst = teleinfo.getLongVal(IINST);

        // iinst = iinst == -1 ? 0 : iinst;
        // iinst1 = iinst1 == -1 ? 0 : iinst1;
        // iinst2 = iinst2 == -1 ? 0 : iinst2;
        // iinst3 = iinst3 == -1 ? 0 : iinst3;

        // papp = teleinfo.getLongVal(PAPP);

        // hc = teleinfo.getLongVal(HC);
        // hp = teleinfo.getLongVal(HP);
        // base = teleinfo.getLongVal(BASE);

        // if(base > 0){
        //     modeBase  = true;
        // }
        // getPhaseMode();

        // imax = teleinfo.getLongVal(IMAX);
        // strncpy(ptec, teleinfo.getStringVal(PTEC), 20);

        // if (connectMqtt())
        // {
        //     // send all data in the data topic
        //     if(sendGenericData()){
        //         for(uint8_t i= 0; i<teleinfo.dataCount; i++){
        //             if(strncmp(values_old[i], teleinfo.values[i], DATA_MAX_SIZE + 1) != 0){
        //                 sprintf(strDataTopic, "teleinfokit_dev/data/%s", teleinfo.labels[i]);
        //                 mqttClient.publish(strDataTopic, teleinfo.values[i], true);
        //                 snprintf(values_old[i], DATA_MAX_SIZE+1, "%s", teleinfo.values[i]);
        //             }
        //         }
        //         ts_generic = millis();
        //     }

        //     // send specific data - backwards compatibility
        //     if (!modeTriphase && iinst != iinst_old && sendPower)
        //     {
        //         mqttClient.publish("teleinfokit_dev/iinst", teleinfo.getStringVal(IINST));
        //     }

        //     if (modeTriphase && sendPower){
        //         // mode triphasé only : intensités des 3 phases
        //         if (iinst1 != iinst1_old)
        //         {
        //             mqttClient.publish("teleinfokit_dev/iinst1", teleinfo.getStringVal(IINST1));
        //         }
        //         if (iinst2 != iinst2_old)
        //         {
        //             mqttClient.publish("teleinfokit_dev/iinst2", teleinfo.getStringVal(IINST2));
        //         }
        //         if (iinst3 != iinst3_old)
        //         {
        //             mqttClient.publish("teleinfokit_dev/iinst3", teleinfo.getStringVal(IINST3));
        //         }
        //     }

        //     if (papp != papp_old && sendPower)
        //     {
        //         mqttClient.publish("teleinfokit_dev/papp", teleinfo.getStringVal(PAPP));
        //     }
        //     if (hc != hc_old && hc != 0 && sendIndex)
        //     {
        //         mqttClient.publish("teleinfokit_dev/hc", teleinfo.getStringVal(HC), true);
        //     }
        //     if (hp != hp_old && hp != 0 && sendIndex)
        //     {
        //         mqttClient.publish("teleinfokit_dev/hp", teleinfo.getStringVal(HP), true);
        //     }
        //     if (base != base_old && base != 0 && sendIndex)
        //     {
        //         mqttClient.publish("teleinfokit_dev/base", teleinfo.getStringVal(BASE), true);
        //     }
        //     if (imax != imax_old)
        //     {
        //         mqttClient.publish("teleinfokit_dev/imax", teleinfo.getStringVal(IMAX));
        //     }
        //     if (strcmp(ptec, ptec_old) != 0)
        //     {
        //         mqttClient.publish("teleinfokit_dev/ptec", teleinfo.getStringVal(PTEC));
        //     }
        // }

        // if(sendPower)
        // {
        //     iinst_old = iinst;
        //     iinst1_old = iinst1;
        //     iinst2_old = iinst2;
        //     iinst3_old = iinst3;
        //     papp_old = papp;
        //     ts_power = millis();
        // }

        // if(sendIndex){
        //     hc_old = hc;
        //     hp_old = hp;
        //     base_old = base;
        //     ts_index = millis();
        // }

        // imax_old = imax;
        // strncpy(ptec_old, ptec, 20);

        // if (!staticInfoSsent && connectMqtt())
        // {
        //     if (teleinfo.getStringVal(ADCO)[0] != '\n')
        //     {
        //         strncpy(adc0, teleinfo.getStringVal(ADCO), 20);
        //         mqttClient.publish("teleinfokit_dev/adc0", teleinfo.getStringVal(ADCO), true);
        //     }
        //     if (teleinfo.getStringVal(ISOUSC)[0] != '\n')
        //     {
        //         isousc = teleinfo.getLongVal(ISOUSC);
        //         mqttClient.publish("teleinfokit_dev/isousc", teleinfo.getStringVal(ISOUSC), true);
        //     }

        //     staticInfoSsent = true;
        // }

        // teleinfo.resetAvailable();
    }
}

// void ESPTeleInfo::getPhaseMode(){
//     modeTriphase = !(
//         iinst > 0 &&
//         iinst1 <= 0 &&
//         iinst2 <= 0 &&
//         iinst3 <= 0);
// }

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
        mqttClient.publish("teleinfokit_dev/log", "Startup");
        strcpy(str, "Version: ");
        strcat(str, VERSION);
        mqttClient.publish("teleinfokit_dev/log", str);
#ifdef _HW_VER
        sprintf(str, "HW Version: %d", _HW_VER);
        mqttClient.publish("teleinfokit_dev/log", str);
#endif
        strcpy(str, "IP: ");
        strcat(str, WiFi.localIP().toString().c_str());
        mqttClient.publish("teleinfokit_dev/log", str);
        strcpy(str, "MAC: ");
        strcat(str, WiFi.macAddress().c_str());
        mqttClient.publish("teleinfokit_dev/log", str);
        return true;
    }
    else
    {
        return false;
    }
}

bool ESPTeleInfo::sendGenericData()
{
    return (delay_generic <= 0) || (millis() - ts_generic > (delay_generic));
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
        mqttClient.publish("teleinfokit_dev/log", buffer);
    }
}
