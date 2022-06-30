/*****************************************************************************************************************
ComBox v.3.1 for V-tech Dyno software (min. ver. 6.3.22.28)
Maciej Strzebonski fuse@vtech.pl
Arduino UNO
COM7 port by default
Bit rate: 115200, Data bits: 8, Parity: none, Stop bits: 1, Flow control: no

D2 - rpm input (Garrett RPM)
D3, D4, D5 - blades_config (number of blades = blades_config + 8)
 D3 - blades_config bit0 (D3 short to gnd = 1)
 D4 - blades_config bit1 (D4 short to gnd = 1)
 D5 - blades_config bit2 (D5 short to gnd = 1)

D6 - engine rpm mode
  D6 short to gnd, D3, D4, D5 unconnected = one spark per two revolutions of the crankshaft
  D6, D3, D4, D5 unconnected = one spark per one revolution of the crankshaft
D7 - rpm reading averaging mode (short to gnd 2x faster reading)

A0 - analog input (AIN 0)
A1 - analog input (AIN 1)
A2 - analog input (AIN 2)
A3 - analog input (AIN 3)

MAX6675#1 (EGT 1):
 SO  -> D12
 SCK -> D13
 CS  -> D10

MAX6675#2 (EGT 2):
 SO  -> D12
 SCK -> D13
 CS  -> D9

Data frame: <STX>G00011122233344444<ETX><CR><LF>
 G - header
 000 - A0 (analog input in V) * 1023 / 5 in hex format min.000h (0V), max.3FFh (1023 * 5 / 1023 = 5V)
 111 - A1 (analog input in V) * 1023 / 5 in hex format min.000h (0V), max.3FFh (1023 * 5 / 1023 = 5V)
 222 - A2 (analog input in V) * 1023 / 5 in hex format min.000h (0V), max.3FFh (1023 * 5 / 1023 = 5V)
 333 - A3 (analog input in V) * 1023 / 5 in hex format min.000h (0V), max.3FFh (1023 * 5 / 1023 = 5V)
 44444 - D2 (digital input in rpm) in hex format min.00000h, max.FFFFFh

Data frame: <STX>H000111XX222244444<ETX><CR><LF>
 H - header
 000 - A0 (analog input in V) * 1023 / 5 in hex format min.000h (0V), max.3FFh (1023 * 5 / 1023 = 5V)
 111 - A1 (analog input in V) * 1023 / 5 in hex format min.000h (0V), max.3FFh (1023 * 5 / 1023 = 5V)
 XX - "00" characters without meaning
 2222 - MAX6675#1 (temperature input in °C) * 10 in hex format min.0000h (0°C), max.27FEh (10238 / 10 = 1023.8°C)
 44444 - D2 (digital input in rpm) in hex format min.00000h, max.FFFFFh

Data frame: <STX>I00001111222244444<ETX><CR><LF>
 I - header
 0000 - A0 (analog input in V) * 1023 / 5 in hex format min.0000h (0V), max.03FFh (1023 * 5 / 1023 = 5V)
 1111 - MAX6675#2 (temperature input in °C) * 10 in hex format min.0000h (0°C), max.27FEh (10238 / 10 = 1023.8°C)
 2222 - MAX6675#1 (temperature input in °C) * 10 in hex format min.0000h (0°C), max.27FEh (10238 / 10 = 1023.8°C)
 44444 - D2 (digital input in rpm) in hex format min.00000h, max.FFFFFh
*****************************************************************************************************************/

//choose one of the three options:
//#define HEADER_G // "G" = 4x AIN + 1x RPM
#define HEADER_H // "H" = 2x AIN + 1x TK + 1x RPM
//#define HEADER_I // "I" = 1x AIN + 2x TK + 1x RPM

#if defined(HEADER_H) || defined(HEADER_I)
  #include "MAX6675.h"
  const int dataPin   = 12;
  const int clockPin  = 13;
  const int selectPin = 10;
  const int selectPin2 = 9;
  MAX6675 thermoCouple;
  MAX6675 thermoCouple2;
  uint32_t start, stop;
  int cnt = 0;
  int status;
  float temp;
  int temp_int;
  int status2;
  float temp2;
  int temp_int2;
#endif

volatile unsigned long currentUs;
volatile unsigned long printUs;
volatile unsigned long startUs;
volatile unsigned long sparkUs;
volatile unsigned long rpmMode;
volatile byte sparkCounter;
volatile byte rpmReady;
volatile unsigned long rpm;
volatile unsigned long rpmMaxError;
volatile double rpmDouble;
volatile unsigned long rpmOld;
volatile byte rpmErrorCnt;
volatile byte watchDog;
volatile byte bladesCfg = 0;
volatile byte blades = 8;
volatile byte average = 10;
volatile unsigned long ain = 0;
volatile double vin = 0;
char STX = 2;
char ETX = 3;
String stringToSend = "";
String stringToSendRpm = "";
String nullStringToSendRpm = "00000";
String stringToSendAin = "";
String nullStringToSendAin = "000000000000";

#if defined(HEADER_G)
  String stringHeaderAin = "G";
#endif
  
#if defined(HEADER_H)
  String stringHeaderAin = "H";
#endif
  
#if defined(HEADER_I)
  String stringHeaderAin = "I";
#endif

#define bit0Blade 3
#define bit1Blade 4
#define bit2Blade 5
#define rpmInput 2
#define engineRpmMode 6
#define averageNumber 7
#define rpmTest 8

void setup() {
  pinMode(bit0Blade, INPUT_PULLUP);
  pinMode(bit1Blade, INPUT_PULLUP);
  pinMode(bit2Blade, INPUT_PULLUP);
  pinMode(rpmInput, INPUT_PULLUP);
  pinMode(engineRpmMode, INPUT_PULLUP);
  pinMode(averageNumber, INPUT_PULLUP);
  pinMode(rpmTest, OUTPUT);
  digitalWrite(rpmTest, LOW);
  stringHeaderAin = STX + stringHeaderAin;
  nullStringToSendAin = stringHeaderAin + nullStringToSendAin;
  nullStringToSendAin += nullStringToSendRpm;
  nullStringToSendAin += ETX;
  Serial.begin(115200);
  while (!Serial);
  Serial.println(nullStringToSendAin);
  delay(100);
#if defined(HEADER_H) || defined(HEADER_I)
  thermoCouple.begin(clockPin, selectPin, dataPin);
  thermoCouple.setSPIspeed(4000000);
  thermoCouple2.begin(clockPin, selectPin2, dataPin);
  thermoCouple2.setSPIspeed(4000000);
#endif
  delay(100);
  attachInterrupt(digitalPinToInterrupt(rpmInput), ISR0, FALLING);
}

void loop() {

#if defined(HEADER_H) || defined(HEADER_I)
  if (cnt == 0) {
    cnt = 1;
    status = thermoCouple.read();
    
    if (status == 4) {
      temp_int = -10;
    }
    else {
      temp = thermoCouple.getTemperature();
      temp *= 10;
      temp_int = round(temp);
    }
    temp_int2 = -1000;
  }
  else {
    cnt = 0;
    status2 = thermoCouple2.read();

    if (status2 == 4) {
      temp_int2 = -10;
    }
    else {
      temp2 = thermoCouple2.getTemperature();
      temp2 *= 10;
      temp_int2 = round(temp2);
    }
    temp_int = -1000;
  }
#endif

  stringToSend = stringHeaderAin;

  for (int ainChannel = 0; ainChannel < 4; ainChannel++) {
    vin = 0;

    for (int i = 0; i < 200; i++) {
      ain += analogRead(ainChannel);
    }

    vin = (double)ain / 200.0;
    ain = round(vin);

    if (ain > 4095) {
      ain = 0;
    }
    
#if defined(HEADER_I)
    if (ainChannel == 0) {
      stringToSendAin = String(ain, HEX);
      for (int i = stringToSendAin.length(); i < 4; i++) {
        stringToSendAin = "0" + stringToSendAin;
      }
    }
    else if (ainChannel == 1) {
      stringToSendAin = String(temp_int2, HEX);
      for (int i = stringToSendAin.length(); i < 4; i++) {
        stringToSendAin = "0" + stringToSendAin;     
      }
    }
    else if (ainChannel == 2) {
      stringToSendAin = String(temp_int, HEX);
      for (int i = stringToSendAin.length(); i < 4; i++) {
        stringToSendAin = "0" + stringToSendAin;     
      }
    }
    else if (ainChannel == 3) {
      stringToSendAin = "";
    }
#else 
  #if defined(HEADER_H)
      if (ainChannel == 0 || ainChannel == 1) {
  #endif
        stringToSendAin = String(ain, HEX);
        for (int i = stringToSendAin.length(); i < 3; i++) {
          stringToSendAin = "0" + stringToSendAin;
        }
  #if defined(HEADER_H)
      }   
      else if (ainChannel == 2) {
        stringToSendAin = String(temp_int, HEX);
        for (int i = stringToSendAin.length(); i < 4; i++) {
          stringToSendAin = "0" + stringToSendAin;
        }
        stringToSendAin = "00" + stringToSendAin;
      }
      else if (ainChannel == 3) {
        stringToSendAin = "";
      }
  #endif
#endif

    stringToSend += stringToSendAin;
  }
  watchDog++;

  if (watchDog == 10) {
    rpm = 0;    
  }

  stringToSendRpm = String(rpm, HEX);
  rpmReady = 0;

  for (int i = stringToSendRpm.length(); i < 5; i++) {
    stringToSendRpm = "0" + stringToSendRpm;
  }

  stringToSend += stringToSendRpm;
  stringToSend += ETX;
  Serial.println(stringToSend);
  
  if (digitalRead(bit0Blade) == 0) bitSet(bladesCfg, 0);
  else bitClear(bladesCfg, 0);

  if (digitalRead(bit1Blade) == 0) bitSet(bladesCfg, 1);
  else bitClear(bladesCfg, 1);

  if (digitalRead(bit2Blade) == 0) bitSet(bladesCfg, 2);
  else bitClear(bladesCfg, 2);

  if (digitalRead(engineRpmMode) == 0) {
    rpmMode = 4;
    rpmMaxError = 500;
  }
  else {
    rpmMode = 8;
    rpmMaxError = 5000;
  }

  blades = bladesCfg + rpmMode;

  if (digitalRead(averageNumber) == 0) average = 5;
  else average = 10;
  
  delay(10);
}

void ISR0() {
  currentUs = micros();
  watchDog = 0;

  digitalWrite(rpmTest, HIGH);

  if (rpmReady == 0) {
    sparkCounter++;

    if (sparkCounter == 1) {
      startUs = currentUs;
    }
    else if (sparkCounter > average) {
      rpmReady = 1;
      sparkUs = currentUs - startUs;
      sparkCounter = 0;   
      sparkUs *= blades;

      if (sparkUs > 0) {
        
        if (average == 10) {
          rpmDouble = 4800000000.0 / (double)sparkUs;
        }
        else {
          rpmDouble = 2400000000.0 / (double)sparkUs;          
        }

        rpm = round(rpmDouble);
        
        if (rpm > 1048575) {
          rpm = 0;
        }
      }
      else {
        rpm = 0;
      }
     
      if (rpm > rpmOld + rpmMaxError && rpmErrorCnt < 1) {
        rpmErrorCnt++;
        rpm = rpmOld;
      }
      else {
        rpmErrorCnt = 0;
        rpmOld = rpm;
      }
    }
  }
  digitalWrite(rpmTest, LOW);
}
