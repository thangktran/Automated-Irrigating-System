#pragma once

#include <stdint.h>

#include "common_structs.h"

#include "Arduino.h"

namespace automated_irrigating_system
{
    
class AutomatedIrrigatingSystem
{
public:
    AutomatedIrrigatingSystem(const AutomatedIrrigatingSystem& op) = delete;
    AutomatedIrrigatingSystem& operator=(const AutomatedIrrigatingSystem& op) = delete;
    AutomatedIrrigatingSystem(const AutomatedIrrigatingSystem&& op) = delete;
    AutomatedIrrigatingSystem& operator=(const AutomatedIrrigatingSystem&& op) = delete;

    AutomatedIrrigatingSystem& getInstance();
    void initializeDevice();
    void handleEvents();

    void setPinMode (common_structs::DeviceId device, uint8_t mode);
    void switchOnDevice (common_structs::DeviceId device);
    void switchOffDevice (common_structs::DeviceId device);
    bool digitalReadDevice (common_structs::DeviceId device);
    int analogReadDevice (common_structs::DeviceId device);
    
    bool timeInRange (const common_structs::Time& currentTime, 
    const common_structs::Time& toCompare);

    // TODO : Implement flow sensor
    float getDeviceTemperature();
    bool isWaterTankPreAlert();
    bool isWaterTankEmpty();
    void checkDeviceTemperature();
    void startPumping (common_structs::DeviceId device, uint16_t ml);
    void stopPumping (common_structs::DeviceId device);
    void resetWaterPlan (uint8_t scheduleIndex);
    void checkWaterPlan();
    // NOTE: The system was not design to run the pump and open all 
    // solenoids at the same time.
    // Hence: only the the pump and 1 solenoid can be turned on simultanously.
    bool waterPlant (common_structs::DeviceId device, uint16_t ml);

private:
    AutomatedIrrigatingSystem();
    ~AutomatedIrrigatingSystem();

    void initializePins();
    int REST_waterPlant (String param);
    void flowSensorInterruptHandler();


    uint8_t _deviceTemperature = 0;
    bool _waterTankIsPreAlert  = false;
    bool _waterTankIsEmpty     = false;
    // TODO : where to check this value to prevent action.
    bool _criticalTemperatureReached = false;
    // TODO : where to check this value to prevent action.
    bool _isPumping         = false;
    uint16_t _expectedFlowSensorCount  = 0;
    // TODO : where to check this value to prevent action.
    bool _pumpDisabled       = false;

    unsigned int _eepromSchedulesStartAddress = 0;
    unsigned int _eepromJournalStartAddress   = 0;
    uint8_t _nSchedules = 0;
};

} // namespace automated_irrigating_system
