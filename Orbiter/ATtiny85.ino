#include "TinyWireS.h"

#define SLAVE_ADDR 0x50
int i=0;
byte bb[4] ;

void intToByte ( int i){
 bb[0]= (i >> 24) & 0xFF;
 bb[1]= (i >> 16) & 0xFF;
 bb[2]= (i >> 8) & 0xFF;
 bb[3]= i  & 0xFF;
}


void setup()
{
  delay(5000);
  TinyWireS.begin(SLAVE_ADDR);
  delay(1000);
  TinyWireS.onRequest(requestEvent);
}


void loop()
{
}

void requestEvent()
{
  i=analogRead(A3);
  intToByte(i);
  TinyWireS.send(bb[0]);
  TinyWireS.send(bb[1]);
  TinyWireS.send(bb[2]);
  TinyWireS.send(bb[3]);
}
