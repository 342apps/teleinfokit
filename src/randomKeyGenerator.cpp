#include "randomKeyGenerator.h"

RandomKeyGenerator::RandomKeyGenerator(){
    randomSeed(ESP.getChipId());
    generateRandomHexKey(apPwd);
}

char RandomKeyGenerator::generateRandomHexChar() {
    const char hexChars[] = "0123456789ABCDEF";
    return hexChars[random(16)];
}

// Generates a key with format "XXXX-XXXX-XXXX"
void RandomKeyGenerator::generateRandomHexKey(char hexKey[16]) {
    for (int i = 0; i < 16; ++i) {
        hexKey[i] = generateRandomHexChar();

        // Ajoute un tiret après chaque groupe de 4 caractères hexadécimaux
        if ((i + 1) % 5 == 0 && i != 15) {
            hexKey[i] = '-';
        }
    }
}