class Signal
{
  public:
    Signal(unsigned int startBit) : m_StartBit(startBit), m_Data(0)
    {
    }

    virtual boolean dccEvent(int address, boolean active) = 0;
    
    unsigned long getData()
    {
      return m_Data;
    }

  protected:
    unsigned int m_StartBit;
    unsigned long m_Data;
};

class ThreeAspectSignal : public Signal
{
  public:
    ThreeAspectSignal(unsigned int startBit, 
                      int dangerAddr, 
                      int proceedAddr, 
                      int cautionAddr, 
                      int randomProceedAddr) : Signal(startBit), m_DangerAddr(dangerAddr), m_ProceedAddr(proceedAddr), m_CautionAddr(cautionAddr), m_RandomProceedAddr(randomProceedAddr)
    {
      m_Data = dangerAspect();
    }

    virtual boolean dccEvent(int address, boolean active)
    {
      if (!active) return false; // We use positive messages for all aspects, i.e. we don't set danger when proceed is turned off
      
      if (address == m_DangerAddr && m_Data != dangerAspect())
      {
        m_Data = dangerAspect();
        return true;
      }

      if (address == m_ProceedAddr && m_Data != proceedAspect())
      {
        m_Data = proceedAspect();
        return true;
      }

      if (address == m_CautionAddr && m_Data != cautionAspect())
      {
        m_Data = cautionAspect();
        return true;
      }

      if (address == m_RandomProceedAddr && m_Data == dangerAspect())
      {
        m_Data = randomProceedAspect();
        return true;
      }

      return false;
    }

  private:
    unsigned long proceedAspect()
    {
      return 1ul << m_StartBit + 2;
    }
    
    unsigned long cautionAspect()
    {
      return 1ul << m_StartBit + 1;      
    }
    
    unsigned long randomProceedAspect()
    {
      return 1ul << (m_StartBit + random(1, 3));
    }
    
    unsigned long dangerAspect()
    {
      return 1ul << m_StartBit;      
    }

  private:
    int m_DangerAddr;
    int m_ProceedAddr;
    int m_CautionAddr;
    int m_RandomProceedAddr;
};

class SubsidiarySignal : public Signal
{
  public:
    SubsidiarySignal(unsigned int startBit, int proceedAddr) : Signal(startBit), m_ProceedAddr(proceedAddr)
    {
      m_Data = 0;
    }

    virtual boolean dccEvent(int address, boolean active)
    {
      if (address == m_ProceedAddr)
      {
        if (active == (m_Data == 0))
        {
          m_Data = active ? 1ul << m_StartBit : 0ul;
          return true;
        }
      }

      return false;
    }

   private:
   
    int m_ProceedAddr;
};

class GroundSignal : public Signal
{
  public:
    GroundSignal(unsigned int startBit, int dangerAddr, int proceedAddr) : Signal(startBit), m_DangerAddr(dangerAddr), m_ProceedAddr(proceedAddr)
    {
      m_Data = dangerAspect();
    }

    virtual boolean dccEvent(int address, boolean active)
    {
      if (!active) return false; // We use positive messages for all aspects, i.e. we don't set danger when proceed is turned off
      
      if (address == m_DangerAddr and m_Data != dangerAspect())
      {
        m_Data = dangerAspect();
        return true;
      }

      if (address == m_ProceedAddr and m_Data != proceedAspect())
      {
        m_Data = proceedAspect();
        return true;
      }

      return false;
    }

  private:
    unsigned long dangerAspect()
    {
      return 1ul << m_StartBit;
    }

    unsigned long proceedAspect()
    {
      return 1ul << (m_StartBit + 1);
    }


   private:
    int m_DangerAddr;
    int m_ProceedAddr;
    boolean m_bProceed;
};

class Feather : public Signal
{
  public:
    Feather(unsigned int startBit, int divergeAddr) : Signal(startBit), m_DivergeAddr(divergeAddr)
    {
      m_Data = 0;
    }

    virtual boolean dccEvent(int address, boolean active)
    {
      if (address == m_DivergeAddr)
      {
        if (active == (m_Data == 0))
        {
          m_Data = active ? 1ul << m_StartBit : 0ul;
          return true;
        }
      }

      return false;
    }

   private:
   
    int m_DivergeAddr;
};
