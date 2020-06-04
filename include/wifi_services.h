#pragma once

#include <ESP8266WiFi.h>
#include <aREST.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

namespace automated_irrigating_system
{

class WifiServices
{
public:
    static WifiServices& getInstance();

    void initializeServices();
    void handleServices();

    void addOtaStartExtraHandler (std::function<void(void)> handler);

    template<typename T>
    void addRestVariable(const char *name, T *var);
    void addRestFunction(char * function_name, std::function<int(String)> f);

    int getDay();
    String getWeekdayString();
    int getHours();
    int getMinutes();
    int getSeconds();
    String getFormattedTime();

private:
    WifiServices();
    WifiServices(WifiServices const&)     = delete;
    void operator=(WifiServices const&)   = delete;

    void setupWifi();
    void setupRestServices();
    void setupOtaServices();
    void setupNtpServices();


    WiFiServer  _server;
    aREST       _aRest;
    WiFiUDP     _udp;
    NTPClient   _ntpClient;
    std::function<void(void)> _otaStartExtraHandler;
};

} // namespace automated_irrigating_system