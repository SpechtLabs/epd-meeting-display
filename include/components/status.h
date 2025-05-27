#pragma once

#include <string>

#include "components.h"
#include "client/calendar_client.h"
#include "components/display_buffer.h"
#include "config.h"

class Status : public DisplayComponent
{
private:
    calendar_client::CalendarClient *calClient;

public:
#if defined(DISP_3C) || defined(DISP_7C)
    Status(DisplayBuffer *buffer, calendar_client::CalendarClient *calClient, Color accentColor);
#else
    Status(DisplayBuffer *buffer, calendar_client::CalendarClient *calClient);
#endif
    virtual void render(time_t now) const override;
    virtual bool hasToRedraw(time_t now) const override;
};