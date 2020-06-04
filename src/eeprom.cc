#include "eeprom.h"

#include <EEPROM.h>

using namespace automated_irrigating_system;

Eeprom& Eeprom::getInstance()
{
    static Eeprom e;
    return e;
}

template<typename T>
const T& Eeprom::put(int address, const T& t)
{
    return EEPROM.put(address, t);
}

template<typename T>
T& Eeprom::get(int address, T& t)
{
    EEPROM.get(address, t);
    EEPROM.commit();
    return t;
}

uint8_t Eeprom::dataChecksum(void* s, size_t size)
{
    uint8_t* data = reinterpret_cast<uint8_t*>(s);
    uint8_t c = 0;
    while(size-- > 0)
        c ^= *reinterpret_cast<uint8_t*>(data++); 
    return c;
}

bool Eeprom::objectIsEmpty (void* o, size_t size)
{
    uint8_t* data = reinterpret_cast<uint8_t*>(o);
    while (size-- > 0)
    {
      uint8_t* check = reinterpret_cast<uint8_t*>(data++);
    
      if (*check != 0)
      {
          return false;
      }
    }

    return true;
}

// return true if same. False if not.
// std::memcmp(l, r, size);
bool Eeprom::compareObject (void* l, void* r, size_t size)
{
    uint8_t* a = reinterpret_cast<uint8_t*>(l);
    uint8_t* b = reinterpret_cast<uint8_t*>(r);

    while (size-- > 0)
    {
      if (*a++ != *b++)
      {
          return false;
      }
    }

    return true;
}

Eeprom::Eeprom()
{
    EEPROM.begin(_eepromSize);
}

Eeprom::~Eeprom()
{
    EEPROM.end();
}