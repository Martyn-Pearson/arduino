#include <DCC_Decoder.h>
#define NUMACCESSORIES 2 // Enter the number of accessories here

typedef struct DCCAccessoryData {
  int   address;   // User Configurable DCC address
  byte  outputpin; // User Configurable Arduino pin
  byte  dccstate;  // Internal use DCC state of accessory, 1=on, 0=off
};
DCCAccessoryData accessory[NUMACCESSORIES];

// The DCC library calls this function to set / reset accessories
void BasicAccDecoderPacket_Handler(int address, boolean activate, byte data) {
  address -= 1;
  address *= 4;
  address += 1;
  address += (data & 0x06) >> 1;
  // address = address - 4; // uncomment this line for Roco Maus or z21
  boolean enable = (data & 0x01) ? 1 : 0;
  for (byte i=0; i<NUMACCESSORIES; i++) {
    if (address == accessory[i].address) {
      if (enable) accessory[i].dccstate = 1;
      else accessory[i].dccstate = 0;
    }
  }
}

void setup() { 
// CONFIGURATION OF ACCESSORIES
// Copy & Paste as many times as you have accessories 
// The amount must be same as NUMACCESSORIES
// Don't forget to increment the array index
  accessory[0].address   =  1; // DCC address
  accessory[0].outputpin = 13; // Arduino pin

  accessory[1].address   =  2; // DCC address
  accessory[1].outputpin = 12; // Arduino pin
// END OF CONFIGURATION OF ACCESSORIES

  DCC.SetBasicAccessoryDecoderPacketHandler(BasicAccDecoderPacket_Handler, true);
  DCC.SetupDecoder( 0x00, 0x00, 0 );

  for(byte i=0; i<NUMACCESSORIES; i++) {
    pinMode     (accessory[i].outputpin, OUTPUT);
    digitalWrite(accessory[i].outputpin, LOW);
  }
}

void loop() {
  static int n = 0;
  DCC.loop(); // Call the DCC library to read incoming DCC data
  if( ++n >= NUMACCESSORIES ) n = 0; // Next address to test
  if (accessory[n].dccstate)
    digitalWrite(accessory[n].outputpin, HIGH);
  else
    digitalWrite(accessory[n].outputpin, LOW);
  }
