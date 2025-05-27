#include "components/status.h"
#include "components/statusbar.h"
#include "config.h"

#include <Preferences.h>

#if defined(DISP_3C) || defined(DISP_7C)
Status::Status(DisplayBuffer *buffer, calendar_client::CalendarClient *calClient, Color accentColor)
	: DisplayComponent(buffer, 0, StatusBar::StatusBarHeight, buffer->width() / 2, buffer->height() - StatusBar::StatusBarHeight, accentColor),
#else
Status::Status(DisplayBuffer *buffer, calendar_client::CalendarClient *calClient)
	: DisplayComponent(buffer, 0, StatusBar::StatusBarHeight, buffer->width() / 2, buffer->height() - StatusBar::StatusBarHeight),
#endif
	  calClient(calClient)
{
}

bool Status::hasToRedraw(time_t now) const
{
	const calendar_client::CalendarEntry *currentEvent = calClient->getCurrentEvent(now);
	String statusMsg = currentEvent == NULL ? TXT_FREE : currentEvent->getMessage() != "" ? currentEvent->getMessage()
																						  : currentEvent->getTitle();
	uint32_t statusHash = fnv1aHash(statusMsg);

	// Check if something needs to be re-drawn
	Preferences prefs;
	prefs.begin(NVS_NAMESPACE, false);

	// get the previous status message
	uint32_t currStatusHash = prefs.getULong("curStatHash", 0);
	bool mustRedraw = currStatusHash != statusHash;

	// If our previous status is the same as the current one, we don't need to re-draw
	if (mustRedraw)
	{
		// save the current status message
		prefs.putULong("curStatHash", statusHash);
		Serial.printf("[info] Status changed, must redraw: %s\n", statusMsg.c_str());
	}
#if DEBUG_LEVEL >= 1
	else
	{
		Serial.printf("[debug] No status change, no redraw.\n");
	}
#endif

	prefs.end();

	return mustRedraw;
}

void Status::render(time_t now) const
{
	int maxTextWidth = width;

	const calendar_client::CalendarEntry *currentEvent = calClient->getCurrentEvent(now);

	bool isImportant = false;
	String statusMsg(TXT_FREE);
	String icon("");
	int iconSize = 0;

	if (currentEvent != NULL)
	{
#if DEBUG_LEVEL >= 2
		Serial.printf("[verbose] Current Event: %s, busy_state: %s\n",
					  currentEvent->getTitle().c_str(),
					  currentEvent->getBusy() == calendar_client::Busy ? "Busy" : currentEvent->getBusy() == calendar_client::Tentative ? "Tentative"
																																		: "Free");
#endif
		if (currentEvent->getBusy() == calendar_client::Busy)
		{
			icon = "wi_time_2";
			iconSize = 128;
		}

		if (currentEvent->isImportant())
		{
			icon = "warning_icon";
			iconSize = 196;
			isImportant = true;
		}

		statusMsg = currentEvent->getMessage() != "" ? currentEvent->getMessage() : currentEvent->getTitle();
	}

	Color fgSave = buffer->getForegroundColor();
	Color bgSave = buffer->getBackgroundColor();

	if (isImportant)
	{

#if defined(DISP_3C) || defined(DISP_7C)
		buffer->setForegroundColor(accentColor);
#elif INVERT_AS_ACCENT == true
		buffer->invert();
		buffer->fillBackground(x, y, width, height);
#endif

		buffer->drawRect(x + 20, y + 20, width - 40, height - 40, 10);
		maxTextWidth = width - 40;
	}

	// get the center of the screen
	uint8_t alignment = Alignment::Center;
	int startX = x + width / 2;
	int startY = y + height / 2;

	buffer->setFontSize(24);
	TextSize *size = buffer->getStringBounds(statusMsg, maxTextWidth, 3);

	if (icon != "")
	{
		// total_height = text_height + icon_height + padding
		int totalHeight = size->height + iconSize + 5;
		int iconYOffset = totalHeight / 2;

		startY -= iconYOffset;

		// draw the icon
		buffer->drawIcon(startX, startY, icon, iconSize, Alignment::HorizontalCenter | Alignment::Top);

		startY += iconSize;

		// Now, since we have the Icon, we can't position the text
		// vertically center, but let's do Top H-Center
		alignment = Alignment::HorizontalCenter | Alignment::Top;
	}

	buffer->drawString(startX, startY, statusMsg, alignment, maxTextWidth, 3);

	// reset display color
	buffer->setForegroundColor(fgSave);
	buffer->setBackgroundColor(bgSave);
}
