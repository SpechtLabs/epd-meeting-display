// built-in C++ libraries
#include <cstring>
#include <vector>
#include <cmath>

// arduino/esp32 libraries
#include <Arduino.h>
#include <esp_sntp.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <time.h>
#include <WiFi.h>

// additional libraries
#include <Adafruit_BusIO_Register.h>
#include <ArduinoJson.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <WiFiClient.h>

#ifndef USE_HTTP
#include <WiFiClient.h>
#endif

// header files
#include "utils.h"
#include "config.h"
#include "_strftime.h"

// icons
#include "icons/icons_24x24.h"
#include "icons/icons_32x32.h"
#include "icons/icons_48x48.h"
#include "icons/icons_128x128.h"
#include "icons/icons_196x196.h"

// Power-on and connect WiFi.
wl_status_t startWiFi()
{
	WiFi.mode(WIFI_STA);
	Serial.printf("%s '%s'", "Connecting to", WIFI_SSID);
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

	// timeout if WiFi does not connect in WIFI_TIMEOUT ms from now
	unsigned long timeout = millis() + WIFI_TIMEOUT;
	wl_status_t connection_status;

	int attempts = 0;
	do
	{
		do
		{
			connection_status = WiFi.status();
			Serial.print(".");
			delay(50);
		} while ((connection_status != WL_CONNECTED) && (millis() < timeout));

		if (connection_status != WL_CONNECTED)
		{
			Serial.println("Failed!");
			Serial.println();
			Serial.printf("Attempt %d/%d failed, retrying...\n", attempts + 1, RETRY_COUNT);
			delay(attempts * 500); // ms
		}

		++attempts;
	} while (connection_status != WL_CONNECTED && attempts <= RETRY_COUNT);

	if (connection_status != WL_CONNECTED)
	{
		Serial.println("Failed!");
		Serial.println();
		Serial.printf("%s '%s'\n", "Could not connect to", WIFI_SSID);
		return connection_status;
	}
	else
	{
		Serial.println("Success!");
		Serial.println("IP: " + WiFi.localIP().toString());
		Serial.printf("RSSI: %d\n", WiFi.RSSI());
	}

	return connection_status;
}

// Disconnect and power-off WiFi.
void killWiFi()
{
	WiFi.disconnect();
	WiFi.mode(WIFI_OFF);
}

// Prints the local time to serial monitor.
// Returns true if getting local time was a success, otherwise false.
bool printLocalTime(tm *timeInfo)
{
	int attempts = 0;
	while (!getLocalTime(timeInfo) && attempts++ < 3)
	{
		Serial.println("Failed to get the time!");
		return false;
	}
	Serial.println(timeInfo, "%A, %B %d, %Y %H:%M:%S");
	return true;
} // printLocalTime

// Waits for NTP server time sync, adjusted for the time zone specified in
// config.cpp.
//
// Returns true if time was set successfully, otherwise false.
//
// Note: Must be connected to WiFi to get time from NTP server.
bool getNtpTime(tm *timeInfo)
{
	// Wait for SNTP synchronization to complete
	unsigned long timeout = millis() + NTP_TIMEOUT;
	sntp_sync_status_t sync_status = sntp_get_sync_status();
	if ((sync_status == SNTP_SYNC_STATUS_RESET) && (millis() < timeout))
	{
		Serial.print("Waiting for SNTP synchronization.");
		delay(100); // ms
		int attempts = 0;
		do
		{
			do
			{
				sync_status = sntp_get_sync_status();
				Serial.print(".");
				delay(50);
			} while ((sync_status != SNTP_SYNC_STATUS_RESET) && (millis() < timeout));

			if (sync_status != SNTP_SYNC_STATUS_RESET)
			{
				Serial.println("Failed!");
				Serial.println();
				Serial.printf("Attempt %d/%d failed, retrying...\n", attempts + 1, RETRY_COUNT);
				delay(attempts * 500); // ms
			}

			++attempts;
		} while (sync_status != SNTP_SYNC_STATUS_RESET && attempts <= RETRY_COUNT);

		if (sync_status != SNTP_SYNC_STATUS_RESET)
		{
			Serial.println("Failed!");
			Serial.println();
			Serial.println("Could not synchronize time with NTP server!");
		}
		else
		{
			Serial.println("Success!");
		}
	}

	return printLocalTime(timeInfo);
}

// Prints debug information about heap usage.
void printHeapUsage()
{
	Serial.println("[debug] Heap Size       : " + String(ESP.getHeapSize()) + " B");
	Serial.println("[debug] Available Heap  : " + String(ESP.getFreeHeap()) + " B");
	Serial.println("[debug] Min Free Heap   : " + String(ESP.getMinFreeHeap()) + " B");
	Serial.println("[debug] Max Allocatable : " + String(ESP.getMaxAllocHeap()) + " B");
	return;
}

// Returns battery voltage in millivolts (mv).
uint32_t readBatteryVoltage()
{
	esp_adc_cal_characteristics_t adc_chars;
	// __attribute__((unused)) disables compiler warnings about this variable
	// being unused (Clang, GCC) which is the case when DEBUG_LEVEL == 0.
	esp_adc_cal_value_t val_type __attribute__((unused));
	adc_power_acquire();
	uint16_t adc_val = analogRead(PIN_BAT_ADC);
	adc_power_release();

	// We will use the eFuse ADC calibration bits, to get accurate voltage
	// readings. The DFRobot FireBeetle Esp32-E V1.0's ADC is 12 bit, and uses
	// 11db attenuation, which gives it a measurable input voltage range of 150mV
	// to 2450mV.
	val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_11db, ADC_WIDTH_BIT_12, 1100, &adc_chars);

#if DEBUG_LEVEL >= 1
	if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
	{
		Serial.println("[debug] ADC Cal eFuse Vref");
	}
	else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
	{
		Serial.println("[debug] ADC Cal Two Point");
	}
	else
	{
		Serial.println("[debug] ADC Cal Default");
	}
#endif

	uint32_t batteryVoltage = esp_adc_cal_raw_to_voltage(adc_val, &adc_chars);
	// DFRobot FireBeetle Esp32-E V1.0 voltage divider (1M+1M), so readings are
	// multiplied by 2.
	batteryVoltage *= 2;
	return batteryVoltage;
} // end readBatteryVoltage

// Returns battery percentage, rounded to the nearest integer.
// Takes a voltage in millivolts and uses a sigmoidal approximation to find an
// approximation of the battery life percentage remaining.
//
// This function contains LGPLv3 code from
// <https://github.com/rlogiacco/BatterySense>.
//
// Symmetric sigmoidal approximation
// <https://www.desmos.com/calculator/7m9lu26vpy>
// c - c / (1 + k*x/v)^3
uint32_t calcBatPercent(uint32_t v, uint32_t minv, uint32_t maxv)
{
	// slow
	// uint32_t p = 110 - (110 / (1 + pow(1.468 * (v - minv)/(maxv - minv), 6)));

	// steep
	// uint32_t p = 102 - (102 / (1 + pow(1.621 * (v - minv)/(maxv - minv), 8.1)));

	// normal
	uint32_t p = 105 - (105 / (1 + pow(1.724 * (v - minv) / (maxv - minv), 5.5)));
	return p >= 100 ? 100 : p;
}

// Gets string with the current time.
String getTimeStr(tm *timeInfo)
{
	String timeStr;
	char buf[48] = {};
	_strftime(buf, sizeof(buf), TIME_FORMAT, timeInfo);
	timeStr = buf;

	// remove double spaces. %e will add an extra space, ie. " 1" instead of "1"
	timeStr.replace("  ", " ");
	return timeStr;
}

// Gets string with the current date and time of the current refresh attempt.
String getRefreshTimeStr(tm *timeInfo, bool timeSuccess)
{
	String timeStr = "unknown";

	if (timeSuccess == false)
	{
		return timeStr;
	}

	char buf[48] = {};
	_strftime(buf, sizeof(buf), REFRESH_TIME_FORMAT, timeInfo);
	timeStr = buf;

	// remove double spaces.
	timeStr.replace("  ", " ");

	return timeStr;
}

// This function sets the builtin LED to LOW and disables it even during deep sleep.
void disableBuiltinLED()
{
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);
	gpio_hold_en(static_cast<gpio_num_t>(LED_BUILTIN));
	gpio_deep_sleep_hold_en();
	return;
}

// Calculate a simple FNV-1a hash of the given data.
const uint32_t fnv1aHash(const String &string)
{
	const uint8_t *data = reinterpret_cast<const uint8_t *>(string.c_str());
	size_t len = string.length();
	uint32_t hash = 2166136261u; // FNV-1a 32-bit offset basis
	for (size_t i = 0; i < len; ++i)
	{
		hash ^= data[i];
		hash *= 16777619u; // FNV-1a 32-bit prime
	}
	return hash;
}

const uint8_t *getIcon(String iconName, int16_t iconSize)
{
	switch (iconSize)
	{
	case 24:
		return getIcon24(iconName);
		break;

	case 32:
		return getIcon32(iconName);
		break;

	case 48:
		return getIcon48(iconName);
		break;
	case 128:
		return getIcon128(iconName);
		break;

	case 196:
		return getIcon196(iconName);
		break;

	default:
		Serial.printf("[error]: Icon size '%d' is not available in this firmware", iconSize);
		return NULL;
		break;
	}
}

const uint8_t *getIcon24(String iconName)
{
	if (iconName == "battery_0_bar_90deg")
	{
		return battery_0_bar_90deg_24x24;
	}
	if (iconName == "calendar")
	{
		return calendar_24x24;
	}
	if (iconName == "battery_full_90deg")
	{
		return battery_full_90deg_24x24;
	}
	if (iconName == "battery_6_bar_90deg")
	{
		return battery_6_bar_90deg_24x24;
	}
	if (iconName == "wifi_off")
	{
		return wifi_off_24x24;
	}
	if (iconName == "wi_time_12")
	{
		return wi_time_12_24x24;
	}
	if (iconName == "battery_4_bar_90deg")
	{
		return battery_4_bar_90deg_24x24;
	}
	if (iconName == "wi_time_4")
	{
		return wi_time_4_24x24;
	}
	if (iconName == "wifi_1_bar")
	{
		return wifi_1_bar_24x24;
	}
	if (iconName == "wi_time_11")
	{
		return wi_time_11_24x24;
	}
	if (iconName == "biological_hazard_symbol")
	{
		return biological_hazard_symbol_24x24;
	}
	if (iconName == "battery_1_bar_90deg")
	{
		return battery_1_bar_90deg_24x24;
	}
	if (iconName == "wi_fire")
	{
		return wi_fire_24x24;
	}
	if (iconName == "wi_cloud_up")
	{
		return wi_cloud_up_24x24;
	}
	if (iconName == "x_symbol")
	{
		return x_symbol_24x24;
	}
	if (iconName == "wi_small_craft_advisory")
	{
		return wi_small_craft_advisory_24x24;
	}
	if (iconName == "wi_time_5")
	{
		return wi_time_5_24x24;
	}
	if (iconName == "wi_cloud")
	{
		return wi_cloud_24x24;
	}
	if (iconName == "battery_5_bar_90deg")
	{
		return battery_5_bar_90deg_24x24;
	}
	if (iconName == "wifi_x")
	{
		return wifi_x_24x24;
	}
	if (iconName == "wi_alien")
	{
		return wi_alien_24x24;
	}
	if (iconName == "wi_time_7")
	{
		return wi_time_7_24x24;
	}
	if (iconName == "battery_2_bar_90deg")
	{
		return battery_2_bar_90deg_24x24;
	}
	if (iconName == "battery_charging_full_90deg")
	{
		return battery_charging_full_90deg_24x24;
	}
	if (iconName == "wi_time_8")
	{
		return wi_time_8_24x24;
	}
	if (iconName == "wi_time_10")
	{
		return wi_time_10_24x24;
	}
	if (iconName == "wifi_3_bar")
	{
		return wifi_3_bar_24x24;
	}
	if (iconName == "warning_icon")
	{
		return warning_icon_24x24;
	}
	if (iconName == "wi_refresh")
	{
		return wi_refresh_24x24;
	}
	if (iconName == "wi_time_6")
	{
		return wi_time_6_24x24;
	}
	if (iconName == "wi_na")
	{
		return wi_na_24x24;
	}
	if (iconName == "wi_time_3")
	{
		return wi_time_3_24x24;
	}
	if (iconName == "wi_time_2")
	{
		return wi_time_2_24x24;
	}
	if (iconName == "wi_cloud_refresh")
	{
		return wi_cloud_refresh_24x24;
	}
	if (iconName == "wi_time_9")
	{
		return wi_time_9_24x24;
	}
	if (iconName == "error_icon")
	{
		return error_icon_24x24;
	}
	if (iconName == "wi_cloud_down")
	{
		return wi_cloud_down_24x24;
	}
	if (iconName == "wifi")
	{
		return wifi_24x24;
	}
	if (iconName == "battery_3_bar_90deg")
	{
		return battery_3_bar_90deg_24x24;
	}
	if (iconName == "battery_alert_90deg")
	{
		return battery_alert_90deg_24x24;
	}
	if (iconName == "wifi_2_bar")
	{
		return wifi_2_bar_24x24;
	}
	if (iconName == "wi_time_1")
	{
		return wi_time_1_24x24;
	}

	Serial.printf("[error]: Icon '%s' is not available in this firmware", iconName.c_str());
	return NULL;
}
const uint8_t *getIcon32(String iconName)
{
	if (iconName == "battery_0_bar_90deg")
	{
		return battery_0_bar_90deg_32x32;
	}
	if (iconName == "calendar")
	{
		return calendar_32x32;
	}
	if (iconName == "battery_full_90deg")
	{
		return battery_full_90deg_32x32;
	}
	if (iconName == "battery_6_bar_90deg")
	{
		return battery_6_bar_90deg_32x32;
	}
	if (iconName == "wifi_off")
	{
		return wifi_off_32x32;
	}
	if (iconName == "wi_time_12")
	{
		return wi_time_12_32x32;
	}
	if (iconName == "battery_4_bar_90deg")
	{
		return battery_4_bar_90deg_32x32;
	}
	if (iconName == "wi_time_4")
	{
		return wi_time_4_32x32;
	}
	if (iconName == "wifi_1_bar")
	{
		return wifi_1_bar_32x32;
	}
	if (iconName == "wi_time_11")
	{
		return wi_time_11_32x32;
	}
	if (iconName == "biological_hazard_symbol")
	{
		return biological_hazard_symbol_32x32;
	}
	if (iconName == "battery_1_bar_90deg")
	{
		return battery_1_bar_90deg_32x32;
	}
	if (iconName == "wi_fire")
	{
		return wi_fire_32x32;
	}
	if (iconName == "wi_cloud_up")
	{
		return wi_cloud_up_32x32;
	}
	if (iconName == "x_symbol")
	{
		return x_symbol_32x32;
	}
	if (iconName == "wi_small_craft_advisory")
	{
		return wi_small_craft_advisory_32x32;
	}
	if (iconName == "wi_time_5")
	{
		return wi_time_5_32x32;
	}
	if (iconName == "wi_cloud")
	{
		return wi_cloud_32x32;
	}
	if (iconName == "battery_5_bar_90deg")
	{
		return battery_5_bar_90deg_32x32;
	}
	if (iconName == "wifi_x")
	{
		return wifi_x_32x32;
	}
	if (iconName == "wi_alien")
	{
		return wi_alien_32x32;
	}
	if (iconName == "wi_time_7")
	{
		return wi_time_7_32x32;
	}
	if (iconName == "battery_2_bar_90deg")
	{
		return battery_2_bar_90deg_32x32;
	}
	if (iconName == "battery_charging_full_90deg")
	{
		return battery_charging_full_90deg_32x32;
	}
	if (iconName == "wi_time_8")
	{
		return wi_time_8_32x32;
	}
	if (iconName == "wi_time_10")
	{
		return wi_time_10_32x32;
	}
	if (iconName == "wifi_3_bar")
	{
		return wifi_3_bar_32x32;
	}
	if (iconName == "warning_icon")
	{
		return warning_icon_32x32;
	}
	if (iconName == "wi_refresh")
	{
		return wi_refresh_32x32;
	}
	if (iconName == "wi_time_6")
	{
		return wi_time_6_32x32;
	}
	if (iconName == "wi_na")
	{
		return wi_na_32x32;
	}
	if (iconName == "wi_time_3")
	{
		return wi_time_3_32x32;
	}
	if (iconName == "wi_time_2")
	{
		return wi_time_2_32x32;
	}
	if (iconName == "wi_cloud_refresh")
	{
		return wi_cloud_refresh_32x32;
	}
	if (iconName == "wi_time_9")
	{
		return wi_time_9_32x32;
	}
	if (iconName == "error_icon")
	{
		return error_icon_32x32;
	}
	if (iconName == "wi_cloud_down")
	{
		return wi_cloud_down_32x32;
	}
	if (iconName == "wifi")
	{
		return wifi_32x32;
	}
	if (iconName == "battery_3_bar_90deg")
	{
		return battery_3_bar_90deg_32x32;
	}
	if (iconName == "battery_alert_90deg")
	{
		return battery_alert_90deg_32x32;
	}
	if (iconName == "wifi_2_bar")
	{
		return wifi_2_bar_32x32;
	}
	if (iconName == "wi_time_1")
	{
		return wi_time_1_32x32;
	}

	Serial.printf("[error]: Icon '%s' is not available in this firmware", iconName.c_str());
	return NULL;
}
const uint8_t *getIcon48(String iconName)
{
	if (iconName == "battery_0_bar_90deg")
	{
		return battery_0_bar_90deg_48x48;
	}
	if (iconName == "calendar")
	{
		return calendar_48x48;
	}
	if (iconName == "battery_full_90deg")
	{
		return battery_full_90deg_48x48;
	}
	if (iconName == "battery_6_bar_90deg")
	{
		return battery_6_bar_90deg_48x48;
	}
	if (iconName == "wifi_off")
	{
		return wifi_off_48x48;
	}
	if (iconName == "wi_time_12")
	{
		return wi_time_12_48x48;
	}
	if (iconName == "battery_4_bar_90deg")
	{
		return battery_4_bar_90deg_48x48;
	}
	if (iconName == "wi_time_4")
	{
		return wi_time_4_48x48;
	}
	if (iconName == "wifi_1_bar")
	{
		return wifi_1_bar_48x48;
	}
	if (iconName == "wi_time_11")
	{
		return wi_time_11_48x48;
	}
	if (iconName == "biological_hazard_symbol")
	{
		return biological_hazard_symbol_48x48;
	}
	if (iconName == "battery_1_bar_90deg")
	{
		return battery_1_bar_90deg_48x48;
	}
	if (iconName == "wi_fire")
	{
		return wi_fire_48x48;
	}
	if (iconName == "wi_cloud_up")
	{
		return wi_cloud_up_48x48;
	}
	if (iconName == "x_symbol")
	{
		return x_symbol_48x48;
	}
	if (iconName == "wi_small_craft_advisory")
	{
		return wi_small_craft_advisory_48x48;
	}
	if (iconName == "wi_time_5")
	{
		return wi_time_5_48x48;
	}
	if (iconName == "wi_cloud")
	{
		return wi_cloud_48x48;
	}
	if (iconName == "battery_5_bar_90deg")
	{
		return battery_5_bar_90deg_48x48;
	}
	if (iconName == "wifi_x")
	{
		return wifi_x_48x48;
	}
	if (iconName == "wi_alien")
	{
		return wi_alien_48x48;
	}
	if (iconName == "wi_time_7")
	{
		return wi_time_7_48x48;
	}
	if (iconName == "battery_2_bar_90deg")
	{
		return battery_2_bar_90deg_48x48;
	}
	if (iconName == "battery_charging_full_90deg")
	{
		return battery_charging_full_90deg_48x48;
	}
	if (iconName == "wi_time_8")
	{
		return wi_time_8_48x48;
	}
	if (iconName == "wi_time_10")
	{
		return wi_time_10_48x48;
	}
	if (iconName == "wifi_3_bar")
	{
		return wifi_3_bar_48x48;
	}
	if (iconName == "warning_icon")
	{
		return warning_icon_48x48;
	}
	if (iconName == "wi_refresh")
	{
		return wi_refresh_48x48;
	}
	if (iconName == "wi_time_6")
	{
		return wi_time_6_48x48;
	}
	if (iconName == "wi_na")
	{
		return wi_na_48x48;
	}
	if (iconName == "wi_time_3")
	{
		return wi_time_3_48x48;
	}
	if (iconName == "wi_time_2")
	{
		return wi_time_2_48x48;
	}
	if (iconName == "wi_cloud_refresh")
	{
		return wi_cloud_refresh_48x48;
	}
	if (iconName == "wi_time_9")
	{
		return wi_time_9_48x48;
	}
	if (iconName == "error_icon")
	{
		return error_icon_48x48;
	}
	if (iconName == "wi_cloud_down")
	{
		return wi_cloud_down_48x48;
	}
	if (iconName == "wifi")
	{
		return wifi_48x48;
	}
	if (iconName == "battery_3_bar_90deg")
	{
		return battery_3_bar_90deg_48x48;
	}
	if (iconName == "battery_alert_90deg")
	{
		return battery_alert_90deg_48x48;
	}
	if (iconName == "wifi_2_bar")
	{
		return wifi_2_bar_48x48;
	}
	if (iconName == "wi_time_1")
	{
		return wi_time_1_48x48;
	}

	Serial.printf("[error]: Icon '%s' is not available in this firmware", iconName.c_str());
	return NULL;
}
const uint8_t *getIcon128(String iconName)
{
	if (iconName == "battery_0_bar_90deg")
	{
		return battery_0_bar_90deg_128x128;
	}
	if (iconName == "calendar")
	{
		return calendar_128x128;
	}
	if (iconName == "battery_full_90deg")
	{
		return battery_full_90deg_128x128;
	}
	if (iconName == "battery_6_bar_90deg")
	{
		return battery_6_bar_90deg_128x128;
	}
	if (iconName == "wifi_off")
	{
		return wifi_off_128x128;
	}
	if (iconName == "wi_time_12")
	{
		return wi_time_12_128x128;
	}
	if (iconName == "battery_4_bar_90deg")
	{
		return battery_4_bar_90deg_128x128;
	}
	if (iconName == "wi_time_4")
	{
		return wi_time_4_128x128;
	}
	if (iconName == "wifi_1_bar")
	{
		return wifi_1_bar_128x128;
	}
	if (iconName == "wi_time_11")
	{
		return wi_time_11_128x128;
	}
	if (iconName == "biological_hazard_symbol")
	{
		return biological_hazard_symbol_128x128;
	}
	if (iconName == "battery_1_bar_90deg")
	{
		return battery_1_bar_90deg_128x128;
	}
	if (iconName == "wi_fire")
	{
		return wi_fire_128x128;
	}
	if (iconName == "wi_cloud_up")
	{
		return wi_cloud_up_128x128;
	}
	if (iconName == "x_symbol")
	{
		return x_symbol_128x128;
	}
	if (iconName == "wi_small_craft_advisory")
	{
		return wi_small_craft_advisory_128x128;
	}
	if (iconName == "wi_time_5")
	{
		return wi_time_5_128x128;
	}
	if (iconName == "wi_cloud")
	{
		return wi_cloud_128x128;
	}
	if (iconName == "battery_5_bar_90deg")
	{
		return battery_5_bar_90deg_128x128;
	}
	if (iconName == "wifi_x")
	{
		return wifi_x_128x128;
	}
	if (iconName == "wi_alien")
	{
		return wi_alien_128x128;
	}
	if (iconName == "wi_time_7")
	{
		return wi_time_7_128x128;
	}
	if (iconName == "battery_2_bar_90deg")
	{
		return battery_2_bar_90deg_128x128;
	}
	if (iconName == "battery_charging_full_90deg")
	{
		return battery_charging_full_90deg_128x128;
	}
	if (iconName == "wi_time_8")
	{
		return wi_time_8_128x128;
	}
	if (iconName == "wi_time_10")
	{
		return wi_time_10_128x128;
	}
	if (iconName == "wifi_3_bar")
	{
		return wifi_3_bar_128x128;
	}
	if (iconName == "warning_icon")
	{
		return warning_icon_128x128;
	}
	if (iconName == "wi_refresh")
	{
		return wi_refresh_128x128;
	}
	if (iconName == "wi_time_6")
	{
		return wi_time_6_128x128;
	}
	if (iconName == "wi_na")
	{
		return wi_na_128x128;
	}
	if (iconName == "wi_time_3")
	{
		return wi_time_3_128x128;
	}
	if (iconName == "wi_time_2")
	{
		return wi_time_2_128x128;
	}
	if (iconName == "wi_cloud_refresh")
	{
		return wi_cloud_refresh_128x128;
	}
	if (iconName == "wi_time_9")
	{
		return wi_time_9_128x128;
	}
	if (iconName == "error_icon")
	{
		return error_icon_128x128;
	}
	if (iconName == "wi_cloud_down")
	{
		return wi_cloud_down_128x128;
	}
	if (iconName == "wifi")
	{
		return wifi_128x128;
	}
	if (iconName == "battery_3_bar_90deg")
	{
		return battery_3_bar_90deg_128x128;
	}
	if (iconName == "battery_alert_90deg")
	{
		return battery_alert_90deg_128x128;
	}
	if (iconName == "wifi_2_bar")
	{
		return wifi_2_bar_128x128;
	}
	if (iconName == "wi_time_1")
	{
		return wi_time_1_128x128;
	}

	Serial.printf("[error]: Icon '%s' is not available in this firmware", iconName.c_str());
	return NULL;
}
const uint8_t *getIcon196(String iconName)
{
	if (iconName == "battery_0_bar_90deg")
	{
		return battery_0_bar_90deg_196x196;
	}
	if (iconName == "calendar")
	{
		return calendar_196x196;
	}
	if (iconName == "battery_full_90deg")
	{
		return battery_full_90deg_196x196;
	}
	if (iconName == "battery_6_bar_90deg")
	{
		return battery_6_bar_90deg_196x196;
	}
	if (iconName == "wifi_off")
	{
		return wifi_off_196x196;
	}
	if (iconName == "wi_time_12")
	{
		return wi_time_12_196x196;
	}
	if (iconName == "battery_4_bar_90deg")
	{
		return battery_4_bar_90deg_196x196;
	}
	if (iconName == "wi_time_4")
	{
		return wi_time_4_196x196;
	}
	if (iconName == "wifi_1_bar")
	{
		return wifi_1_bar_196x196;
	}
	if (iconName == "wi_time_11")
	{
		return wi_time_11_196x196;
	}
	if (iconName == "biological_hazard_symbol")
	{
		return biological_hazard_symbol_196x196;
	}
	if (iconName == "battery_1_bar_90deg")
	{
		return battery_1_bar_90deg_196x196;
	}
	if (iconName == "wi_fire")
	{
		return wi_fire_196x196;
	}
	if (iconName == "wi_cloud_up")
	{
		return wi_cloud_up_196x196;
	}
	if (iconName == "x_symbol")
	{
		return x_symbol_196x196;
	}
	if (iconName == "wi_small_craft_advisory")
	{
		return wi_small_craft_advisory_196x196;
	}
	if (iconName == "wi_time_5")
	{
		return wi_time_5_196x196;
	}
	if (iconName == "wi_cloud")
	{
		return wi_cloud_196x196;
	}
	if (iconName == "battery_5_bar_90deg")
	{
		return battery_5_bar_90deg_196x196;
	}
	if (iconName == "wifi_x")
	{
		return wifi_x_196x196;
	}
	if (iconName == "wi_alien")
	{
		return wi_alien_196x196;
	}
	if (iconName == "wi_time_7")
	{
		return wi_time_7_196x196;
	}
	if (iconName == "battery_2_bar_90deg")
	{
		return battery_2_bar_90deg_196x196;
	}
	if (iconName == "battery_charging_full_90deg")
	{
		return battery_charging_full_90deg_196x196;
	}
	if (iconName == "wi_time_8")
	{
		return wi_time_8_196x196;
	}
	if (iconName == "wi_time_10")
	{
		return wi_time_10_196x196;
	}
	if (iconName == "wifi_3_bar")
	{
		return wifi_3_bar_196x196;
	}
	if (iconName == "warning_icon")
	{
		return warning_icon_196x196;
	}
	if (iconName == "wi_refresh")
	{
		return wi_refresh_196x196;
	}
	if (iconName == "wi_time_6")
	{
		return wi_time_6_196x196;
	}
	if (iconName == "wi_na")
	{
		return wi_na_196x196;
	}
	if (iconName == "wi_time_3")
	{
		return wi_time_3_196x196;
	}
	if (iconName == "wi_time_2")
	{
		return wi_time_2_196x196;
	}
	if (iconName == "wi_cloud_refresh")
	{
		return wi_cloud_refresh_196x196;
	}
	if (iconName == "wi_time_9")
	{
		return wi_time_9_196x196;
	}
	if (iconName == "error_icon")
	{
		return error_icon_196x196;
	}
	if (iconName == "wi_cloud_down")
	{
		return wi_cloud_down_196x196;
	}
	if (iconName == "wifi")
	{
		return wifi_196x196;
	}
	if (iconName == "battery_3_bar_90deg")
	{
		return battery_3_bar_90deg_196x196;
	}
	if (iconName == "battery_alert_90deg")
	{
		return battery_alert_90deg_196x196;
	}
	if (iconName == "wifi_2_bar")
	{
		return wifi_2_bar_196x196;
	}
	if (iconName == "wi_time_1")
	{
		return wi_time_1_196x196;
	}

	Serial.printf("[error]: Icon '%s' is not available in this firmware", iconName.c_str());
	return NULL;
}