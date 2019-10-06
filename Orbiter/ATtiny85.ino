#include "TinyWireS.h"

#define SLAVE_ADDR 0x50

int sol=4,bat=2;
byte bb[8] ;

void intToByte (){
 bb[0]= (bat >> 24) & 0xFF;
 bb[1]= (bat >> 16) & 0xFF;
 bb[2]= (bat >> 8) & 0xFF;
 bb[3]= bat  & 0xFF;
 bb[4]= (sol >> 24) & 0xFF;
 bb[5]= (sol >> 16) & 0xFF;
 bb[6]= (sol >> 8) & 0xFF;
 bb[7]= sol  & 0xFF;
}

void setup()
{
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  TinyWireS.begin(SLAVE_ADDR);
  TinyWireS.onRequest(requestEvent);
}


void loop()
{
  bat=analogRead(A3);
  sol=analogRead(A2);
  intToByte();
  delay(100);
}

void requestEvent()
{
  TinyWireS.send(bb[0]);
  TinyWireS.send(bb[1]);
  TinyWireS.send(bb[2]);
  TinyWireS.send(bb[3]);
  TinyWireS.send(bb[4]);
  TinyWireS.send(bb[5]);
  TinyWireS.send(bb[6]);
  TinyWireS.send(bb[7]);
}
