#include <Arduino.h>
#include <Preferences.h>
#include <time.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <ctime>
#include <string>
#include <sstream>

#include "config.h"
#include "utils.h"
#include "client/calendar_client.h"
#include "components/display_config.h"

#include "display.h"

Preferences prefs;
calendar_client::CalendarClient calClient(API_ENDPOINT, API_ENDPOINT_PORT);

#if defined(DISP_3C) || defined(DISP_7C)
Display epd(PIN_EPD_PWR, PIN_EPD_SCK, PIN_EPD_MISO, PIN_EPD_MOSI, PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY, &calClient, ACCENT_COLOR);
#else
Display epd(PIN_EPD_PWR, PIN_EPD_SCK, PIN_EPD_MISO, PIN_EPD_MOSI, PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY, &calClient);
#endif

// Put esp32 into ultra low-power deep sleep (<11μA).
void beginDeepSleep(unsigned long startTime)
{
	killWiFi();

	tm timeInfo = {};
	// only call getLocalTime if we havent gotten the ntp time yet
	if (!getLocalTime(&timeInfo))
	{
		Serial.println("Failed to synchronize time before deep-sleep, referencing older time.");
	}

	time_t now = mktime(&timeInfo);

	int sleepSeconds = SLEEP_DURATION * 60; // SLEEP_DURATION is in minutes, multiply by 60

	// Sleep duration is until the end of this meeting, or till the beginning of the next meeting
	const calendar_client::CalendarEntry *currEvent = calClient.getCurrentEvent(now, false);

	if (currEvent != NULL)
	{
		sleepSeconds = difftime(currEvent->getEnd(), now); // calculate the minutes until the event ends
#if DEBUG_LEVEL >= 1
		Serial.println("[debug] sleeping till end of this event: " + currEvent->getTitle() + " (" + sleepSeconds + "s)");
#endif
	}
	else
	{
		const calendar_client::CalendarEntry *nextEvent = calClient.getNextEvent(now);
		if (nextEvent != NULL)
		{
			sleepSeconds = difftime(nextEvent->getStart(), now); // calculate the minutes until the next event starts
#if DEBUG_LEVEL >= 1
			Serial.println("[debug] sleeping till start of next event: " + nextEvent->getTitle() + " (" + sleepSeconds + "s)");
#endif
		}
	}

	// if we sleep for more than SLEEP_DURATION, wake up a bit earlier,
	// to check for potential new calendar invites
	if (sleepSeconds > SLEEP_DURATION * 60)
	{
		sleepSeconds = SLEEP_DURATION * 60;
	}

	// add extra delay to compensate for esp32's with fast RTCs.
	sleepSeconds += 10ULL;

#if DEBUG_LEVEL >= 1
	printHeapUsage();
#endif

	Serial.println("\nAwake for " + String((millis() - startTime) / 1000.0, 3) + "s");
	Serial.println("Entering deep sleep for " + String(sleepSeconds) + "s");
	esp_sleep_enable_timer_wakeup(sleepSeconds * 1000000ULL);

	esp_deep_sleep_start();
}

// Program entry point.
void setup()
{
	unsigned long startTime = millis();
	Serial.begin(115200);

#if DEBUG_LEVEL >= 1
	printHeapUsage();
#endif

	disableBuiltinLED();

	// Open namespace for read/write to non-volatile storage
	prefs.begin(NVS_NAMESPACE, false);

	uint32_t batteryVoltage = readBatteryVoltage();
	Serial.print("Battery voltage");
	Serial.println(": " + String(batteryVoltage) + "mv");

	// When the battery is low, the display should be updated to reflect that, but
	// only the first time we detect low voltage. The next time the display will
	// refresh is when voltage is no longer low. To keep track of that we will
	// make use of non-volatile storage.
	bool lowBat = prefs.getBool("lowBat", false);

	// low battery, deep sleep now
	if (batteryVoltage <= LOW_BATTERY_VOLTAGE)
	{
		if (lowBat == false)
		{ // battery is now low for the first time
			prefs.putBool("lowBat", true);
			prefs.end();
			epd.error("battery_alert_90deg", "Low Battery");
		}

		// critically low battery
		// don't set esp_sleep_enable_timer_wakeup();
		// We won't wake up again until someone manually presses the RST button.
		if (batteryVoltage <= CRIT_LOW_BATTERY_VOLTAGE)
		{
			Serial.println("Critically low battery voltage!");
			Serial.println("Hibernating without wake time!");
		}
		// very low battery
		else if (batteryVoltage <= VERY_LOW_BATTERY_VOLTAGE)
		{
			Serial.println("Very low battery voltage!");
			Serial.print("Entering deep sleep for " + String(VERY_LOW_BATTERY_SLEEP_INTERVAL) + "min");
			esp_sleep_enable_timer_wakeup(VERY_LOW_BATTERY_SLEEP_INTERVAL * 60ULL * 1000000ULL);
		}
		// low battery
		else
		{
			Serial.println("Low battery voltage!");
			Serial.println("Entering deep sleep for " + String(LOW_BATTERY_SLEEP_INTERVAL) + "min");
			esp_sleep_enable_timer_wakeup(LOW_BATTERY_SLEEP_INTERVAL * 60ULL * 1000000ULL);
		}

		esp_deep_sleep_start();
	}

	// battery is no longer low, reset variable in non-volatile storage
	if (lowBat == true)
	{
		prefs.putBool("lowBat", false);
	}

	// All data should have been loaded from NVS. Close filesystem.
	prefs.end();

	tm timeInfo = {};

	// START WIFI
	wl_status_t wifiStatus = startWiFi();

	// WiFi Connection Failed
	if (wifiStatus != WL_CONNECTED)
	{

		if (wifiStatus == WL_NO_SSID_AVAIL)
		{
			epd.error("wifi_x", "Network Not Available");
		}
		else
		{
			epd.error("wifi_x", "WiFi Connection Failed");
		}

		beginDeepSleep(startTime);
	}

	// TIME SYNCHRONIZATION
	configTzTime(TIMEZONE, NTP_SERVER_1, NTP_SERVER_2);
	bool timeConfigured = getNtpTime(&timeInfo);
	if (!timeConfigured)
	{
		epd.error("wi_time_4", "Time Synchronization Failed");
		beginDeepSleep(startTime);
	}

	int httpStatus = calClient.fetchCustomStatus();
	if (httpStatus != HTTP_CODE_OK && httpStatus != HTTP_CODE_GONE)
	{
		std::stringstream ss;
		ss << "Fetching status failed";

		epd.error("wi_cloud_down", ss.str().c_str(), calendar_client::CalendarClient::getHttpResponsePhrase(httpStatus), mktime(&timeInfo));
		beginDeepSleep(startTime);
	}

	const calendar_client::CustomStatus *stat = calClient.getCustomStatus();
	if (stat != NULL && !stat->getTitle().isEmpty())
	{
		epd.fullPageStatus(stat->getIcon(), stat->getIconSize(), stat->getTitle(), stat->getDescription(), mktime(&timeInfo));
		beginDeepSleep(startTime);
	}

	httpStatus = calClient.fetchCalendar();
	if (httpStatus != HTTP_CODE_OK && httpStatus != HTTP_CODE_GONE)
	{
		std::stringstream ss;
		ss << "Fetching calendar failed";

		epd.error("wi_cloud_down", ss.str().c_str(), calendar_client::CalendarClient::getHttpResponsePhrase(httpStatus), mktime(&timeInfo));
		beginDeepSleep(startTime);
	}

	epd.render(mktime(&timeInfo));
	beginDeepSleep(startTime);
}

// This will never run
void loop()
{
}
