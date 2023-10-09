// 
#include <Mouse.h>
int d = 20;
int s = 1; 
int t = 2;

void setup() {
  //Serial.begin(9600);
  Mouse.begin();
}

void loop() {
  //Serial.println("Jiggle, Jiggle!");
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
  delay(30000);
}
