#include "wifi_services.h"

#include "system_configuration.h"

using namespace automated_irrigating_system;
using namespace automated_irrigating_system::system_configuration;

WifiServices& WifiServices::getInstance()
{
    static WifiServices instance;
    return instance;
}

void WifiServices::initializeServices()
{
    setupWifi();
    setupRestServices();
    setupOtaServices();
    setupNtpServices();
}

void WifiServices::handleServices()
{
    _ntpClient.update();
    ArduinoOTA.handle();

    // TODO : millis(), non-blocking handling.
    // TODO : How to Handle this better and ansynchronously.
    //     unsigned long timeout = millis();
    // while (client.available() == 0) {
    //   if (millis() - timeout > 5000) {
    //     Serial.println(">>> Client Timeout !");
    //     client.stop();
    //     delay(60000);
    //     return;
    //   }
    WiFiClient client = _server.available();
    if (!client) {
        return;
    }
    while(!client.available()){
        delay(1);
    }
  _aRest.handle(client);
}

void WifiServices::addOtaStartExtraHandler (std::function<void(void)> handler)
{
    _otaStartExtraHandler = handler;
}

template<typename T>
void WifiServices::addRestVariable(const char *name, T *var)
{
    _aRest.variable(name, var);
}

void WifiServices::addRestFunction(char * function_name, std::function<int(String)> f)
{
    _aRest.function(function_name, f);
}

int WifiServices::getDay()
{
    return _ntpClient.getDay();
}

String WifiServices::getWeekdayString()
{
    static String weekday_store[] = {
        "sun", 
        "mon", 
        "tues", 
        "wed", 
        "thurs", 
        "fri", 
        "sat"};

    return weekday_store[getDay()];
}

int WifiServices::getHours()
{
    return _ntpClient.getHours();
}

int WifiServices::getMinutes()
{
    return _ntpClient.getMinutes();
}

int WifiServices::getSeconds()
{
    return _ntpClient.getSeconds();
}

String WifiServices::getFormattedTime()
{
    return _ntpClient.getFormattedTime();
}

WifiServices::WifiServices()
    :   _server (REST_LISTEN_PORT),
        _ntpClient (_udp, NTP_HOST)
{}

void WifiServices::setupWifi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, WIFI_PASSWORD);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(1000);
        ESP.restart();
    }
    Serial.println("WiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

void WifiServices::setupRestServices()
{
    _aRest.set_id(REST_ID);
    _aRest.set_name(REST_NAME);

    _server.begin();
    Serial.println("REST Server started");
}

void WifiServices::setupOtaServices()
{
    ArduinoOTA.setHostname(OTA_HOST_NAME);
    ArduinoOTA.setPasswordHash(OTA_MD5_HASHED_PASSWORD);
    ArduinoOTA.setPort(OTA_LISTEN_PORT);
    ArduinoOTA.onStart([this]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_FS
            type = "filesystem";
        }

        if (_otaStartExtraHandler)
            _otaStartExtraHandler();

        // NOTE: if updating FS this would be the place to unmount FS using FS.end()
        Serial.println("OTA: Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("OTA Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) 
        {
            Serial.println("Auth Failed");
        } 
        else if (error == OTA_BEGIN_ERROR) 
        {
            Serial.println("Begin Failed");
        } 
        else if (error == OTA_CONNECT_ERROR) 
        {
            Serial.println("Connect Failed");
        } 
        else if (error == OTA_RECEIVE_ERROR) 
        {
            Serial.println("Receive Failed");
        } 
        else if (error == OTA_END_ERROR) 
        {
            Serial.println("End Failed");
        }
    });
    ArduinoOTA.begin();
    Serial.println("OTA Server started");
}

void WifiServices::setupNtpServices()
{
    _ntpClient.begin();
    _ntpClient.setTimeOffset(NTP_TIME_OFFSET);
    _ntpClient.update();
}