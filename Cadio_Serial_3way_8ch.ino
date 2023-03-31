// in this project we have arduino board conncted with esp8266
// esp8266 has cadio firmware and software serial option is actived
// this board is based on 3way switches so we are using optpcouples to know loads current status
// on arduino board we have 8 optocouplers and 8 relays and cooling fan
// wiring :
// 8266(gpio 4) -> arduino(rx)
// 8266(gpio 5) -> arduino(tx)

#include <CadioSerial.h> //include CadioSerial library
CadioSerial cadio; //create instance

int relay[8] = {2, 3, 4, 5, 6, 7, 8, 9}; //output pins
int opto[8] = {10, 11, 12, 13, A0, A1, A2, A3};  //optocouplers pins

int fan = A4; //fan pin

byte optoLast[8]; //for saving last optocouplers status

bool isOptoReversed = false; //if optocoupler HIGH means OFF make it true

void cadioOrder(int index, int value) { //new order form esp
  if ((!isOptoReversed && ((value > 0 && !optoLast[index]) || (value == 0 && optoLast[index]))) ||
      (isOptoReversed && ((value > 0 && optoLast[index]) || (value == 0 && !optoLast[index])))) { //forward only if needed
    digitalWrite(relay[index], !digitalRead(relay[index])); //change relay status
  }
}

void fanOrder(int value) { //new fan order form esp
  if (value > 0) {
    digitalWrite(fan, HIGH); //change fan status
  } else {
    digitalWrite(fan, LOW); //change fan status
  }
}

void opto_check() { //check real optocouplers changes after ignoring noise
  for (int i = 0; i < 8; i++) {
    if (digitalRead(opto[i]) != optoLast[i]) {
      bool confirm = true;
      unsigned long deNoise = millis();
      while (millis() - deNoise < 100) {
        if (digitalRead(opto[i]) == optoLast[i]) {
          confirm = false;
          break;
        }
      }
      if (confirm) {
        optoLast[i] = digitalRead(opto[i]);
        opto_changed(i);
      }
    }
  }
}

void opto_changed(int i) { //optocoupler status changed
  int optoValue;
  if (isOptoReversed) {
    optoValue = (!optoLast[i]) ? 255 : 0;
  } else {
    optoValue = (optoLast[i]) ? 255 : 0;
  }
  cadio.set(i, optoValue); //send new status to esp
}

void setup() {
  for (int i = 0; i < 8; i++) {
    pinMode(relay[i], OUTPUT); //set relays pinMode
    pinMode(opto[i], INPUT); //set optocouplers pinMode
    optoLast[i] = digitalRead(opto[i]); //save last optocouplers digitalRead
  }
  pinMode(fan, OUTPUT); //set fan pinMode

  cadio.begin(); //start cadio library
  cadio.onOrder(cadioOrder); //get orders
  cadio.onFanOrder(fanOrder); //get fan orders
  cadio.getCurrentStatus(); //request all current status from esp
}

void loop() {
  opto_check(); //check optocouplers status
  cadio.loop(); //check for cadio orders
}
