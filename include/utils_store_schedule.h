#pragma once

#include <list>

#include "common_structs.h"

namespace automated_irrigating_system
{

class StoreSchedule
{
public:
    ~StoreSchedule();
    
    static StoreSchedule& getInstance();
    
    void addSchedule (common_structs::DeviceId device, 
                      common_structs::Weekdays days,
                      common_structs::WaterPlan plan1,
                      common_structs::WaterPlan plan2,
                      common_structs::WaterPlan plan3);
    void generateDefaultSchedules();
    bool storeDataToEeprom();
    
private:
    StoreSchedule();
    StoreSchedule(StoreSchedule const&)     = delete;
    void operator=(StoreSchedule const&)    = delete;
    
    void packDataBeforeStoring();
    void writeWaterPlansToEeprom();
    bool checkEepromData();
    
    std::list<common_structs::WaterSchedule>        _schedules;
    std::list<common_structs::EepromStoreStructure> _eepromStores;
};

} // namespace automated_irrigating_system