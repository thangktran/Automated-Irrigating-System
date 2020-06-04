#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <NTPClient.h>

#include <functional>

const char* ssid = "TD-LINK-TP";
const char* password = "IgaveYouItalready";

const uint8_t TEMP_SENSOR         = A0;
const uint8_t WATER_EMPTY_WARNING = D0;
const uint8_t FLOW_SENSOR         = D1;
const uint8_t GATE_SOLENOID_1     = D2;
const uint8_t GATE_SOLENOID_2     = D3;
const uint8_t GATE_SOLENOID_3     = D4;
const uint8_t GATE_SOLENOID_4     = D5;
const uint8_t GATE_SOLENOID_5     = D6;
const uint8_t GATE_SOLENOID_6     = D7;
const uint8_t GATE_SOLENOIDS[]    = {
  GATE_SOLENOID_1,
  GATE_SOLENOID_2,
  GATE_SOLENOID_3,
  GATE_SOLENOID_4,
  GATE_SOLENOID_5,
  GATE_SOLENOID_6
};
const uint8_t GATE_PUMP           = D8;
const uint8_t WATER_MONITOR_POWER = D9;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "de.pool.ntp.org", 7200);

void setupWifi();
void setupOta();
void initializeDevice();
void setupNtp();

bool waterIsEmpty();

void setup() {
  Serial.begin(115200);
  Serial.println("Hello world!");

  initializeDevice();
  setupWifi();
  setupOta();
  setupNtp();
}

const uint8_t nPlants = 6;
const float flowSensorRatio = 7.5; // ml/count.
bool waterTime = false;
// water in small amount then stop
// to give the water some times to
// be soaked evenly in the water.
const int loopCount = 3;
int currentCount = 0;
unsigned long lastWaterTime = 0;
uint16_t delayBetweenWatering = 20000; // ms.
// difference plant box needs different delay
// before the water reachs them.
uint8_t compensationInSecond[] = {
  1,
  2,
  3,
  4,
  5,
  6
};
uint16_t waterAmountInMl[] = {
  1500,
  1800,
  1800,
  900,
  900,
  1500
};

const bool pumping = true;
const uint8_t nSchedules = 2;
const uint8_t scheduleTime[nSchedules][2] = {
  // {hour, mintue}
  {6, 00},
  {12, 00}
};

const uint8_t highAmountWater = 18; // seconds
const uint8_t lowAmountWater = 12; // seconds
const uint8_t waterAmountInSecond[nSchedules][nPlants] = {
  {highAmountWater,
  highAmountWater,
  highAmountWater,
  highAmountWater,
  highAmountWater,
  highAmountWater},

  {highAmountWater,
  highAmountWater,
  highAmountWater,
  0,
  0,
  0}
};

void loop() {
  ArduinoOTA.handle();

  timeClient.update();
  int hour = timeClient.getHours();
  int minute = timeClient.getMinutes();

  int scheduleIdx = -1;

  for (int i=0; i<nSchedules; ++i)
  {
    if ( scheduleTime[i][0]==hour && scheduleTime[i][1]==minute)
    {
      scheduleIdx = i;
      break;
    }
  }
  
  if ( scheduleIdx!=-1 && pumping )
  {
    for (int i=0; i<nPlants; ++i)
    {
      digitalWrite(GATE_PUMP, HIGH);
      digitalWrite(GATE_SOLENOIDS[i], HIGH);

      if (waterAmountInSecond[scheduleIdx][i] != 0)
      {
        delay( compensationInSecond[i]*1000 );
        delay(waterAmountInSecond[scheduleIdx][i]*1000); 
      }

      digitalWrite(GATE_PUMP, LOW);
      digitalWrite(GATE_SOLENOIDS[i], LOW);
    }
  }

//  if (waterTime)
//  {
//    Serial.println("WaterTime");
//    unsigned long timeElapsed = millis() - lastWaterTime;
//    if (timeElapsed >= delayBetweenWatering)
//    {
//      for (int i=0; i<nPlants; ++i)
//      {
//        // convert ml to flow-sensor count.
//        uint16_t flowSensorShould = floor((waterAmountInMl[i]/loopCount)/flowSensorRatio);
//        uint16_t flowSensorCount = 0;
//        
//        digitalWrite(GATE_PUMP, HIGH);
//        digitalWrite(GATE_SOLENOIDS[i], HIGH);
//        
//        if (currentCount == 0)
//          delay( compensationInSecond[i]*1000 );
//
//        do{
//          if ( digitalRead(FLOW_SENSOR) )
//            ++flowSensorCount;
//          ESP.wdtFeed();
//        } while (flowSensorCount < flowSensorShould);
//
//        digitalWrite(GATE_PUMP, LOW);
//        digitalWrite(GATE_SOLENOIDS[i], LOW);
//      }
//      
//      lastWaterTime = millis();
//      ++currentCount;
//    }
//
//    if (currentCount == loopCount)
//    {
//      currentCount = 0;
//      waterTime = false;
//    }
//  }

// ==============================
//  Serial.println("Wait 5s before starting");
//  delay (5000);
//  
//  digitalWrite(GATE_SOLENOID_6, HIGH);
//  digitalWrite(GATE_PUMP, HIGH);
//  while(interruptCounter != 50)
//  {
//    yield();
//  }
//  digitalWrite(GATE_SOLENOID_6, LOW);
//  digitalWrite(GATE_PUMP, LOW);
//  Serial.print("interruptCounter: ");
//  Serial.println(interruptCounter);
//  delay (5000);
//  interruptCounter = 0;

  delay(1000);
}


void setupWifi()
{
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
}

void setupOta()
{
  ArduinoOTA.setPort(49867);
  ArduinoOTA.setHostname("Irrigating-System-Controller");
  ArduinoOTA.setPasswordHash("8a9d4daa5b42ac810aff33d1c43fa424");

  ArduinoOTA.onStart([]() {
    initializeDevice();
    
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

//     NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void initializeDevice()
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

void setupNtp()
{
  timeClient.begin();
}

bool waterIsEmpty()
{
  digitalWrite(WATER_MONITOR_POWER, HIGH);
  delay(50);
  bool result = !digitalRead(WATER_EMPTY_WARNING);
  // turn off to save a little bit of current.
  // since I(OL) needs less current than I(OH).
  digitalWrite(WATER_MONITOR_POWER, LOW);

  return result;
}