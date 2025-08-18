#include <Mouse.h>
#include <Keyboard.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
//#include <GeoIP.h>

// Declarations
//location_t loc;
int d = 20;                                   // "Distance", how far to move per jiggle
int s = 1;                                    // "Speed", number of pixels to move at a time
int t = 2;                                    // "Time", number of times to jiggle
int timeout = 0;                              // Timeout for connecting to wifi before disabling wifi features
int dayStartHour = 6;                         // Starting hour in local time
int dayStartMin = 28;                         // Starting minute in local time
int dayStartSec = 0;                          // Starting second in local time
int dayEndHour = 15;                          // Ending hour in local time
int dayEndMin = 0;                            // Ending minute in local time
int dayEndSec = 0;                            // Ending second in locaal time
int currentHour = 0;                          // Variable for current hour
int currentMinute = 0;                        // Variable for current minute
int currentSecond = 0;                        // Variable for current second
int currentDay = 0;                           // Variable for current day of the week
bool wifiEnabled = false;                     // If connected to wifi, this variable will get set to enabled
uint32_t ntpTimeRefresh = 0;                  // Counter for how long since NTP Time data was last refreshed
uint32_t jiggleFreq = 30000;                  // How often you want the mouse to jiggle in ms
uint32_t lastJiggle = 0;                      // Counter for how long since jiggle last occurred
//uint32_t dayMins = 0;                        // Conversion for daySecs
uint32_t daySecs = 0;                         // Counter for seconds elapsed today, easier to measure difference in time
uint32_t dayStartSecs = ( dayStartHour * 3600 ) + ( dayStartMin * 60 ) + dayStartSec;
uint32_t dayEndSecs = ( dayEndHour * 3600 ) + ( dayEndMin * 60 ) + dayEndSec;
// ########## Choose your timezone
  //int32_t tzOffset = 0;                     // UTC
  //long tzOffset = -14400;                   // GMT-4 - EDT
  //long tzOffset = -18000;                   // GMT-5 - CDT / EST
  long tzOffset = -21600;                     // GMT-6 - MDT / CST
  //int32_t tzOffset = -25200;                // GMT-7 - PDT / MST
  //int32_t tzOffset = -28800;                // GMT-8 - PST
//long tzMap[] = { 0, -14400, -18000, -21600, -25200, -28800 };
//long tz = 0;
//char *tzName = "UTC";
// ############## Sensitive Credentials Section #####################
//const char *ssid = "YOUR_SSID";               // Wifi Network
//const char *password = "YOUR_PASSWORD";       // Wifi Password
//const char *loginPW = "YOUR_LOGIN_PASSWORD";  // System password signs in automatically at start time or with press of BootSel
const char *ssid = "Go Away_IoT"; //"YOUR_SSID";
const char *password = "1 l0v3 K1mb3rly C@mpb3ll"; //"YOUR_PASSWORD";
const char *loginPW = "Mari@ie321"; //"YOUR_LOGIN_PASSWORD";
// ############## Sensitive Credentials Section #####################
const char *macToString(uint8_t mac[6]) {
  static char s[20];
  sprintf(s, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return s;
}
const char *encToString(uint8_t enc) {
  switch (enc) {
   case ENC_TYPE_NONE: return "NONE";
   case ENC_TYPE_TKIP: return "WPA";
   case ENC_TYPE_CCMP: return "WPA2";
   case ENC_TYPE_AUTO: return "AUTO";
  }
    return "UNKN";
}
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");


void flash(int rep, uint32_t ms) {
  for ( int i = 1; i <= rep; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay (ms);
    digitalWrite(LED_BUILTIN, LOW);
    delay (ms);
  }
}


void wifiScan() {
  Serial.printf("Beginning scan at %lu\n", millis());
  auto cnt = WiFi.scanNetworks();
  if (!cnt) {
    Serial.printf("No networks found\n");
  } else {
    Serial.printf("Found %d networks\n", cnt);
    Serial.printf("%32s %5s %17s %2s %4s\n", "SSID", "ENC", "BSSID        ", "CH", "RSSI");
    for (auto i = 0; i < cnt; i++) {
      uint8_t bssid[6];
      WiFi.BSSID(i, bssid);
      Serial.printf("%32s %5s %17s %2d %4ld\n", WiFi.SSID(i), encToString(WiFi.encryptionType(i)), macToString(bssid), WiFi.channel(i), WiFi.RSSI(i));
    }
  }
}


void wifiStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("Pico W is connected to WiFi network ");
    Serial.println(WiFi.SSID());
    Serial.print("Assigned IP Address: ");
    Serial.println(WiFi.localIP());
    wifiEnabled = true;
  } else {
    Serial.println("Pico W is not connected to WiFi!");
    wifiEnabled = false;
  }
}


void mouseJiggle() {
  digitalWrite(LED_BUILTIN, HIGH);
  for ( int i = 1; i <= t; i++) {
    for (int x = 0; x <= d; x++) {
      Mouse.move(-s, 0);
      delay(7);
    }
    for (int x = 0; x <= d; x++) {
      Mouse.move(s, 0);
      delay(7);
    }
  }
  lastJiggle = millis();
  digitalWrite(LED_BUILTIN, LOW);
}


void mouseMoveToLeftCorner() {
  digitalWrite(LED_BUILTIN, HIGH);
  for ( int i = 1; i <= 100; i++) { Mouse.move(-30, -30); }
  digitalWrite(LED_BUILTIN, LOW);
}


void signIn() {
  digitalWrite(LED_BUILTIN, HIGH);
  for ( int i = 1; i <= 4; i++) {
    Keyboard.press(KEY_LEFT_CTRL);
    delay(200);
    Keyboard.releaseAll();
    delay(300);
  }
  delay(750);
  Keyboard.println(loginPW);
  digitalWrite(LED_BUILTIN, LOW);
  }


void timeUpdate() {
  timeClient.update();
  ntpTimeRefresh = millis();
  String formattedTime = timeClient.getFormattedTime();
  currentHour = timeClient.getHours();
  currentMinute = timeClient.getMinutes();
  currentSecond = timeClient.getSeconds();
  currentDay = timeClient.getDay();
  //dayMins = ( currentHour * 60 ) + currentMinute;
  //daySecs = ( dayMins * 60 ) + currentSecond;
  daySecs = ( ( ( currentHour * 60 ) + currentMinute ) * 60 ) + currentSecond;
}


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(9600);
  Mouse.begin();
  Keyboard.begin();
  WiFi.mode(WIFI_STA);
  delay(500);
  wifiScan();
  WiFi.begin(ssid, password);
  while ( (WiFi.status() != WL_CONNECTED) && (timeout < 10) ) {
    Serial.print(".");
    flash(1, 500);
    timeout++;
  }
  wifiStatus();
  if ( wifiEnabled == true ) {
    //GeoIP geoip;
    //unsigned long tzOffset = 
    timeClient.begin();
    timeClient.setTimeOffset(tzOffset);
  }
}


void loop() {
  if ( ( millis() >= ntpTimeRefresh + 1000 ) && ( wifiEnabled == true ) ) {
    timeUpdate();
  }

  if ( ( millis() >= ntpTimeRefresh + 10000 ) && ( wifiEnabled == false ) ) {
    flash(3, 100);
    ntpTimeRefresh = millis();
  }

  if ( daySecs == ( dayStartSecs - 60000 ) ) {
    mouseMoveToLeftCorner();
  } 
  else if ( ( daySecs == dayStartSecs ) || (BOOTSEL) ) {
    if (BOOTSEL) { mouseMoveToLeftCorner(); delay(45000); } 
    signIn();
  }
  
  if ( wifiEnabled == true ) {
    if ( ( daySecs > dayStartSecs) && ( daySecs < dayEndSecs ) && ( currentDay > 0 ) && ( currentDay < 6 ) ) {
      if (millis() >= lastJiggle + jiggleFreq) {
        mouseJiggle();
      }
    }
  } else {
    if (millis() >= lastJiggle + jiggleFreq) {
      mouseJiggle();
    } 
  }
}
