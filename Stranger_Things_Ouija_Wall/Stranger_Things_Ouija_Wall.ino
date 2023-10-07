/*
Board:      Adafruit Feather RP2040
Shield:     Adafruit Featherwing Music Maker
Peripheral: Adafruit 20W stereo amplifier 
  aLED  26 ADC0
  cLED  27 ADC1
  vLED  28 ADC2
        29 ADC3       13  
RE1CLK  24            12  
 RE1DT  25            11  
        18 SCK        10  VS1053 DCS
        19 MOSI       09  VS1053 DREQ
        20 MISO       08  VS1053 CS
        01 RX         07  VS1053 CardDCS
        00 TX     SCL 03  max9744
 RE1SW  06        SDA 02  max9744
*/
// Library Includes
#include <FastLED.h>                                    // FastLED Library to control WS2811 LEDs
#include <Adafruit_VS1053.h>                            // VS1053 library to control FeatherWing Music Maker
#include <SD.h>                                         // SD library to read files off of SD card on FW MM
#include <Wire.h>                                       // Wire library to control MAX9744 via I2C protocol
//#include <SPI.h>                                        // Library for SPI devices like the rotary encoder
// ----- Configuration Options ------
#define encoder1
#define verbose                                         // Enable to show more verbosity on Serial
//#define debug                                         // Enable to show maximum verbosity on Serial
//#define demoEnabled                                   // Enable to ensure the 1st 4 items are predictable
//#define randomTiming                                  // Enable to randomize the timing in the messages to feel more "human"
//#define nonBlockingDelay                              // Enable to disable delays and replace with millis commands
#define ceilingEnabled                                  // Uncomment when ceiling lights are present
// ----- MAX9744 20W Class D amplifier definitions -----
#define max9744Enabled                                  // Enable to enable control of the MAX9744 20W amp
#if defined (max9744Enabled)                            // Pins required: SDA -> SDA, SCL -> SCL, Vi2c -> 3.3V, Gnd -> Gnd
  int8_t max9744Vol = 25;                               // Volume level of the MAX9744, 25 is default
  #define MAX9744_I2CADDR 0x4B                          // 0x4B is the default I2C address
#endif
// ----- Music Maker definitions -----
#define vs1053Enabled                                   // Enable to enable sound playback through the FeatherWing Music Maker
//#define fake_vs1053Enabled
#if defined (vs1053Enabled)
  #define VS1053_RESET   -1                             // VS1053 reset pin (not used!)
  #define VS1053_CS       8                             // VS1053 chip select pin (output)
  #define VS1053_DCS     10                             // VS1053 Data/command select pin (output)
  #define CARDCS          7                             // Card chip select pin
  #define VS1053_DREQ     9                             // VS1053 Data request, ideally an Interrupt pin
  Adafruit_VS1053_FilePlayer musicPlayer =              // Defining the Music Maker command
    Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);
#endif
String ST[6] = {"/KateBush.mp3", "/NeverEndingStory.mp3", 
  "/Journey.mp3", "/MoP.mp3", "/theClash.mp3", "/theme.mp3"};
// ----- Rotary Encoder definitions ------
#if defined (encoder1)
  const uint8_t RE1_CLK = 24;                           // Rotary Encoder's CLK pin, each click in either direction, output cycles HIGH then LOW
  const uint8_t RE1_DT = 25;                            // Rotary Encoder's DT pin, same as CLK, but lags by 90 deg phase shift, determines rot. dir.
  const uint8_t RE1_SW = 6;                             // Rotary Encoder's SW pin, push button built into dial that goes LOW when pushed
  int RE1_stateCLK;                                     // the state of the CLK pin on RE1
  int RE1_lastStateCLK;                                 // the previous state of the RE1 CLK pin, used to track rotation
  int RE1_stateSW;                                      // the state of the pushbutton switch on RE1
  unsigned long dtDelay = 40;                           // the debounce time; increase if output flickers
  unsigned long lastButtonPress = millis();                    // the last time in millis that the button was pressed
  bool turnOff = false;                                 // Future feature to pause playback
  // ----- VU Meter Light Definitions -----
  unsigned long vuTimeDelay = 0;
  const int vuNumLEDs = 17;                             // Number of LEDS on VU Meter Array
  const int vuDataPin = 28;                             // VU Meter Array Data Pin
  CRGBArray<vuNumLEDs> vuLEDs;                          // VU Meter Array Definition
  struct vuArray {                                      // Data structure definition for VU Meter Array
    int displayNum;
    int ledNum;
  };
  vuArray disp2leds[] {
    //{0,0}, {1,4}, {2,5}, {3,6}, {4,7}, {5,8},         // Right to Left
    //{6,12}, {7,13}, {8,14}, {9,15}, {10,16}           // Right to Left
    {0,0}, {1,15}, {2,14}, {3,13}, {4,12}, {5,11},      // Left to Right
    {6,7}, {7,6}, {8,5}, {9,4}, {10,3}                  // Left to Right
  };
#endif
// ----- Alphabet Light Definitions -----
const int alphabetNumLEDs = 50;                         // Number of LEDs on Alphabet array
const int alphabetDataPin = 26;                         // Alphabet Array Data Pin
uint8_t alphabetLightNumColor[alphabetNumLEDs];         // Array to assign colors to individual LEDs
uint8_t alphabetLightHueFade[alphabetNumLEDs];          // Array to use as a placeholder during Fade effects
uint8_t alphabetLightSatFade[alphabetNumLEDs];          // Array to use as a placeholder during Fade effects
uint8_t alphabetLightBrightFade[alphabetNumLEDs];       // Array to use as a placeholder during Fade effects
CRGBArray<alphabetNumLEDs> alphabetLEDs;                // Alphabet LED array definition
struct alphabetArray {                                  // Data structure definition for Alphabet Array
  char letter;
  int led;
};
alphabetArray char2leds[] {                             // Alphabet array element definitions
  {'A', 0},  {'B', 1},  {'C', 2},  {'D', 3},  {'E', 4},  {'F', 5}, {'G', 6},  {'H', 7},  {'I', 8},  
  {'J', 9},  {'K', 10}, {'L', 11}, {'M', 12}, {'N', 13}, {'O', 14}, {'P', 15}, {'Q', 16}, {'R', 17},
  {'S', 18}, {'T', 19}, {'U', 20}, {'V', 21}, {'W', 22}, {'X', 23}, {'Y', 24}, {'Z', 25},
};
const int msgArrayNum = 9;                              // Range of numbers for randMsg
uint8_t prevRandMsg = msgArrayNum + 1;                  // Placeholder for prev randMsg to reduce repeats
uint8_t prev2RandMsg = msgArrayNum + 2;                 // Placeholder for prev randMsg to reduce repeats
uint8_t randMsg;                                        // Random Number for selecting predefined messages randomly
String msgArray[msgArrayNum] = {"help me", "watch out", 
  "happy halloween", "leggo my eggo", "vecna is here", 
  "i am cold", "beware", "demagorgon", "mind flayer"};
// ----- Ceiling Light Definitions -----
#if defined (ceilingEnabled)                            // Definitions for Ceiling Lights when present
  const int ceilingNumLEDs = 400;                       // Number of LEDs on Ceiling array
  const int ceilingDataPin = 27;                        // Ceiling Array Data Pin
  uint8_t ceilingLightNumColor[ceilingNumLEDs];         // Array to assign colors to individual LEDs
  uint8_t ceilingLightHueFade[ceilingNumLEDs];          // Array to use as a placeholder during Fade Effects
  uint8_t ceilingLightSatFade[ceilingNumLEDs];          // Array to use as a placeholder during Fade effects
  uint8_t ceilingLightBrightFade[ceilingNumLEDs];       // Array to use as a placeholder during Fade effects
  CRGBArray<ceilingNumLEDs> ceilingLEDs;                // Ceiling LED array definition
#endif
// ----- 
const uint8_t lightPtn[5] = { 250, 40, 20, 140, 110 };  // Light Pattern: Red, Yellow, Orange, Blue, Green
const uint8_t eventNum = 3;                             // Range of numbers for randNum
uint8_t randNum;                                        // Random Number for selecting events randomly
uint8_t prevRandNum = eventNum + 1;                     // Placeholder for prev RandNum to reduce repeats
uint8_t prev2RandNum = eventNum + 2;                    // Placeholder for prev RandNum to reduce repeats
const uint8_t stNum = 6;                                // Range of numbers for randST
uint8_t prevRandST = stNum + 1;                         // Placeholder for prev randST to reduce repeats
uint8_t prev2RandST = stNum + 2;                        // Placeholder for prev randST to reduce repeats
uint8_t prev3RandST = stNum + 3;                        // Placeholder for prev randST to reduce repeats
uint8_t randST;                                         // Random Number for selecting Soundtracks randomly
uint8_t iTimer = 10;                                    // Inactivity timer setting, idle time to wait until next event 
uint8_t inactiveTimer;                                  // Inactivity Timer
const int hueFade = 0;
const int satFade = 255;
const int hbRestBright = 135;
const int hbBeatBright = 255;
uint8_t vuBright = 127;                                 // Brightness of VU Meter lights
uint8_t sat = 255;                                      // Saturation, possible range 0-255
uint8_t bright = 255;                                   // Brightness, possible range 0-255
unsigned long now;                                      // Timer variable
bool dspActive = false;                                 // Display Active, tells rest of script whether somethings running
String cmd;                                             // Command String, saves data coming in from Serial


void onOff(bool off) {

}


void millisDelay(unsigned long d) {
  now = millis(); 
  while(millis() < now + d) { now += now + d; };
}


void vuDisplay() {
  int volDispVal = 0;
  switch (max9744Vol) {
    case 0 ... 15: volDispVal = 0; Serial.print("volDispVal = "); Serial.println(volDispVal); break;
    case 16 ... 19: volDispVal = 1; Serial.print("volDispVal = "); Serial.println(volDispVal); break;
    case 20 ... 21: volDispVal = 2; Serial.print("volDispVal = "); Serial.println(volDispVal); break;
    case 22 ... 23: volDispVal = 3; Serial.print("volDispVal = "); Serial.println(volDispVal); break;
    case 24 ... 25: volDispVal = 4; Serial.print("volDispVal = "); Serial.println(volDispVal); break;
    case 26 ... 27: volDispVal = 5; Serial.print("volDispVal = "); Serial.println(volDispVal); break;
    case 28 ... 29: volDispVal = 6; Serial.print("volDispVal = "); Serial.println(volDispVal); break;
    case 30 ... 31: volDispVal = 7; Serial.print("volDispVal = "); Serial.println(volDispVal); break;
    case 32 ... 33: volDispVal = 8; Serial.print("volDispVal = "); Serial.println(volDispVal); break;
    case 34 ... 35: volDispVal = 9; Serial.print("volDispVal = "); Serial.println(volDispVal); break;
    case 36 ... 64: volDispVal = 10; Serial.print("volDispVal = "); Serial.println(volDispVal); break;
  }
  for (int v = 0; v <= 10; v++) {
    if (v <= volDispVal) {
      vuLEDs[disp2leds[v].ledNum] = CHSV(0, 255, vuBright);
    } else if (v > volDispVal) {
      vuLEDs[disp2leds[v].ledNum] = CHSV(0, 255, 0);
    }
  }
  FastLED.show();
  vuTimeDelay = millis();
}


boolean setVolume(int8_t v) {
  #if defined (max9744Enabled)
    //if (v > 63) v = 63; if (v < 0) v = 0;               // range is 6-bit, cant be higher than 63 or lower than 0
    Serial.print("Setting volume to "); Serial.println(v);
    Wire.beginTransmission(MAX9744_I2CADDR);
    Wire.write(v);
    if (Wire.endTransmission() == 0) {return true;}
    else {return false;};
  #else
    Serial.println("MAX9744 Amp not enabled!");
    return false;
  #endif
}


#if defined (encoder1)
void readEncoder() { //https://lastminuteengineers.com/rotary-encoder-arduino-tutorial/amp/
  RE1_stateSW = digitalRead(RE1_SW);
  if (RE1_stateSW == LOW) {
    if (millis() - lastButtonPress > dtDelay ) {
      if (turnOff == false) { turnOff = true; } else if (turnOff == true) { turnOff = false; }
      onOff(turnOff);
    }
    lastButtonPress = millis();
  }
  RE1_stateCLK = digitalRead(RE1_CLK);          // Read the current state of RE1_CLK
  // If last & current state of CLK are different, then pulse occurred, react to only 1 state change to avoid double count
  //if (RE1_stateCLK != RE1_lastStateCLK && RE1_stateCLK == 1) {
  if (millis() - lastButtonPress > dtDelay) {
    if (RE1_stateCLK != RE1_lastStateCLK) {
      if (digitalRead(RE1_DT) != RE1_stateCLK) {  // If DT state is different than CLK, then encoder is rotating CCW
        if (max9744Vol < 63) {
          lastButtonPress = millis();
          max9744Vol++; //;}; 
          setVolume(max9744Vol); 
          vuDisplay();
          lastButtonPress = millis();
        }
      } else {  // Encoder is rotating CW
        if (max9744Vol > 0) {
          max9744Vol--; //;}; 
          setVolume(max9744Vol); 
          vuDisplay();
          lastButtonPress = millis();
        }
      }
    }
  }
}
#endif


void fadeOut(int fTime) {
  int div = fTime / bright;
  for ( int b=bright; b >= 0; b--) {
    #if defined (ceilingEnabled)
      for ( int i=0; i < (ceilingNumLEDs); i++) { ceilingLEDs[i] = CHSV(ceilingLightNumColor[i], sat, b); }
    #else
      for ( int i=0; i < (alphabetNumLEDs); i++) { alphabetLEDs[i] = CHSV(alphabetLightNumColor[i], sat, b); }
    #endif
    FastLED.show();
    #if defined (nonBlockingDisplay)
      millisDelay(div);
    #else
      delay(div);
    #endif
  }
}


void fadeIn(int fTime) {
  int div = (fTime/ bright);
  for ( int b=0; b <= bright; b++) {
    #if defined (ceilingEnabled)
      for ( int i=0; i < (ceilingNumLEDs); i++) { ceilingLEDs[i] = CHSV(ceilingLightNumColor[i], sat, b); }
    #else
      for ( int i=0; i < (alphabetNumLEDs); i++) { alphabetLEDs[i] = CHSV(alphabetLightNumColor[i], sat, b); }
    #endif
    FastLED.show();
    #if defined (nonBlockingDisplay)
      millisDelay(div);
    #else
      delay(div);
    #endif
  }
}


void heartBeat(int beats) {
  for (int b=0; b < beats; b++) {
    unsigned long beatStart = millis();
    #if defined (ceilingEnabled)
      for (int i=0; i < (ceilingNumLEDs); i++) {
        ceilingLEDs[i] = CHSV(ceilingLightHueFade[i], ceilingLightSatFade[i], hbBeatBright);
    #else
      for (int i=0; i < (alphabetNumLEDs); i++) {
        alphabetLEDs[i] = CHSV(alphabetLightHueFade[i], alphabetLightSatFade[i], hbBeatBright);
    #endif
    }
    FastLED.show();
    delay(10);
    for ( int c=(hbBeatBright); c > (hbRestBright); c-=( (hbBeatBright-hbRestBright)/10) ) {
      #if defined (ceilingEnabled)
        for (int i=0; i < (ceilingNumLEDs); i++) {
          ceilingLEDs[i] = CHSV(ceilingLightHueFade[i], ceilingLightSatFade[i], c);
      #else
        for (int i=0; i < (alphabetNumLEDs); i++) {
          alphabetLEDs[i] = CHSV(alphabetLightHueFade[i], alphabetLightSatFade[i], c);
      #endif
      }
      FastLED.show();
      delay(5);
    }
    delay((178 - (millis() - beatStart)));
  }
}


void fadeToRed() {
  #if defined (ceilingEnabled)
    for ( int i=0; i < (ceilingNumLEDs); i++) { ceilingLightHueFade[i] = ceilingLightNumColor[i]; ceilingLightSatFade[i] = sat; ceilingLightBrightFade[i] = bright;}
  #else
    for ( int i=0; i < (alphabetNumLEDs); i++) { alphabetLightHueFade[i] = alphabetLightNumColor[i]; alphabetLightSatFade[i] = sat; alphabetLightBrightFade[i] = bright;}
  #endif
  for ( int c=0; c < 116; c++) {
    #if defined (ceilingEnabled)
      for (int i=0; i < (ceilingNumLEDs); i++) { 
        if (ceilingLightHueFade[i] != hueFade) {
          if (ceilingLightHueFade[i] > 127) {ceilingLightHueFade[i]++;}
          else if (ceilingLightHueFade[i] < 127) {ceilingLightHueFade[i]--;}
        }
        if (ceilingLightSatFade[i] != satFade) {
          if (ceilingLightSatFade[i] < satFade) {ceilingLightSatFade[i]++;}
          else if (ceilingLightSatFade[i] > satFade) {ceilingLightSatFade[i]--;}
        }
        if (ceilingLightBrightFade[i] != hbRestBright) {
          if (ceilingLightBrightFade[i] < hbRestBright) {ceilingLightBrightFade[i]++;}
          else if (ceilingLightBrightFade[i] > hbRestBright) {ceilingLightBrightFade[i]--;}
        }
        ceilingLEDs[i] = CHSV(ceilingLightHueFade[i], ceilingLightSatFade[i], ceilingLightBrightFade[i]);
      }
    #else
      for (int i=0; i < (alphabetNumLEDs); i++) { 
        if (alphabetLightHueFade[i] != hueFade) {
          if (alphabetLightHueFade[i] > 127) {alphabetLightHueFade[i]++;}
          else if (alphabetLightHueFade[i] < 127) {alphabetLightHueFade[i]--;}
        }
        if (alphabetLightSatFade[i] != satFade) {
          if (alphabetLightSatFade[i] < satFade) {alphabetLightSatFade[i]++;}
          else if (alphabetLightSatFade[i] > satFade) {alphabetLightSatFade[i]--;}
        }
        if (alphabetLightBrightFade[i] != hbRestBright) {
          if (alphabetLightBrightFade[i] < hbRestBright) {alphabetLightBrightFade[i]++;}
          else if (alphabetLightBrightFade[i] > hbRestBright) {alphabetLightBrightFade[i]--;}
        }
        alphabetLEDs[i] = CHSV(alphabetLightHueFade[i], alphabetLightSatFade[i], hbRestBright);
      }
    #endif
    FastLED.show();
    delay(10);
  }
}


void fadeToPattern() {
  #if defined (ceilingEnabled)
    for (int i=0; i < (ceilingNumLEDs); i++) {ceilingLightHueFade[i] = hueFade; ceilingLightSatFade[i] = satFade; ceilingLightBrightFade[i] = hbRestBright;}
  #else
    for (int i=0; i < (alphabetNumLEDs); i++) {alphabetLightHueFade[i] = hueFade; alphabetLightSatFade[i] = satFade; alphabetLightBrightFade[i] = hbRestBright;}
  #endif
  for (int c=0; c < 255; c++) {
    #if defined (ceilingEnabled)
      for (int i=0; i < (ceilingNumLEDs); i++) {
        if (ceilingLightHueFade[i] != ceilingLightNumColor[i]) {
          if (ceilingLightHueFade[i] < ceilingLightNumColor[i]) {ceilingLightHueFade[i]++;}
          else if (ceilingLightHueFade[i] > ceilingLightNumColor[i]) {ceilingLightHueFade[i]--;}
        }
        if (ceilingLightSatFade[i] != sat) {
          if (ceilingLightSatFade[i] > sat) {ceilingLightSatFade[i]--;}
          else if (ceilingLightSatFade[i] < sat) {ceilingLightSatFade[i]++;}
        }
        if (ceilingLightBrightFade[i] != bright) {
          if (ceilingLightBrightFade[i] > bright) {ceilingLightBrightFade[i]--;}
          else if (ceilingLightBrightFade[i] < bright) {ceilingLightBrightFade[i]++;}
        }
        ceilingLEDs[i] = CHSV(ceilingLightHueFade[i], ceilingLightSatFade[i], ceilingLightBrightFade[i]);
      }
    #else
      for (int i=0; i < (alphabetNumLEDs); i++) {
        if (alphabetLightHueFade[i] != alphabetLightNumColor[i]) {
          if (alphabetLightHueFade[i] < alphabetLightNumColor[i]) {alphabetLightHueFade[i]++;}
          else if (alphabetLightHueFade[i] > alphabetLightNumColor[i]) {alphabetLightHueFade[i]--;}
        }
        if (alphabetLightSatFade[i] != sat) {
          if (alphabetLightSatFade[i] > sat) {alphabetLightSatFade[i]--;}
          else if (alphabetLightSatFade[i] < sat) {alphabetLightSatFade[i]++;}
        }
        if (alphabetLightBrightFade[i] != bright) {
          if (alphabetLightBrightFade[i] > bright) {alphabetLightBrightFade[i]--;}
          else if (alphabetLightBrightFade[i] < bright) {alphabetLightBrightFade[i]++;}
        }
        alphabetLEDs[i] = CHSV(alphabetLightHueFade[i], alphabetLightSatFade[i], alphabetLightBrightFade[i]);
      }
    #endif
    FastLED.show();
    delay(10);
  }
}


void dspLetter(char msgChar) {
  int arrayVal = (int) msgChar - 65;
  #if defined (verbose) 
    Serial.print("Displaying: "); Serial.println(char2leds[arrayVal].letter);
  #endif
  alphabetLEDs[char2leds[arrayVal].led] = CHSV(alphabetLightNumColor[arrayVal], sat, bright); FastLED.show();
  #if defined (randomTiming)
    int randOnTime = random(500,1500); 
    delay(randOnTime);
  #else
    delay(1000);
  #endif
  alphabetLEDs[char2leds[arrayVal].led] = CHSV(alphabetLightNumColor[arrayVal], sat, 0); FastLED.show();
  #if defined (randomTiming)
    int randOffTime = random(50,150); 
    delay(randOffTime);
  #else
    delay(100);
  #endif
}


void brightness(int b) {
  bright = b; FastLED.setBrightness(bright);
  Serial.print(F("Setting brightness to: ")); Serial.println(bright);
}


void flash() {
  for (int n=0; n < 10; n++) {
    for (int i=0; i < (alphabetNumLEDs); i++) { alphabetLEDs[i] = CHSV(alphabetLightNumColor[i], sat, bright);};
    #if defined (ceilingEnabled)
      for (int i=0; i < (ceilingNumLEDs); i++) {ceilingLEDs[i] = CHSV(ceilingLightNumColor[i], sat, bright);};
    #endif
    int randTime = random(50, 150);
    FastLED.show();
    #if defined (nonBlockingDisplay)
      millisDelay(randTime); 
    #else
      delay(randTime);
    #endif
    for (int i=0; i < (alphabetNumLEDs); i++) { alphabetLEDs[i] = CHSV(alphabetLightNumColor[i], sat, 0);};
    #if defined (ceilingEnabled)
      for (int i=0; i < (ceilingNumLEDs); i++) { ceilingLEDs[i] = CHSV(ceilingLightNumColor[i], sat, 0);};
    #endif
    FastLED.show();
    delay(50);
  }
}


void creep(int startLED, int endLED) {
  bool forward;
  if (startLED <= endLED) { forward = true; } else { forward = false; }
  if (forward = true) {
    for (int i=startLED; i < endLED; i++) { 
      #if defined (ceilingEnabled)
        ceilingLEDs[i] = CHSV(ceilingLightNumColor[i], sat, bright);
      #else
        alphabetLEDs[i] = CHSV(alphabetLightNumColor[i], sat, bright);
      #endif
      FastLED.show();
      delay(250);
    }
    #if defined (ceilingEnabled)
      for (int i=startLED; i < endLED; i++) {ceilingLEDs[i] = CHSV(ceilingLightNumColor[i], sat, 0);}; 
    #else
      for (int i=startLED; i < endLED; i++) {alphabetLEDs[i] = CHSV(alphabetLightNumColor[i], sat, 0);}; 
    #endif
  }
  else if (forward = false) {
    for (uint16_t i=startLED; i > endLED; i--) { 
      #if defined (ceilingEnabled)
        ceilingLEDs[i] = CHSV(ceilingLightNumColor[i], sat, bright);
      #else
        alphabetLEDs[i] = CHSV(alphabetLightNumColor[i], sat, bright);
      #endif
      FastLED.show();
      delay(250);
    }
    #if defined (ceilingEnabled)
      for (int i=startLED; i > endLED; i--) {ceilingLEDs[i] = CHSV(ceilingLightNumColor[i], sat, 0);}; 
    #else
      for (int i=startLED; i > endLED; i-- {alphabetLEDs[i] = CHSV(alphabetLightNumColor[i], sat, 0);}; 
    #endif
  }
  FastLED.show();
}


void interfaces() {
  //if ( dspActive == false ) {
  /*if (millis() >= (vuTimeDelay + 3000)) {
    for (int v = 0; v <= 10; v++) {vuLEDs[disp2leds[v].ledNum] = CHSV(0, 255, 0);}
    FastLED.show();
  }*/
  if(Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    dspActive = true;
    if(cmd.equals("0")) { onOff(true); }
    else if(cmd.equals("1")) { onOff(false); }
    else if(cmd.startsWith("bright")) { String cmdbright = cmd.substring(7, 10); brightness(cmdbright.toInt()); }
    else if(cmd.startsWith("+")) { 
      #if defined (max9744Enabled)
        if (max9744Vol < 63) {max9744Vol++;}; setVolume(max9744Vol);
      #else
        Serial.println("MAX9744 Amp not enabled!");
      #endif
    }
    else if(cmd.startsWith("-")) { 
      #if defined (max9744Enabled)
        if (max9744Vol > 0) {max9744Vol--;}; setVolume(max9744Vol);
      #else
        Serial.println("MAX9744 Amp not enabled!");
      #endif
    }
    else {
      fadeOut(1000);
      #if defined (verbose) 
        Serial.print("Received msg: "); Serial.println(cmd);
      #endif
      cmd.toUpperCase();
      for (unsigned int n = 0; n < cmd.length(); n++) {
        if(cmd[n] == '!') { 
          #if defined (verbose) 
            Serial.println("Flashing"); flash();
          #endif
        }
        if(cmd[n] == '*') { 
          #if defined (verbose) 
            Serial.println("Creeping"); creep(0,alphabetNumLEDs);
          #endif
        }
        else if (cmd[n] == ' ') { 
          #if defined (verbose) 
            Serial.println("SPACE"); delay(1100);
          #endif
        }
        else {
          if ( dspActive == false ) {
            for (auto &alphabetArray : char2leds) {
              if (alphabetArray.letter == cmd[n]) {
                #if defined (debug) 
                  Serial.print("alphabetArray.letter = "); 
                  Serial.print(alphabetArray.letter); 
                  Serial.print(" n = "); Serial.print(n);
                  Serial.print(" cmd[n] = "); Serial.println(cmd[n]);
                #endif
                dspLetter(cmd[n]);
              }
            }
          }
        }
      }
      fadeIn(1000);
    }
  }
  else {
    dspActive = false;
  }
  cmd = "";
  //}
}


void setLightStringColor() {
  int c = 0;
  for (int a=0; a < (alphabetNumLEDs); a++) {
      if ( c > 4 ) { c = 0; }
      alphabetLightNumColor[a] = lightPtn[c];
      #if defined (verbose) 
        Serial.print("LED Number: "); Serial.print(a); 
        Serial.print(" alphabetLightNumColor: ("); Serial.print(alphabetLightNumColor[a]);
        Serial.print(","); Serial.print(sat); Serial.print(","); Serial.print(bright); Serial.println(")");
      #endif
      c++;
  }
  #if defined (ceilingEnabled)
    c = 0;
    for (int a=0; a < (ceilingNumLEDs); a++) {
        if ( c > 4 ) { c = 0; }
      ceilingLightNumColor[a] = lightPtn[c];
      #if defined (verbose) 
        Serial.print("LED Number: "); Serial.print(a); 
        Serial.print(" ceilingLightNumColor: ("); Serial.print(ceilingLightNumColor[a]);
        Serial.print(","); Serial.print(sat); Serial.print(","); Serial.print(bright); Serial.println(")");
      #endif
      c++;
  }
  #endif
}


void predefined_msgs() {
  #if defined (verbose) 
    Serial.println("predefined_msgs");
  #endif
  fadeOut(1000);
  #if defined (debug)
    Serial.print("randMsg: "); 
  #endif
  for (int n = 0; n < random((msgArrayNum*2),(msgArrayNum*msgArrayNum)); n++) { 
    randMsg = random(0,msgArrayNum);
    #if defined (debug)
      Serial.print(randMsg); Serial.print(" ");
    #endif
  }
  #if defined (debug)
    Serial.println("");
    Serial.print("prevRandMsg: "); Serial.print(prevRandMsg);
    Serial.print(" prev2RandMsg: "); Serial.print(prev2RandMsg);
    Serial.print(" randMsg: "); Serial.println(randMsg);
  #endif
  if (randMsg == prev2RandMsg || randMsg == prevRandMsg) {
    while ( randMsg == prev2RandMsg || randMsg == prevRandMsg ) { 
      randMsg = random(0,msgArrayNum); 
      #if defined (debug)
        Serial.print("prevRandMsg: "); Serial.print(prevRandMsg);
        Serial.print(" prev2RandMsg: "); Serial.print(prev2RandMsg);
        Serial.print(" randMsg: "); Serial.println(randMsg);
      #endif
    } 
  }
  String msg = msgArray[randMsg];
  #if defined (verbose) 
    Serial.print("Predefined msg: "); Serial.println(msg);
  #endif
  msg.toUpperCase();
  for (unsigned int n = 0; n < msg.length(); n++) {
    if(msg[n] == '!') { Serial.println("Flashing"); flash(); }
    if(msg[n] == '*') { Serial.println("Creeping"); creep(0,alphabetNumLEDs); }
    else if (msg[n] == ' ') { Serial.println("SPACE"); delay(1100); }
    else {
      for (auto &alphabetArray : char2leds) {
        if (alphabetArray.letter == msg[n]) {
          dspLetter(msg[n]);
        }
      }
    }
  }
  prev2RandMsg = prevRandMsg; prevRandMsg = randMsg;
  delay(1100);
  fadeIn(1000);
}

#if defined (vs1053Enabled)
void joyceRun() {
  #if defined (verbose) 
    Serial.println("joyceRun");
  #endif
  fadeOut(1000);
  #if defined (fake_vs1053Enabled)
    Serial.println("Fake playing Joyce.mp3");
  #else
    musicPlayer.startPlayingFile("/Joyce.mp3");
  #endif
  //now = millis();
  //creep();
  //int d=11268; while(millis() < now + d) { now += now + d; };
  delay(11268);
  dspLetter('R');delay(3197); dspLetter('I');delay(897); dspLetter('G');delay(558); dspLetter('H'); dspLetter('T');delay(401);
  dspLetter('H'); delay(256); dspLetter('E'); dspLetter('R'); delay(338); dspLetter('E'); delay(12926);
  dspLetter('R'); delay(1030);  dspLetter('U'); delay(2240);  dspLetter('N'); delay(2000);
  for (int i = 0; i < 10; i++) { flash(); };
  #if defined (fake_vs1053Enabled)
    Serial.println("waiting for fake playing to end");
  #else
    while (!musicPlayer.stopped()) { EVERY_N_MILLISECONDS(100) { interfaces(); } };
  #endif
  #if defined (verbose)
    Serial.println("joyceRun finished");
  #endif
  fadeIn(1000);
}


void theClash() {
  #if defined (verbose)
    Serial.println("theClash");
  #endif
  fadeOut(1000);
  #if defined (fake_vs1053Enabled)
    Serial.println("Fake playing Clash.mp3");
  #else
    musicPlayer.startPlayingFile("/Clash.mp3");
    while (!musicPlayer.stopped()) { EVERY_N_MILLISECONDS(100) { interfaces(); } };
  #endif
  #if defined (verbose) 
    Serial.println("theClash finished");
  #endif
  fadeIn(1000);
}


void theme() {
  #if defined (verbose) 
    Serial.println("theme");
  #endif
  unsigned long themeStart;
  unsigned long beatStart;
  unsigned long beatEnd;
  fadeToRed();
  #if defined (fake_vs1053Enabled)
    Serial.println("Fake playing Theme.mp3");
  #else
    musicPlayer.startPlayingFile("/STTheme.mp3");
  #endif
  themeStart = millis();
  while (millis() < (themeStart + 5747)) { }      // first beat at 5.747s
  for (int c=0; c < 52; c++) {                    // 38 beats before pause @ 32.891s, 14 before halftime
    beatStart = millis();                         // measure how long the heartBeat function runs
    heartBeat(2);                                 // heartbeat light effect, timing varies by a few ms
    beatEnd = millis();                           // measure how long the heartBeat function runs
    delay((714 - (beatEnd - beatStart)));         // 714ms is tempo, minus heartBeat function runtime
    if (c == 37) {                                // pause
      heartBeat(1);
      //fade out/fade in
      while (millis() < (themeStart + 42890)) { } // beats resume at 42.891s
    }
  }
  while (millis() < (themeStart + 52891)) { }     // alternate between 1 and 2 beats at halftime tempo
  for (int c=0; c < 3; c++) { 
    for (int b=1; b < 3; b++) {
      beatStart = millis();
      heartBeat(b); 
      beatEnd = millis();
      delay(((714*2) - (beatEnd - beatStart)));   // beats go into halftime, or 714ms * 2
    }
  };
  #if defined (fake_vs1053Enabled)
    Serial.println("Waiting for fake playing to complete");
  #else
    while (!musicPlayer.stopped()) { EVERY_N_MILLISECONDS(100) { interfaces(); } };
  #endif
  #if defined (verbose) 
    Serial.println("Theme finished");
  #endif
  fadeToPattern();
}


void soundTrack() {
  #if defined (verbose) 
    Serial.println("soundTrack");
  #endif
  #if defined (debug)
    Serial.print("randST: "); 
  #endif
  for (int n = 0; n < random((stNum*2),(stNum*stNum)); n++) { 
    randST = random(0,stNum);
    #if defined (debug)
      Serial.print(randST); Serial.print(" ");
    #endif
  }
  #if defined (debug)
    Serial.println("");
    Serial.print("prevRandST: "); Serial.print(prevRandST);
    Serial.print(" prev2RandST: "); Serial.print(prev2RandST);
    Serial.print(" prev3RandST: "); Serial.print(prev3RandST);
    Serial.print(" randST: "); Serial.println(randST);
  #endif
  if (randST == prev2RandST || randST == prevRandST || randST == prev3RandST) {
    while ( randST == prev2RandST || randST == prevRandST || randST == prev3RandST) { 
      randST = random(0,stNum); 
      #if defined (debug)
        Serial.print("prevRandST: "); Serial.print(prevRandST);
        Serial.print(" prev2RandST: "); Serial.print(prev2RandST);
        Serial.print(" prev3RandST: "); Serial.print(prev3RandST);
        Serial.print(" randST: "); Serial.println(randST);
      #endif
    } 
  }
  #if defined (fake_vs1053Enabled)
    Serial.print("Fake ");
  #endif
  #if defined (verbose) 
    Serial.print("Playing "); Serial.println(ST[randST]); 
  #endif
  #if ! defined (fake_vs1053Enabled)
    if ( randST == 0 ) { musicPlayer.startPlayingFile("/KateBush.mp3"); };
    if ( randST == 1 ) { musicPlayer.startPlayingFile("/NeverEndingStory.mp3"); };
    if ( randST == 2 ) { musicPlayer.startPlayingFile("/Journey.mp3"); };
    if ( randST == 3 ) { musicPlayer.startPlayingFile("/MoP.mp3"); };
    if ( randST == 4 ) { theClash(); };
    if ( randST == 5 ) { theme(); };
  #endif
  prev3RandST = prev2RandST; prev2RandST = prevRandST; prevRandST = randST;
  //fadeOut(1000);
  #if defined (fake_vs1053Enabled)
    Serial.println("Waiting for fake playback");
  #else
    while (!musicPlayer.stopped()) { EVERY_N_MILLISECONDS(100) { interfaces(); } };
  #endif
  #if defined (verbose) 
    Serial.print(ST[randST]); Serial.println(" finished");
  #endif
  //fadeIn(1000);
}


void printDirectory(File dir, int numTabs) {    // File listing helper
   while(true) {
     File entry =  dir.openNextFile();
     if (! entry) { break; }                    // no more files
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {                                   // files have sizes, directories do not
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
     entry.close();
   }
}
#endif

void setup() {
  Serial.begin(9600);
  delay(2000);
  pinMode (LED_BUILTIN, OUTPUT);  // used as heartbeat indicator
  FastLED.addLeds<WS2811,alphabetDataPin,RGB>(alphabetLEDs,alphabetNumLEDs).setCorrection( TypicalPixelString ); 
  #if defined (ceilingEnabled)
    FastLED.addLeds<WS2812,ceilingDataPin,RGB>(ceilingLEDs,ceilingNumLEDs).setCorrection( TypicalPixelString ); 
  #endif
  //-------------VS1053 Startup sequence------------------
  #if defined (vs1053Enabled)
    #if defined (fake_vs1053Enabled)
      Serial.println("Pretending vs1053 is present & enabled");
    #else
      if (! musicPlayer.begin()) { Serial.println("Couldn't find VS1053"); while (1); }
      else { 
        #if defined (verbose) 
          Serial.println("VS1053 found");
        #endif
      }
      if (!SD.begin(CARDCS)) { Serial.println("SD failed!"); while (1); }
      else { 
        #if defined (verbose) 
          Serial.println("SD OK!"); printDirectory(SD.open("/"), 0);
        #endif
      }
      Serial.println("SD card defined successfully");
      musicPlayer.setVolume(10,10);
      musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);
    #endif
  #endif
  //-------------MAX9744 Startup sequence-----------------
  #if defined (max9744Enabled)
    Wire.begin();
      if (! setVolume(max9744Vol)) {
        Serial.println("Failed to set volume, MAX9744 not found!");
        //while (1);
      }
  #endif
  //------------------------------------------------------
  setLightStringColor();
  fadeIn(1000);
  delay(5000);
  #if defined (demoEnabled)
    #if defined (vs1053Enabled)
      theme();
      delay(5000);
      joyceRun();
    #endif
    for (int i; i < 4; i++) {
      delay(5000);
      predefined_msgs();
    }
  #else
    #if defined (vs1053Enabled)
      theme();
      prevRandNum = 2;
    #endif
  #endif
}

void setup1() {
  //----------- Rotary Encoder initialization ------------
  #if defined (encoder1)
    pinMode(RE1_CLK,INPUT_PULLUP); attachInterrupt(RE1_CLK, readEncoder, CHANGE); // Rotary Encoder 1 CLK
    //pinMode(RE1_DT,INPUT); attachInterrupt(4, updateEncoder, CHANGE);       // Rotary Encoder 1 DT
    pinMode(RE1_SW,INPUT_PULLUP); attachInterrupt(RE1_SW, readEncoder, CHANGE);  // Rotary Encoder 1 SW
    RE1_lastStateCLK = digitalRead(RE1_CLK);
  //----------- VU Meter Initialization ------------------
    FastLED.addLeds<WS2812,vuDataPin,GRB>(vuLEDs,vuNumLEDs).setCorrection( TypicalPixelString );
    for (int v = 0; v <= 10; v++) {vuLEDs[disp2leds[v].ledNum] = CHSV(0, 255, vuBright); FastLED.show(); delay(100);}
    for (int v = 0; v <= 10; v++) {vuLEDs[disp2leds[v].ledNum] = CHSV(0, 255, 0);}; delay(1000); FastLED.show(); 
    //for (int i = 0; i < vuNumLEDs; i++) {vuLEDs[i] = CHSV(0, 255, 255); Serial.print("Lighting up VU "); Serial.println(i); FastLED.show(); delay(100);}
  #endif
}

void loop1() {
  EVERY_N_MILLISECONDS(3000) { 
    for (int v = 0; v <= 10; v++) {
      vuLEDs[disp2leds[v].ledNum] = CHSV(0, 255, 0);
    }
    FastLED.show();
  }
}

void loop() {
  EVERY_N_MILLISECONDS(100) { unsigned long currentTime = millis(); digitalWrite(LED_BUILTIN, (currentTime / 500) % 2); }
  EVERY_N_MILLISECONDS(100) { interfaces(); }
  /*EVERY_N_MILLISECONDS(3000) { 
    for (int v = 0; v <= 10; v++) {
      vuLEDs[disp2leds[v].ledNum] = CHSV(0, 255, 0);
    }
    FastLED.show();
  }*/
  EVERY_N_MILLISECONDS(1011) { 
    if ( dspActive == false ) { 
      inactiveTimer++;
      #if defined (debug) 
        Serial.print("inactiveTimer = "); Serial.println(inactiveTimer);
      #endif
    } 
    if ( dspActive == true ) { inactiveTimer = 0; }
  }
  if ( inactiveTimer >= iTimer ) {
    dspActive = true;
    #if defined (verbose) 
      Serial.println("Random Event reached");
    #endif
    #if defined (vs1053Enabled)
      #if defined (debug)
        Serial.print("randNum: "); 
      #endif
      for (int n = 0; n < random((eventNum*2),(eventNum*eventNum)); n++) { 
        randNum = random(0,eventNum);
        #if defined (debug)
          Serial.print(randNum); Serial.print(" ");
        #endif
      }
      #if defined (debug)
          Serial.println("");
          Serial.print("prevRandNum: "); Serial.print(prevRandNum);
          Serial.print(" prev2RandNum: "); Serial.print(prev2RandNum);
          Serial.print(" randNum: "); Serial.println(randNum);
        #endif
      //if ( ((randNum != 1 && randNum != 2 && randNum != 4) && (randNum == prev2RandNum))
        //|| ((randNum == 1 || randNum == 2 || randNum == 4) && (randNum == prevRandNum || randNum == prevRandNum)) ) {
      if ( (randNum != 0) && ((randNum == prev2RandNum) || (randNum == prevRandNum))) {
        while ( (prev2RandNum == randNum) || (prevRandNum == randNum) ) { 
          randNum = random(0,eventNum); 
          #if defined (debug)
            Serial.print("prevRandNum: "); Serial.print(prevRandNum);
            Serial.print(" prev2RandNum: "); Serial.print(prev2RandNum);
            Serial.print(" randNum: "); Serial.println(randNum);
          #endif
        }
      }
      if ( randNum == 0 ) { predefined_msgs(); }
      if ( randNum == 1 ) { joyceRun(); }
      //if ( randNum == 2 ) { theClash(); }
      if ( randNum == 2 ) { soundTrack(); }
      //if ( randNum == 4 ) { theme();}
      prev2RandNum = prevRandNum; prevRandNum = randNum;
    #else
      predefined_msgs();
    #endif
    dspActive = false;
    inactiveTimer = 0;
  }
  FastLED.show();
}
