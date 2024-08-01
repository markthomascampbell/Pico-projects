#include <Mouse.h>
//#define debugEnabled                   // Uncomment to enable debug over serial
int w = 20;                              // How many pixels' width do you want the cursor to move?
int p = 1;                               // How many pixels at a time do you want the cursor to move?
int d = 7;                               // How fast do you want the cursor to move? lower = faster
int t = 2;                               // How many times do you want the cursor to move back and forth?
int dTime = 30000;                       // The delay in between moving the cursor in milliseconds

void setup() {
  #if defined (debugEnabled)
    Serial.begin(9600);
  #endif
  Mouse.begin();
}

void loop() {
  #if defined (debugEnabled)
    Serial.println("Jiggle, Jiggle!");
  #endif
  for ( int i = 1; i <= t; i++) {        // Number of times to repeat the whole process defined by t
    for (int x = 0; x <= w; x++) {       // Repeat the process of moving 1 pixel at a time
      Mouse.move(-s, 0);                 // Move left
      delay(d);                          // Delay determines apparent speed of cursor
    }
    for (int x = 0; x <= w; x++) {       // Repeat the process of moving 1 pixel at a time
      Mouse.move(s, 0);                  // Move right
      delay(d);                          // Delay determines apparent speed of cursor
    }
  }
  delay(dTime);                          // Delay determines how long to wait until next cursor jiggle
}
