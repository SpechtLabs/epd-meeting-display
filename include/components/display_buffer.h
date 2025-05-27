#pragma once

#include <string>

#include "config.h"
#include "components/display_config.h"
#include "utils.h"

class DisplayBuffer
{
private:
	GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)> *display;
	Color foregroundColor;
	Color backgroundColor;
	uint8_t fontSize;

public:
	DisplayBuffer(int8_t pin_epd_cs, int16_t pin_epd_dc, int16_t pin_epd_rst, int16_t pin_epd_busy);

	void hibernate() { display->hibernate(); }
	// init method with additional parameters:
	// initial false for re-init after processor deep sleep wake up, if display power supply was kept
	// this can be used to avoid the repeated initial full refresh on displays with fast partial update
	// NOTE: garbage will result on fast partial update displays, if initial full update is omitted after power loss
	// reset_duration = 10 is default; a value of 2 may help with "clever" reset circuit of newer boards from Waveshare
	// pulldown_rst_mode true for alternate RST handling to avoid feeding 5V through RST pin
	void init(uint32_t serial_diag_bitrate = 115200, bool initial = true, uint16_t reset_duration = 10, bool pulldown_rst_mode = false) { display->init(serial_diag_bitrate, initial, reset_duration, pulldown_rst_mode); }

	int16_t width() { return display->width(); }
	int16_t height() { return display->height(); }

	void invert() { setForegroundColor(setBackgroundColor(this->foregroundColor)); }

	bool nextPage() { return display->nextPage(); }

	void setTextSize(uint8_t s) { this->display->setTextSize(s); }
	void setFontSize(uint8_t fontSize);
	uint8_t getFontSize() const { return this->fontSize; }
	Color setForegroundColor(Color c);
	Color setBackgroundColor(Color c);

	Color getForegroundColor() const { return this->foregroundColor; }
	Color getBackgroundColor() const { return this->backgroundColor; }

	void setFullWindow() { display->setFullWindow(); }
	void setPartialWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) { display->setPartialWindow(x, y, w, h); }

	void clearDisplay();

	Rect drawString(int16_t x, int16_t y, const String &text, uint8_t alignment = Alignment::Top | Alignment::Left);
	Rect drawString(int16_t x, int16_t y, const String &text, uint8_t alignment, uint16_t max_width, uint16_t max_lines);

	TextSize *getStringBounds(const String &text);
	TextSize *getStringBounds(const String &text, uint16_t max_width, uint16_t max_lines);

	void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t width, int16_t height);
	void drawIcon(int16_t x, int16_t y, const String &iconName, int16_t size, uint8_t alignment = Alignment::Top | Alignment::Left);
	void drawRect(int16_t x, int16_t y, int16_t w, int16_t h) { display->drawRect(x, y, w, h, foregroundColor); }
	void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t thickness);
	void drawRect(const Rect &r) { drawRect(r.x, r.y, r.width, r.height); }
	void drawRect(const Rect &r, int16_t thickness) { drawRect(r.x, r.y, r.width, r.height, thickness); }

	void fillBackground(int16_t x, int16_t y, int16_t w, int16_t h) { display->fillRect(x, y, w, h, backgroundColor); }

	void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1) { display->drawLine(x0, y0, x1, y1, foregroundColor); }
	void drawHLine(int16_t x, int16_t y, int16_t len) { drawLine(x, y, x + len, y); }
	void drawVLine(int16_t x, int16_t y, int16_t len) { drawLine(x, y, x, y + len); }

protected:
	static void _setFontSize(Adafruit_GFX *buffer, uint8_t fontSize);
};
