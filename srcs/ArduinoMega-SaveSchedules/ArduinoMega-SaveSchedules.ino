#include <EEPROM.h>

#include <AutomatedIrrigatingSystem-Shared.h>
#include <AutomatedIrrigatingSystem-MegaOnly.h>


// --------------------GLOBAL VALUES--------------------//
const uint8_t nSchedules = 1;

WaterSchedule schedule1;
WaterSchedule schedule2;
WaterSchedule schedule3;

EepromStore store1;
EepromStore store2;
EepromStore store3;




// --------------------FUNCTIONS--------------------//
void generateSchedules()
{
  MonitorSerial.println("Generating data ...");
  
  // Mint.
  schedule1.device = RELAY_SOLENOID_1;
  uint8_t* days_1 = reinterpret_cast<uint8_t*>(&schedule1.daysToWater);
  *days_1 = 0xFF; // Everyday.
  // { {hour, minute}, ml, watered };
  schedule1.plans[0] = WaterPlan { {06,00} , 900, false};
  schedule1.plans[1] = WaterPlan { {12,00} , 900, false};
  schedule1.plans[2] = WaterPlan { {21,00} , 900, false};

  // Mediteran
  schedule2.device = RELAY_SOLENOID_2;
  uint8_t* days_2 = reinterpret_cast<uint8_t*>(&schedule2.daysToWater);
  *days_2 = 0xFF; // Everyday.
  // { {hour, minute}, ml, watered };
  schedule2.plans[0] = WaterPlan { {06,00} , 400, false};
  schedule2.plans[1] = WaterPlan { {12,00} , 400, false};
  schedule2.plans[2] = WaterPlan { {21,00} , 200, false};

  // Coconut.
  schedule3.device = RELAY_SOLENOID_3;
  uint8_t* days_3 = reinterpret_cast<uint8_t*>(&schedule3.daysToWater);
  *days_3 = 0xFF; // Everyday.
  // { {hour, minute}, ml, watered };
  schedule3.plans[0] = WaterPlan { {06,00} , 200, false};
  schedule3.plans[1] = WaterPlan { {12,00} , 200, false};
  schedule3.plans[2] = WaterPlan { {21,00} , 100, false};
  
  MonitorSerial.println("Data generated.");
}

void packDataBeforeStoring()
{
  MonitorSerial.println("Packing data ...");
  
  store1.schedule = schedule1;
  store1.checksum = dataChecksum (&schedule1, sizeof(WaterSchedule));

//  store2.schedule = schedule2;
//  store2.checksum = dataChecksum (&schedule2, sizeof(WaterSchedule));
//
//  store3.schedule = schedule3;
//  store3.checksum = dataChecksum (&schedule3, sizeof(WaterSchedule));

  MonitorSerial.println("Data packed.");
}

void writeWaterPlansToEeprom()
{
  MonitorSerial.println("Writing data to Eeporm ...");
  
  unsigned int currentAddress = eepromStartAddress;
  EEPROM.put (currentAddress, nSchedules);
  currentAddress += sizeof(uint8_t);

  EEPROM.put (currentAddress, store1);
  currentAddress += sizeof(EepromStore);

//  EEPROM.put (currentAddress, store2);
//  currentAddress += sizeof(EepromStore);
//
//  EEPROM.put (currentAddress, store3);
//  currentAddress += sizeof(EepromStore);

  MonitorSerial.println("Data written.");
}

void checkEepromData()
{
  MonitorSerial.println("Verifying data consistency ...");
  
  bool allDataGood = true;
  
  unsigned int currentAddress = eepromStartAddress;

  uint8_t checkNPlans;
  EEPROM.get(currentAddress, checkNPlans);
  currentAddress += sizeof(uint8_t);
  allDataGood &= checkNPlans == nSchedules;

  EepromStore store;
  
  EEPROM.get (currentAddress, store);
  currentAddress += sizeof(EepromStore);
  allDataGood &= compareObject(&store, &store1, sizeof(EepromStore));

//  EEPROM.get (currentAddress, store);
//  currentAddress += sizeof(EepromStore);
//  allDataGood &= compareObject(&store, &store2, sizeof(EepromStore));
//
//  EEPROM.get (currentAddress, store);
//  currentAddress += sizeof(EepromStore);
//  allDataGood &= compareObject(&store, &store3, sizeof(EepromStore));

  if (allDataGood)
    MonitorSerial.println("All data good.");
  else
    MonitorSerial.println("Data corrupted.");
}




void setup() 
{
  MonitorSerial.begin (115200);
  
  pinMode(LED_BUILTIN, OUTPUT);

  // On means writing.
  digitalWrite(LED_BUILTIN, HIGH);

  generateSchedules();
  packDataBeforeStoring();
  writeWaterPlansToEeprom();
  checkEepromData();

  // Off means finished..
  digitalWrite(LED_BUILTIN, LOW);
  
}

void loop() 
{
}
