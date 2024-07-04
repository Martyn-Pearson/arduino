#include <DCC_Decoder.h>
#include "signals.h"

const int DATAPIN = 3;
const int CLOCKPIN = 4;
const int LATCHPIN = 5;
const int DCC_PIN = 2;

// Signals defined here
ThreeAspectSignal plat1EDepart(0, 60, 61, 62, 63);
SubsidiarySignal plat1Siding(3, 64);
ThreeAspectSignal plat2EDepart(4, 65, 66, 67, 68);
ThreeAspectSignal plat3EDepart(7, 69, 70, 71, 72);
SubsidiarySignal plat3Depot(10, 73);
GroundSignal sidingExit(11, 74, 75);
GroundSignal depotExit(13, 76, 77);
ThreeAspectSignal eastApproach(15, 78, 79, 80, 81);
Feather eastApproachFeather(18, 82);

Signal * signals[] = { &plat1EDepart, &plat1Siding, &plat2EDepart, &plat3EDepart, &plat3Depot, &sidingExit, &depotExit, &eastApproach, &eastApproachFeather, NULL };

/*
 * Note that the shift registers are arranged such that the higher bits are sent to the first shift register, and then the overflow trickles down to the second
 * meaning that the lower addresses are actually driven by the second shift register
 */

const unsigned int SHIFT_REGISTERS = 3;

void writeToShiftReg(unsigned long value)
{
  digitalWrite(LATCHPIN, LOW);
  for(unsigned int reg = 0; reg < SHIFT_REGISTERS; ++reg)
  {
    shiftOut(DATAPIN, CLOCKPIN, LSBFIRST, (byte)(value & 0xFF));
    value = value >> 8;
  }
  digitalWrite(LATCHPIN, HIGH);
}

// DCC Callback function
void BasicAccDecoderPacket_Handler(int address, boolean activate, byte data) {
  address -= 1;
  address *= 4;
  address += 1;
  address += (data & 0x06) >> 1;
  // address = address - 4; // uncomment this line for Roco Maus or z21
  boolean enable = (data & 0x01) ? 1 : 0;

  processEvent(address, enable);
}

void processEvent(int address, boolean enable)
{
  boolean update = false;
  unsigned long output = 0;
  for (int i = 0; signals[i]; ++i)
  {
    if (signals[i]->dccEvent(address, enable))
      update = true;
    output += signals[i]->getData();
  }

  if (update)
  {
    writeToShiftReg(output);
  }
}


void setup() {
  pinMode(DATAPIN, OUTPUT);
  pinMode(CLOCKPIN, OUTPUT);
  pinMode(LATCHPIN, OUTPUT);

  DCC.SetBasicAccessoryDecoderPacketHandler(BasicAccDecoderPacket_Handler, true);
  DCC.SetupDecoder( 0x00, 0x00, 0 );

  unsigned long output = 0;
  for (int i = 0; signals[i]; ++i)
  {
    output += signals[i]->getData();
  }
  writeToShiftReg(output);
}

int iter = 0;
bool processed = false;
void loop() {
  DCC.loop(); // Call the DCC library to read incoming DCC data
}
