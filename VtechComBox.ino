/*****************************************************************************************************************
ComBox ver. 3.3.2 for V-tech Dyno software (min. ver. 6.3.23.87)
Maciej Strzebonski fuse@vtech.pl
Arduino UNO
COM7 port by default
Bit rate: 115200, Data bits: 8, Parity: none, Stop bits: 1, Flow control: no
MAX6675 by Rob Tillaart 0.3.0

D11 - calibration pin
  Connect power
  Connect a 128 Hz square signal to D2, e.g. external clock quartz crystal generator (32.768 kHz) divided by 256
  Leave the D4, D5, D6, D7 unconnected
  Short D11 to ground for a moment to calibrate the rpm calculation

  To reset the calibration, connect D11 and D7 to ground, connect power for a few seconds,
  disconnect power, wait for a few second, disconnect D11 and D7 from ground and reconnect power

D2 - rpm input (Garrett rotor speed sensor, injectors, etc.)

   (Schottky rectifier 150V eg. STPS2150)
 D2-·-|>|---------· to rpm signal
    |
    = (220 pF)
    |
   ¯¯¯
   gnd

D3 - must by connected to D2

D4, D5, D6 - blades_config (number of blades = blades_config + 8)
 D4 - blades_config bit0 = 1 when D4 is shorted to gnd, 0 when unconnected
 D5 - blades_config bit1 = 1 when D5 is shorted to gnd, 0 when unconnected
 D6 - blades_config bit2 = 1 when D6 is shorted to gnd, 0 when unconnected
 D7 - blades_config bit3 = 1 when D7 is shorted to gnd, 0 when unconnected 

 n/c = unconnected
  8       blades = D7(n/c), D6(n/c), D5(n/c), D4(n/c) (divisor 2.00)
  9       blades = D7(n/c), D6(n/c), D5(n/c), D4(gnd) (divisor 2.25)
 10       blades = D7(n/c), D6(n/c), D5(gnd), D4(n/c) (divisor 2.50)
 11       blades = D7(n/c), D6(n/c), D5(gnd), D4(gnd) (divisor 2.75)
 12 (6+6) blades = D7(n/c), D6(gnd), D5(n/c), D4(n/c) (divisor 3.00)
 13       blades = D7(n/c), D6(gnd), D5(n/c), D4(gnd) (divisor 3.25)
 14 (7+7) blades = D7(n/c), D6(gnd), D5(gnd), D4(n/c) (divisor 3.50)
 15       blades = D7(n/c), D6(gnd), D5(gnd), D4(gnd) (divisor 3.75)

engine rpm mode
    divisor 2.00 = D7(gnd), D6(n/c), D5(n/c), D4(n/c) (one spark per one revolution of the crankshaft - wasted spark system)
    divisor 1.00 = D7(gnd), D6(n/c), D5(n/c), D4(gnd) (one spark per two revolutions of the crankshaft - sequential ignition system)
  
D8 - rpm reading averaging mode (short to gnd for increase filtering)

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

Data frame: <STX>G0001112223334444455555<ETX><CR><LF>
     G - header
   000 - A0 (analog input in V) * 1023 / 5 in hex format min.000h (0V), max.3FFh (1023 * 5 / 1023 = 5V)
   111 - A1 (analog input in V) * 1023 / 5 in hex format min.000h (0V), max.3FFh (1023 * 5 / 1023 = 5V)
   222 - A2 (analog input in V) * 1023 / 5 in hex format min.000h (0V), max.3FFh (1023 * 5 / 1023 = 5V)
   333 - A3 (analog input in V) * 1023 / 5 in hex format min.000h (0V), max.3FFh (1023 * 5 / 1023 = 5V)
 44444 - D2 (digital input in rpm) in hex format min.00000h, max.FFFFFh
 55555 - D2 (digital input duty cycle low level in percents) in hex format min.00000h, max.FFFFFh

Data frame: <STX>H000111XX22224444455555<ETX><CR><LF>
     H - header
   000 - A0 (analog input in V) * 1023 / 5 in hex format min.000h (0V), max.3FFh (1023 * 5 / 1023 = 5V)
   111 - A1 (analog input in V) * 1023 / 5 in hex format min.000h (0V), max.3FFh (1023 * 5 / 1023 = 5V)
    XX - "00" characters without meaning
  2222 - MAX6675#1 (temperature input in °C) * 10 in hex format min.0000h (0°C), max.27FEh (10238 / 10 = 1023.8°C)
 44444 - D2 (digital input in rpm) in hex format min.00000h, max.FFFFFh
 55555 - D2 (digital input duty cycle low level in percents) in hex format min.00000h, max.FFFFFh

Data frame: <STX>I000X111122224444455555<ETX><CR><LF>
     I - header
   000 - A0 (analog input in V) * 1023 / 5 in hex format min.000h (0V), max.3FFh (1023 * 5 / 1023 = 5V)
     X - "0" character without meaning
  1111 - MAX6675#2 (temperature input in °C) * 10 in hex format min.0000h (0°C), max.27FEh (10238 / 10 = 1023.8°C)
  2222 - MAX6675#1 (temperature input in °C) * 10 in hex format min.0000h (0°C), max.27FEh (10238 / 10 = 1023.8°C)
 44444 - D2 (digital input in rpm) in hex format min.00000h, max.FFFFFh
 55555 - D2 (digital input duty cycle low level in percents) in hex format min.00000h, max.FFFFFh
*****************************************************************************************************************/

//choose one of the three options:
//#define HEADER_G // "G" = 4x AIN + 1x RPM
#define HEADER_H // "H" = 2x AIN + 1x TK + 1x RPM
//#define HEADER_I // "I" = 1x AIN + 2x TK + 1x RPM

const char* firmwareRevision = "3.3.2";

#include <EEPROM.h>

#if defined(HEADER_H) || defined(HEADER_I)
  #include "MAX6675.h"
  const int dataPin    = 12;
  const int clockPin   = 13;
  const int selectPin  = 10;
  const int selectPin2 =  9;
  MAX6675 thermoCouple1(selectPin, dataPin, clockPin);
  MAX6675 thermoCouple2(selectPin2, dataPin, clockPin);
  uint32_t start, stop;
  int cnt = 0;
  int status;
  float temp;
  int temp_int;
  int status2;
  float temp2;
  int temp_int2;
#endif

volatile unsigned long currentUsISR0      = 0;
volatile unsigned long currentUsISR1      = 0;
volatile unsigned long startUs            = 0;
volatile unsigned long totalUsISR0        = 0;
volatile unsigned long rpmDutyTimeLow     = 0;
volatile unsigned long rpmDutyTimeHigh    = 0;
volatile unsigned long rpmDutyPrc         = 0;
volatile unsigned long rpm         = 0;
volatile unsigned long rpmOld      = 0;
volatile unsigned long rpmMaxError = 5000;
//total us in 1 min (60000000) * average  (6) = 360000000 * bladesMultiplier (2) = 720000000
volatile unsigned long  dividendAverage6  = 720000000;
//total us in 1 min (60000000) * average (10) = 360000000 * bladesMultiplier (2) = 1200000000
volatile unsigned long dividendAverage10 = 1200000000;

volatile bool rpmDutySecondPass;
volatile bool rpmIsCounting;
volatile bool rpmDutyReady;
volatile bool rpmErrorCnt;
volatile byte average              = 6;
volatile byte calibrationOK        = 255;
volatile byte counterISR0          = 0;
volatile byte watchDog             = 0;
volatile byte bladesCfg            = 0;
volatile float rpmDouble           = 0.0;
volatile float bladesMultiplier    = 2.0;
volatile unsigned long ain         = 0;
volatile float vin                 = 0.0;

char STX                           = 2;
char ETX                           = 3;
String stringToSend                = "0000000000000000000000";
String stringToSendRpm             = "";
String stringToSendDuty            = "";
String stringToSendDutyTime        = "";
String stringToSendAin             = "";

#if defined(HEADER_G)
  String stringHeader = "G";
#endif
  
#if defined(HEADER_H)
  String stringHeader = "H";
#endif
  
#if defined(HEADER_I)
  String stringHeader = "I";
#endif

#define rpmInputPin      2
#define rpmDutyPin       3
#define bit0Blade        4
#define bit1Blade        5
#define bit2Blade        6
#define bit3Blade        7
#define averageNumberPin 8
#define calibrationPin  11

void setup() {

  pinMode(bit0Blade, INPUT_PULLUP);
  pinMode(bit1Blade, INPUT_PULLUP);
  pinMode(bit2Blade, INPUT_PULLUP);
  pinMode(bit3Blade, INPUT_PULLUP);
  pinMode(rpmDutyPin, INPUT_PULLUP);
  pinMode(rpmInputPin, INPUT_PULLUP);
  pinMode(averageNumberPin, INPUT_PULLUP);
  pinMode(calibrationPin, INPUT_PULLUP);

  stringHeader = STX + stringHeader;
  stringToSend = stringHeader + stringToSend;
  stringToSend += ETX;
  Serial.begin(115200);
  while (!Serial);
  Serial.println(stringToSend);
  stringToSend = "";
  delay(100);
#if defined(HEADER_H) || defined(HEADER_I)
  thermoCouple1.begin();
  thermoCouple1.setSPIspeed(4000000);
  thermoCouple2.begin();
  thermoCouple2.setSPIspeed(4000000);
#endif
  delay(100);
  attachInterrupt(digitalPinToInterrupt(rpmInputPin), ISR0, FALLING);
  attachInterrupt(digitalPinToInterrupt(rpmDutyPin), ISR1, RISING);

  calibrationOK = EEPROM.read(0);

  if (calibrationOK != 255) {
    EEPROM.get(10, dividendAverage6);
    EEPROM.get(20, dividendAverage10);
  }
}

void loop() {

#if defined(HEADER_H) || defined(HEADER_I)  
  if (cnt == 0) {
    cnt = 1;
    status = thermoCouple1.read();
    
    if (status > 0) {
      temp_int = -100;
    }
    else {
      temp = thermoCouple1.getTemperature();
      temp *= 10.0;
      temp_int = round(temp);
    }
    #if defined(HEADER_H)
      temp_int2 = -1000;
    #endif
  }
  else {
    cnt = 0;
    #if defined(HEADER_I)    
        status2 = thermoCouple2.read();

        if (status2 > 0) {
          temp_int2 = -100;
        }
        else {
          temp2 = thermoCouple2.getTemperature();
          temp2 *= 10.0;
          temp_int2 = round(temp2);
        }
    #endif
  }
#endif

  stringToSend = stringHeader;

  for (int ainChannel = 0; ainChannel < 4; ainChannel++) {
    vin = 0;

    for (int i = 1; i < 200; i++) {
      ain += analogRead(ainChannel);
    }

    vin = (float)ain / 200.0;
    ain = round(vin);

    if (ain > 4095) {
      ain = 0;
    }
    
#if defined(HEADER_I)
    if (ainChannel == 0) {
      stringToSendAin = String(ain, HEX);
      for (int i = stringToSendAin.length(); i < 3; i++) {
        stringToSendAin = "0" + stringToSendAin;
      }
      stringToSendAin += "0";
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
        stringToSendAin = String(ain, HEX); //if HEADER_G send ainCannel 0, 1, 2, 3
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
    if (digitalRead(rpmDutyPin) == 0) rpmDutyPrc = 10000;
    else rpmDutyPrc = 0;
  }

  stringToSendRpm = String(rpm, HEX);

  for (int i = stringToSendRpm.length(); i < 5; i++) {
    stringToSendRpm = "0" + stringToSendRpm;
  }

  stringToSend += stringToSendRpm;

  stringToSendDuty = String(rpmDutyPrc, HEX);

  for (int i = stringToSendDuty.length(); i < 5; i++) {
    stringToSendDuty = "0" + stringToSendDuty;
  }

  stringToSend += stringToSendDuty;

  stringToSend += ETX;
  Serial.println(stringToSend);
  Serial.flush();
  rpmIsCounting = false;
  
  if (digitalRead(bit0Blade) == 0) bitSet(bladesCfg, 0);
  else bitClear(bladesCfg, 0);

  if (digitalRead(bit1Blade) == 0) bitSet(bladesCfg, 1);
  else bitClear(bladesCfg, 1);

  if (digitalRead(bit2Blade) == 0) bitSet(bladesCfg, 2);
  else bitClear(bladesCfg, 2);

  if (digitalRead(bit3Blade) == 0) bitSet(bladesCfg, 3);
  else bitClear(bladesCfg, 3);

  //if (digitalRead(engineRpmModePin) == 0) {
  //  bladesMultiplier = 1.0; //one spark per two rev
  //  rpmMaxError = 500;
  //}
  //else {
  //2.0 = one spark per one rev, 8 blades (Garrett turbo speed sensor output = f/8), engine wasted spark
    switch (bladesCfg) {
      case 0: //8 blades
        bladesMultiplier = 2.0;
        rpmMaxError = 5000;
        break;
      case 1: //9 blades
        bladesMultiplier = 2.25;
        rpmMaxError = 5000;
        break;
      case 2: //10 blades
        bladesMultiplier = 2.50;
        rpmMaxError = 5000;
        break;
      case 3: //11 blades
        bladesMultiplier = 2.75;
        rpmMaxError = 5000;
        break;
      case 4: //12 blades
        bladesMultiplier = 3.0;
        rpmMaxError = 5000;
        break;
      case 5: //13 blades
        bladesMultiplier = 3.25;
        rpmMaxError = 5000;
        break;
      case 6: //14 blades
        bladesMultiplier = 3.50;
        rpmMaxError = 5000;
        break;
      case 7: //15 blades
        bladesMultiplier = 3.75;
        rpmMaxError = 5000;
        break;
      case 8: //engine RPM mode
        bladesMultiplier = 1.0;
        rpmMaxError = 500;
        break;
      case 9: //engine RPM mode
        bladesMultiplier = 2.0;
        rpmMaxError = 500;
        break;        
      default: //engine RPM mode
        bladesMultiplier = 1.0;
        rpmMaxError = 500;
        break;
    }
  //}

  if (digitalRead(averageNumberPin) == 0) average = 10;
  else average = 6;

//f in = 128Hz
  if (digitalRead(calibrationPin) == 0) {
    EEPROM.update(0, 255);

    if (average == 6 && bladesMultiplier == 2) {
      dividendAverage6 = totalUsISR0 * 15360;
      dividendAverage10 = totalUsISR0 * 25600;
      EEPROM.put(10, dividendAverage6);
      EEPROM.put(20, dividendAverage10);
      EEPROM.update(0, 0);
    }
  }
  
  delay(10);
}

void ISR0() {
  currentUsISR0 = micros();
  watchDog = 0;

  if (rpmDutyReady) {
    rpmDutyTimeHigh += currentUsISR0 - currentUsISR1;

    if(rpmDutySecondPass) {
      unsigned long _timeTotal = rpmDutyTimeHigh + rpmDutyTimeLow;

      if (_timeTotal > 0)  rpmDutyPrc = round(((float)rpmDutyTimeLow / (float)_timeTotal) * 10000.0);

      rpmDutyTimeLow = 0;
      rpmDutyTimeHigh = 0;
      rpmDutySecondPass = false;
    }
    else rpmDutySecondPass = true;
  }

  rpmDutyReady = false;

  if (!rpmIsCounting) {
    counterISR0++;

    if (counterISR0 == 1) {
      startUs = currentUsISR0;
    }
    else if (counterISR0 > average) {
      rpmIsCounting = true;
      totalUsISR0 = currentUsISR0 - startUs;
      counterISR0 = 0;

      if (totalUsISR0 > 0) {
        
        if (average == 6) {
          rpmDouble = (float)dividendAverage6 / ((float)totalUsISR0 * bladesMultiplier);
        }
        else { // 10
          rpmDouble = (float)dividendAverage10 / ((float)totalUsISR0 * bladesMultiplier);
        }

        rpm = round(rpmDouble);
        
        if (rpm > 1048575) {
          rpm = 0;
        }
      }
      else {
        rpm = 0;
      }
     
      if (rpm > rpmOld + rpmMaxError && !rpmErrorCnt) {
        rpmErrorCnt = true;
        rpm = rpmOld;
      }
      else {
        rpmErrorCnt = false;
        rpmOld = rpm;
      }
    }
  }
}

void ISR1() {
  currentUsISR1 = micros();

  if (currentUsISR1 > currentUsISR0) rpmDutyTimeLow += currentUsISR1 - currentUsISR0;
  else rpmDutyTimeLow = 0;

  rpmDutyReady = true;
}