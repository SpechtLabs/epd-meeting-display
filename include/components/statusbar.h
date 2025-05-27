#pragma once

#include <vector>
#include <string>

#include "components.h"
#include "client/calendar_client.h"
#include "config.h"

class StatusBarComponent
{
protected:
    DisplayBuffer *buffer;
    int height;
    int padding = 4;
#if defined(DISP_3C) || defined(DISP_7C)
    Color accentColor;
#endif

public:
#if defined(DISP_3C) || defined(DISP_7C)
    StatusBarComponent(DisplayBuffer *buffer, int height, Color accentColor) : buffer(buffer), height(height), accentColor(accentColor) {}
#else
    StatusBarComponent(DisplayBuffer *buffer, int height) : buffer(buffer), height(height) {}
#endif

    virtual void render(int x, int y, time_t now) const = 0;
    virtual int getWidth() const = 0;
    virtual int getHeight() const { return height; }
};

class StatusBar : public DisplayComponent
{
private:
    std::vector<StatusBarComponent *> leftBound;
    std::vector<StatusBarComponent *> rightBound;

public:
#if defined(DISP_3C) || defined(DISP_7C)
    StatusBar(DisplayBuffer *buffer, calendar_client::CalendarClient *calClient, Color accentColor);
#else
    StatusBar(DisplayBuffer *buffer, calendar_client::CalendarClient *calClient);
#endif

    virtual void render(time_t now) const override;
    virtual bool hasToRedraw(time_t now) const override { return false; }

    const static int StatusBarHeight = 24;
};

class BatteryPercentage : public StatusBarComponent
{
protected:
    const int iconSize = 24;

public:
#if defined(DISP_3C) || defined(DISP_7C)
    BatteryPercentage(DisplayBuffer *buffer, int height, Color accentColor) : StatusBarComponent(buffer, height, accentColor) {}
#else
    BatteryPercentage(DisplayBuffer *buffer, int height) : StatusBarComponent(buffer, height) {}
#endif

    virtual void render(int x, int y, time_t now) const override;
    virtual int getWidth() const override;

protected:
    String getBatBitmap(uint32_t batPercent) const;
};

class WiFiStatus : public StatusBarComponent
{
protected:
    const int iconSize = 24;

public:
#if defined(DISP_3C) || defined(DISP_7C)
    WiFiStatus(DisplayBuffer *buffer, int height, Color accentColor) : StatusBarComponent(buffer, height, accentColor) {}
#else
    WiFiStatus(DisplayBuffer *buffer, int height) : StatusBarComponent(buffer, height) {}
#endif

    virtual void render(int x, int y, time_t now) const override;
    virtual int getWidth() const override;

protected:
    String getWiFiBitmap(int rssi) const;
};

class DateTime : public StatusBarComponent
{
protected:
    calendar_client::CalendarClient *calClient;
    const int iconSize = 32;

public:
#if defined(DISP_3C) || defined(DISP_7C)
    DateTime(DisplayBuffer *buffer, int height, calendar_client::CalendarClient *calClient, Color accentColor) : StatusBarComponent(buffer, height, accentColor), calClient(calClient) {}
#else
    DateTime(DisplayBuffer *buffer, int height, calendar_client::CalendarClient *calClient) : StatusBarComponent(buffer, height), calClient(calClient) {}
#endif

    virtual void render(int x, int y, time_t now) const override;
    virtual int getWidth() const override;
};