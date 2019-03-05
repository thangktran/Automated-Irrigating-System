#include <avr/wdt.h>

#include <EEPROM.h>

#include <AutomatedIrrigatingSystem-Shared.h>
#include <AutomatedIrrigatingSystem-MegaOnly.h>


// --------------------GLOBAL VALUES--------------------//
const uint8_t maxTemperature = 85; // Celcius.
const uint8_t maxMinuteDeviation = 30;

uint8_t deviceTemperature = 0;
bool notifiedWaterTankPreAlert = false;
bool notifiedWaterTankIsEmpty = false;
bool waterTankIsPreAlert  = false;
bool waterTankIsEmpty     = false;

unsigned int eepromSchedulesStartAddress = eepromStartAddress;
unsigned int eepromJournalStartAddress = 0;
uint8_t nSchedules = 0;

unsigned long lastUpdate = 0;
Time selfTiming = Time{25, 61};
Weekdays selfWeekdays = Weekdays {0,0,0,0,0,0,0,0};


void setPinMode (DeviceId device, uint8_t mode);
void switchOnDevice (DeviceId device);
void switchOffDevice (DeviceId device);
void switchOnRelayDevice (DeviceId device);
void switchOffRelayDevice (DeviceId device);
bool digitalReadDevice (DeviceId device);
int analogReadDevice (DeviceId device);
void deviceDefaultMode();
void restartWifiModule();


float getDeviceTemperature();
bool isWaterTankPreAlert();
bool isWaterTankEmpty();

bool timeInRange (Time* currentTime, Time* toCompare);
// true if ml was filled.
// false if not.
bool waitTillMl (uint16_t ml);
bool waterPlant (DeviceId device, uint16_t ml);

void setTime(Esp8266Data* data);
void setWeekdays (Esp8266Data* data);

// Delete returned value when done.
// nullptr is returnt if timeouted.
Esp8266Data* getEsp8266Data ();
// true if success, false otherwise.
bool sendMegaData (MegaData* data);
void sendDataToEsp8266();
void updateDataFromEsp8266 ();
void checkEsp8266();


void resetWaterPlan (uint8_t scheduleIndex);
// WARNING: DO NOT run the pump and open all 3 solenoids at the same time.
// The power supply and wires weren't design to handle those.
void checkWaterPlan();
void checkDeviceTemperature();


void setup() {
  wdt_enable(WDTO_8S);
  
  MonitorSerial.begin (115200);
  Esp8266Serial.begin (115200);
  
  setPinMode (WIFI_MODULE_RST, OUTPUT);
  setPinMode (WIFI_MODULE_CPD, OUTPUT);
  
  
  setPinMode (WATER_LEVEL_POWER, OUTPUT);
  setPinMode (WATER_LEVEL_PREALERT, INPUT);
  setPinMode (WATER_LEVEL_BOTTOM, INPUT);

  setPinMode (RELAY_SOLENOID_1, OUTPUT);
  setPinMode (RELAY_SOLENOID_2, OUTPUT);
  setPinMode (RELAY_SOLENOID_3, OUTPUT);
  setPinMode (RELAY_PUMP, OUTPUT);

  deviceDefaultMode();
  restartWifiModule();
  
  // Call these functions to update their values.
  isWaterTankEmpty();
  isWaterTankPreAlert();
  getDeviceTemperature();
  
  bool handshook = false;

  wdt_reset();
  while (!handshook)
  {
    if (Esp8266Serial.available())
    {
        String value = Esp8266Serial.readStringUntil('\n');
        MonitorSerial.println (value);
        
        if (value.indexOf (HANDSHAKE) != -1)
          handshook = true;
    }
  }
  MonitorSerial.println ("WiFi Module initialized...");
  
  wdt_reset();
  
  EEPROM.get (eepromSchedulesStartAddress, nSchedules); 
  eepromSchedulesStartAddress += sizeof(uint8_t);
  MonitorSerial.print ("GOT ");
  MonitorSerial.print (nSchedules);
  MonitorSerial.println (" schedules from EEPROM.");
  eepromJournalStartAddress = 
  eepromSchedulesStartAddress + nSchedules*sizeof(EepromStore);

  // Check journal and restore data.
  unsigned int currentAddress = eepromJournalStartAddress;
  uint8_t scheduleIndex = 0;
  EEPROM.get (currentAddress, scheduleIndex);
  currentAddress += sizeof(uint8_t);
  EepromStore store;
  EEPROM.get (currentAddress, store);
  
  // Journal is empty.
  if (isObjectEmpty(&store, sizeof(EepromStore)))
    MonitorSerial.println ("No JOURNAL exists.");
  // Journal exists but checksum doesn't match -> simply delete journal.
  else if ( store.checksum != dataChecksum(&store.schedule, sizeof(WaterSchedule)) )
  {
    MonitorSerial.println ("JOURNAL exists but checksum doesn't match. Skip restoring...");
    memset (&store, 0, sizeof(EepromStore));
    EEPROM.put (currentAddress, store);
  }
  // Journal exists and checksum match. restoring...
  else if ( store.checksum == dataChecksum(&store.schedule, sizeof(WaterSchedule)) )
  {
    MonitorSerial.println ("JOURNAL exists. Restoring...");
    // Checksum match, restore data.
    unsigned int toWriteAddress = eepromSchedulesStartAddress;
    toWriteAddress += scheduleIndex*sizeof(EepromStore);
    EEPROM.put (toWriteAddress, store);
    // Empty it.
    memset (&store, 0, sizeof(EepromStore));
    EEPROM.put (currentAddress, store);
    MonitorSerial.println ("Restoring finshed.");
  }

  // Tell Esp to start now.
  Esp8266Serial.println(HANDSHAKE);
  Esp8266Serial.flush();

  wdt_reset();
  
  // Wait till it gets data from Esp8266.
  MonitorSerial.print("Synchonizing");
  unsigned long last = 0;
  while (selfTiming.hour>=24 || selfTiming.minute>=60)
  {
    updateDataFromEsp8266();
    if (millis()-last > 100)
    {
      MonitorSerial.print(".");
      last = millis();
    }
  }
  MonitorSerial.println();
  MonitorSerial.println ("Irrigating System started.");
  wdt_reset();
}


unsigned long lastCheck = 0;
void loop() 
{
  
  checkDeviceTemperature();
  isWaterTankPreAlert();
  isWaterTankEmpty();

  checkEsp8266();
  checkWaterPlan();
    
  unsigned long elapsedSecond = (millis()-lastCheck)/1000;
  if (elapsedSecond > 1)
  {
    lastCheck = millis();
    MonitorSerial.print ("Time from loop ");
    MonitorSerial.print (*reinterpret_cast<uint8_t*>(&selfWeekdays));
    MonitorSerial.print (" ");
    MonitorSerial.print (selfTiming.hour);
    MonitorSerial.print (":");
    MonitorSerial.println (selfTiming.minute);
  }

  wdt_reset();

//  MonitorSerial.println ("Testing water. Give seconds...");
//  while (!MonitorSerial.available());
//  String second = MonitorSerial.readString();
//  int realSec = second.toInt();
//  MonitorSerial.print ("Got ");
//  MonitorSerial.print (realSec);
//  MonitorSerial.print (" seconds...");
//  delay (500);
//  switchOnRelayDevice (RELAY_PUMP);
//  delay (realSec*1000);
//  switchOffRelayDevice (RELAY_PUMP);
}


void setPinMode (DeviceId device, uint8_t mode)
{
    pinMode (static_cast<uint8_t>(device), mode);
}

void switchOnDevice (DeviceId device)
{
  digitalWrite (static_cast<uint8_t>(device), HIGH);
}

void switchOffDevice (DeviceId device)
{
  digitalWrite (static_cast<uint8_t>(device), LOW); 
}


void switchOnRelayDevice (DeviceId device)
{
  // Relay is active LOW.
  digitalWrite (static_cast<uint8_t>(device), LOW);
}

void switchOffRelayDevice (DeviceId device)
{
  // Relay is active LOW.
  digitalWrite (static_cast<uint8_t>(device), HIGH); 
}

bool digitalReadDevice (DeviceId device)
{
  int rc = digitalRead (static_cast<uint8_t>(device));
  return rc == LOW ? false : true;
}


int analogReadDevice (DeviceId device)
{
  return analogRead(static_cast<uint8_t>(device));
}


void deviceDefaultMode()
{
  switchOffDevice (WIFI_MODULE_RST);
  
  switchOffDevice (WATER_LEVEL_POWER);
  
  switchOffRelayDevice (RELAY_SOLENOID_1);
  switchOffRelayDevice (RELAY_SOLENOID_2);
  switchOffRelayDevice (RELAY_SOLENOID_3);
  switchOffRelayDevice (RELAY_PUMP);
}


void restartWifiModule()
{
  switchOffDevice (WIFI_MODULE_CPD);
  switchOffDevice (WIFI_MODULE_RST);
  delay (100);
  switchOnDevice (WIFI_MODULE_CPD);
  switchOnDevice (WIFI_MODULE_RST);
  
}


float getDeviceTemperature()
{
  int tempReading =analogReadDevice(NTC_SENSOR);

  // This is OK
  double tempK = log(10000.0 * ((1024.0 / tempReading - 1)));
  tempK = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * tempK * tempK )) * tempK );
  // Convert Kelvin to Celcius  
  deviceTemperature = tempK - 273.15; 

  return deviceTemperature;
}


bool isWaterTankPreAlert()
{
  switchOnDevice (WATER_LEVEL_POWER);
  waterTankIsPreAlert = !digitalReadDevice(WATER_LEVEL_PREALERT);
  switchOffDevice (WATER_LEVEL_POWER);
  return waterTankIsPreAlert;
}


bool isWaterTankEmpty()
{
  switchOnDevice (WATER_LEVEL_POWER);
  waterTankIsEmpty = !digitalReadDevice(WATER_LEVEL_BOTTOM);
  switchOffDevice (WATER_LEVEL_POWER);
  return waterTankIsEmpty;
}


bool timeInRange (Time* currentTime, Time* toCompare)
{
  int8_t hourDiff = currentTime->hour - toCompare->hour;
  int8_t minuteDiff = currentTime->minute - toCompare->minute;

  if (minuteDiff < 0)
  {
    hourDiff -= 1;
    minuteDiff += 60;
  }

  if (hourDiff < 0)
    return false;
    
  uint16_t timeToCompare = (hourDiff*60 + minuteDiff);
  
  if ( timeToCompare <= maxMinuteDeviation )
    return true;
  else
    return false;
}


// true if ml was filled.
// false if not.
bool waitTillMl (uint16_t ml)
{
  // From experiment: 500ml took 12 seconds.
  uint16_t secondsToRun = ml*12/500;

  MonitorSerial.print ("secondsToRun ");
  MonitorSerial.println (secondsToRun);
  
  unsigned long start = millis();

  while ( (millis()-start)/1000 < secondsToRun)
  {
    wdt_reset();

    // FIXME : It seems like the motor draw lots 
    // of current and cause a voltage drop to the
    // WaterCheckPower.
//      isWaterTankEmpty();
//      if (waterTankIsEmpty)
//      {
//        MonitorSerial.println ("Water Tank is empty.");
//        return false;    
//      }
  }


  return true;
}


bool waterPlant (DeviceId device, uint16_t ml)
{
  if (ml == 0)
    return true;
    
  switchOnRelayDevice (device);
  switchOnRelayDevice (RELAY_PUMP);

  bool filled = waitTillMl (ml);

  switchOffRelayDevice (RELAY_PUMP);
  switchOffRelayDevice (device);

  return filled;
}


void setTime(Esp8266Data* data)
{
  if (data == nullptr)
  {
    unsigned long second = (millis()-lastUpdate) / 1000;
    long minute = second / 60;

    if (minute <= 0)
      return;
    else
      lastUpdate = millis();

    uint16_t minutes = selfTiming.minute + minute;
  
    if (minutes >= 60)
    {
      selfTiming.hour += minutes / 60;
      selfTiming.minute = minutes % 60;
    }
    else
      selfTiming.minute = minutes;

    if (selfTiming.hour >= 24)
    {
      selfTiming.hour = 0;
      uint8_t* castedDay = reinterpret_cast<uint8_t*>(&selfWeekdays);
      if (*castedDay == 0)
        *castedDay = 1;
      else
        *castedDay = *castedDay << 1;
    }
  }
  else
  {
    lastUpdate = millis();
    selfTiming = data->time;  
  }

  return selfTiming;
}


void setWeekdays (Esp8266Data* data)
{
  Weekdays toReturn;

  if (data != nullptr)
  {
    selfWeekdays = data->weekdays;
  }

  return selfWeekdays;
}

void checkWaterPlantRequest (Esp8266Data* data)
{
  bool devExists = data->toWater.device==RELAY_SOLENOID_1
                || data->toWater.device==RELAY_SOLENOID_2
                || data->toWater.device==RELAY_SOLENOID_3;
  if (data == nullptr || data->toWater.ml == 0 || !devExists)
    return;

  MonitorSerial.print ("checkWaterPlantRequest() ");
  MonitorSerial.print (*reinterpret_cast<uint8_t*>(&data->toWater.device));
  MonitorSerial.print (":");
  MonitorSerial.println (*reinterpret_cast<uint16_t*>(&data->toWater.ml));
  
  waterPlant (data->toWater.device, data->toWater.ml);
}

// true if success, false otherwise.
bool sendMegaData (MegaData* data)
{
  uint8_t checksum = dataChecksum (data, sizeof(MegaData));

  uint8_t count = 0;

  while (count < nMaxSend)
  {
    int bytesWritten;
    
    do
    {
      bytesWritten = 0;
      
      Esp8266Serial.println (START);
      Esp8266Serial.flush();
      bytesWritten += Esp8266Serial.write(reinterpret_cast<unsigned char*>(&data->deviceTemperature), sizeof(uint8_t));
      bytesWritten += Esp8266Serial.write(reinterpret_cast<unsigned char*>(&data->waterTankPreAlert), sizeof(uint8_t));
      bytesWritten += Esp8266Serial.write(reinterpret_cast<unsigned char*>(&data->waterTankEmpty), sizeof(uint8_t));
      bytesWritten += Esp8266Serial.write(reinterpret_cast<unsigned char*>(&data->wateredSequence), sizeof(uint8_t));
      bytesWritten += Esp8266Serial.write(reinterpret_cast<unsigned char*>(&checksum), sizeof(uint8_t));
      Esp8266Serial.flush();
      Esp8266Serial.println (STOP);
      Esp8266Serial.flush();
      
      // If success, delay to give target sometimes to read.
      // If fail, simply delay before next stream so we dont overload
      // target port.
      delay (10);
        
    } while (bytesWritten != (sizeof(MegaData)+1) );

    
    String reply;
    reply = Esp8266Serial.readStringUntil('\n');

    // If timeouted or data not ACK.
    if ( reply.equals(ACK) )
      break;
    else
      ++count;    
  }

  // Number of tries reached.
  if (count >= nMaxSend)
    return false;
  else
    return true;
}

Esp8266Data* getEsp8266Data ()
{
  Esp8266Data* data = new Esp8266Data;

  uint8_t count = 0;

  while (count < nMaxGet)
  {
    String start = Esp8266Serial.readStringUntil ('\n');

    // use indexOf because there might be garbage data before
    // start signal.
    if (start.indexOf(START) == -1)
    {
      ++count;
      continue;
    }
    
    uint16_t bytesRead = 0;
    bool checksumMatch = false; 

    memset ( data, 0, sizeof(Esp8266Data));
    uint8_t receivedChecksum = 0;

    bytesRead += Esp8266Serial.readBytes(reinterpret_cast<unsigned char*>(&data->time.hour), sizeof(uint8_t));
    bytesRead += Esp8266Serial.readBytes(reinterpret_cast<unsigned char*>(&data->time.minute), sizeof(uint8_t));
    bytesRead += Esp8266Serial.readBytes(reinterpret_cast<unsigned char*>(&data->weekdays), sizeof(uint8_t));
    bytesRead += Esp8266Serial.readBytes(reinterpret_cast<unsigned char*>(&data->toWater.sequence), sizeof(uint8_t));
    bytesRead += Esp8266Serial.readBytes(reinterpret_cast<unsigned char*>(&data->toWater.device), sizeof(uint8_t));
    bytesRead += Esp8266Serial.readBytes(reinterpret_cast<unsigned char*>(&data->toWater.ml), sizeof(uint16_t));
    bytesRead += Esp8266Serial.readBytes(reinterpret_cast<unsigned char*>(&receivedChecksum), sizeof(uint8_t));

    String stop = Esp8266Serial.readStringUntil ('\n');
    // Serial.println() appends \r\n. We read until \n,
    // hence \r still trailing at the end.    
    stop.remove ( stop.length()-1 );

    bool enoughBytes = bytesRead == ( sizeof(Esp8266Data)+1 );
    bool objectEmpty = isObjectEmpty (data, sizeof(Esp8266Data));

    // Use equals() since we expect to receive the correct
    // amount of bytes.
    if (!enoughBytes || objectEmpty || !stop.equals(STOP))
    {
      ++count;
      continue;
    }

    uint8_t checksumToCheck = dataChecksum(data, sizeof(Esp8266Data));
    checksumMatch = receivedChecksum == checksumToCheck;

    if (checksumMatch)
      Esp8266Serial.println (ACK);
    else
      Esp8266Serial.println (NACK);

    Esp8266Serial.flush();
    
    if (checksumMatch)
      break;
    else
      ++count;

  }

  // Number of tries reached.
  if (count >= nMaxGet)
  {
    // If time out, delete data and return nullptr.
    // The user will simply delete a nullptr and nothing happens.
    delete data;
    data = nullptr;
  }
  
  return data; 
}

void sendDataToEsp8266()
{
  MegaData toSendData;
  toSendData.deviceTemperature = deviceTemperature;
  toSendData.waterTankPreAlert = waterTankIsPreAlert;
  toSendData.waterTankEmpty = waterTankIsEmpty;
  sendMegaData (&toSendData);
}

uint8_t count = 0;
const uint8_t maxCount =30;
void updateDataFromEsp8266 ()
{
  Esp8266Data* data = getEsp8266Data();

  // If we failed to get data from Esp8266
  // more than maxCount in a row, Esp8266 may have been hanging.
  // Stay in a loop and let watchdog reset the Mega.
  while (count++ > maxCount);
  
  setTime (data);
  setWeekdays (data);
  checkWaterPlantRequest(data);
  delete data;
}

void checkEsp8266()
{
  sendDataToEsp8266();
  updateDataFromEsp8266();
}

void resetWaterPlan (uint8_t scheduleIndex)
{
  unsigned int eepromSchedulesAddress = eepromSchedulesStartAddress;
  eepromSchedulesAddress += scheduleIndex*sizeof(EepromStore);

  EepromStore store;
  EEPROM.get (eepromSchedulesAddress, store);
  WaterSchedule* schedule = &store.schedule;

  for (uint8_t j=0; j < maxWaterPlans; ++j)
  {
    schedule->plans[j].watered = false;
  }

  // Calculate new checksum.
  store.checksum = dataChecksum (schedule, sizeof(WaterSchedule));
      
  unsigned int eepromJournalToWrite = eepromJournalStartAddress;

  // Write schedule index.
  EEPROM.put (eepromJournalToWrite, scheduleIndex);
  eepromJournalToWrite += sizeof(uint8_t);

  // Write journal.
  EEPROM.put (eepromJournalToWrite, store);

  // Write real data.
  EEPROM.put (eepromSchedulesAddress, store);

  // Earase journal.
  memset (&store, 0, sizeof(EepromStore));
  EEPROM.put (eepromJournalToWrite, store);
}

// WARNING: DO NOT run the pump and open all 3 solenoids at the same time.
// The power supply and wires weren't design to handle those.
void checkWaterPlan()
{
  if (waterTankIsEmpty)
  {
    // Recheck.
    isWaterTankEmpty();
    return;
  }

  Weekdays weekdays = selfWeekdays;
  uint8_t* weekdaysToCompare = reinterpret_cast<uint8_t*>(&weekdays);
  
  // If getWeekdays() failed.
  if (weekdaysToCompare == 0)
  {
    MonitorSerial.println ("getWeekdays() failed... Skipping watering plants...");
    return;
  }

  Time  time = selfTiming;
  
  // If getTime() failed.
  if (time.hour >= 24 || time.minute >= 60)
  {
    MonitorSerial.println ("getTime() failed... Skipping watering plants...");
    return;
  }
    
  unsigned int eepromSchedulesAddress = eepromSchedulesStartAddress;
  
  for (uint8_t i=0; i<nSchedules; ++i)
  {
    
    EepromStore store;
    EEPROM.get (eepromSchedulesAddress, store);
    WaterSchedule* schedule = &store.schedule;
    
    if ( store.checksum != dataChecksum(schedule, sizeof(WaterSchedule)) )
    {
      String message = "Data is corrupted. Skipping schedule for device ";
      message.concat( String(static_cast<uint8_t>(schedule->device)) );
      MonitorSerial.println (message);
      continue;   
    }

    Weekdays* weekdays_tmp =  &schedule->daysToWater;
    uint8_t* weekdaysToCompare_tmp = reinterpret_cast<uint8_t*>(weekdays_tmp);

    // If today is not watering day.
    bool isWaterDay = *weekdaysToCompare | *weekdaysToCompare_tmp;
    if ( !isWaterDay )
    {
      MonitorSerial.println ("Not water day... Skipping watering plants...");
      MonitorSerial.print ("todayWeekDay ");
      MonitorSerial.println(*weekdaysToCompare);
      MonitorSerial.print ("weekdaysToCompare_tmp ");
      MonitorSerial.println(*weekdaysToCompare_tmp);
      MonitorSerial.print ("isWaterDay ");
      MonitorSerial.println(isWaterDay);
      continue;
    }

    for (uint8_t j=0; j < maxWaterPlans; ++j)
    {
      Time* time_tmp = &schedule->plans[j].time;

      bool timeToWater = timeInRange (&time, time_tmp);
      
      if ( schedule->plans[j].watered )
      {
        // If this is the last WaterPlan and it's
        // already watered. Reset all plans of this schedule
        // to prepare for next day.
        // Note that we should only reset if the plan is not
        // in the allowed deviation. Otherwise we stuck in a
        // watering loop until timeToWater is false.
        if (j == (maxWaterPlans-1) && !timeToWater)
        {
          MonitorSerial.print ("Resetting schedule ");
          MonitorSerial.println (i);
          resetWaterPlan (i);    
          MonitorSerial.println ("Schedule resetted...");
        }
        continue;
      }

      if ( !timeToWater )
        continue;

      // Water this plan.
      bool watered = waterPlant (schedule->device, schedule->plans[j].ml);

      // If not watered, simply skip it.
      if (!watered)
      {
        MonitorSerial.println ("Error : Couldn't Water plant...");
        continue;
      }
      
      // If watered, write back to EEPROM.
      schedule->plans[j].watered = watered;

      // Calculate new checksum.
      store.checksum = dataChecksum (schedule, sizeof(WaterSchedule));
      
      unsigned int eepromJournalToWrite = eepromJournalStartAddress;

      // Write schedule index.
      EEPROM.put (eepromJournalToWrite, i);
      eepromJournalToWrite += sizeof(uint8_t);

      // Write journal.
      EEPROM.put (eepromJournalToWrite, store);

      // Write real data.
      EEPROM.put (eepromSchedulesAddress, store);

      // Earase journal.
      memset (&store, 0, sizeof(EepromStore));
      EEPROM.put (eepromJournalToWrite, store);
    }
      

    eepromSchedulesAddress += sizeof(EepromStore);
  }
}

void checkDeviceTemperature()
{
  bool notified = false;

  // If it's overheated. Donothing and run
  // in loop till it cooled down.
  // Avoid switch on power and stuffs.
  while(getDeviceTemperature() > maxTemperature)
  {
    if (!notified)
    { 
      // TODO Notify over heat.

      MonitorSerial.println("Device is overheated....");
      notified = true;
    }
  }

  if (notified)
  {
    MonitorSerial.println("Device cooled down....");
    // Device is not overheated or tt has cooled down.
    notified = false;
  }
}
