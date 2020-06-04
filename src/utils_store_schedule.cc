#include "utils_store_schedule.h"

#include "pin_mapping.h"
#include "eeprom.h"

#include <HardwareSerial.h>

using namespace automated_irrigating_system;
using namespace automated_irrigating_system::common_structs;
using namespace automated_irrigating_system::pin_mapping;

StoreSchedule::~StoreSchedule()
{}

StoreSchedule& StoreSchedule::getInstance()
{
    static StoreSchedule instance;
    return instance;
}

bool StoreSchedule::storeDataToEeprom()
{
    packDataBeforeStoring();
    writeWaterPlansToEeprom();
    return checkEepromData();
}

StoreSchedule::StoreSchedule()
{}

void StoreSchedule
::addSchedule (DeviceId device, 
               Weekdays days,
               WaterPlan plan1,
               WaterPlan plan2,
               WaterPlan plan3)
{
    _schedules
        .push_back( {device, days, {plan1, plan2, plan3}} );
}


void StoreSchedule::generateDefaultSchedules()
{
    const Weekdays days =
        *reinterpret_cast<Weekdays*>(0xFF); // everyday

    addSchedule(
        GATE_SOLENOID_1, 
        days,
        // { {hour, minute}, ml, watered };
        { {06,00} , 900, false },
        { {12,00} , 900, false },
        { {21,00} , 900, false }
    );
}

void StoreSchedule::packDataBeforeStoring()
{
    Serial.println("Packing data ...");
    
    uint8_t nSchedules = _schedules.size();

    for (auto it=_schedules.begin(); it!=_schedules.end(); ++it)
    {
        EepromStoreStructure eepromStore;
        eepromStore.schedule = *it;
        eepromStore.checksum = Eeprom::dataChecksum (static_cast<void*>(&(*it)), 
            sizeof(EepromStoreStructure));

        _eepromStores.push_back(eepromStore);
    }
    
    
    Serial.println("Data packed.");
}

void StoreSchedule::writeWaterPlansToEeprom()
{
    uint8_t nSchedules = static_cast<uint8_t>( _schedules.size() );
    
    // Initialize EEPROM.
    const int totalEepromBytes =  1 // 1 byte to store nSchedules
            + nSchedules*sizeof(EepromStoreStructure);
    
    Serial.println("Writing data to EEPROM ...");
    
    unsigned int currentAddress = 0;
    Eeprom::getInstance().put (currentAddress, nSchedules);
    currentAddress += sizeof(uint8_t);

    for (auto it=_eepromStores.begin(); it!=_eepromStores.end(); ++it)
    {
        Eeprom::getInstance().put (currentAddress, *it);
        currentAddress += sizeof(EepromStoreStructure);
    }
    
    Serial.println("Data written.");
}

bool StoreSchedule::checkEepromData()
{
    uint8_t nSchedules = static_cast<uint8_t>( _schedules.size() );
    
    // Initialize EEPROM.
    const int totalEepromBytes =  1 // 1 byte to store nSchedules
            + nSchedules*sizeof(EepromStoreStructure);
    
    Serial.println("Verifying data consistency ...");
    
    bool allDataGood = true;
    unsigned int currentAddress = 0;
    
    uint8_t checkNPlans;
    Eeprom::getInstance().get(currentAddress, checkNPlans);
    // check value.
    allDataGood &= checkNPlans == nSchedules;
    // advance memory address
    currentAddress += sizeof(uint8_t);
    
    for (auto it=_eepromStores.begin(); it!=_eepromStores.end(); ++it)
    {
        EepromStoreStructure currentEepromStore;
        Eeprom::getInstance().get (currentAddress, currentEepromStore);
        // advance memory address.
        currentAddress += sizeof(EepromStoreStructure);

        // check value.
        allDataGood &= 
            Eeprom::compareObject(&currentEepromStore, 
                                  static_cast<void*>(&(*it)), 
                                  sizeof(EepromStoreStructure));
    }
    
    if (allDataGood)
        Serial.println("All data good.");
    else
        Serial.println("Data corrupted.");
    
    return allDataGood;
}