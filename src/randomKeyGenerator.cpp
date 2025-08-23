#include "randomKeyGenerator.h"

RandomKeyGenerator::RandomKeyGenerator()
{
    #ifdef ESP8266
    randomSeed(ESP.getChipId());
    generateRandomHexKey(apPwd);
    #elif defined(ESP32)
    randomSeed((uint32_t)ESP.getEfuseMac());
    generateRandomHexKey(apPwd);
    #endif
}

char RandomKeyGenerator::generateRandomHexChar()
{
    const char hexChars[] = "0123456789ABCDEF";
    return hexChars[random(16)];
}

// Generates a key with format "XXXX-XXXX-XXXX"
void RandomKeyGenerator::generateRandomHexKey(char hexKey[14])
{
    for (int i = 0; i < 14; ++i)
    {
        hexKey[i] = generateRandomHexChar();

        // Ajoute un tiret après chaque groupe de 4 caractères hexadécimaux
        if ((i + 1) % 5 == 0 && i != 13)
        {
            hexKey[i] = '-';
        }
    }
}