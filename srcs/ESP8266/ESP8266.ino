#include <ESP8266WiFi.h>
#include <aREST.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#include <AutomatedIrrigatingSystem-Shared.h>

// --------------------GLOBAL VALUES--------------------//
#define LED 2

// WIFI Parameters.
const uint8_t REST_LISTEN_PORT = 80;
// WIFI credential must match your Access Point.
const char* ssid = "YOUR-ACCESS-POINT-SSID-COMES-HERE";
const char* password = "YOUR-PASSWORD-COMES-HERE";

int devTemp = 0;
bool tankPreAlert = false;
bool tankEmpty = false;
bool waterPlantIsSet = false;
ToWater waterPlantRequest = {0,0};


aREST rest = aREST();
// Create an instance of the server
WiFiServer server(REST_LISTEN_PORT);
WiFiUDP udp;
NTPClient ntpClient(udp, "de.pool.ntp.org");


// --------------------FUNCTIONS--------------------//
int getDeviceTemperature();
int getWaterTankPreAlert();
int getWaterTankEmpty();
int waterPlant();

int waterPlant_REST (String param);

// Delete returned value when done.
// nullptr is returned if timeouted.
MegaData* getMegaData ();
// True if success, false otherwise.
bool sendEsp8266Data (Esp8266Data* data);

void updateDataFromMega();
void sendDataToMega ();
void checkMega ();


void setup() 
{
  Esp8266Serial.begin (115200);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  rest.variable("tankPreAlert",&tankPreAlert);
  rest.variable("tankEmpty",&tankEmpty);
  rest.variable("devTemp",&devTemp);

  // Function to be exposed
  rest.function("waterPlant",waterPlant_REST);

  // Give name & ID to the device (ID should be 6 characters long)
  rest.set_id("Esp8266");
  rest.set_name("IrrigatingSystem");
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Esp8266Serial.print(".");
  }
  Esp8266Serial.println("");
  Esp8266Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Esp8266Serial.println("Server started");
  ntpClient.begin();
  ntpClient.setTimeOffset(2*60*60); // 2 hours.
  ntpClient.update();
  String message = "NTPClient started on ";
  String weekday = "null";
  
  String weekday_store[] = {
    "sun", 
    "mon", 
    "tues", 
    "wed", 
    "thurs", 
    "fri", 
    "sat"};
  weekday = weekday_store[  ntpClient.getDay() ];
  message.concat(weekday);
  message.concat(" at ");
  message.concat(ntpClient.getFormattedTime());
  Esp8266Serial.println(message);

  // Print the IP address
  Esp8266Serial.print("IP: ");
  Esp8266Serial.println(WiFi.localIP());
  Esp8266Serial.println (HANDSHAKE);

  String reply;
  while (!Esp8266Serial.available())
  {
    digitalWrite(LED, LOW);
    delay (2000);
    digitalWrite(LED, HIGH);
    delay (2000);
  }
  
  reply = Esp8266Serial.readStringUntil('\n');

  if (reply.indexOf(HANDSHAKE) == -1)
  {
    // Turn off to indicate initializing finished.
    digitalWrite(LED, LOW);
  }
  else
    digitalWrite(LED, HIGH);

  
}


unsigned long lastCheck = 0;
void loop() 
{  
  // Handle REST calls
  WiFiClient client = server.available();
  if (!client) {
    unsigned long elapsedSecond = (millis()-lastCheck)/1000;
    if (elapsedSecond > checkDataAfter)
    {
      checkMega();
      lastCheck = millis();
    }
    
    return;
  }
  while(!client.available()){
    delay(1);
  }

  rest.handle(client);
}


MegaData* getMegaData ()
{
  MegaData* data = new MegaData;

  uint8_t count = 0;
  
  while (count < nMaxGet)
  {
    String start = Esp8266Serial.readStringUntil ('\n');
    // Not start.
    // use indexOf because there might be garbage data before
    // start signal.
    if (start.indexOf(START) == -1)
    {
      ++count;
      continue;
    }
    
    uint16_t bytesRead = 0;
    bool checksumMatch = false; 

    memset ( data, 0, sizeof(MegaData));
    uint8_t receivedChecksum = 0;
    
    bytesRead += Esp8266Serial.readBytes(reinterpret_cast<unsigned char*>(&data->deviceTemperature), sizeof(uint8_t));
    bytesRead += Esp8266Serial.readBytes(reinterpret_cast<unsigned char*>(&data->waterTankPreAlert), sizeof(uint8_t));
    bytesRead += Esp8266Serial.readBytes(reinterpret_cast<unsigned char*>(&data->waterTankEmpty), sizeof(uint8_t));
    bytesRead += Esp8266Serial.readBytes(reinterpret_cast<unsigned char*>(&data->wateredSequence), sizeof(uint8_t));
    bytesRead += Esp8266Serial.readBytes(reinterpret_cast<unsigned char*>(&receivedChecksum), sizeof(uint8_t));

    String stop = Esp8266Serial.readStringUntil ('\n');
    // Serial.println() appends \r\n. We read until \n,
    // hence \r still trailing at the end.    
    stop.remove ( stop.length()-1 );
    bool enoughBytes = bytesRead== ( sizeof(MegaData)+1 );
    bool objectEmpty = isObjectEmpty (data, sizeof(MegaData));

    if (!enoughBytes || objectEmpty || !stop.equals(STOP))
    {
      ++count;    
      continue;
    }

    uint8_t checksumToCheck = dataChecksum(data, sizeof(MegaData));
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


bool sendEsp8266Data (Esp8266Data* data)
{
  uint8_t checksum = dataChecksum (data, sizeof(Esp8266Data));

  uint8_t count = 0;

  while (count < nMaxSend)
  {
    int bytesWritten;

    do
    {
      bytesWritten = 0;
      
      Esp8266Serial.println (START);
      Esp8266Serial.flush();
      
      bytesWritten += Esp8266Serial.write(reinterpret_cast<unsigned char*>(&data->time.hour), sizeof(uint8_t));
      bytesWritten += Esp8266Serial.write(reinterpret_cast<unsigned char*>(&data->time.minute), sizeof(uint8_t));
      bytesWritten += Esp8266Serial.write(reinterpret_cast<unsigned char*>(&data->weekdays), sizeof(uint8_t));
      bytesWritten += Esp8266Serial.write(reinterpret_cast<unsigned char*>(&data->toWater.sequence), sizeof(uint8_t));
      bytesWritten += Esp8266Serial.write(reinterpret_cast<unsigned char*>(&data->toWater.device), sizeof(uint8_t));
      bytesWritten += Esp8266Serial.write(reinterpret_cast<unsigned char*>(&data->toWater.ml), sizeof(uint16_t));
      bytesWritten += Esp8266Serial.write(reinterpret_cast<unsigned char*>(&checksum), sizeof(uint8_t));
      Esp8266Serial.flush();
      
      Esp8266Serial.println (STOP);
      Esp8266Serial.flush();
      
      // If success, delay to give target sometimes to read.
      // If fail, simply delay before next stream so we dont overload
      // target port.
      delay (10);
      
    } while (bytesWritten != sizeof(Esp8266Data)+1);

    String reply;

    reply = Esp8266Serial.readStringUntil('\n');

    if ( reply.indexOf(ACK) != -1 )
      break;
    else // If timeouted or data not ACK.
      ++count;
  }

  // Number of tries reached.
  if (count >= nMaxSend)
    return false;
  else
    return true;
}


void updateDataFromMega()
{
  MegaData* data = getMegaData();

  if (data == nullptr)
    return;

  devTemp = data->deviceTemperature;
  tankPreAlert = data->waterTankPreAlert;
  tankEmpty = data->waterTankEmpty;

  delete data;
}


void sendDataToMega ()
{
  Esp8266Data dataToSend;

  ntpClient.update();
  uint8_t* castedDay = reinterpret_cast<uint8_t*>(&dataToSend.weekdays);
  *castedDay = 0;
  uint8_t ntpDay = ntpClient.getDay();
  *castedDay = 1 << ntpDay;

  dataToSend.time.hour    = ntpClient.getHours();
  dataToSend.time.minute  = ntpClient.getMinutes();
  
  dataToSend.toWater = {0, 0};
  if (waterPlantIsSet)
  {
    dataToSend.toWater = waterPlantRequest;
    waterPlantIsSet = false;
  }

  sendEsp8266Data (&dataToSend);
}

void checkMega ()
{
  updateDataFromMega();
  sendDataToMega();
}

int waterPlant_REST (String param)
{
  int colonIndex = param.indexOf(':');
  String deviceStr = param.substring (0, colonIndex);
  String mlStr = param.substring (colonIndex+1);
  uint8_t device = static_cast<uint8_t>(deviceStr.toInt());
  uint16_t ml = static_cast<uint16_t>(mlStr.toInt());
  
  DeviceId dev = 0;
  switch (device)
  {
  case 1:
  dev = RELAY_SOLENOID_1;
  break;

  case 2:
  dev = RELAY_SOLENOID_2;
  break;

  case 3:
  dev = RELAY_SOLENOID_3;
  break;
  }

  waterPlantIsSet = true;
  waterPlantRequest.device = dev;
  waterPlantRequest.ml = ml;
  
  return 0;
}
