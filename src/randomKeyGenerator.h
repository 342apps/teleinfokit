#ifndef RANDOMKEYGENERATOR_H
#define RANDOMKEYGENERATOR_H

#include <Arduino.h>

class RandomKeyGenerator
{
public:
    RandomKeyGenerator();

    void InitAccessPointPassword();

    char apPwd[14];

private:
    void generateRandomHexKey(char hexKey[14]);
    char generateRandomHexChar();
};

#endif // RANDOMKEYGENERATOR_H