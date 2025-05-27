#include <Preferences.h>

#include "client/calendar_client.h"
#include "config.h"

using namespace calendar_client;

CustomStatus::CustomStatus(const JsonObject &json)
{
	if (json["icon"].is<const char *>())
	{
		icon = json["icon"].as<const char *>();
	}
	else
	{
		icon = ""; // Default value if key is missing or not a string
	}

	if (json["icon_size"].is<int>())
	{
		icon_size = json["icon_size"].as<int32_t>();
	}
	else
	{
		icon_size = 0; // Default value if key is missing or not an integer
	}

	if (json["title"].is<const char *>())
	{
		title = json["title"].as<const char *>();
	}
	else
	{
		title = ""; // Default value if key is missing or not a string
	}

	if (json["description"].is<const char *>())
	{
		description = json["description"].as<const char *>();
	}
	else
	{
		description = ""; // Default value if key is missing or not a string
	}
}

CalendarEntry::CalendarEntry(const JsonObject &json)
{
	if (json["title"].is<const char *>())
	{
		title = String(json["title"].as<const char *>());
	}

	if (json["message"].is<const char *>())
	{
		message = String(json["message"].as<const char *>());
	}

	if (json["start"].is<time_t>())
	{
		start = json["start"].as<time_t>();
	}

	if (json["end"].is<time_t>())
	{
		end = json["end"].as<time_t>();
	}

	if (json["all_day"].is<bool>())
	{
		all_day = json["all_day"].as<bool>();
	}

	if (json["busy"].is<BusyState>())
	{
		busy = json["busy"].as<BusyState>();
	}
	else
	{
		busy = BusyState::Free;
	}

	if (json["important"].is<bool>())
	{
		important = json["important"].as<bool>();
	}
	else
	{
		important = false;
	}
}

int CalendarClient::fetchCustomStatus()
{
	int attempts = 0;
	bool rxSuccess = false;

	int httpResponse = 0;
	do
	{
		wl_status_t connection_status = WiFi.status();
		if (connection_status != WL_CONNECTED)
		{
			// -512 offset distinguishes these errors from httpClient errors
			return -512 - static_cast<int>(connection_status);
		}

		HTTPClient http;
		http.setConnectTimeout(HTTP_CLIENT_TCP_TIMEOUT); // default 10s
		http.setTimeout(HTTP_CLIENT_TCP_TIMEOUT);		 // default 10s
		http.addHeader(String("Content-Type"), String("application/protobuf"));

		http.begin(client, apiEndpoint, apiPort, String("/status?calendar=") + String(API_ENDPOINT_FETCH_CALENDAR));
		httpResponse = http.GET();
#if DEBUG >= 1
		Serial.println("[debug] HTTP Response: " + String(httpResponse, DEC));
#endif

		switch (httpResponse)
		{
		case HTTP_CODE_OK:
			rxSuccess = parseCustomStatus(http);
			break;

		case HTTP_CODE_GONE:
			rxSuccess = true;
			break;
		}

		client.stop();
		http.end();
		delay(attempts * 500); // ms
		++attempts;
	} while (!rxSuccess && attempts <= RETRY_COUNT);

	return httpResponse;
}

int CalendarClient::fetchCalendar()
{
	int attempts = 0;
	bool rxSuccess = false;

	int httpResponse = 0;
	do
	{
		wl_status_t connection_status = WiFi.status();
		if (connection_status != WL_CONNECTED)
		{
			// -512 offset distinguishes these errors from httpClient errors
			return -512 - static_cast<int>(connection_status);
		}

		HTTPClient http;
		http.setConnectTimeout(HTTP_CLIENT_TCP_TIMEOUT); // default 10s
		http.setTimeout(HTTP_CLIENT_TCP_TIMEOUT);		 // default 10s
		http.addHeader(String("Content-Type"), String("application/protobuf"));

		http.begin(client, apiEndpoint, apiPort, String("/calendar?calendar=") + String(API_ENDPOINT_FETCH_CALENDAR));
		httpResponse = http.GET();
#if DEBUG >= 1
		Serial.println("[debug] HTTP Response: " + String(httpResponse, DEC));
#endif

		switch (httpResponse)
		{
		case HTTP_CODE_OK:
			rxSuccess = parseCalendar(http);
			break;

		case HTTP_CODE_GONE:
			rxSuccess = true;
			break;
		}

		client.stop();
		http.end();
		delay(attempts * 500); // ms
		++attempts;
	} while (!rxSuccess && attempts <= RETRY_COUNT);

	return httpResponse;
}

const CalendarEntry *CalendarClient::getCurrentEvent(time_t now, bool nowClosestToStart) const
{
	CalendarEntries possibleCurrentEvents;

	// because we can have multiple calendar events going at the same time, we need to find all that happen now
	for (CalendarEntries::const_iterator it = entries.begin(); it != entries.end(); it++)
	{
		if (it->getStart() < now && it->getEnd() > now)
		{
#if DEBUG_LEVEL >= 2
			Serial.printf("[verbose] Event %s happening right now\n", it->getTitle().c_str());
#endif
			CalendarEntry entry = *it;
			possibleCurrentEvents.push_back(entry);
		}
	}

#if DEBUG_LEVEL >= 2
	Serial.printf("[verbose] Found %d events happening right now\n", possibleCurrentEvents.size());
#endif

	// if there is none, or multiple, return early
	switch (possibleCurrentEvents.size())
	{
	case 0:
		return NULL;
	case 1:
		CalendarEntry *currentEntry = new CalendarEntry();
		*currentEntry = possibleCurrentEvents.at(0);
		return currentEntry;
	}

	// Now lets try to find the one that starts closest to now
	CalendarEntry *closest = new CalendarEntry();
	time_t closestDelta = LONG_MAX;

	// start from ++begin() in order to skip the first one
	for (CalendarEntries::const_iterator it = possibleCurrentEvents.begin(); it != possibleCurrentEvents.end(); it++)
	{
		time_t delta = difftime(nowClosestToStart ? now : it->getEnd(), nowClosestToStart ? it->getStart() : now);

#if DEBUG_LEVEL >= 2
		Serial.printf("[verbose] Event %s has a delta of %ld to now", it->getTitle().c_str(), delta);
#endif

		// if two events happen at the same time, then prefer the important one
		if (delta == closestDelta && it->isImportant() && !closest->isImportant())
		{
#if DEBUG_LEVEL >= 2
			Serial.printf(" which has the same delta to now than the previous event %s (delta=%ld) but is marked important", closest->getTitle().c_str(), closestDelta);
#endif
			*closest = *it;
		}
		else if (delta < closestDelta)
		{
#if DEBUG_LEVEL >= 2
			Serial.printf(" which is closer to now than the previous event %s (delta=%ld)", closest->getTitle().c_str(), closestDelta);
#endif
			closestDelta = delta;
			*closest = *it;
		}
#if DEBUG_LEVEL >= 2
		else
		{
			Serial.printf(" which has further away from now than the previously found closest event %s (delta=%ld)", closest->getTitle().c_str(), closestDelta);
		}
		Serial.printf("\n");
#endif
	}

	return closest;
}

const CalendarEntry *CalendarClient::getNextEvent(time_t now) const
{
	// Since we know that events are sorted, we can just return the first meeting that starts
	// after the now
	for (CalendarEntries::const_iterator it = entries.begin(); it != entries.end(); it++)
	{
		if (it->getStart() > now)
		{
			return &(*it);
		}
	}

	return NULL;
}

bool CalendarClient::parseCustomStatus(HTTPClient &client)
{
	JsonDocument doc;
	DeserializationError error = deserializeJson(doc, client.getStream());

#if DEBUG_LEVEL >= 1
	if (doc.overflowed())
	{
		Serial.println("[debug] JsonDocument.overflowed() == true");
	}
#endif
#if DEBUG_LEVEL >= 2
	serializeJsonPretty(doc, Serial);
#endif

	if (error)
	{
		return false;
	}

	customStatus = new CustomStatus(doc.as<JsonObject>());

#if DEBUG_LEVEL >= 1
	Serial.printf("[debug] lastUpdated: %ld\n", last_updated);
#endif

#if DEBUG_LEVEL >= 1
	Serial.printf("[debug] calendar_events: %d\n", entries.size());
#endif

	return true;
}

bool CalendarClient::parseCalendar(HTTPClient &client)
{
	JsonDocument doc;
	DeserializationError error = deserializeJson(doc, client.getStream());

#if DEBUG_LEVEL >= 1
	if (doc.overflowed())
	{
		Serial.println("[debug] JsonDocument.overflowed() == true");
	}
#endif
#if DEBUG_LEVEL >= 2
	serializeJsonPretty(doc, Serial);
#endif

	if (error)
	{
		return false;
	}

	last_updated = doc["last_updated"].as<time_t>();
	for (JsonObject entry : doc["entries"].as<JsonArray>())
	{
		entries.push_back(CalendarEntry(entry));
	}

#if DEBUG_LEVEL >= 1
	Serial.printf("[debug] calendar_events: %d\n", entries.size());
#endif

	return true;
}

// This function returns a pointer to a string representing the meaning for a
// HTTP response status code or an arduino client error code.
// ArduinoJson DeserializationError codes are also included here and are given a
// negative 100 offset to distinguish them from other client error codes.
//
// HTTP response status codes [100, 599]
// https://developer.mozilla.org/en-US/docs/Web/HTTP/Status
//
// HTTP client errors [0, -255]
// https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPClient/src/HTTPClient.h
//
// ArduinoJson DeserializationError codes [-256, -511]
// https://arduinojson.org/v6/api/misc/deserializationerror/
//
// WiFi Status codes [-512, -767]
// https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFiType.h
const char *CalendarClient::getHttpResponsePhrase(int code)
{
	switch (code)
	{
	// 1xx - Informational Responses
	case 100:
		return TXT_HTTP_RESPONSE_100;
	case 101:
		return TXT_HTTP_RESPONSE_101;
	case 102:
		return TXT_HTTP_RESPONSE_102;
	case 103:
		return TXT_HTTP_RESPONSE_103;

	// 2xx - Successful Responses
	case 200:
		return TXT_HTTP_RESPONSE_200;
	case 201:
		return TXT_HTTP_RESPONSE_201;
	case 202:
		return TXT_HTTP_RESPONSE_202;
	case 203:
		return TXT_HTTP_RESPONSE_203;
	case 204:
		return TXT_HTTP_RESPONSE_204;
	case 205:
		return TXT_HTTP_RESPONSE_205;
	case 206:
		return TXT_HTTP_RESPONSE_206;
	case 207:
		return TXT_HTTP_RESPONSE_207;
	case 208:
		return TXT_HTTP_RESPONSE_208;
	case 226:
		return TXT_HTTP_RESPONSE_226;

	// 3xx - Redirection Responses
	case 300:
		return TXT_HTTP_RESPONSE_300;
	case 301:
		return TXT_HTTP_RESPONSE_301;
	case 302:
		return TXT_HTTP_RESPONSE_302;
	case 303:
		return TXT_HTTP_RESPONSE_303;
	case 304:
		return TXT_HTTP_RESPONSE_304;
	case 305:
		return TXT_HTTP_RESPONSE_305;
	case 307:
		return TXT_HTTP_RESPONSE_307;
	case 308:
		return TXT_HTTP_RESPONSE_308;

	// 4xx - Client Error Responses
	case 400:
		return TXT_HTTP_RESPONSE_400;
	case 401:
		return TXT_HTTP_RESPONSE_401;
	case 402:
		return TXT_HTTP_RESPONSE_402;
	case 403:
		return TXT_HTTP_RESPONSE_403;
	case 404:
		return TXT_HTTP_RESPONSE_404;
	case 405:
		return TXT_HTTP_RESPONSE_405;
	case 406:
		return TXT_HTTP_RESPONSE_406;
	case 407:
		return TXT_HTTP_RESPONSE_407;
	case 408:
		return TXT_HTTP_RESPONSE_408;
	case 409:
		return TXT_HTTP_RESPONSE_409;
	case 410:
		return TXT_HTTP_RESPONSE_410;
	case 411:
		return TXT_HTTP_RESPONSE_411;
	case 412:
		return TXT_HTTP_RESPONSE_412;
	case 413:
		return TXT_HTTP_RESPONSE_413;
	case 414:
		return TXT_HTTP_RESPONSE_414;
	case 415:
		return TXT_HTTP_RESPONSE_415;
	case 416:
		return TXT_HTTP_RESPONSE_416;
	case 417:
		return TXT_HTTP_RESPONSE_417;
	case 418:
		return TXT_HTTP_RESPONSE_418;
	case 421:
		return TXT_HTTP_RESPONSE_421;
	case 422:
		return TXT_HTTP_RESPONSE_422;
	case 423:
		return TXT_HTTP_RESPONSE_423;
	case 424:
		return TXT_HTTP_RESPONSE_424;
	case 425:
		return TXT_HTTP_RESPONSE_425;
	case 426:
		return TXT_HTTP_RESPONSE_426;
	case 428:
		return TXT_HTTP_RESPONSE_428;
	case 429:
		return TXT_HTTP_RESPONSE_429;
	case 431:
		return TXT_HTTP_RESPONSE_431;
	case 451:
		return TXT_HTTP_RESPONSE_451;

	// 5xx - Server Error Responses
	case 500:
		return TXT_HTTP_RESPONSE_500;
	case 501:
		return TXT_HTTP_RESPONSE_501;
	case 502:
		return TXT_HTTP_RESPONSE_502;
	case 503:
		return TXT_HTTP_RESPONSE_503;
	case 504:
		return TXT_HTTP_RESPONSE_504;
	case 505:
		return TXT_HTTP_RESPONSE_505;
	case 506:
		return TXT_HTTP_RESPONSE_506;
	case 507:
		return TXT_HTTP_RESPONSE_507;
	case 508:
		return TXT_HTTP_RESPONSE_508;
	case 510:
		return TXT_HTTP_RESPONSE_510;
	case 511:
		return TXT_HTTP_RESPONSE_511;

	// HTTP client errors [0, -255]
	case HTTPC_ERROR_CONNECTION_REFUSED:
		return TXT_HTTPC_ERROR_CONNECTION_REFUSED;
	case HTTPC_ERROR_SEND_HEADER_FAILED:
		return TXT_HTTPC_ERROR_SEND_HEADER_FAILED;
	case HTTPC_ERROR_SEND_PAYLOAD_FAILED:
		return TXT_HTTPC_ERROR_SEND_PAYLOAD_FAILED;
	case HTTPC_ERROR_NOT_CONNECTED:
		return TXT_HTTPC_ERROR_NOT_CONNECTED;
	case HTTPC_ERROR_CONNECTION_LOST:
		return TXT_HTTPC_ERROR_CONNECTION_LOST;
	case HTTPC_ERROR_NO_STREAM:
		return TXT_HTTPC_ERROR_NO_STREAM;
	case HTTPC_ERROR_NO_HTTP_SERVER:
		return TXT_HTTPC_ERROR_NO_HTTP_SERVER;
	case HTTPC_ERROR_TOO_LESS_RAM:
		return TXT_HTTPC_ERROR_TOO_LESS_RAM;
	case HTTPC_ERROR_ENCODING:
		return TXT_HTTPC_ERROR_ENCODING;
	case HTTPC_ERROR_STREAM_WRITE:
		return TXT_HTTPC_ERROR_STREAM_WRITE;
	case HTTPC_ERROR_READ_TIMEOUT:
		return TXT_HTTPC_ERROR_READ_TIMEOUT;

	// ArduinoJson DeserializationError codes  [-256, -511]
	case -256 - (DeserializationError::Code::Ok):
		return TXT_DESERIALIZATION_ERROR_OK;
	case -256 - (DeserializationError::Code::EmptyInput):
		return TXT_DESERIALIZATION_ERROR_EMPTY_INPUT;
	case -256 - (DeserializationError::Code::IncompleteInput):
		return TXT_DESERIALIZATION_ERROR_INCOMPLETE_INPUT;
	case -256 - (DeserializationError::Code::InvalidInput):
		return TXT_DESERIALIZATION_ERROR_INVALID_INPUT;
	case -256 - (DeserializationError::Code::NoMemory):
		return TXT_DESERIALIZATION_ERROR_NO_MEMORY;
	case -256 - (DeserializationError::Code::TooDeep):
		return TXT_DESERIALIZATION_ERROR_TOO_DEEP;

	// WiFi Status codes [-512, -767]
	case -512 - WL_NO_SHIELD:
		return TXT_WL_NO_SHIELD;
	// case -512 - WL_STOPPED:       return TXT_WL_STOPPED; // future
	case -512 - WL_IDLE_STATUS:
		return TXT_WL_IDLE_STATUS;
	case -512 - WL_NO_SSID_AVAIL:
		return TXT_WL_NO_SSID_AVAIL;
	case -512 - WL_SCAN_COMPLETED:
		return TXT_WL_SCAN_COMPLETED;
	case -512 - WL_CONNECTED:
		return TXT_WL_CONNECTED;
	case -512 - WL_CONNECT_FAILED:
		return TXT_WL_CONNECT_FAILED;
	case -512 - WL_CONNECTION_LOST:
		return TXT_WL_CONNECTION_LOST;
	case -512 - WL_DISCONNECTED:
		return TXT_WL_DISCONNECTED;

	default:
		return "";
	}
}