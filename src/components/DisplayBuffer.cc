#include "components/components.h"

#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

#include "config.h"

DisplayBuffer::DisplayBuffer(int8_t pin_epd_cs, int16_t pin_epd_dc, int16_t pin_epd_rst, int16_t pin_epd_busy) : fontSize(9)
{
	this->display = new GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)>(GxEPD2_DRIVER_CLASS(pin_epd_cs, pin_epd_dc, pin_epd_rst, pin_epd_busy));
	display->setRotation(0);
	display->setTextWrap(false);

	setTextSize(1);
	display->setFullWindow();
	setForegroundColor(Color::Black);
	setBackgroundColor(Color::White);
	setFontSize(9);
}

Color DisplayBuffer::setForegroundColor(Color c)
{
	Color old = this->foregroundColor;
	this->foregroundColor = c;
	return old;
}

Color DisplayBuffer::setBackgroundColor(Color c)
{
	Color old = this->backgroundColor;
	this->backgroundColor = c;
	return old;
}

// Function to test if a specific alignment flag is set
bool hasAlignment(uint8_t alignment, Alignment flag)
{
	return (alignment & flag) == flag;
}

void DisplayBuffer::clearDisplay()
{
	display->setFullWindow();
	display->fillScreen(backgroundColor);
}

void DisplayBuffer::setFontSize(uint8_t fontSize)
{
	this->fontSize = fontSize;
	DisplayBuffer::_setFontSize(display, fontSize);
}

void DisplayBuffer::_setFontSize(Adafruit_GFX *buffer, uint8_t fontSize)
{
	switch (fontSize)
	{
	case 9:
		buffer->setFont(&FreeSans9pt7b);
		break;

	case 12:
		buffer->setFont(&FreeSans12pt7b);
		break;

	case 18:
		buffer->setFont(&FreeSans18pt7b);
		break;

	case 24:
		buffer->setFont(&FreeSans24pt7b);
		break;

	default:
		buffer->setFont(&FreeSans12pt7b);
		Serial.printf("[error]: font-size %d is not available. Only one of 9, 12, 18, 24 is supported\n", fontSize);
	}
}

bool containsAnyChar(const String &str1, const String &str2)
{
	for (size_t i = 0; i < str2.length(); i++)
	{
		if (str1.indexOf(str2[i]) != -1)
		{
			return true;
		}
	}
	return false;
}

bool beginsAnyChar(const String &str1, const String &str2)
{
	for (size_t i = 0; i < str2.length(); i++)
	{
		if (str1.indexOf(str2[i]) == 0)
		{
			return true;
		}
	}
	return false;
}

// Draw a String on x/y coordinate
Rect DisplayBuffer::drawString(int16_t x, int16_t y, const String &text, uint8_t alignment)
{
	TextSize *size = getStringBounds(text);

	if (hasAlignment(alignment, Alignment::HorizontalCenter))
	{
		x -= size->width / 2;
	}
	else if (hasAlignment(alignment, Alignment::Right))
	{
		x -= size->width;
	}
	else if (hasAlignment(alignment, Alignment::Left))
	{
		x = x;
	}

	if (hasAlignment(alignment, Alignment::VerticalCenter))
	{
		y -= size->height / 2;
	}
	else if (hasAlignment(alignment, Alignment::Top))
	{
		y = y;
	}
	else if (hasAlignment(alignment, Alignment::Bottom))
	{
		y = y - size->height;
	}

	int offsetY = y + size->height;
	int offsetX = x;

	// I hate this!
	// if the string to draw contains a descender, than we have to
	// adjust the Y offset to include the descender height since print
	if (containsAnyChar(text, "qypgj()"))
	{
		offsetY -= size->height / 3;
	}

	if (beginsAnyChar(text, "1J"))
	{
		int16_t x1, y1;
		uint16_t w1, h1;
		display->getTextBounds("1", 0, 0, &x1, &y1, &w1, &h1);
		offsetX -= w1 / 4 * 3;
	}

	display->setTextColor(foregroundColor);
	display->setCursor(offsetX, offsetY);
	display->print(text);

	Rect r;
	r.x = x;
	r.y = y;
	r.width = size->width;
	r.height = size->height;

	return r;
}

TextSize *DisplayBuffer::getStringBounds(const String &text)
{
	int16_t x1, y1;
	uint16_t w, h;
	display->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

	TextSize *size = new TextSize();
	size->width = w;
	size->height = h;
	return size;
}

TextSize *DisplayBuffer::getStringBounds(const String &text, uint16_t max_width, uint16_t max_lines)
{
	uint16_t current_line = 0;
	String textRemaining = text;

	TextSize *biggestTextSize = new TextSize();
	biggestTextSize->width = 0;
	biggestTextSize->height = 0;

	// print until we reach max_lines or no more text remains
	while (current_line < max_lines && !textRemaining.isEmpty())
	{
		int16_t x1, y1;
		uint16_t w, h;

		display->getTextBounds(textRemaining, 0, 0, &x1, &y1, &w, &h);

		if (w > biggestTextSize->width)
		{
			biggestTextSize->width = w;
		}

		biggestTextSize->height += h;

		int endIndex = textRemaining.length();
		// check if remaining text is to wide, if it is then print what we can
		String subStr = textRemaining;
		int splitAt = 0;
		int keepLastChar = 0;
		while (w > max_width && splitAt != -1)
		{
			if (keepLastChar)
			{
				// if we kept the last character during the last iteration of this while
				// loop, remove it now so we don't get stuck in an infinite loop.
				subStr.remove(subStr.length() - 1);
			}

			// find the last place in the string that we can break it.
			if (current_line < max_lines - 1)
			{
				splitAt = std::max(subStr.lastIndexOf(" "),
								   subStr.lastIndexOf("-"));
			}
			else
			{
				// this is the last line, only break at spaces so we can add ellipsis
				splitAt = subStr.lastIndexOf(" ");
			}

			// if splitAt == -1 then there is an unbroken set of characters that is
			// longer than max_width. Otherwise if splitAt != -1 then we can continue
			// the loop until the string is <= max_width
			if (splitAt != -1)
			{
				endIndex = splitAt;
				subStr = subStr.substring(0, endIndex + 1);

				char lastChar = subStr.charAt(endIndex);
				if (lastChar == ' ' || lastChar == '-' || lastChar == ':')
				{
					// remove this char now so it is not counted towards line width
					keepLastChar = 0;
					subStr.remove(endIndex);
					--endIndex;
				}

				if (current_line < max_lines - 1)
				{
					// this is not the last line
					display->getTextBounds(subStr, 0, 0, &x1, &y1, &w, &h);
				}
				else
				{
					// this is the last line, we need to make sure there is space for
					// ellipsis
					display->getTextBounds(subStr + "...", 0, 0, &x1, &y1, &w, &h);
					if (w <= max_width)
					{
						// ellipsis fit, add them to subStr
						subStr = subStr + "...";
					}
				}
			}
		}

		// update textRemaining to no longer include what was printed
		// +1 for exclusive bounds, +1 to get passed space/dash
		textRemaining = textRemaining.substring(endIndex + 2 - keepLastChar);

		++current_line;
	}

	return biggestTextSize;
}

void DisplayBuffer::drawIcon(int16_t x, int16_t y, const String &iconName, int16_t size, uint8_t alignment)
{
	if (hasAlignment(alignment, Alignment::HorizontalCenter))
	{
		x -= size / 2;
	}
	else if (hasAlignment(alignment, Alignment::Right))
	{
		x -= size;
	}
	else if (hasAlignment(alignment, Alignment::Left))
	{
		x = x;
	}

	if (hasAlignment(alignment, Alignment::VerticalCenter))
	{
		y -= size / 2;
	}
	else if (hasAlignment(alignment, Alignment::Top))
	{
		y = y;
	}
	else if (hasAlignment(alignment, Alignment::Bottom))
	{
		y -= size;
	}

	drawBitmap(x, y, getIcon(iconName, size), size, size);
}

void DisplayBuffer::drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t width, int16_t height)
{
	display->drawInvertedBitmap(x, y, bitmap, width, height, foregroundColor);
}

void DisplayBuffer::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t thickness)
{
	display->fillRect(x, y, thickness, h, foregroundColor);					// left column
	display->fillRect(x + w - thickness, y, thickness, h, foregroundColor); // right column

	display->fillRect(x, y, w, thickness, foregroundColor);					// top column
	display->fillRect(x, y + h - thickness, w, thickness, foregroundColor); // bottom column
}

// Draws a string that will flow into the next line when max_width is reached.
// If a string exceeds max_lines an ellipsis (...) will terminate the last word.
// Lines will break at spaces(' ') and dashes('-').
//
// Note: max_width should be big enough to accommodate the largest word that
//       will be displayed. If an unbroken string of characters longer than
//       max_width exist in text, then the string will be printed beyond
//       max_width.
Rect DisplayBuffer::drawString(int16_t x, int16_t y, const String &text, uint8_t alignment, uint16_t max_width, uint16_t max_lines)
{
	Rect textRect;
	textRect.x = x;
	textRect.y = y;
	textRect.width = 0;
	textRect.height = 0;

	uint16_t current_line = 0;
	String textRemaining = text;

	int16_t x1, y1;
	uint16_t w, h;

	display->getTextBounds(textRemaining, 0, 0, &x1, &y1, &w, &h);
	int16_t line_spacing = h;

	// print until we reach max_lines or no more text remains
	while (current_line < max_lines && !textRemaining.isEmpty())
	{
		display->getTextBounds(textRemaining, 0, 0, &x1, &y1, &w, &h);

		int endIndex = textRemaining.length();
		// check if remaining text is to wide, if it is then print what we can
		String subStr = textRemaining;
		int splitAt = 0;
		int keepLastChar = 0;
		while (w > max_width && splitAt != -1)
		{

			if (keepLastChar)
			{
				// if we kept the last character during the last iteration of this while
				// loop, remove it now so we don't get stuck in an infinite loop.
				subStr.remove(subStr.length() - 1);
			}

			// find the last place in the string that we can break it.
			if (current_line < max_lines - 1)
			{
				splitAt = std::max(subStr.lastIndexOf(" "),
								   subStr.lastIndexOf("-"));
			}
			else
			{
				// this is the last line, only break at spaces so we can add ellipsis
				splitAt = subStr.lastIndexOf(" ");
			}

			// if splitAt == -1 then there is an unbroken set of characters that is
			// longer than max_width. Otherwise if splitAt != -1 then we can continue
			// the loop until the string is <= max_width
			if (splitAt != -1)
			{
				endIndex = splitAt;
				subStr = subStr.substring(0, endIndex + 1);

				char lastChar = subStr.charAt(endIndex);
				if (lastChar == ' ' || lastChar == '-' || lastChar == ':')
				{
					// remove this char now so it is not counted towards line width
					keepLastChar = 0;
					subStr.remove(endIndex);
					--endIndex;
				}

				if (current_line < max_lines - 1)
				{
					// this is not the last line
					display->getTextBounds(subStr, 0, 0, &x1, &y1, &w, &h);
				}
				else
				{
					// this is the last line, we need to make sure there is space for
					// ellipsis
					display->getTextBounds(subStr + "...", 0, 0, &x1, &y1, &w, &h);
					if (w <= max_width)
					{
						// ellipsis fit, add them to subStr
						subStr = subStr + "...";
					}
				}
			}
		}

#if DEBUG_LEVEL >= 2
		if (max_lines > 1)
		{
			Serial.printf("[verbose] Draw String (line %d): %s (x=%d / y=%d, height=%d)\n",
						  current_line + 1,
						  subStr.c_str(),
						  x, y + (current_line * line_spacing),
						  line_spacing);
		}
#endif

		Rect r = drawString(x, y + (current_line * line_spacing), subStr, alignment);

		if (r.x < textRect.x)
		{
			textRect.x = r.x;
		}

		if (r.width > textRect.width)
		{
			textRect.width = r.width;
		}

		// the alignment made the y move upwards
		if (r.y < textRect.y)
		{
			textRect.y = r.y;
		}

		textRect.height += r.height + (current_line * line_spacing);

		// update textRemaining to no longer include what was printed
		// +1 for exclusive bounds, +1 to get passed space/dash
		textRemaining = textRemaining.substring(endIndex + 2 - keepLastChar);

		++current_line;
	}

	return textRect;
}