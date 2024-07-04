#include <DCC_Decoder.h>

void BasicAccDecoderPacket_Handler(int address, boolean activate, byte data) {
  address -= 1;
  address *= 4;
  address += 1;
  address += (data & 0x06) >> 1;
  // address = address - 4; // uncomment this line for Roco Maus or z21
  boolean enable = (data & 0x01) ? 1 : 0;

  Serial.print(address);
  Serial.print(" received ");
  Serial.println(enable ? "enable" : "disable");
}


void setup() {
  DCC.SetBasicAccessoryDecoderPacketHandler(BasicAccDecoderPacket_Handler, true);
  DCC.SetupDecoder( 0x00, 0x00, 0 );
  Serial.begin(9600);
}

void loop() {
  DCC.loop(); // Call the DCC library to read incoming DCC data
}
