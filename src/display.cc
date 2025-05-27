#include "display.h"

#include "components/statusbar.h"
#include "components/calendar.h"
#include "components/status.h"

#include "config.h"

#include <Preferences.h>

#if defined(DISP_3C) || defined(DISP_7C)
Display::Display(int8_t pin_epd_pwr, int8_t pin_epd_sck, int8_t pin_epd_miso, int8_t pin_epd_mosi, int8_t pin_epd_cs, int16_t pin_epd_dc, int16_t pin_epd_rst, int16_t pin_epd_busy, calendar_client::CalendarClient *calClient, Color accentColor)
#else
Display::Display(int8_t pin_epd_pwr, int8_t pin_epd_sck, int8_t pin_epd_miso, int8_t pin_epd_mosi, int8_t pin_epd_cs, int16_t pin_epd_dc, int16_t pin_epd_rst, int16_t pin_epd_busy, calendar_client::CalendarClient *calClient)
#endif
	: pin_epd_pwr(pin_epd_pwr),
	  pin_epd_sck(pin_epd_sck),
	  pin_epd_miso(pin_epd_miso),
	  pin_epd_mosi(pin_epd_mosi),
	  pin_epd_cs(pin_epd_cs),
	  calClient(calClient),
	  initialized(false)
{
	// initialize power pin as output pin
	pinMode(pin_epd_pwr, OUTPUT);
	buffer = new DisplayBuffer(pin_epd_cs, pin_epd_dc, pin_epd_rst, pin_epd_busy);

#if defined(DISP_3C) || defined(DISP_7C)
	statusBar = new StatusBar(buffer, calClient, accentColor);
	calendar = new Calendar(buffer, calClient, accentColor);
	statusIndicator = new Status(buffer, calClient, accentColor);
#else
	statusBar = new StatusBar(buffer, calClient);
	calendar = new Calendar(buffer, calClient);
	statusIndicator = new Status(buffer, calClient);
#endif

	String calendarEntries = "";
	for (calendar_client::CalendarEntries::const_iterator calIt = calClient->getCalendarEntries()->begin();
		 calIt != calClient->getCalendarEntries()->end(); calIt++)
	{
		calendarEntries += calIt->getTitle() + calIt->getStart() + calIt->getEnd() + calIt->getMessage() + calIt->isAllDay() + calIt->isImportant() + calIt->getBusy();
	}
}

void Display::init()
{
	// turn on the 3.3 power to the driver board
	digitalWrite(pin_epd_pwr, HIGH);

	// initialize epd display
	buffer->init();

	// remap spi
	SPI.end();
	SPI.begin(pin_epd_sck, pin_epd_miso, pin_epd_mosi, pin_epd_cs);

	// draw first page
	buffer->clearDisplay();

	initialized = true;
}

void Display::powerOff()
{
	if (!initialized)
	{
		return;
	}

	// turns powerOff() and sets controller to deep sleep
	// for minimum power use, ONLY if wakeable by RST (rst >= 0)
	buffer->hibernate();

	// turn off the 3.3 power to the driver board
	digitalWrite(pin_epd_pwr, LOW);
	initialized = false;
}

void Display::render(time_t now)
{
	bool b1 = statusBar->hasToRedraw(now);
	bool b2 = calendar->hasToRedraw(now);
	bool b3 = statusIndicator->hasToRedraw(now);

	bool mustRedraw = b1 || b2 || b3;

	if (!mustRedraw)
	{
		return;
	}

	if (!initialized)
	{
		init();
	}

	do
	{
		_render(now);
	} while (buffer->nextPage());

	powerOff();
}

void Display::_render(time_t now) const
{
	statusBar->render(now);
	calendar->render(now);
	statusIndicator->render(now);

	buffer->drawVLine(buffer->width() / 2, StatusBar::StatusBarHeight, buffer->height());
}

// Draw an error message to the display.
// If only title is specified, content of tilte is wrapped across two lines
void Display::error(String icon, const String &title, const String &description, time_t now)
{
	if (!title.isEmpty())
	{
		Serial.printf("[error] %s", title.c_str());

		if (!description.isEmpty())
		{
			Serial.printf(" (%s)", description.c_str());
		}
		Serial.printf("\n");
	}

	if (!initialized)
	{
		init();
	}

	do
	{
		_fullPageStatus(icon, 196, title, description, now);
	} while (buffer->nextPage());

	// If we're drawing an error page, we reset the current calendar and status hashes
	Preferences prefs;
	prefs.begin(NVS_NAMESPACE, false);
	prefs.putULong("curCalHash", 0);
	prefs.putULong("curStatHash", 0);
	prefs.end();

	powerOff();
}

void Display::fullPageStatus(String icon, int16_t iconSize, const String &title, const String &description, time_t now)
{
	// If we're drawing a full page status, we reset the current calendar and status hashes
	// but store our status message
	String statusMsg = title + description + icon;
	uint32_t statusHash = fnv1aHash(statusMsg);

	Preferences prefs;
	prefs.begin(NVS_NAMESPACE, false);
	prefs.putULong("curCalHash", 0);
	prefs.putULong("curStatHash", 0);
	uint32_t currentFullPageHash = prefs.getULong("fullPageHash", 0);

	if (currentFullPageHash == statusHash)
	{
#if DEBUG >= 1
		Serial.printf("[debug] No full page status change, no redraw.\n");
#endif
		prefs.end();
		return;
	}
	else
	{
		prefs.putULong("fullPageHash", statusHash);
		Serial.printf("[info] Full page status changed, must redraw: %s\n", statusMsg.c_str());
	}

	prefs.end();

	if (!initialized)
	{
		init();
	}

	do
	{
		_fullPageStatus(icon, 196, title, description, now);
	} while (buffer->nextPage());

	powerOff();
}

void Display::_fullPageStatus(String icon, int16_t iconSize, const String &title, const String &description, time_t now) const
{
	int startX = buffer->width() / 2;
	int startY = buffer->height() / 2;

	if (now != 0)
	{
		statusBar->render(now);
		startY = StatusBar::StatusBarHeight + ((buffer->height() - StatusBar::StatusBarHeight) / 2);
	}

	uint8_t textAlignment = Alignment::HorizontalCenter | Alignment::Top;

	buffer->setFontSize(24);
	TextSize *titleSize = buffer->getStringBounds(title, buffer->width(), 1);
	int totalDrawingSize = titleSize->height;

	if (!description.isEmpty())
	{
		buffer->setFontSize(18);
		TextSize *descriptionSize = buffer->getStringBounds(title, buffer->width(), 3);
		totalDrawingSize += descriptionSize->height;
		totalDrawingSize += 5;
	}

	if (icon != "")
	{
		totalDrawingSize += iconSize;
		totalDrawingSize += 5;

		if (!description.isEmpty())
		{
			buffer->setFontSize(24);
			buffer->drawString(startX, startY - totalDrawingSize / 2, title, textAlignment, buffer->width(), 1);
		}

		buffer->drawIcon(startX, startY, icon, iconSize, Alignment::Center);
	}

	if (icon != "" && description.isEmpty())
	{
		buffer->setFontSize(24);
		buffer->drawString(startX, startY + totalDrawingSize / 2, title, textAlignment, buffer->width(), 1);
	}

	if (!description.isEmpty())
	{
		buffer->setFontSize(18);
		buffer->drawString(startX, startY + totalDrawingSize / 2, description, textAlignment, buffer->width(), 4);
	}
}