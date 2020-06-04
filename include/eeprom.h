#pragma once

#include <stdint.h>
#include <stddef.h>

namespace automated_irrigating_system
{

class Eeprom
{
public:
    Eeprom(const Eeprom& op) = delete;
    Eeprom& operator=(const Eeprom& op) = delete;
    Eeprom(const Eeprom&& op) = delete;
    Eeprom& operator=(const Eeprom&& op) = delete;

    static Eeprom& getInstance();

    template<typename T>
    const T& put(int address, const T& t);

    template<typename T>
    T& get(int address, T& t);

    bool commit();

    // Get checksum value of object d.
    static uint8_t dataChecksum(void* d, size_t size);
    // Check if the object is empty (every bit is 0)
    static bool objectIsEmpty (void* o, size_t size);
    // This function do a bitwise-comparision.
    // Return true if all the bits are the same,
    // false if not.
    static bool compareObject (void* l, void* r, size_t size);

private:
    Eeprom();
    ~Eeprom();

    const int _eepromSize = 512;
};

} // namespace automated_irrigating_system