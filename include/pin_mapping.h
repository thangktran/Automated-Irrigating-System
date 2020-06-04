#pragma once

#include "common_structs.h"

namespace automated_irrigating_system
{
namespace pin_mapping
{

const uint8_t N_SOLENOIDS          = 6;

const common_structs::DeviceId TEMP_SENSOR         = A0;
const common_structs::DeviceId WATER_EMPTY_WARNING = D0;
const common_structs::DeviceId WATER_MONITOR_POWER = D9;
const common_structs::DeviceId FLOW_SENSOR         = D1;
const common_structs::DeviceId GATE_SOLENOID_1     = D2;
const common_structs::DeviceId GATE_SOLENOID_2     = D3;
const common_structs::DeviceId GATE_SOLENOID_3     = D4;
const common_structs::DeviceId GATE_SOLENOID_4     = D5;
const common_structs::DeviceId GATE_SOLENOID_5     = D6;
const common_structs::DeviceId GATE_SOLENOID_6     = D7;
const common_structs::DeviceId GATE_PUMP           = D8;


} // namespace common_structs
} // namespace automated_irrigating_system