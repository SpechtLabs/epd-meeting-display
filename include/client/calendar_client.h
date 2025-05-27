#pragma once

#include <vector>
#include <string>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

namespace calendar_client
{
    enum BusyState : int
    {
        Free = 0,
        Tentative = 1,
        Busy = 2
    };

    class CalendarEntry
    {
    protected:
        String title;
        time_t start;
        time_t end;
        bool all_day;
        BusyState busy;
        bool important;
        String message;

    public:
        CalendarEntry() : title(""),
                          start(0),
                          end(LONG_MAX),
                          all_day(false),
                          busy(BusyState::Free),
                          important(false),
                          message("") {};

        CalendarEntry(const JsonObject &json);

        String getTitle() const { return title; }
        time_t getStart() const { return start; }
        time_t getEnd() const { return end; }
        bool isAllDay() const { return all_day; }
        BusyState getBusy() const { return busy; }
        bool isImportant() const { return important; }
        String getMessage() const { return message; }
    };

    typedef std::vector<CalendarEntry> CalendarEntries;

    class CustomStatus
    {
    private:
        String icon;
        int32_t icon_size;
        String title;
        String description;

    public:
        CustomStatus() : icon(""), icon_size(0), title(""), description("") {}
        CustomStatus(const JsonObject &json);

        String getIcon() const { return icon; }
        int32_t getIconSize() const { return icon_size; }
        String getTitle() const { return title; }
        String getDescription() const { return description; }
    };

    class CalendarClient
    {
    protected:
        WiFiClient client;
        String apiEndpoint;
        int apiPort;

        time_t last_updated;
        CalendarEntries entries;
        CustomStatus *customStatus;

    public:
        CalendarClient(String apiEndpoint, int apiPort) : apiEndpoint(apiEndpoint), apiPort(apiPort) {}
        int fetchCalendar();
        int fetchCustomStatus();

        // get the current calendar event. If multiple events are going at the same time,
        // nowClosestToStart=true will return the event where the starting-time is closest to now
        // wile nowClosestToStart=false will return the event where the end-time is closest to now
        const CalendarEntry *getCurrentEvent(time_t now, bool nowClosestToStart = true) const;
        const CalendarEntry *getNextEvent(time_t now) const;

        time_t getLastUpdated() const { return last_updated; }
        const CalendarEntries *getCalendarEntries() const { return &entries; }
        const CustomStatus *getCustomStatus() const { return customStatus; }

        static const char *getHttpResponsePhrase(int code);

    protected:
        bool parseCalendar(HTTPClient &client);
        bool parseCustomStatus(HTTPClient &client);
    };
};