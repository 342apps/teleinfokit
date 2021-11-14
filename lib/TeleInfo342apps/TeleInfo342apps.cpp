// Original library source: https://github.com/gbrd/arduino-teleinfo
// under The MIT License (MIT)
// Copyright (c) 2015 Gaël Bréard
// this library is adapted for the need of this project

#include "Arduino.h"
#include "TeleInfo342apps.h"


TeleInfo::TeleInfo(Stream* serial)
{
  this->_cptSerial = serial;
  _frame[0] = '\0'; 
}
/*
TeleInfo::TeleInfo(uint8_t rxPin,uint8_t txPin) : _serial(rxPin,txPin){

  //_cptSerial = &_serial;
  _frame[0] = '\0'; 
}
*/

void TeleInfo::setDebug(boolean debug){
  _isDebug = debug;
}

const char * TeleInfo::getStringVal(const char * label){
  int i = 0;
  char * res = NULL;
  while((strcmp(label,labels[i]) != 0) && i<dataCount){
    i++;
  }
  if(i >= dataCount){
    res = NULL;
  }else{
    res = values[i];
  }
  return res;
}
long TeleInfo::getLongVal(const char * label){
  const char * stringVal = getStringVal(label);
  long res;
  if(stringVal == NULL){
    res = -1;
  }else{
    res = atol(stringVal);
  }
  return res;
}
void TeleInfo::printAllToSerial(){
  for(int i = 0 ; i< dataCount ; i++){
    Serial.print(labels[i]);
    Serial.print(" => "); 
    Serial.println(values[i]);
  }
}

boolean TeleInfo::available(){
  return _isAvailable;
}


void TeleInfo::process(){
  char caractereRecu ='\0';
  while (_cptSerial->available() && !_isAvailable) {


    // checksum is enough...
    //if(_cptSerial->overflow()){
      //_frameIndex = 0;
      //if(_isDebug){
      //  Serial.println("overflow");
      //}
    //}

    caractereRecu = _cptSerial->read() & 0x7F;
    
    if(_isDebug){
      Serial.print(caractereRecu,HEX);
      Serial.print(" ");
      if(caractereRecu == 0x20 || caractereRecu == 0x0A || caractereRecu == 0x0D || caractereRecu == 0x03)
        Serial.println("");
    }
    
    //"Start Text 002" - frame start
    if(caractereRecu == 0x02){
      _frameIndex = 0;
    }
    //TODO check _ frame overflow !!!
    _frame[_frameIndex] = caractereRecu;
    _frameIndex++;
    
    //  "EndText 003" - frame end
    if(caractereRecu == 0x03 && _frame[0] == 0x02){
      _frame[_frameIndex]='\0';
      _frameIndex++;
      
      if(_isDebug){
        Serial.println("");
        Serial.println("*** will try to decode a new frame");
        ////Serial.println(_frame);
        //Serial.println("*** END frame ***");
      }
      _isAvailable = readFrame();
    }
  }
}

void TeleInfo::resetAvailable(){
  _isAvailable = false;
}


void TeleInfo::resetAll(){
  _frameIndex=0;
  dataCount = 0;
  _isAvailable = false;
}




void TeleInfo::begin()
{
  //_cptSerial->begin(1200);
  resetAll();
}


boolean TeleInfo::isChecksumValid(char *label, char *data, char checksum) 
{
  unsigned char sum = 32 ;      // Somme des codes ASCII du message + un espace
  int i ;
  
  for (i=0; i < strlen(label); i++) sum = sum + label[i] ;
  for (i=0; i < strlen(data); i++) sum = sum + data[i] ;
  sum = (sum & 63) + 32 ;
  return ( sum == checksum);
}

/**
 * Read a label in 'label' buffer. Start reading at beginInder in frame buffer.
 * return index pointing just after the label 
 */
  
int TeleInfo::readLabel(int beginIndex, char* label){
  int i = beginIndex;
  int j=0;
  while(_frame[i] != 0x20 && j < LABEL_MAX_SIZE){
    label[j] = _frame[i];
    j++;
    i++;
  }
  if(j == LABEL_MAX_SIZE+1){
    return -1;
  }else{
    label[j] = '\0';
    i++;
    return i;
  }
}


int TeleInfo::readData(int beginIndex, char *data){
  int i = beginIndex;
  int j=0;
  while(_frame[i] != 0x20 && j < DATA_MAX_SIZE){
    data[j] = _frame[i];
    j++;
    i++;
  }
  if(j == DATA_MAX_SIZE+1){
    return -1;
  }else{
    data[j] = '\0';
    i++;
    return i;
  }
}




boolean TeleInfo::readFrame(){
  if(_isDebug) Serial.println("will read a frame");
  int j=0;
  int lineIndex = 0;
  boolean frameOk = true;
  //start i at 1 to skip first char 0x02 (start byte)
  int i=1;
  while(_frame[i] != 0x03 && lineIndex < LINE_MAX_COUNT){
    if(_frame[i] != 0x0A){
      if(_isDebug){
        Serial.print(_frame[i], HEX);
        Serial.println(" frame KO 0A -- ");
      }
      frameOk = false;
      break;
    }
    
    i++;
    i = readLabel(i,labels[lineIndex]);
    if(i<0) {
      if(_isDebug){
        Serial.println("frame KO label");
      }
      frameOk = false;
      break;
    }
    if(_isDebug) {
      Serial.print("label=");
      Serial.println(labels[lineIndex]);
    }
    
    i = readData(i,values[lineIndex]);
    if(i<0) {
      if(_isDebug) Serial.println("frame KO data");
      frameOk = false;
      break;
    }
    
    if(_isDebug) {
      Serial.print("data=");
      Serial.println(values[lineIndex]);
    }
    
    char checksum = _frame[i];
    i++;
    if(_isDebug) {
      Serial.print("checksum is ");
      Serial.println(checksum, HEX);
    }
    if(_frame[i] != 0x0D){
      if(_isDebug) Serial.println("  -frame KO 0D");
      frameOk = false;
      break;
    }
    i++;
    if(!isChecksumValid(labels[lineIndex],values[lineIndex],checksum)){
      if(_isDebug) Serial.println("frame KO checksum");
      frameOk = false;
      break;
    }
    lineIndex++;
  }
  if(lineIndex >= LINE_MAX_COUNT){
    if(_isDebug) Serial.println("frame KO LINE_MAX_COUNT");
    frameOk = false;
  }else{
    dataCount = lineIndex;
  }
  if(!frameOk){
    dataCount = 0;
  }
  if(_isDebug) {
    if(!frameOk){
      Serial.println("frame KO ");
    }else{
      Serial.println("frame OK !!! ");
    }
  }
  
  return frameOk;
}




