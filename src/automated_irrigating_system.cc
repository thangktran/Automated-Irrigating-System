#include "automated_irrigating_system.h"

#include "wifi_services.h"
#include "system_configuration.h"
#include "pin_mapping.h"
#include "eeprom.h"

using namespace automated_irrigating_system;
using namespace automated_irrigating_system::common_structs;
using namespace automated_irrigating_system::pin_mapping;
using namespace automated_irrigating_system::system_configuration;

AutomatedIrrigatingSystem& AutomatedIrrigatingSystem::getInstance()
{
    static AutomatedIrrigatingSystem i;
    return i;
}

void AutomatedIrrigatingSystem::setPinMode (DeviceId device, uint8_t mode)
{
    pinMode (static_cast<uint8_t>(device), mode);
}

void AutomatedIrrigatingSystem::switchOnDevice (DeviceId device)
{
    digitalWrite (static_cast<uint8_t>(device), HIGH);
}

void AutomatedIrrigatingSystem::switchOffDevice (DeviceId device)
{   
    digitalWrite (static_cast<uint8_t>(device), LOW); 
}

bool AutomatedIrrigatingSystem::digitalReadDevice (DeviceId device)
{
    int rc = digitalRead (static_cast<uint8_t>(device));
    return rc == HIGH ? true : false;
}

int AutomatedIrrigatingSystem::analogReadDevice (DeviceId device)
{
    return analogRead(static_cast<uint8_t>(device));
}

void AutomatedIrrigatingSystem::initializeDevice()
{
    initializePins();
    WifiServices::getInstance().initializeServices();

    // TODO : EEPROM journal checking, restoring?
    //  EEPROM.get (eepromSchedulesStartAddress, nSchedules); 
    // eepromSchedulesStartAddress += sizeof(uint8_t);
    // MonitorSerial.print ("GOT ");
    // MonitorSerial.print (nSchedules);
    // MonitorSerial.println (" schedules from EEPROM.");
    // eepromJournalStartAddress = 
    // eepromSchedulesStartAddress + nSchedules*sizeof(EepromStore);

    // // Check journal and restore data.
    // unsigned int currentAddress = eepromJournalStartAddress;
    // uint8_t scheduleIndex = 0;
    // EEPROM.get (currentAddress, scheduleIndex);
    // currentAddress += sizeof(uint8_t);
    // EepromStore store;
    // EEPROM.get (currentAddress, store);
  
    // // Journal is empty.
    // if (isObjectEmpty(&store, sizeof(EepromStore)))
    //     MonitorSerial.println ("No JOURNAL exists.");
    // // Journal exists but checksum doesn't match -> simply delete journal.
    // else if ( store.checksum != dataChecksum(&store.schedule, sizeof(WaterSchedule)) )
    // {
    //     MonitorSerial.println ("JOURNAL exists but checksum doesn't match. Skip restoring...");
    //     memset (&store, 0, sizeof(EepromStore));
    //     EEPROM.put (currentAddress, store);
    // }
    // // Journal exists and checksum match. restoring...
    // else if ( store.checksum == dataChecksum(&store.schedule, sizeof(WaterSchedule)) )
    // {
    //     MonitorSerial.println ("JOURNAL exists. Restoring...");
    //     // Checksum match, restore data.
    //     unsigned int toWriteAddress = eepromSchedulesStartAddress;
    //     toWriteAddress += scheduleIndex*sizeof(EepromStore);
    //     EEPROM.put (toWriteAddress, store);
    //     // Empty it.
    //     memset (&store, 0, sizeof(EepromStore));
    //     EEPROM.put (currentAddress, store);
    //     MonitorSerial.println ("Restoring finshed.");
    // }

    // TODO : register aREST variables and functions.

    // TODO : FlowSensor INTERRUPT HANDLER ?
}

void AutomatedIrrigatingSystem::handleEvents()
{
    WifiServices::getInstance().handleServices();
}

bool AutomatedIrrigatingSystem::timeInRange (const Time& currentTime, 
                                             const Time& toCompare)
{
    int8_t hourDiff = currentTime.hour - toCompare.hour;
    int8_t minuteDiff = currentTime.minute - toCompare.minute;

    if (minuteDiff < 0)
    {
        hourDiff -= 1;
        minuteDiff += 60;
    }

    if (hourDiff < 0)
        return false;
    
    uint16_t timeToCompare = (hourDiff*60 + minuteDiff);
  
    if ( timeToCompare <= MAX_MINUTE_DIFFERENCE )
        return true;
    else
        return false;
}

float AutomatedIrrigatingSystem::getDeviceTemperature()
{
    // TODO : Analog pin name?
    int tempReading = analogReadDevice(TEMP_SENSOR);

    // TODO : Verify formular .
    // This is OK
    double tempK = log(10000.0 * ((1024.0 / tempReading - 1)));
    tempK = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * tempK * tempK )) * tempK );
    // Convert Kelvin to Celcius  
    _deviceTemperature = tempK - 273.15; 

    return _deviceTemperature;
}

// TODO : Check Implementation.
bool AutomatedIrrigatingSystem::isWaterTankEmpty()
{
    switchOnDevice (WATER_MONITOR_POWER);
    // true == there is connection => there's still water.
    _waterTankIsEmpty = !digitalReadDevice(WATER_EMPTY_WARNING);
    switchOffDevice (WATER_MONITOR_POWER);
    return _waterTankIsEmpty;
}

void AutomatedIrrigatingSystem::checkDeviceTemperature()
{
    getDeviceTemperature();

    if (_deviceTemperature >= CRITICAL_TEMPERATURE)
        _criticalTemperatureReached = true;
    else
        _criticalTemperatureReached = false;
}

// TODO : Implement.
void AutomatedIrrigatingSystem::startPumping (DeviceId device, uint16_t ml)
{
  // TODO : calculate number of flow sensor turn.
  // TODO : set is pumping to true.
  // TODO : turn on pump.
  // TODO : turn on solenoid.
}
void AutomatedIrrigatingSystem::stopPumping (DeviceId device)
{
  switchOffDevice(device);
  switchOffDevice(GATE_PUMP);
  _isPumping = false;
}

// TODO : implement
void AutomatedIrrigatingSystem::resetWaterPlan (uint8_t scheduleIndex)
{}

// TODO : implement
void AutomatedIrrigatingSystem::checkWaterPlan()
{}

bool AutomatedIrrigatingSystem::waterPlant (DeviceId device, uint16_t ml)
{
    if(_isPumping)
    {
        return false;
    }

    startPumping(device, ml);
    return true;
}

void AutomatedIrrigatingSystem::initializePins()
{
    pinMode(WATER_EMPTY_WARNING, INPUT);
    pinMode(FLOW_SENSOR, INPUT);
    pinMode(GATE_SOLENOID_1, OUTPUT);
    pinMode(GATE_SOLENOID_2, OUTPUT);
    pinMode(GATE_SOLENOID_3, OUTPUT);
    pinMode(GATE_SOLENOID_4, OUTPUT);
    pinMode(GATE_SOLENOID_5, OUTPUT);
    pinMode(GATE_SOLENOID_6, OUTPUT);
    pinMode(GATE_PUMP, OUTPUT);
    pinMode(WATER_MONITOR_POWER, OUTPUT);

    digitalWrite(GATE_SOLENOID_1, LOW);
    digitalWrite(GATE_SOLENOID_2, LOW);
    digitalWrite(GATE_SOLENOID_3, LOW);
    digitalWrite(GATE_SOLENOID_4, LOW);
    digitalWrite(GATE_SOLENOID_5, LOW);
    digitalWrite(GATE_SOLENOID_6, LOW);
    digitalWrite(GATE_PUMP, LOW);
    digitalWrite(WATER_MONITOR_POWER, LOW);
}

// TODO : implement
int AutomatedIrrigatingSystem::REST_waterPlant (String param)
{}

// TODO : set up interrupt mode and interurpt pin for flow sensor.
// TODO : implement (disable all interrupt, handle, enable all interrupt.)
void AutomatedIrrigatingSystem::flowSensorInterruptHandler()
{}