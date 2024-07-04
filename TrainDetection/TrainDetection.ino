#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SENSOR_COUNT 14
#define DETECTOR_BOARD_COUNT 1
#define BOARD_ADDRESS_LINES 4
#define SENSOR_THRESHOLD 500 // Threshold below which a train is present

class Board
{
private:
  int m_AddressLines[BOARD_ADDRESS_LINES];
  int m_SignalLine;

public:
  Board(int addressLines[], int signalLine)
  {
    for (int n = 0; n < BOARD_ADDRESS_LINES; n++)
    {
      m_AddressLines[n] = addressLines[n];
    }
    m_SignalLine = signalLine;
  }

  void initialise()
  {
      pinMode(m_SignalLine, INPUT);
      for (int line = 0; line < BOARD_ADDRESS_LINES; line++)
      {
        pinMode(m_AddressLines[line], OUTPUT);
      }
  }

  int getValue(int address)
  {
    // Set the address lines which tell the multiplexer board which input we're interested in
    for(int line = 0; line < BOARD_ADDRESS_LINES; line++)
    {
      digitalWrite(m_AddressLines[line], address & (1 << line) ? HIGH : LOW);
    }
    
    // Short delay 
    delay(1);

    // Then read the value from the multiplexer
    return analogRead(m_SignalLine);
  }
};

class Detector
{
private:
  const int BOARD_0_ADDRESS_LINES[BOARD_ADDRESS_LINES] = {5, 4, 3, 2};
  const int BOARD_0_SENSORLINE = A0;

  const int SENSOR_ADDRESSES[SENSOR_COUNT] = {13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
  const int SENSOR_BOARDS[SENSOR_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const int LASER_OUTPUT = 13;
  
  Board * m_pBoards[DETECTOR_BOARD_COUNT];
  bool * m_pSensorArray;
  int m_Sensors;

public:
  Detector(int sensors)
  {
    m_Sensors = sensors;
    m_pSensorArray = new bool[sensors];
  }

  void initialise()
  {
    for(int n = 0; n < m_Sensors; n++)
    {
      m_pSensorArray[n] = false;
    }

    m_pBoards[0] = new Board(BOARD_0_ADDRESS_LINES, BOARD_0_SENSORLINE);
    m_pBoards[0]->initialise();

    // Set up the lasers output and make sure it is off
    pinMode(LASER_OUTPUT, OUTPUT);
    digitalWrite(LASER_OUTPUT, LOW);
  }

  bool * getState()
  {
    return m_pSensorArray;
  }

  void detect()
  {
    // Turn on the lasers and pause for a short moment, give those photons time to do their stuff
    digitalWrite(LASER_OUTPUT, HIGH);
    delay(5);

    // Do the detection
    for(int sensor = 0; sensor < SENSOR_COUNT; sensor++)
    {
      int value = m_pBoards[SENSOR_BOARDS[sensor]]->getValue(SENSOR_ADDRESSES[sensor]);
      m_pSensorArray[sensor] = value < SENSOR_THRESHOLD;
    }

    // And turn the lasers off again
    digitalWrite(LASER_OUTPUT, LOW);
  }
};

class Display
{
  const int WIDTH = 16;
  const int HEIGHT = 2;
  const char * TRAIN = "T";
  const char * CLEAR = "_";
  
private:
  LiquidCrystal_I2C * m_pLCD;
  int m_hash;

public:
  Display()
  {
    m_pLCD = NULL;
    m_hash = -1;
  }

  void initialise()
  {
    m_pLCD = new LiquidCrystal_I2C(0x27,WIDTH,HEIGHT);
    m_pLCD->init();
    m_pLCD->backlight();    
  }

  void showTrains(bool * trains, int len)
  {
    int hash = calculateHash(trains, len);
    if(hash != m_hash)
    {
      m_hash = hash;
      refreshDisplay(trains, len);
    }
  }

private:
  int calculateHash(bool * trains, int len)
  {
    int hash = 0;
    for (int n = 0; n < len; n++)
    {
      hash = hash << 1;
      if(trains[n])
      {
        hash = hash | 1;
      }  
    }

    return hash;
  }
  void refreshDisplay(bool * trains, int len)
  {  
    m_pLCD->clear();
    
    if(len > WIDTH)
    {
      // We show the middle 16 on the top line, then show the two ends as if they have wrapped back, e.g.
      // fghijklmnopqrstu 
      // edcba      zyxwv
      int topStartPos = (len - WIDTH) / 2;
      m_pLCD->setCursor(0, 0);
      for(int n = 0; n < WIDTH; n++)
      {
        m_pLCD->print(trains[n + topStartPos] ? TRAIN : CLEAR);
      }

      // Then print the left train sensors before those on the top line
      m_pLCD->setCursor(0, 1);
      for(int n = 1; n <= topStartPos; n++)
      {
        m_pLCD->print(trains[topStartPos - n] ? TRAIN : CLEAR);
      }

      // Finally print the right train sensors after those on the top line
      m_pLCD->setCursor(WIDTH - (len - (WIDTH + topStartPos)), 1);
      for(int n = len - 1; n >= WIDTH + topStartPos; n--)
      {
        m_pLCD->print(trains[n] ? TRAIN : CLEAR);
      }
    }
    else
    {
      // Simply show the trains centred on the top line
      m_pLCD->setCursor((WIDTH - len) / 2, 0);
      for(int n = 0; n < len; n++)
      {
        m_pLCD->print(trains[n] ? TRAIN : CLEAR);
      }      
    }
  }
};

Display display;
Detector detector(SENSOR_COUNT);

void setup()
{
  display.initialise();
  detector.initialise();
}

void loop()
{
  detector.detect();
  display.showTrains(detector.getState(), SENSOR_COUNT);
  delay(50);
}
