#ifndef RANDOMKEYGENERATOR_H
#define RANDOMKEYGENERATOR_H

#include <Arduino.h>

class RandomKeyGenerator
{
public:
    RandomKeyGenerator();

    void InitAccessPointPassword();

    char apPwd[15];

private:
    void generateRandomHexKey(char hexKey[15]);
    char generateRandomHexChar();
};

#endif // RANDOMKEYGENERATOR_H