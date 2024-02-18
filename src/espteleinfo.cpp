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
    else {
        addOrReplaceValueInList(unsentList, label, value);
    }
}

void ESPTeleInfo::SendAllData()
{
    UnsentValueList* current = unsentList;
    while (current != nullptr) {
        SendData(current->name, current->value);
        current = current->next;
    }
    freeList(unsentList);
    // ValueList *item = teleinfo.getList();

    // if (item)
    // {
    //     while (item->next)
    //     {
    //         item = item->next;
    //         SendData(item->name, item->value);
    //     }
    // }
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

        if (delay_generic > 0 && sendGenericData())
        {
            SendAllData();
            ts_generic = millis();
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
        Log("Startup");
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
    //return (delay_generic <= 0) || (millis() - ts_generic > (delay_generic));
    return (millis() - ts_generic) > (delay_generic);
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
        s.toCharArray(buffer, 200);
        mqttClient.publish("teleinfokit_dev/log", buffer);
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
            sendMqttDiscoveryText(F("RELAI"), F("Etat relais"));
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
    F(",\"val_tpl\":\"{{float(value)/1000.0}}\",\"stat_t\":\"teleinfokit_dev/data/")+label+F("\",\"uniq_id\":\"")+String(UNIQUE_ID)+"-"+label+
    F("\",\"obj_id\":\"")+String(UNIQUE_ID)+"-"+label+F("\",\"ic\":\"mdi:counter\",")+
    discoveryDevice + "}";

    sensor.toCharArray(payloadDiscovery, 500);
    mqttClient.publish(strDiscoveryTopic, payloadDiscovery);
}

void ESPTeleInfo::sendMqttDiscoveryForType(String label, String friendlyName, String deviceClass, String unit, String icon){

    label.toCharArray(bufLabel, 10);
    sprintf(strDiscoveryTopic, "homeassistant/sensor/%s/%s/config", UNIQUE_ID, bufLabel);

    String sensor = F("{\"name\":\"")+friendlyName+F("\",\"dev_cla\":\"")+deviceClass+F("\",\"unit_of_meas\":\"")+unit+"\""+
    F(",\"stat_t\":\"teleinfokit_dev/data/")+label+F("\",\"uniq_id\":\"")+String(UNIQUE_ID)+"-"+label+F("\",\"obj_id\":\"")+String(UNIQUE_ID)+"-"+label+"\",\"ic\":\""+icon+"\","+
    discoveryDevice + "}";

    sensor.toCharArray(payloadDiscovery, 500);
    mqttClient.publish(strDiscoveryTopic, payloadDiscovery);
}

void ESPTeleInfo::sendMqttDiscoveryText(String label, String friendlyName){
    label.toCharArray(bufLabel, 10);
    sprintf(strDiscoveryTopic, "homeassistant/sensor/%s/%s/config", UNIQUE_ID, bufLabel);

    String sensor = F("{\"name\":\"")+friendlyName+F("\",\"stat_t\":\"teleinfokit_dev/data/")+label+F("\",\"uniq_id\":\"")+String(UNIQUE_ID)+"-"+label+
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
            free(current->value);  // Libérer la mémoire de l'ancienne valeur
            current->value = strdup(newValue);  // Copier la nouvelle valeur
            return;  // Arrêter la recherche et sortir de la fonction une fois que l'élément est trouvé et remplacé
        }
        current = current->next;
    }

    // not found -> add
    UnsentValueList* newNode = new UnsentValueList;
    newNode->name = strdup(name);  // Utiliser strdup pour allouer dynamiquement et copier la chaîne
    newNode->value = strdup(newValue);
    newNode->next = head;
    head = newNode;
}