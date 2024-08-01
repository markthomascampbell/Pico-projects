#include <Mouse.h>                        // Load mouse/HID library
//#define debugEnabled                    // Uncomment to enable debug over serial
int w = 20;                               // How many pixels' width do you want the cursor to move?
int p = 1;                                // How many pixels at a time do you want the cursor to move? a speed multiplier
int d = 7;                                // How fast do you want the cursor to move in milliseconds? lower = faster
int t = 2;                                // How many times do you want the cursor to move back and forth?
int f = 30000;                            // The frequency in moving the cursor in milliseconds

void setup() {
  #if defined (debugEnabled)
    Serial.begin(9600);                   // Enable Serial if debug is enabled
  #endif
  Mouse.begin();                          // Enable Mouse functionality
}

void loop() {
  #if defined (debugEnabled)
    Serial.println("Jiggle, Jiggle!");    // If debug enabled, print serial message during every jiggle
  #endif
  for ( int i = 1; i <= t; i++) {         // Number of times to repeat the whole process defined by t
    for (int x = 0; x <= w; x+p) {        // Repeat the process of moving "p" pixel(s) at a time until "w" pixels reached
      Mouse.move(-p, 0);                  // Move left "p" pixel(s)
      delay(d);                           // How long to wait until we move to the next pixel? Delay determines apparent speed of cursor
    }
    for (int x = 0; x <= w; x+p) {        // Repeat the process of moving "p" pixel(s) at a time until "w" pixels reached
      Mouse.move(p, 0);                   // Move right "p" pixel(s)
      delay(d);                           // How long to wait until we move to the next pixel? Delay determines apparent speed of cursor
    }
  }
  delay(f);                               // Delay determines how long to wait until next cursor jiggle
}
