// Original library source: https://github.com/gbrd/arduino-teleinfo
// under The MIT License (MIT)
// Copyright (c) 2015 Gaël Bréard
// this library is adapted for the need of this project

#ifndef Teleinfo_h
#define Teleinfo_h

#include <stdint.h>

#include "Arduino.h"
#include <Stream.h>


#define LABEL_MAX_SIZE 8
#define DATA_MAX_SIZE 13
#define LINE_MAX_COUNT 30
#define FRAME_MAX_SIZE 500

class TeleInfo
{
public:
  //TeleInfo(uint8_t rxPin,uint8_t txPin);
  TeleInfo(Stream* serial);
  
  void begin();
  
  boolean available();
  void process();
  
  void resetAvailable();
  
  /**
   * return NULL if this label is not found
   */
  const char * getStringVal(const char * label);
  /**
   * return a negative valure if this label is not found
   */
  long getLongVal(const char * label);  
  void printAllToSerial();
  
  void setDebug(boolean debug);
  
  char labels[LINE_MAX_COUNT][LABEL_MAX_SIZE+1]; //+1 for '\0' ending
  char values[LINE_MAX_COUNT][DATA_MAX_SIZE+1];
  int dataCount = 0;
  
private:
  void resetAll();
  int readLabel(int beginIndex, char* label);
  int readData(int beginIndex, char* data);  
  
  boolean isChecksumValid(char *label, char *data, char checksum); 
  
  boolean readFrame(); 
  
  Stream* _cptSerial;
  //SoftwareSerial _serial;
  boolean _isDebug = false;
  boolean _isAvailable = false;
  
  char _frame[FRAME_MAX_SIZE];
  int _frameIndex = 0;
  
  
};

#endif
