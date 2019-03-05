#ifndef AUTOMATEDIRRIGATINGSYSTEM_MEGAONLY_H
#define AUTOMATEDIRRIGATINGSYSTEM_MEGAONLY_H
// --------------------GLOBAL VALUES--------------------//
const unsigned int eepromStartAddress = 0;
const uint8_t maxWaterPlans = 3;

// --------------------STRUCTS--------------------//
struct WaterPlan
{
  Time		time;
  uint16_t 	ml;
  bool    	watered;
};
typedef struct WaterPlan WaterPlan;

struct WaterSchedule
{
  DeviceId    device;
  Weekdays    daysToWater;
  WaterPlan   plans[maxWaterPlans];
};
typedef struct WaterSchedule WaterSchedule;

struct EepromStore
{
  WaterSchedule schedule;
  uint8_t       checksum;
};
typedef struct EepromStore EepromStore;


// --------------------FUNTIONS--------------------//

#endif // AUTOMATEDIRRIGATINGSYSTEM_MEGAONLY_H
