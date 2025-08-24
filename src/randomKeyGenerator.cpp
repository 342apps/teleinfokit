#include "randomKeyGenerator.h"

RandomKeyGenerator::RandomKeyGenerator()
{
    #if _HW_VER <= 4
    randomSeed(ESP.getChipId());
    #elif _HW_VER == 5
    randomSeed(ESP.getEfuseMac());
    #endif
    // init with 0
    memset(apPwd, 0, sizeof(apPwd));
    generateRandomHexKey(apPwd);
}

char RandomKeyGenerator::generateRandomHexChar()
{
    const char hexChars[] = "0123456789ABCDEF";
    return hexChars[random(16)];
}

// Generates a key with format "XXXX-XXXX-XXXX"
void RandomKeyGenerator::generateRandomHexKey(char hexKey[15])
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
    hexKey[14] = '\0';
}