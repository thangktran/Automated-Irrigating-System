#ifndef AUTOMATEDIRRIGATINGSYSTEM_SHARED_H
#define AUTOMATEDIRRIGATINGSYSTEM_SHARED_H

#if !defined(ARDUINO_ESP8266_THING) && !defined(ARDUINO_ESP8266_GENERIC)
#define MonitorSerial Serial
#define Esp8266Serial Serial1
#else
#define Esp8266Serial Serial
#endif


typedef uint8_t DeviceId;

// --------------------GLOBAL VALUES--------------------//
const DeviceId WIFI_MODULE_TX   = 18;
const DeviceId WIFI_MODULE_RX   = 19;
const DeviceId WIFI_MODULE_RST  = 9;
const DeviceId WIFI_MODULE_CPD  = 10;

const DeviceId WATER_LEVEL_POWER     = 2;
const DeviceId WATER_LEVEL_PREALERT  = 3;
const DeviceId WATER_LEVEL_BOTTOM    = 4;

const DeviceId NTC_SENSOR = A0;

const DeviceId RELAY_SOLENOID_1 = 5;
const DeviceId RELAY_SOLENOID_2 = 6;
const DeviceId RELAY_SOLENOID_3 = 7;
const DeviceId RELAY_PUMP       = 8;

const String ACK    = "ACK";
const String NACK   = "NACK";
const String START  = "START";
const String STOP   = "STOP";
const String HANDSHAKE = "HANDSHAKE";

const uint8_t nMaxGet = 2;
const uint8_t nMaxSend = 2;

const uint8_t checkDataAfter = 1; // second.

// ====== STRUCTS ====== //
struct Time
{
  uint8_t hour;
  uint8_t minute;
}__attribute__((packed));
typedef struct Time Time;

struct Weekdays
{
  uint8_t sunday    : 1;
  uint8_t monday    : 1;
  uint8_t tuesday   : 1;
  uint8_t wednesday : 1;
  uint8_t thursday  : 1;
  uint8_t friday    : 1;
  uint8_t saturday  : 1;
  uint8_t unused    : 1;
}__attribute__((packed));
typedef struct Weekdays Weekdays;


struct ToWater
{
  uint8_t   sequence;
  DeviceId  device;
  uint16_t  ml;
}__attribute__((packed));
typedef struct ToWater ToWater;


struct MegaData
{
  uint8_t deviceTemperature;
  bool waterTankPreAlert;
  bool waterTankEmpty;
  uint8_t wateredSequence;
}__attribute__((packed));
typedef struct MegaData MegaData;

struct Esp8266Data
{
  Time      time;
  Weekdays  weekdays;
  ToWater   toWater;
}__attribute__((packed));
typedef struct Esp8266Data Esp8266Data;



// ====== FUNCTIONS ====== //
uint8_t dataChecksum(void* s, size_t size)
{
    uint8_t* data = reinterpret_cast<uint8_t*>(s);
    uint8_t c = 0;
    while(size-- > 0)
        c ^= *reinterpret_cast<uint8_t*>(data++); 
    return c;
}

bool isObjectEmpty (void* o, size_t size)
{
  uint8_t* data = reinterpret_cast<uint8_t*>(o);
  while (size-- > 0)
  {
    uint8_t* check = reinterpret_cast<uint8_t*>(data++);
    
    if (*check != 0)
      return false;
  }

  return true;
}

// return true if same. False if not
bool compareObject (void* l, void* r, size_t size)
{
  uint8_t* a = reinterpret_cast<uint8_t*>(l);
  uint8_t* b = reinterpret_cast<uint8_t*>(r);

  while (size-- > 0)
  {
    if (*a++ != *b++)
      return false;
  }

  return true;
}
#endif // AUTOMATEDIRRIGATINGSYSTEM_SHARED_H
