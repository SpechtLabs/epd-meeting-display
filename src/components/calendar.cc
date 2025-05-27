#include <ctime>
#include <Preferences.h>

#include "config.h"
#include "utils.h"

#include "components/calendar.h"
#include "components/statusbar.h"

#if defined(DISP_3C) || defined(DISP_7C)
Calendar::Calendar(DisplayBuffer *buffer, calendar_client::CalendarClient *calClient, Color accentColor)
	: DisplayComponent(buffer, buffer->width() / 2, StatusBar::StatusBarHeight, buffer->width() / 2, buffer->height() - StatusBar::StatusBarHeight, accentColor),

#else
Calendar::Calendar(DisplayBuffer *buffer, calendar_client::CalendarClient *calClient)
	: DisplayComponent(buffer, buffer->width() / 2, StatusBar::StatusBarHeight, buffer->width() / 2, buffer->height() - StatusBar::StatusBarHeight),
#endif
	  calClient(calClient),
	  entryHeight(48)
{
}

bool Calendar::checkTruncatedEvents(time_t now, int *displayable, int *skipped, int *more) const
{
	// If there are more calendar items than fit on the display,
	// we need to truncate them a bit so they'll fit
	int maxCalendarEvents = height / entryHeight;

	bool mustRetry = false;

	do
	{
		*skipped = 0;
		*more = 0;

		int calendarEntryCount = calClient->getCalendarEntries()->size();
		bool skipPast = calendarEntryCount > maxCalendarEvents;

#if DEBUG_LEVEL >= 1
		Serial.printf("[debug] calendar_entries: %d (max: %d)\n", calendarEntryCount, maxCalendarEvents);
#endif
		int itemCntr = 0;
		for (calendar_client::CalendarEntries::const_iterator calIt = calClient->getCalendarEntries()->begin();
			 calIt != calClient->getCalendarEntries()->end(); calIt++)
		{
			// if we must truncate, lets hide the the calendar item after now
			if (skipPast && calIt->getEnd() < now)
			{
#if DEBUG_LEVEL >= 1
				Serial.printf("[debug] skipping past event: %s\n", calIt->getTitle().c_str());
#endif
				(*skipped)++;
				// and re-evaluate if we still have to truncate
				calendarEntryCount--;
				skipPast = calendarEntryCount > maxCalendarEvents;
				continue;
			}

			itemCntr++;

			if (itemCntr > maxCalendarEvents)
			{
#if DEBUG_LEVEL >= 1
				Serial.printf("[debug] skipping upcoming event due to no more available space: %s\n", calIt->getTitle().c_str());
#endif
				(*more)++;
				calendarEntryCount--;
				continue;
			}
		}

		// if we need to add "more..." markers on the top AND bottom, we re-run this
		// but with one less maxEntry, so we can be sure everything fits on screen nicely
		if (*more > 0 && *skipped > 0)
		{
			mustRetry = !mustRetry;
			if (mustRetry)
			{
				maxCalendarEvents--;
#if DEBUG_LEVEL >= 1
				Serial.println("[debug] We have both, skipped and more meetings. Re-run checkTruncated but with one less maxEntry");
#endif
			}
		}
	} while (mustRetry);

	*displayable = maxCalendarEvents;

#if DEBUG_LEVEL >= 1
	Serial.printf("[debug] Display supports a maximum of %d events\n", maxCalendarEvents);
	Serial.printf("[debug] displaying %d events\n", *displayable);
	Serial.printf("[debug] skipping %d past events\n", *skipped);
	Serial.printf("[debug] omitting %d future events\n", *more);
#endif

	return (*more > 0 || *skipped > 0);
}

int Calendar::renderSkippedEntries(int y, int skipped, int maxEntries, String eventsTxt, bool more) const
{
	int availableHeight;

	if (more)
	{
		availableHeight = (height - maxEntries * entryHeight) / 2;
	}
	else
	{
		availableHeight = height - maxEntries * entryHeight;
	}

	String remainingText("+" + String(skipped) + " " + eventsTxt + "...");

	buffer->setFontSize(9);
	buffer->drawString(x + width / 2, y + availableHeight / 2, remainingText, Alignment::Center, width, 1);
	buffer->drawHLine(x, y + availableHeight, width);

	return availableHeight;
}

bool Calendar::hasToRedraw(time_t now) const
{
	// If our previous status is the same as the current one, we don't need to re-draw
	String calendarEntries = "";
	for (calendar_client::CalendarEntries::const_iterator calIt = calClient->getCalendarEntries()->begin();
		 calIt != calClient->getCalendarEntries()->end(); calIt++)
	{
		calendarEntries += calIt->getTitle() + calIt->getStart() + calIt->getEnd() + calIt->getMessage() + calIt->isAllDay() + calIt->isImportant() + calIt->getBusy();
	}

	uint32_t calendarEntriesHash = fnv1aHash(calendarEntries);

	// Check if something needs to be re-drawn
	Preferences prefs;
	prefs.begin(NVS_NAMESPACE, false);

	// get the previous status message
	uint32_t currCalenderHash = prefs.getULong("curCalHash", 0);

	bool mustRedraw = currCalenderHash != calendarEntriesHash;

	if (mustRedraw)
	{
		// save the current calendar hash
		prefs.putULong("curCalHash", calendarEntriesHash);
		Serial.printf("[info] Calendar changed, must redraw: %d\n", calendarEntriesHash);
	}
#if DEBUG_LEVEL >= 1
	else
	{
		Serial.printf("[debug] No calendar change, no redraw.\n");
	}
#endif

	prefs.end();

	return mustRedraw;
}

void Calendar::render(time_t now) const
{
	int yOffset = y;

	int maxCalendarEntries = 0;
	int skippedMeetings = 0;
	int moreMeetings = 0;
	bool truncate = checkTruncatedEvents(now, &maxCalendarEntries, &skippedMeetings, &moreMeetings);

	// If we have more meetings than we can display,
	// at least add an indicator that there's more to come at the bottom
	if (skippedMeetings > 0)
	{
		yOffset += renderSkippedEntries(yOffset, skippedMeetings, maxCalendarEntries, TXT_PAST_EVENTS, moreMeetings > 0);
	}

	int itemCntr = 0;
	int drawnItems = 0;
	for (calendar_client::CalendarEntries::const_iterator calIt = calClient->getCalendarEntries()->begin();
		 calIt != calClient->getCalendarEntries()->end(); calIt++)
	{
		itemCntr++;

		// If we truncate
		// check if we either have past meetings to skip OR
		// check if we have upcoming meetings to omit
		if (truncate &&
			((itemCntr < skippedMeetings + 1) ||
			 (truncate && drawnItems >= maxCalendarEntries)))
		{
			continue;
		}

		yOffset += renderCalendarEntry(x, yOffset, *calIt, now);
		drawnItems++;
	}

	// If we have more meetings than we can display,
	// at least add an indicator that there's more to come at the bottom
	if (moreMeetings > 0)
	{
		yOffset += renderSkippedEntries(yOffset, moreMeetings, maxCalendarEntries, TXT_UPCOMING_EVENTS, skippedMeetings > 0);
	}
}

int Calendar::renderCalendarEntry(int x, int y, const calendar_client::CalendarEntry &entry, time_t now) const
{
	bool isPastEvent = entry.getEnd() < now;
	bool isCurrentEvent = entry.getStart() < now && entry.getEnd() > now;

#if DEBUG_LEVEL >= 2
	Serial.printf("[verbose] rendering_calendar entry: %s. is_important: %d, is_current: %d, is_past: %d\n", entry.getTitle().c_str(), entry.isImportant(), isCurrentEvent, isPastEvent);
#endif

#if defined(DISP_3C) || defined(DISP_7C)
	Color fgSave;
#endif

	if (isCurrentEvent)
	{
#if defined(DISP_3C) || defined(DISP_7C)
		fgSave = buffer->setForegroundColor(accentColor);
#elif INVERT_AS_ACCENT == true
		buffer->invert();
		buffer->fillBackground(x, y, width, entryHeight);
#endif
	}

	renderCalendarEntryIcon(x, y, entry, isPastEvent, isCurrentEvent);
	renderCalendarEntryTitle(x + entryHeight, y, entry, isPastEvent);
	renderCalendarEntryTime(x + entryHeight, y + entryHeight, entry, isPastEvent);

	if (isCurrentEvent)
	{
#if defined(DISP_3C) || defined(DISP_7C)
		buffer->setForegroundColor(fgSave);
#elif INVERT_AS_ACCENT == true
		buffer->invert();
#endif
	}

	buffer->drawHLine(x, y + entryHeight, width);

	return entryHeight;
}

void Calendar::renderCalendarEntryIcon(int x, int y, const calendar_client::CalendarEntry &entry, bool isPast, bool isCurrent) const
{
	int offsetX = x;
	int offsetY = y;

	int bitmapSize = 32;
	String icon = "calendar";

	if (isCurrent)
	{
		bitmapSize = 48;
		icon = "wi_time_2";
	}
	else if (entry.isImportant() && !isPast)
	{
		bitmapSize = 48;
		icon = "warning_icon";
	}

	offsetX += entryHeight / 2;
	offsetY += entryHeight / 2;

	buffer->drawIcon(offsetX, offsetY, icon, bitmapSize, Alignment::HorizontalCenter | Alignment::VerticalCenter);

	if (isPast)
	{
		// for past events, we cross out the calendar icon
		buffer->drawIcon(offsetX, offsetY, "x_symbol", bitmapSize, Alignment::HorizontalCenter | Alignment::VerticalCenter);
	}
}
void Calendar::renderCalendarEntryTitle(int x, int y, const calendar_client::CalendarEntry &entry, bool isPast) const
{
	int offsetY = y + 5;

	buffer->setFontSize(12);
	Rect r = buffer->drawString(x, offsetY, entry.getTitle(), Alignment::Top | Alignment::Left, width - entryHeight, 1);

	if (isPast)
	{
		// for past events, we strike through the title
		buffer->drawHLine(r.x, r.y + r.height / 2, r.width);
	}
}

void Calendar::renderCalendarEntryTime(int x, int y, const calendar_client::CalendarEntry &entry, bool isPast) const
{
	// build strings
	time_t entryStart = entry.getStart();
	tm tmFrom = *localtime(&entryStart);
	String fromStr = getTimeStr(&tmFrom);

	time_t entryEnd = entry.getEnd();
	tm tmTo = *localtime(&entryEnd);
	String toStr = getTimeStr(&tmTo);

	String timeString(fromStr + " " + TXT_UNTIL + " " + toStr);

	// Draw the hours
	buffer->setFontSize(9);
	int offsetY = y - 5;
	Rect r = buffer->drawString(x, offsetY, timeString, Alignment::Bottom | Alignment::Left, width - entryHeight, 1);

	if (isPast)
	{
		buffer->drawHLine(r.x, r.y + r.height / 2, r.width);
	}
}