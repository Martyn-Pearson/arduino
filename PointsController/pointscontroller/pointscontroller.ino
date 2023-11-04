#include <DCC_Decoder.h>
#include <Servo.h>
#include <EEPROM.h>

#define TURNOUT_COUNT 10
#define MOVE_DELAY 10
#define START_OFFSET 40

const byte ADDRESSES[] = {  START_OFFSET, // Stat Plat1 Sdng 
                            START_OFFSET + 1, // Stat Down Plat1 
                            START_OFFSET + 2, // Stat Down Cross 
                            START_OFFSET + 3, // Stat Up Cross 
                            START_OFFSET + 4, // Stat Depot 
                            START_OFFSET + 9, // Plat1 Coupler (Note out of order)
                            START_OFFSET + 5, // Depot Down Cross 
                            START_OFFSET + 6, // Depot Up Cross 
                            START_OFFSET + 7, // Depot Entry 
                            START_OFFSET + 8, // Depot Sidings
                          };
const byte PINS[] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };

class Turnout
{
private:
  byte m_address;
  byte m_pin;
  byte m_leftPos;
  byte m_rightPos;
  byte m_currentPos;
  byte m_targetPos;
  Servo m_servo;
  bool m_isMoving;
  unsigned long m_timetomove;

public:
  Turnout(byte address, byte pin, byte leftPos, byte rightPos)
  {
    m_address = address;    
    m_pin = pin;
    m_leftPos = leftPos;
    m_rightPos = rightPos;
    m_targetPos = (m_leftPos + m_rightPos) / 2; // Default to the middle for now, we only move once asked, rather than moving to a default position
    m_currentPos = m_targetPos;
    m_isMoving = false;
    m_timetomove = 0;
  }

  int getLeftPos()
  {
    return m_leftPos;
  }

  int getRightPos()
  {
    return m_rightPos;
  }

  byte getAddress()
  {
    return m_address;
  }

  bool isAtLeft()
  {
    return m_currentPos == m_leftPos;
  }

  bool isAtRight()
  {
    return m_currentPos == m_rightPos;
  }

  bool isMoving()
  {
    return m_isMoving;
  }

  void setToLeft()
  {
    m_targetPos = m_leftPos;
  }

  void setToRight()
  {
    m_targetPos = m_rightPos;
  }

  bool move()
  {
    if (m_currentPos == m_targetPos)
    {
      if (m_isMoving)
      {
        m_isMoving = false;
        m_servo.detach();
      }
      return false;
    }
    if (!m_isMoving)
    {
      m_isMoving = true;
      m_servo.attach(m_pin);
    }
    if (millis() > m_timetomove)
    {
      m_timetomove = millis() + (unsigned long)MOVE_DELAY;
      m_currentPos = (m_currentPos > m_targetPos) ? m_currentPos - 1 : m_currentPos + 1;
      m_servo.write(m_currentPos);
    }
    return true;
  }

};

class TurnoutManager
{
private:
  Turnout * turnouts[TURNOUT_COUNT];
  int m_currentTurnout;

public:
  TurnoutManager()
  {
    m_currentTurnout = 0;
  }
  
  void loadTurnouts()
  {
    for (int nCount = 0; nCount < TURNOUT_COUNT; nCount++)
    {
      turnouts[nCount] = new Turnout( ADDRESSES[nCount],
                                      PINS[nCount],
                                      EEPROM.read(nCount * 2) - 2, 
                                      EEPROM.read(nCount * 2 + 1) + 2);
    }
  }

  void saveTurnouts()
  {
    for (int nCount = 0; nCount < TURNOUT_COUNT; nCount++)
    {
      EEPROM.write(nCount * 2, turnouts[nCount]->getLeftPos());
      EEPROM.write(nCount * 2 + 1, turnouts[nCount]->getRightPos());
    }
  }

  Turnout * getTurnout(int index)
  {
    return turnouts[index];
  }

  Turnout * getTurnoutByAddress(byte address)
  {
    for (int nCount = 0; nCount < TURNOUT_COUNT; nCount++)
    {
      Turnout * pTurnout = getTurnout(nCount);
      if (pTurnout->getAddress() == address)
        return pTurnout;
    }
    return NULL;
  }

  void doMoves()
  {
    // First off, see if there are any points that are moving, if so we move them and do nothing else - we don't want to be moving points at the same time
    for (int nCount = 0; nCount < TURNOUT_COUNT; nCount++)
    {
      if (getTurnout(nCount)->isMoving())
      {
        getTurnout(nCount)->move();
        return;
      }
    }

    // No points are currently moving, see if there are any that need to move; again we only move one
    for (int nCount = 0; nCount < TURNOUT_COUNT; nCount++)
    {
      if (getTurnout(nCount)->move())
      { 
        return;
      }
    }
  }
};


TurnoutManager  turnouts;


// The DCC library calls this function to set / reset accessories
void BasicAccDecoderPacket_Handler(int address, boolean activate, byte data)
{
  // Get the "human readable" address from the NMRA format address
  address -= 1;
  address *= 4;
  address += 1;
  address += (data & 0x06) >> 1;
  // address = address - 4; // uncomment this line for Roco Maus or z21

  // Find the turnout at this address
  Turnout * pTurnout = turnouts.getTurnoutByAddress(address);

  if (pTurnout)
  {
    // Determine which way to set the turnout
    boolean setLeft = (data & 0x01) ? true : false;

    if (setLeft)
    {
      pTurnout->setToLeft();
    }
    else
    {
      pTurnout->setToRight();
    }
  }
  else
  {
  }
}

void setup()
{  
  // Load the turnouts
  turnouts.loadTurnouts();

  DCC.SetBasicAccessoryDecoderPacketHandler(BasicAccDecoderPacket_Handler, true);
  DCC.SetupDecoder( 0x00, 0x00, 0 );
}

void loop() {
  static int iteration = 0;
  
  DCC.loop(); // Call the DCC library to read incoming DCC data

  iteration = (iteration + 1) % 16;
  if (iteration == 0)
  {
    turnouts.doMoves();
  }
}
