#pragma once

#include <stdint.h>

namespace automated_irrigating_system
{
namespace common_structs
{

typedef uint8_t DeviceId;

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

struct WaterPlan
{
  Time      time;
  uint16_t  ml;
  bool      watered;
}__attribute__((packed));
typedef struct WaterPlan WaterPlan;

const uint8_t MAX_WATERPLAN_PER_SCHEDULE = 3;
struct WaterSchedule
{
  DeviceId    device;
  Weekdays    daysToWater;
  WaterPlan   plans[MAX_WATERPLAN_PER_SCHEDULE];
}__attribute__((packed));
typedef struct WaterSchedule WaterSchedule;

struct EepromStoreStructure
{
  WaterSchedule schedule;
  uint8_t       checksum;
}__attribute__((packed));
typedef struct EepromStoreStructure EepromStoreStructure;

} // namespace common_structs
} // namespace automated_irrigating_system