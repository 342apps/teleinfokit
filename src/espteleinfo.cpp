#include "espteleinfo.h"

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
    ts_startup = 0;
    started = false;
}

static void DataCallback(ValueList *me, uint8_t flags)
{
    getESPTeleInfo()->SetData(me->name, me->value);
}

void ESPTeleInfo::init(_Mode_e tic_mode)
{
    instanceEsp = this;
    ticMode = tic_mode;
    previousMillis = millis();
    iinst = 0;
    papp = 0;
    index = 0;
    adresseCompteur[0] = '\0';

    snprintf(UNIQUE_ID, 30, "teleinfokit-%06X", ESP.getChipId());
    snprintf(bufLogTopic, 35, "%s/log", UNIQUE_ID);
    snprintf(bufDataTopic, 35, "%s/data", UNIQUE_ID);

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

    // we use the power delay for generic data, *1000 to get ms
    delay_generic = period_data * 1000;

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

void ESPTeleInfo::AnalyzeTicForInternalData()
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
        // Send data in real time
        SendData(label, value);
    }
    else {
        // Store updated data to send later
        addOrReplaceValueInList(unsentList, label, value);
    }
}

void ESPTeleInfo::SendAllUnsentData()
{
    UnsentValueList* current = unsentList;
    while (current != nullptr) {
        SendData(current->name, current->value);
        current = current->next;
    }
    freeList(unsentList);
}

void ESPTeleInfo::SendAllData()
{
    ValueList *item = teleinfo.getList();

    if (item)
    {
        while (item->next)
        {
            item = item->next;
            SendData(item->name, item->value);
        }
    }
}

void ESPTeleInfo::SendData(char *label, char *value)
{
    // send all data in the data topic
    if (connectMqtt())
    {
        sprintf(strDataTopic, "%s/%s", bufDataTopic, label);
        mqttClient.publish(strDataTopic, value, true);
    }
}

void ESPTeleInfo::loop(void)
{
    if(ts_startup == 0){
        ts_startup = millis();
    }

    if (Serial.available())
    {
        teleinfo.process(Serial.read());

        if (millis() - ts_analyzeData > 1000)
        {
            AnalyzeTicForInternalData();
            ts_analyzeData = millis();
        }

        if (delay_generic > 0 && sendGenericData())
        {
            SendAllUnsentData();
            ts_generic = millis();
        }

        if (adresseCompteur[0] == '\0')
        {

            if (ticMode == TINFO_MODE_HISTORIQUE)
            {
                teleinfo.valueGet(_adc0_, adresseCompteur);
            }
            else
            {
                teleinfo.valueGet(_adsc_, adresseCompteur);
            }
        }

        // SendAll data 5s after start of loop to be sure all labels have been processed and are available
        if(!started && (millis() - ts_startup > 5000)){
            SendAllData();
            started = true;
        }
    }
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
        Log("Startup " + String(UNIQUE_ID));
        strcpy(str, "Version: ");
        strcat(str, VERSION);
        Log(str);
#ifdef _HW_VER
        sprintf(str, "HW Version: %d", _HW_VER);
        Log(str);
#endif
        strcpy(str, "IP: ");
        strcat(str, WiFi.localIP().toString().c_str());
        Log(str);
        strcpy(str, "MAC: ");
        strcat(str, WiFi.macAddress().c_str());
        Log(str);
        return true;
    }
    else
    {
        return false;
    }
}

bool ESPTeleInfo::sendGenericData()
{
    return (millis() - ts_generic) > (delay_generic);
}

// 100 char max !
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
        s.toCharArray(logBuffer, 200);
        mqttClient.publish(bufLogTopic, logBuffer);
    }
}

void ESPTeleInfo::sendMqttDiscovery(){
    discoveryDevice = "\"dev\":{\"ids\":\"" + String(UNIQUE_ID) +"\" ,\"name\":\"TeleInfoKit\",\"sw\":\""+String(VERSION)+"\",\"mdl\":\"TeleInfoKit v4\",\"mf\": \"342apps\"}";
    mqttClient.setBufferSize(500);

     if (connectMqtt()){
        if(ticMode == TINFO_MODE_STANDARD){
            sendMqttDiscoveryIndex(F("EAST"), "Index total");
            sendMqttDiscoveryIndex(F("EASF01"), F("Index fournisseur 01"));
            sendMqttDiscoveryIndex(F("EASF02"), F("Index fournisseur 02"));
            sendMqttDiscoveryIndex(F("EASF03"), F("Index fournisseur 03"));
            sendMqttDiscoveryIndex(F("EASF04"), F("Index fournisseur 04"));
            sendMqttDiscoveryIndex(F("EASF05"), F("Index fournisseur 05"));
            sendMqttDiscoveryIndex(F("EASF06"), F("Index fournisseur 06"));
            sendMqttDiscoveryIndex(F("EASF07"), F("Index fournisseur 07"));
            sendMqttDiscoveryIndex(F("EASF08"), F("Index fournisseur 08"));
            sendMqttDiscoveryIndex(F("EASF09"), F("Index fournisseur 09"));
            sendMqttDiscoveryIndex(F("EASF10"), F("Index fournisseur 10"));
            sendMqttDiscoveryIndex(F("EASD01"), F("Index distributeur 01"));
            sendMqttDiscoveryIndex(F("EASD02"), F("Index distributeur 02"));
            sendMqttDiscoveryIndex(F("EASD03"), F("Index distributeur 03"));
            sendMqttDiscoveryIndex(F("EASD04"), F("Index distributeur 04"));

            sendMqttDiscoveryForType(F("SINSTS"), F("Puissance apparente"), F("apparent_power"), "VA", F("mdi:power-plug"));
            sendMqttDiscoveryForType(F("IRMS1"), F("Intensité"), F("current"), "A", F("mdi:lightning-bolt-circle"));
            sendMqttDiscoveryForType(F("URMS1"), F("Tension"), F("voltage"), "V", F("mdi:sine-wave"));

            sendMqttDiscoveryText(F("ADSC"), F("Adresse compteur"));
            sendMqttDiscoveryText(F("NGTF"), F("Option tarifaire"));
            sendMqttDiscoveryText(F("LTARF"), F("Libellé tarif en cours"));
            sendMqttDiscoveryText(F("NTARF"), F("Numéro index tarifaire en cours"));
            sendMqttDiscoveryText(F("NJOURF+1"), F("Numéro du prochain jour calendrier fournisseur"));
            sendMqttDiscoveryText(F("MSG1"), F("Message"));
            sendMqttDiscoveryText(F("RELAIS"), F("Etat relais"));
        }
        else {
            sendMqttDiscoveryIndex(F("BASE"), F("Index BASE"));
            sendMqttDiscoveryIndex(F("HCHC"), F("Index heure cruse"));
            sendMqttDiscoveryIndex(F("HCHP"), F("Index heure pleine"));
            sendMqttDiscoveryIndex(F("EJPHN"), F("Index EJP heure normale"));
            sendMqttDiscoveryIndex(F("EJPHPM"), F("Index EJP heure de pointe mobile"));
            sendMqttDiscoveryIndex(F("BBRHCJB"), F("Index Tempo heures creuses jours Bleus"));
            sendMqttDiscoveryIndex(F("BBRHPJB"), F("Index Tempo heures pleines jours Bleus"));
            sendMqttDiscoveryIndex(F("BBRHCJW"), F("Index Tempo heures creuses jours Blancs"));
            sendMqttDiscoveryIndex(F("BBRHPJW"), F("Index Tempo heures pleines jours Blancs"));
            sendMqttDiscoveryIndex(F("BBRHCJR"), F("Index Tempo heures creuses jours Rouges"));
            sendMqttDiscoveryIndex(F("BBRHPJR"), F("Index Tempo heures pleines jours Rouges"));

            sendMqttDiscoveryForType(F("PAPP"), F("Puissance apparente"), F("apparent_power"), "VA", F("mdi:power-plug"));
            sendMqttDiscoveryForType(F("IINST"), F("Intensité"), F("current"), "A", F("mdi:lightning-bolt-circle"));

            sendMqttDiscoveryText(F("ADCO"), F("Adresse compteur"));
            sendMqttDiscoveryText(F("OPTARIF"), F("Option tarifaire"));
            sendMqttDiscoveryText(F("PTEC"), F("Période tarifaire en cours"));
            sendMqttDiscoveryText(F("DEMAIN"), F("Couleur du lendemain"));
        }
    }
    mqttClient.setBufferSize(256);
}

void ESPTeleInfo::sendMqttDiscoveryIndex(String label, String friendlyName){
    label.toCharArray(bufLabel, 10);
    sprintf(strDiscoveryTopic, "homeassistant/sensor/%s/%s/config", UNIQUE_ID, bufLabel);

    String sensor = F("{\"name\":\"")+friendlyName+F("\",\"dev_cla\":\"energy\",\"stat_cla\":\"total_increasing\",\"unit_of_meas\":\"kWh\"")+
    F(",\"val_tpl\":\"{{float(value)/1000.0}}\",\"stat_t\":\"")+bufDataTopic+"/"+label+F("\",\"uniq_id\":\"")+String(UNIQUE_ID)+"-"+label+
    F("\",\"obj_id\":\"")+String(UNIQUE_ID)+"-"+label+F("\",\"ic\":\"mdi:counter\",")+
    discoveryDevice + "}";

    sensor.toCharArray(payloadDiscovery, 500);
    mqttClient.publish(strDiscoveryTopic, payloadDiscovery);
}

void ESPTeleInfo::sendMqttDiscoveryForType(String label, String friendlyName, String deviceClass, String unit, String icon){

    label.toCharArray(bufLabel, 10);
    sprintf(strDiscoveryTopic, "homeassistant/sensor/%s/%s/config", UNIQUE_ID, bufLabel);

    String sensor = F("{\"name\":\"")+friendlyName+F("\",\"dev_cla\":\"")+deviceClass+F("\",\"unit_of_meas\":\"")+unit+"\""+
    F(",\"stat_t\":\"")+bufDataTopic+"/"+label+F("\",\"uniq_id\":\"")+String(UNIQUE_ID)+"-"+label+F("\",\"obj_id\":\"")+String(UNIQUE_ID)+"-"+label+"\",\"ic\":\""+icon+"\","+
    discoveryDevice + "}";

    sensor.toCharArray(payloadDiscovery, 500);
    mqttClient.publish(strDiscoveryTopic, payloadDiscovery);
}

void ESPTeleInfo::sendMqttDiscoveryText(String label, String friendlyName){
    label.toCharArray(bufLabel, 10);
    sprintf(strDiscoveryTopic, "homeassistant/sensor/%s/%s/config", UNIQUE_ID, bufLabel);

    String sensor = F("{\"name\":\"")+friendlyName+F("\",\"stat_t\":\"")+bufDataTopic+"/"+label+F("\",\"uniq_id\":\"")+String(UNIQUE_ID)+"-"+label+
    F("\",\"obj_id\":\"")+String(UNIQUE_ID)+"-"+label+F("\",\"ic\":\"mdi:information-outline\",")+
    discoveryDevice + "}";

    sensor.toCharArray(payloadDiscovery, 500);
    mqttClient.publish(strDiscoveryTopic, payloadDiscovery);
}

void ESPTeleInfo::freeList(UnsentValueList*& head) {
    UnsentValueList* current = head;
    while (current != nullptr) {
        UnsentValueList* next = current->next;
        free(current->name);
        free(current->value);
        delete current;
        current = next;
    }
    head = nullptr;
}

void ESPTeleInfo::addOrReplaceValueInList(UnsentValueList*& head, const char* name, const char* newValue) {
    UnsentValueList* current = head;
    while (current != nullptr) {
        if (strcmp(current->name, name) == 0) {
            free(current->value);
            current->value = strdup(newValue); 
            return; 
        }
        current = current->next;
    }

    // not found -> add
    UnsentValueList* newNode = new UnsentValueList;
    newNode->name = strdup(name);
    newNode->value = strdup(newValue);
    newNode->next = head;
    head = newNode;
}