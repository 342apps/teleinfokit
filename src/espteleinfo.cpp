#include "espteleinfo.h"

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

void ESPTeleInfo::init(_Mode_e tic_mode, bool triphase)
{
    instanceEsp = this;
    ticMode = tic_mode;
    this->triphase = triphase;
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

    delay(1000);
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
    // store intensity value for HIST or STD modes
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
    else
    {
        // Store updated data to send later
        addOrReplaceValueInList(unsentList, label, value);
    }
}

void ESPTeleInfo::SendAllUnsentData()
{
    UnsentValueList *current = unsentList;
    while (current != nullptr)
    {
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
    // send only if bufDataTopic and label not empty
    if (bufDataTopic[0] == '\0' || label[0] == '\0')
    {
        return;
    }
    // send all data in the data topic
    if (connectMqtt())
    {
        // sanitize the label to remove spaces and special characters
        String sanitizedLabel = sanitizeLabel(String(label));
        strncpy(bufLabel, sanitizedLabel.c_str(), sizeof(bufLabel) - 1);
        bufLabel[sizeof(bufLabel) - 1] = '\0';

        // prepare the topic
        int n = snprintf(strDataTopic, sizeof(strDataTopic), "%s/%s", bufDataTopic, bufLabel);
        strDataTopic[sizeof(strDataTopic) - 1] = '\0'; // ensure null-termination
        if (n < 0 || n >= (int)sizeof(strDataTopic))
        {
            // handle truncation or error if needed (optional: log or skip)
            return;
        }
        mqttClient.publish(strDataTopic, value, true);
    }
}

void ESPTeleInfo::loop(void)
{
    if (ts_startup == 0)
    {
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
        if (!started && (millis() - ts_startup > 5000))
        {
            SendAllData();
            started = true;
        }
    }
    mqttClient.loop();
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

void ESPTeleInfo::sendMqttDiscovery()
{
    discoveryDevice = "\"dev\":{\"ids\":\"" + String(UNIQUE_ID) + "\" ,\"name\":\"TeleInfoKit\",\"sw\":\"" + String(VERSION) + "\",\"mdl\":\"TeleInfoKit v4\",\"mf\": \"342apps\"}";
    mqttClient.setBufferSize(500);

    int8_t nbTry = 0;
    while (nbTry < NBTRY && !connectMqtt())
    {
        delay(250);
        nbTry++;
    }
    if (nbTry < NBTRY)
    {
        clearAllDiscovery();

        if (ticMode == TINFO_MODE_STANDARD)
        {
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
            sendMqttDiscoveryIndex(F("EAIT"), F("Energie active injectée totale "));

            sendMqttDiscoveryForType(F("SINSTS"), F("Puissance apparente instantanée"), F("apparent_power"), "VA", F("mdi:power-plug"));
            sendMqttDiscoveryForType(F("SINSTI"), F("Puissance app. Instantanée injectée"), F("apparent_power"), "VA", F("mdi:power-plug"));
            sendMqttDiscoveryForType(F("SMAXSN"), F("Puissance app. max. soutirée n"), F("apparent_power"), "VA", F("mdi:power-plug"));
            sendMqttDiscoveryForType(F("SMAXSN-1"), F("Puissance app. max. soutirée n-1"), F("apparent_power"), "VA", F("mdi:power-plug"));
            
            
            sendMqttDiscoveryText(F("ADSC"), F("Adresse compteur"));
            sendMqttDiscoveryText(F("NGTF"), F("Option tarifaire"));
            sendMqttDiscoveryText(F("LTARF"), F("Libellé tarif en cours"));
            sendMqttDiscoveryText(F("NTARF"), F("Numéro index tarifaire en cours"));
            sendMqttDiscoveryText(F("PREF"), F("Puissance app. de référence"));
            sendMqttDiscoveryText(F("NJOURF+1"), F("Numéro du prochain jour calendrier fournisseur"));
            sendMqttDiscoveryText(F("MSG1"), F("Message"));
            sendMqttDiscoveryText(F("RELAIS"), F("Etat relais"));
            
            // pour répondre au broker et ne pas être considéré comme un client inactif
            mqttClient.loop();
            
            if (triphase)
            {
                sendMqttDiscoveryForType(F("IRMS1"), F("Intensité phase 1"), F("current"), "A", F("mdi:lightning-bolt-circle"));
                sendMqttDiscoveryForType(F("IRMS2"), F("Intensité phase 2"), F("current"), "A", F("mdi:lightning-bolt-circle"));
                sendMqttDiscoveryForType(F("IRMS3"), F("Intensité phase 3"), F("current"), "A", F("mdi:lightning-bolt-circle"));
                
                sendMqttDiscoveryForType(F("URMS1"), F("Tension phase 1"), F("voltage"), "V", F("mdi:sine-wave"));
                sendMqttDiscoveryForType(F("URMS2"), F("Tension phase 2"), F("voltage"), "V", F("mdi:sine-wave"));
                sendMqttDiscoveryForType(F("URMS3"), F("Tension phase 3"), F("voltage"), "V", F("mdi:sine-wave"));
                sendMqttDiscoveryForType(F("UMOY1"), F("Tension moyenne Phase 1"), F("voltage"), "V", F("mdi:sine-wave"));
                sendMqttDiscoveryForType(F("UMOY2"), F("Tension moyenne Phase 2"), F("voltage"), "V", F("mdi:sine-wave"));
                sendMqttDiscoveryForType(F("UMOY3"), F("Tension moyenne Phase 3"), F("voltage"), "V", F("mdi:sine-wave"));
                
                sendMqttDiscoveryForType(F("SINSTS1"), F("Puissance apparente instantanée Phase 1"), F("apparent_power"), "VA", F("mdi:power-plug"));
                sendMqttDiscoveryForType(F("SINSTS2"), F("Puissance apparente instantanée Phase 2"), F("apparent_power"), "VA", F("mdi:power-plug"));
                sendMqttDiscoveryForType(F("SINSTS3"), F("Puissance apparente instantanée Phase 3"), F("apparent_power"), "VA", F("mdi:power-plug"));
                
                sendMqttDiscoveryForType(F("SMAXSN1"), F("Puissance app. max. soutirée n Phase 1"), F("apparent_power"), "VA", F("mdi:power-plug"));
                sendMqttDiscoveryForType(F("SMAXSN1-1"), F("Puissance app. max. soutirée n-1 Phase 1"), F("apparent_power"), "VA", F("mdi:power-plug"));
                sendMqttDiscoveryForType(F("SMAXSN2"), F("Puissance app. max. soutirée n Phase 2"), F("apparent_power"), "VA", F("mdi:power-plug"));
                sendMqttDiscoveryForType(F("SMAXSN2-1"), F("Puissance app. max. soutirée n-1 Phase 2"), F("apparent_power"), "VA", F("mdi:power-plug"));
                sendMqttDiscoveryForType(F("SMAXSN3"), F("Puissance app. max. soutirée n Phase 3"), F("apparent_power"), "VA", F("mdi:power-plug"));
                sendMqttDiscoveryForType(F("SMAXSN3-1"), F("Puissance app. max. soutirée n-1 Phase 3"), F("apparent_power"), "VA", F("mdi:power-plug"));
                
                
            }
            else
            {
                sendMqttDiscoveryForType(F("IRMS1"), F("Intensité"), F("current"), "A", F("mdi:lightning-bolt-circle"));
                sendMqttDiscoveryForType(F("URMS1"), F("Tension"), F("voltage"), "V", F("mdi:sine-wave"));
                sendMqttDiscoveryForType(F("UMOY1"), F("Tension moyenne"), F("voltage"), "V", F("mdi:sine-wave"));
            }
        }
        else
        {
            sendMqttDiscoveryIndex(F("BASE"), F("Index BASE"));
            sendMqttDiscoveryIndex(F("HCHC"), F("Index heure creuse"));
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
            sendMqttDiscoveryForType(F("ADPS"), F("Avertissement Dépassement Puissance Souscrite"), F("current"), "A", F("mdi:lightning-bolt-circle"));

            // pour répondre au broker et ne pas être considéré comme un client inactif
            mqttClient.loop();
            if (triphase)
            {
                sendMqttDiscoveryForType(F("IINST1"), F("Intensité phase 1"), F("current"), "A", F("mdi:lightning-bolt-circle"));
                sendMqttDiscoveryForType(F("IINST2"), F("Intensité phase 2"), F("current"), "A", F("mdi:lightning-bolt-circle"));
                sendMqttDiscoveryForType(F("IINST3"), F("Intensité phase 3"), F("current"), "A", F("mdi:lightning-bolt-circle"));

                sendMqttDiscoveryForType(F("PMAX"), F("Puissance maximale triphasée atteinte"), F("power"), "W", F("mdi:lightning-bolt-circle"));
                sendMqttDiscoveryText(F("PPOT"), F("Présence des potentiels"));
            }
            else
            {
                sendMqttDiscoveryForType(F("IINST"), F("Intensité"), F("current"), "A", F("mdi:lightning-bolt-circle"));
            }

            sendMqttDiscoveryText(F("ADCO"), F("Adresse compteur"));
            sendMqttDiscoveryText(F("OPTARIF"), F("Option tarifaire"));
            sendMqttDiscoveryText(F("PTEC"), F("Période tarifaire en cours"));
            sendMqttDiscoveryText(F("DEMAIN"), F("Couleur du lendemain"));
            sendMqttDiscoveryText(F("ISOUSC"), F("Intensité souscrite"));
        }
    }
    mqttClient.setBufferSize(256);
}

void ESPTeleInfo::clearAllDiscovery()
{

    deleteMqttDiscovery(F("EAST"));
    deleteMqttDiscovery(F("EASF01"));
    deleteMqttDiscovery(F("EASF02"));
    deleteMqttDiscovery(F("EASF03"));
    deleteMqttDiscovery(F("EASF04"));
    deleteMqttDiscovery(F("EASF05"));
    deleteMqttDiscovery(F("EASF06"));
    deleteMqttDiscovery(F("EASF07"));
    deleteMqttDiscovery(F("EASF08"));
    deleteMqttDiscovery(F("EASF09"));
    deleteMqttDiscovery(F("EASF10"));
    deleteMqttDiscovery(F("EASD01"));
    deleteMqttDiscovery(F("EASD02"));
    deleteMqttDiscovery(F("EASD03"));
    deleteMqttDiscovery(F("EASD04"));
    // pour répondre au broker et ne pas être considéré comme un client inactif
    mqttClient.loop();
    deleteMqttDiscovery(F("EAIT"));
    deleteMqttDiscovery(F("SINSTS"));
    deleteMqttDiscovery(F("SINSTI"));
    deleteMqttDiscovery(F("ADSC"));
    deleteMqttDiscovery(F("NGTF"));
    deleteMqttDiscovery(F("LTARF"));
    deleteMqttDiscovery(F("NTARF"));
    deleteMqttDiscovery(F("PREF"));
    deleteMqttDiscovery(F("NJOURF+1"));
    deleteMqttDiscovery(F("MSG1"));
    deleteMqttDiscovery(F("RELAIS"));
    mqttClient.loop();
    deleteMqttDiscovery(F("IRMS1"));
    deleteMqttDiscovery(F("IRMS2"));
    deleteMqttDiscovery(F("IRMS3"));
    deleteMqttDiscovery(F("URMS1"));
    deleteMqttDiscovery(F("URMS2"));
    deleteMqttDiscovery(F("URMS3"));

    deleteMqttDiscovery(F("BASE"));
    deleteMqttDiscovery(F("HCHC"));
    deleteMqttDiscovery(F("HCHP"));
    deleteMqttDiscovery(F("EJPHN"));
    deleteMqttDiscovery(F("EJPHPM"));
    deleteMqttDiscovery(F("BBRHCJB"));
    deleteMqttDiscovery(F("BBRHPJB"));
    deleteMqttDiscovery(F("BBRHCJW"));
    mqttClient.loop();
    deleteMqttDiscovery(F("BBRHPJW"));
    deleteMqttDiscovery(F("BBRHCJR"));
    deleteMqttDiscovery(F("BBRHPJR"));
    deleteMqttDiscovery(F("PAPP"));
    deleteMqttDiscovery(F("IINST1"));
    deleteMqttDiscovery(F("IINST2"));
    deleteMqttDiscovery(F("IINST3"));
    deleteMqttDiscovery(F("PMAX"));
    deleteMqttDiscovery(F("IINST"));
    deleteMqttDiscovery(F("ADCO"));
    deleteMqttDiscovery(F("OPTARIF"));
    deleteMqttDiscovery(F("PTEC"));
    deleteMqttDiscovery(F("DEMAIN"));
    mqttClient.loop();
    // ...existing code...
    deleteMqttDiscovery(F("SMAXSN"));
    deleteMqttDiscovery(F("SMAXSN-1"));
    deleteMqttDiscovery(F("SINSTI"));
    deleteMqttDiscovery(F("SMAXSN1"));
    deleteMqttDiscovery(F("SMAXSN1-1"));
    deleteMqttDiscovery(F("SMAXSN2"));
    deleteMqttDiscovery(F("SMAXSN2-1"));
    deleteMqttDiscovery(F("SMAXSN3"));
    deleteMqttDiscovery(F("SMAXSN3-1"));
    deleteMqttDiscovery(F("SINSTS1"));
    deleteMqttDiscovery(F("SINSTS2"));
    deleteMqttDiscovery(F("SINSTS3"));
    deleteMqttDiscovery(F("UMOY1"));
    deleteMqttDiscovery(F("UMOY2"));
    deleteMqttDiscovery(F("UMOY3"));
    deleteMqttDiscovery(F("PPOT"));
    mqttClient.loop();
}

void ESPTeleInfo::deleteMqttDiscovery(String label)
{
    label = sanitizeLabel(label);

    label.toCharArray(bufLabel, 10);
    sprintf(strDiscoveryTopic, "homeassistant/sensor/%s/%s/config", UNIQUE_ID, bufLabel);

    // clear the retained message
    mqttClient.publish(strDiscoveryTopic, "\0", true);
}

void ESPTeleInfo::sendMqttDiscoveryIndex(String label, String friendlyName)
{
    label = sanitizeLabel(label);

    label.toCharArray(bufLabel, 10);
    sprintf(strDiscoveryTopic, "homeassistant/sensor/%s/%s/config", UNIQUE_ID, bufLabel);

    String sensor = F("{\"name\":\"") + friendlyName + F("\",\"dev_cla\":\"energy\",\"stat_cla\":\"total_increasing\",\"unit_of_meas\":\"kWh\"") +
                    F(",\"val_tpl\":\"{{float(value)/1000.0}}\",\"stat_t\":\"") + bufDataTopic + "/" + label + F("\",\"uniq_id\":\"") + String(UNIQUE_ID) + "-" + label +
                    F("\",\"obj_id\":\"") + String(UNIQUE_ID) + "-" + label + F("\",\"ic\":\"mdi:counter\",") +
                    discoveryDevice + "}";

    sensor.toCharArray(payloadDiscovery, 500);
    mqttClient.publish(strDiscoveryTopic, payloadDiscovery, true);
}

void ESPTeleInfo::sendMqttDiscoveryForType(String label, String friendlyName, String deviceClass, String unit, String icon)
{
    label = sanitizeLabel(label);

    label.toCharArray(bufLabel, 10);
    sprintf(strDiscoveryTopic, "homeassistant/sensor/%s/%s/config", UNIQUE_ID, bufLabel);

    String sensor = F("{\"name\":\"") + friendlyName + F("\",\"dev_cla\":\"") + deviceClass + F("\",\"unit_of_meas\":\"") + unit + "\"" +
                    F(",\"stat_t\":\"") + bufDataTopic + "/" + label + F("\",\"uniq_id\":\"") + String(UNIQUE_ID) + "-" + label + F("\",\"obj_id\":\"") + String(UNIQUE_ID) + "-" + label + "\",\"ic\":\"" + icon + "\"," +
                    discoveryDevice + "}";

    sensor.toCharArray(payloadDiscovery, 500);
    mqttClient.publish(strDiscoveryTopic, payloadDiscovery, true);
}

void ESPTeleInfo::sendMqttDiscoveryText(String label, String friendlyName)
{
    label = sanitizeLabel(label);

    label.toCharArray(bufLabel, 10);
    sprintf(strDiscoveryTopic, "homeassistant/sensor/%s/%s/config", UNIQUE_ID, bufLabel);

    String sensor = F("{\"name\":\"") + friendlyName + F("\",\"stat_t\":\"") + bufDataTopic + "/" + label + F("\",\"uniq_id\":\"") + String(UNIQUE_ID) + "-" + label +
                    F("\",\"obj_id\":\"") + String(UNIQUE_ID) + "-" + label + F("\",\"ic\":\"mdi:information-outline\",") +
                    discoveryDevice + "}";

    sensor.toCharArray(payloadDiscovery, 500);
    mqttClient.publish(strDiscoveryTopic, payloadDiscovery, true);
}

void ESPTeleInfo::freeList(UnsentValueList *&head)
{
    UnsentValueList *current = head;
    while (current != nullptr)
    {
        UnsentValueList *next = current->next;
        free(current->name);
        free(current->value);
        delete current;
        current = next;
    }
    head = nullptr;
}

void ESPTeleInfo::addOrReplaceValueInList(UnsentValueList *&head, const char *name, const char *newValue)
{
    UnsentValueList *current = head;
    while (current != nullptr)
    {
        if (strcmp(current->name, name) == 0)
        {
            free(current->value);
            current->value = strdup(newValue);
            return;
        }
        current = current->next;
    }

    // not found -> add
    UnsentValueList *newNode = new UnsentValueList;
    newNode->name = strdup(name);
    newNode->value = strdup(newValue);
    newNode->next = head;
    head = newNode;
}

String ESPTeleInfo::sanitizeLabel(String input)
{
    input.replace("+", "_");
    input.replace("#", "_");
    input.replace("/", "_");
    // ajoute d’autres si besoin
    return input;
}