// Pins used for the outputs
const int SignalStation_Red = 2;
const int SignalStation_Yellow = 3;
const int SignalStation_Green = 4;
const int GroundLoop_Red = 5;
const int GroundLoop_White = 6;
const int GroundDepot_Red = 7;
const int GroundDepot_White = 8;
const int GroundOil_Red = 9;
const int GroundOil_White = 10;

// Pins used for the inputs
const int TurnoutDepotOilLoop_Platform = A0;
const int TurnoutDepot_OilLoop = A1;
const int TurnoutOil_Loop = A2;

const int Occupation_Platform = A3;
const int Occupation_Depot = A4;
const int Occupation_Oil = A5;
const int Occupation_Loop = 11; // was A6, moved as A6 does not support pull up

const int LoopDelay = 250;
const int MinTurnoutPendingDelay = (1000 / LoopDelay) * 8; // Wait for at least this many seconds before clearing signal
const int MaxTurnoutPendingDelay = (1000 / LoopDelay) * 16; // Clear signal within this many seconds of state change
const int MinPendingDelay = (1000 / LoopDelay) * 25; // Wait for at least this many seconds before clearing signal
const int MaxPendingDelay = (1000 / LoopDelay) * 50; // Clear signal within this many seconds of state change

enum SignalState
{
  Clear,
  PendingClear,
  Red
};

class Signal
{
public:
  Signal::Signal(String name)
  {
    m_Name = name;
    m_State = SignalState::Red;
  }
  
  void Signal::setClear(bool clear, bool bTurnoutChanged)
  {
    if (clear)
    {
      switch(m_State)
      {
        case SignalState::Red:
          setPendingDelay(bTurnoutChanged);
          m_State = SignalState::PendingClear;
          break;
        case SignalState::PendingClear:
          if (--m_PendingDelay == 0)
          {
            m_State = SignalState::Clear;
            setSignal(true);
          }
          break;
      }
    }
    else
    {
      // Always go straight to red
      m_State = SignalState::Red;
      setSignal(false);
    }
  }

protected:
  virtual void setSignal(bool clear) = 0;

  String Signal::getName()
  {
    return m_Name;
  }

private:
  void Signal::setPendingDelay(bool turnoutChanged)
  {
    // We have a slightly shorter delay if the turnout changes, leaving longer for occupation changes to allow the occupying train time to stop etc
    if (turnoutChanged)
    {
      m_PendingDelay = random(MinTurnoutPendingDelay, MaxTurnoutPendingDelay);
    }
    else
    {
      m_PendingDelay = random(MinPendingDelay, MaxPendingDelay);
    }
  }

  String m_Name;
  SignalState m_State;
  int m_PendingDelay;
};

class MainSignal : public Signal
{
private:
  int m_PinRed;
  int m_PinYellow;
  int m_PinGreen;

  bool m_Clear;

public:
  MainSignal::MainSignal(String name, int pinRed, int pinYellow, int pinGreen) : Signal(name)
  {
    m_PinRed = pinRed;
    m_PinYellow = pinYellow;
    m_PinGreen = pinGreen;
  }

  void MainSignal::initialise()
  {
    pinMode(m_PinRed, OUTPUT);
    pinMode(m_PinYellow, OUTPUT);
    pinMode(m_PinGreen, OUTPUT);

    setClear(false, false);
  }

  virtual void MainSignal::setSignal(bool clear)
  {
    // Avoid changing the signal if the signal is already showing a clear aspect
    if (m_Clear && clear) return;

    // Set the red accordingly
    digitalWrite(m_PinRed, clear ? LOW : HIGH);

    if (clear)
    {
      // Signal is clear, so turn on either yellow or green, choose at random
      bool bGreen = random(100) % 2 == 0;
      digitalWrite(m_PinYellow, bGreen ? LOW : HIGH);
      digitalWrite(m_PinGreen, bGreen ? HIGH : LOW);
    }
    else
    {
      // Signal is not clear, so turn off the yellow and green
      digitalWrite(m_PinYellow, LOW);
      digitalWrite(m_PinGreen, LOW);
    }

    // Keep track of the state to avoid changing the clear status
    m_Clear = clear;
  }
};

class GroundSignal : public Signal
{
private:
  int m_PinRed;
  int m_PinWhite;
    
public:
  GroundSignal::GroundSignal(String name, int pinRed, int pinWhite) : Signal(name)
  {
    m_PinRed = pinRed;
    m_PinWhite = pinWhite;
  }

  void GroundSignal::initialise()
  {
    pinMode(m_PinRed, OUTPUT);
    pinMode(m_PinWhite, OUTPUT);

    setClear(false, false);
  }

  virtual void GroundSignal::setSignal(bool clear)
  {
    digitalWrite(m_PinRed, clear ? LOW : HIGH);
    digitalWrite(m_PinWhite, clear ? HIGH : LOW);
  }
};

class HighLowAnalogPin
{
  
private:
  const int DifferenceCount = 4; // Number of updates that must match before considering the state to have changed
  String m_Name;
  int m_Pin;
  int m_DifferentUpdates; // Number of sequential updates that have been different to the current state
  bool m_Level;

public:
  HighLowAnalogPin::HighLowAnalogPin(String name, int pin)
  {
    m_Pin = pin;
    m_DifferentUpdates = 0;
    m_Name = name; 
    m_Level = false;
  }

  void HighLowAnalogPin::initialise()
  {
    pinMode(m_Pin, INPUT_PULLUP);
    m_Level = getCurrentLevel();
  }

  bool HighLowAnalogPin::getLevel()
  {
    return m_Level;
  }

protected:
  String HighLowAnalogPin::getName()
  {
    return m_Name;
  }
  
  void HighLowAnalogPin::updateValue()
  {
    if (getCurrentLevel() == m_Level)
    {
      m_DifferentUpdates = 0;
    }
    else
    {
      m_DifferentUpdates++;
      if (m_DifferentUpdates > DifferenceCount)
      {
        m_DifferentUpdates = 0;
        m_Level = !m_Level;
      }
    }
  }

private:
  bool HighLowAnalogPin::getCurrentLevel()
  {
    return digitalRead(m_Pin);
  }
};

enum Orientation
{
  Left,
  Right
};

class TurnoutState : public HighLowAnalogPin
{
  // Turnout state is measured on an analog pin, which is connected to +5v via a resistor, 0v via the switched side of an optoisolator.
  // The optoisolator is wired from the frog to the front DCC bus which for turnouts with the toe to the left of the frog (as the turnouts we are interested in are oriented)
  // this means that the optoisolator is activated when the turnout blades are set to the right, hence
  //    Turnout blades to right = Optoisolator active = Pin low
  //    Turnout blades to left = Optoisolator inactive = Pin high

  Orientation m_Orientation; 

public:
  TurnoutState::TurnoutState(String name, int pin) : HighLowAnalogPin(name, pin)
  {
    setOrientation();
  }

  void TurnoutState::update()
  {
    updateValue();
    setOrientation();
  }

  Orientation TurnoutState::getOrientation()
  {
    return m_Orientation;
  }

private:
  void TurnoutState::setOrientation()
  {
    m_Orientation = (getLevel() ? Orientation::Left : Orientation::Right);
  }
};

enum Occupation
{
  Occupied,
  Empty
};

class OccupationState : public HighLowAnalogPin
{
  // Occupied state is measured on an analog pin, which is connected to +5v via a resistor, 0v via the switched side of an optoisolator.
  // The optoisolator is wired from a MERG TOTI kit which activates the optoisolator when a train is detected in the detection zone, hence
  //    Train present = Optoisolator active = Pin low
  //    Train absent = Optoisolator inactive = Pin high

  Occupation m_Occupation; 

public:
  OccupationState::OccupationState(String name, int pin) : HighLowAnalogPin(name, pin)
  {
    setOccupation();
  }

  void OccupationState::update()
  {
    updateValue();
    setOccupation();
  }

  Occupation OccupationState::getOccupation()
  {
    return m_Occupation;
  }

private:
  void OccupationState::setOccupation()
  {
    m_Occupation = (getLevel() ? Occupation::Empty : Occupation::Occupied);
  }
};

enum PreviousState
{
  NoConditionsMet = 0,
  TrainPresent = 1,
  TurnoutsSet = 2
};

class SignalManager
{
private:
  Signal * m_Signal;
  TurnoutState ** m_Left;
  TurnoutState ** m_Right;
  OccupationState * m_Occupation;
  PreviousState m_PreviousState;

public:
  SignalManager::SignalManager(Signal * signal, TurnoutState ** left, TurnoutState ** right, OccupationState * occupation)
  {
    m_Signal = signal;
    m_Left = left;
    m_Right = right;
    m_Occupation = occupation;
    m_PreviousState = PreviousState::NoConditionsMet;
  }

  void setSignal()
  {

    PreviousState newState = PreviousState::NoConditionsMet;
    
    // Set the signal to clear IF the turnouts we need to be set left are set left, the turnouts we need to be set right are set right, and the section passed is occupied
    // Otherwise the signal is set to red

    if (m_Occupation->getOccupation() == Occupation::Occupied) newState = PreviousState::TrainPresent;

    bool turnoutsSet = true;
    
    // All good on the occupation front, check the left right turnouts
    TurnoutState ** pIterator = m_Left;
    while (turnoutsSet && *pIterator)
    {
      if ((*pIterator)->getOrientation() != Orientation::Left) turnoutsSet = false;
      ++pIterator;      
    }

    // All good on the occupation front, check the right right turnouts
    pIterator = m_Right;
    while (turnoutsSet && *pIterator)
    {
      if ((*pIterator)->getOrientation() != Orientation::Right) turnoutsSet = false;      
      ++pIterator;      
    }

    if (turnoutsSet) newState = newState | PreviousState::TurnoutsSet;

    m_Signal->setClear(newState == (PreviousState::TurnoutsSet | PreviousState::TrainPresent), (m_PreviousState & PreviousState::TurnoutsSet) == 0);
    
    m_PreviousState = newState;
  }
  
};

/*
class PinDebugger
{
  private:
    String m_Name;
    int m_Pin;
    bool m_Value;
    
  public:
  PinDebugger::PinDebugger(String pinName, int pin)
  {
    m_Pin = pin;
    m_Name = pinName;
    m_Value = false;
  }

  void PinDebugger::initialise()
  {
    pinMode(m_Pin, INPUT_PULLUP);
    update();
  }

  void update()
  {
    bool bValue = digitalRead(m_Pin);
    if (bValue != m_Value)
    {
      m_Value = bValue;
      Serial.print(m_Name + "(" + m_Pin + ") is now ");
      if (m_Value) 
        Serial.println(" high ");
      else
        Serial.println(" low ");
    }
  }
};
*/

// Create the classes
GroundSignal signalLoop("Ground Loop", GroundLoop_Red, GroundLoop_White);
GroundSignal signalOil("Ground Oil", GroundOil_Red, GroundOil_White);
GroundSignal signalDepot("Ground Depot", GroundDepot_Red, GroundDepot_White);

MainSignal signalStation("Main Platform", SignalStation_Red, SignalStation_Yellow, SignalStation_Green);

TurnoutState turnoutDepotOilLoop_Platform("TurnoutDepotOilLoop_Platform", TurnoutDepotOilLoop_Platform);
TurnoutState turnoutDepot_OilLoop("TurnoutDepot_OilLoop", TurnoutDepot_OilLoop);
TurnoutState turnoutOil_Loop("TurnoutOil_Loop", TurnoutOil_Loop);

OccupationState occupationPlatform("Occupation_Platform", Occupation_Platform);
OccupationState occupationDepot("Occupation_Depot", Occupation_Depot);
OccupationState occupationOil("Occupation_Oil", Occupation_Oil);
OccupationState occupationLoop("Occupation_Loop", Occupation_Loop);

TurnoutState * stationLeft[] = { NULL };
TurnoutState * stationRight[] = { &turnoutDepotOilLoop_Platform, NULL };

TurnoutState * loopLeft[] = { &turnoutDepotOilLoop_Platform, NULL };
TurnoutState * loopRight[] = { &turnoutDepot_OilLoop, &turnoutOil_Loop, NULL };

TurnoutState * oilLeft[] = { &turnoutDepotOilLoop_Platform, &turnoutOil_Loop, NULL };
TurnoutState * oilRight[] = { &turnoutDepot_OilLoop, NULL };

TurnoutState * depotLeft[] = { &turnoutDepotOilLoop_Platform, &turnoutDepot_OilLoop, NULL };
TurnoutState * depotRight[] = { NULL };

SignalManager managerStation(&signalStation, stationLeft, stationRight, &occupationPlatform);
SignalManager managerLoop(&signalLoop, loopLeft, loopRight, &occupationLoop);
SignalManager managerOil(&signalOil, oilLeft, oilRight, &occupationOil);
SignalManager managerDepot(&signalDepot, depotLeft, depotRight, &occupationDepot);

void setup()
{
  // Seed the random number generator
  randomSeed(analogRead(A7));

// Uncomment for debugging to serial monitor
//  Serial.begin(9600);

  // Initialise everything
  signalLoop.initialise();
  signalOil.initialise();
  signalDepot.initialise();

  signalStation.initialise();

  turnoutDepotOilLoop_Platform.initialise();
  turnoutDepot_OilLoop.initialise();
  turnoutOil_Loop.initialise();

  occupationPlatform.initialise();
  occupationDepot.initialise();
  occupationOil.initialise();
  occupationLoop.initialise();

}

void loop()
{
  // Update the states of the turnouts and the occupation of the blocks we are interested in
  turnoutDepotOilLoop_Platform.update();
  turnoutDepot_OilLoop.update();
  turnoutOil_Loop.update();

  occupationPlatform.update();
  occupationDepot.update();
  occupationOil.update();
  occupationLoop.update();

  managerStation.setSignal();
  managerLoop.setSignal();
  managerOil.setSignal();
  managerDepot.setSignal();

  delay(LoopDelay);
}
