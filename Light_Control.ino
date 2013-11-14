#include <Wire.h>

#define    LBASS  3
#define    RBASS  5
#define    LTREB  6
#define    RTREB  9

byte state;

void setup() {
  Serial.begin(57600);

  pinMode(RBASS, OUTPUT);
  pinMode(LBASS, OUTPUT);
  pinMode(LTREB, OUTPUT);
  pinMode(RTREB, OUTPUT);
  
  analogWrite(LBASS, 0);
  analogWrite(RBASS, 0);
  analogWrite(LTREB, 0);
  analogWrite(RTREB, 0);
  
  state = 0;
}

void loop() {
  int r;
  while( (r = Serial.read()) != -1) {
    if(r == 0 || r == '0')
      state = -1;
    
    state++;
    
    switch(state) {
      case 0:
      break;
      
      case 1:
        analogWrite(LBASS, (r==1) ? 0 : r);
      break;
      
      case 2:
        analogWrite(RBASS, (r==1) ? 0 : r);
      break;
      
      case 3:
        analogWrite(LTREB, (r==1) ? 0 : r);
      break;
      
      case 4:
        analogWrite(RTREB, (r==1) ? 0 : r);
      break;
      
      default:
        Serial.print("ERROR: ");
        Serial.println(state);
      break;
    }
  }
}
