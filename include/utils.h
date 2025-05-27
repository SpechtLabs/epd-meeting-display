#pragma once

#include <Arduino.h>
#include <vector>
#include <time.h>
#include <string.h>
#include <WiFiType.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

wl_status_t startWiFi();
void killWiFi();
bool getNtpTime(tm *timeInfo);
bool printLocalTime(tm *timeInfo);
void printHeapUsage();
uint32_t readBatteryVoltage();
uint32_t calcBatPercent(uint32_t v, uint32_t minv, uint32_t maxv);
String getTimeStr(tm *timeInfo);
String getRefreshTimeStr(tm *timeInfo, bool timeSuccess);
const char *getWiFidesc(int rssi);
const char *getWifiStatusPhrase(wl_status_t status);
void disableBuiltinLED();
const uint8_t *getIcon(String iconName, int16_t iconSize);
const uint8_t *getIcon24(String iconName);
const uint8_t *getIcon32(String iconName);
const uint8_t *getIcon48(String iconName);
const uint8_t *getIcon128(String iconName);
const uint8_t *getIcon196(String iconName);
const uint32_t fnv1aHash(const String &string);