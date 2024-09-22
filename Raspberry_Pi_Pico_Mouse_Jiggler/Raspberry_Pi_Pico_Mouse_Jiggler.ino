#include <Mouse.h>
int d = 20;                             // "Distance", how far to move per jiggle
int v = 1;                              // "Velocity", number of pixels to move at a time
int t = 2;                              // "Time", number of times to jiggle
int r = 7;                              // "Rate", Delay in ms to wait in between movements
long dTime = 30000;                     // "Delay Time", time in ms to wait until jiggling again

void setup() {
  Mouse.begin();                        // Start the Mouse library
}

void loop() {
  for ( int i = 1; i <= t; i++) {       // For loop repeats movement action according to "t"
    for (int x = 0; x <= d; x++) {      // For loop acheives the movement distance according to "d"
      Mouse.move(-v, 0);                // Moving the mouse to the left "v" number of pixels at a time
      delay(r);                         // Delay to make the mouse movement per "v" pixels slower
    }
    for (int x = 0; x <= d; x++) {      // For loop acheives the movement distance according to "d"
      Mouse.move(v, 0);                 // Moving the mouse to the right "v" number of pixels at a time
      delay(r);                         // Delay to make the mouse movement per "v" pixels slower
    }
  }
  delay(dTime);                         // Wait to jiggle the mouse again
}
