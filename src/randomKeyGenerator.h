#ifndef RANDOMKEYGENERATOR_H
#define RANDOMKEYGENERATOR_H

#include <Arduino.h>

class RandomKeyGenerator
{
public:
    RandomKeyGenerator();

    void InitAccessPointPassword();

    char apPwd[16];

private:

    void generateRandomHexKey(char hexKey[16]);
    char generateRandomHexChar();

};

#endif // RANDOMKEYGENERATOR_H