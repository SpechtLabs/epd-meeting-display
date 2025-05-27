#pragma once

#include <vector>
#include <string>

#include "components/statusbar.h"
#include "components/status.h"
#include "components/calendar.h"
#include "client/calendar_client.h"
#include "utils.h"
#include "components/display_buffer.h"

class Display
{
private:
	DisplayBuffer *buffer;
	int8_t pin_epd_pwr;
	int8_t pin_epd_sck;
	int8_t pin_epd_miso;
	int8_t pin_epd_mosi;
	int8_t pin_epd_cs;

	StatusBar *statusBar;
	Calendar *calendar;
	Status *statusIndicator;
	calendar_client::CalendarClient *calClient;

	bool initialized;

	// working with the display
public:
#if defined(DISP_3C) || defined(DISP_7C)
	Display(int8_t pin_epd_pwr, int8_t pin_epd_sck, int8_t pin_epd_miso, int8_t pin_epd_mosi, int8_t pin_epd_cs, int16_t pin_epd_dc, int16_t pin_epd_rst, int16_t pin_epd_busy, calendar_client::CalendarClient *calClient, Color accentColor);
#else
	Display(int8_t pin_epd_pwr, int8_t pin_epd_sck, int8_t pin_epd_miso, int8_t pin_epd_mosi, int8_t pin_epd_cs, int16_t pin_epd_dc, int16_t pin_epd_rst, int16_t pin_epd_busy, calendar_client::CalendarClient *calClient);

#endif

	// turn on the display and initialize it
	void init();

	// turn the power to the display off.
	// first puts the epd driver to deep sleep for, and then
	// cuts power to the power pin
	void powerOff();

	// Display configuration
public:
	void setStatus(String message, bool isImportant = false, const uint8_t *icon = NULL);

	// Rendering functions
public:
	// renders the display in a single refresh cycle for the display
	void render(time_t now);

	// Draw an error message to the display.
	// If only title is specified, content of tilte is wrapped across two lines
	void error(String icon, const String &title, const String &description = "", time_t now = 0);
	void fullPageStatus(String icon, int16_t iconSize, const String &title, const String &description, time_t now);

	// internal rendering functions.
	// they draw the display, but do not take care of the nextPage. That is done in the public variants
protected:
	void _fullPageStatus(String icon, int16_t iconSize, const String &title, const String &description, time_t now) const;
	void _render(time_t now) const;
};