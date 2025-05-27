#pragma once

#include <vector>
#include "components.h"
#include "utils.h"
#include "client/calendar_client.h"

class Calendar : public DisplayComponent
{
private:
    calendar_client::CalendarClient *calClient;
    const int entryHeight;

public:
#if defined(DISP_3C) || defined(DISP_7C)
    Calendar(DisplayBuffer *buffer, calendar_client::CalendarClient *calClient, Color accentColor);
#else
    Calendar(DisplayBuffer *buffer, calendar_client::CalendarClient *calClient);
#endif

    virtual void render(time_t now) const override;
    virtual bool hasToRedraw(time_t now) const override;

protected:
    bool checkTruncatedEvents(time_t now, int *displayable, int *skipped, int *more) const;
    int renderSkippedEntries(int y, int skipped, int maxEntries, String eventsTxt, bool more) const;
    int renderCalendarEntry(int x, int y, const calendar_client::CalendarEntry &entry, time_t now) const;
    void renderCalendarEntryIcon(int x, int y, const calendar_client::CalendarEntry &entry, bool isPast, bool isCurrent) const;
    void renderCalendarEntryTitle(int x, int y, const calendar_client::CalendarEntry &entry, bool isPast) const;
    void renderCalendarEntryTime(int x, int y, const calendar_client::CalendarEntry &entry, bool isPast) const;
};
