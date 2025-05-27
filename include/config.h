#pragma once

// WIFI
#define WIFI_SSID "<your_wifi_ssid>"
#define WIFI_PASSWORD "<your_wifi_password>"

// API
#define API_ENDPOINT "<your_api_endpoint>" // e.g. "homeassistant.local"
#define API_ENDPOINT_PORT 8099
#define API_ENDPOINT_FETCH_CALENDAR "<your_calendar_name>" // e.g. "work" or "home" or "all" to fetch all calendars

// Sleep duration in minutes. (aka how often esp32 will wake for an update)
#define SLEEP_DURATION 15 // minutes

// Display Strings
// This is usefull for localization and translation.
// If you want to change the language, you can do so by changing the
// TXT_... defines below.
#define TXT_PAST_EVENTS "past events"
#define TXT_UPCOMING_EVENTS "upcoming events"
#define TXT_UNTIL "until"
#define TXT_FREE "free"

// E-PAPER PANEL
// This project supports the following E-Paper panels:
//   DISP_BW  - 7.5in e-Paper (v2)      800x480px  Black/White
//   DISP_3C  - 7.5in e-Paper (B)       800x480px  Red/Black/White
//   DISP_7C  - 7.3in ACeP e-Paper (F)  800x480px  7-Color
// Uncomment the macro that identifies your physical panel.
#define DISP_BW
// #define DISP_3C
// #define DISP_7C

// DEBUG
//   If defined, enables increase verbosity over the serial port.
//   level 0: basic status information, assists troubleshooting (default)
//   level 1: increased verbosity for debugging
//   level 2: print api responses and verbose info to serial monitor
//   level 3: print display_buffer drawings (very noisy)
//   level 4: draw boundaries on draws
#define DEBUG_LEVEL 1

// TIME
// For list of time zones see
// https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"

// Time format used when displaying sunrise/set times. (Max 11 characters)
// For more information about formatting see
// https://man7.org/linux/man-pages/man3/strftime.3.html
#define TIME_FORMAT "%H:%M" // 24-hour ex: 01:23   23:00

// Time format used when displaying axis labels. (Max 11 characters)
// For more information about formatting see
// https://man7.org/linux/man-pages/man3/strftime.3.html
#define HOUR_FORMAT "%H" // 24-hour ex: 01   23

// Date format used when displaying date in top-right corner.
// For more information about formatting see
// https://man7.org/linux/man-pages/man3/strftime.3.html
#define DATE_FORMAT "%a, %B %e" // ex: Sat, January 1

// Date/Time format used when displaying the last refresh time along the bottom
// of the screen.
// For more information about formatting see
// https://man7.org/linux/man-pages/man3/strftime.3.html
#define REFRESH_TIME_FORMAT "%e.%m.%Y %H:%M"

// NON-VOLATILE STORAGE (NVS) NAMESPACE
#define NVS_NAMESPACE "meeting_epd"

#ifdef DISP_BW
#define INVERT_AS_ACCENT true
#else
#define INVERT_AS_ACCENT false
#define ACCENT_COLOR Color::Red
#endif

// PINS
// The configuration below is intended for use with the project's official
// wiring diagrams using the FireBeetle 2 ESP32-E microcontroller board.
//
// Note: LED_BUILTIN pin will be disabled to reduce power draw.  Refer to your
//       board's pinout to ensure you avoid using a pin with this shared
//       functionality.
//
// ADC pin used to measure battery voltage
#define PIN_BAT_ADC A2 // A0 for micro-usb firebeetle

// Pins for E-Paper Driver Board
#define PIN_EPD_BUSY 14 // 5 for micro-usb firebeetle
#define PIN_EPD_CS 13
#define PIN_EPD_RST 21
#define PIN_EPD_DC 22
#define PIN_EPD_SCK 18
#define PIN_EPD_MISO 19 // 19 Master-In Slave-Out not used, as no data from display
#define PIN_EPD_MOSI 23
#define PIN_EPD_PWR 26 // Irrelevant if directly connected to 3.3V

// HTTP
// The following errors are likely the result of insuffient http client tcp timeout:
//   -1   Connection Refused
//   -11  Read Timeout
//   -258 Deserialization Incomplete Input
#define HTTP_CLIENT_TCP_TIMEOUT 1000 // ms

// ms, WiFi connection timeout.
#define WIFI_TIMEOUT 10000
// Number of times to retry connecting to WiFi or fetching the API before giving up.
#define RETRY_COUNT 10

// NTP_SERVER_1 is the primary time server, while NTP_SERVER_2 is a fallback.
// pool.ntp.org will find the closest available NTP server to you.
#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.nist.gov"

// If you encounter the 'Failed To Fetch The Time' error, try increasing
// NTP_TIMEOUT or select closer/lower latency time servers.
#define NTP_TIMEOUT 20000 // ms

// BATTERY
// To protect the battery upon LOW_BATTERY_VOLTAGE, the display will cease to
// update until battery is charged again. The ESP32 will deep-sleep (consuming
// < 11μA), waking briefly check the voltage at the corresponding interval (in
// minutes). Once the battery voltage has fallen to CRIT_LOW_BATTERY_VOLTAGE,
// the esp32 will hibernate and a manual press of the reset (RST) button to
// begin operating again.
#define LOW_BATTERY_VOLTAGE 3462            // (millivolts) ~10%
#define VERY_LOW_BATTERY_VOLTAGE 3442       // (millivolts)  ~8%
#define CRIT_LOW_BATTERY_VOLTAGE 3404       // (millivolts)  ~5%
#define LOW_BATTERY_SLEEP_INTERVAL 30       // (minutes)
#define VERY_LOW_BATTERY_SLEEP_INTERVAL 120 // (minutes)

// Battery voltage calculations are based on a typical 3.7v LiPo.
#define MAX_BATTERY_VOLTAGE 4200 // (millivolts)
#define MIN_BATTERY_VOLTAGE 3000 // (millivolts)

// HTTP CLIENT ERRORS
#define TXT_HTTPC_ERROR_CONNECTION_REFUSED "Connection Refused";
#define TXT_HTTPC_ERROR_SEND_HEADER_FAILED "Send Header Failed";
#define TXT_HTTPC_ERROR_SEND_PAYLOAD_FAILED "Send Payload Failed";
#define TXT_HTTPC_ERROR_NOT_CONNECTED "Not Connected";
#define TXT_HTTPC_ERROR_CONNECTION_LOST "Connection Lost";
#define TXT_HTTPC_ERROR_NO_STREAM "No Stream";
#define TXT_HTTPC_ERROR_NO_HTTP_SERVER "No HTTP Server";
#define TXT_HTTPC_ERROR_TOO_LESS_RAM "Too Less Ram";
#define TXT_HTTPC_ERROR_ENCODING "Transfer-Encoding Not Supported";
#define TXT_HTTPC_ERROR_STREAM_WRITE "Stream Write Error";
#define TXT_HTTPC_ERROR_READ_TIMEOUT "Read Timeout";

// HTTP RESPONSE STATUS CODES
// 1xx - Informational Responses
#define TXT_HTTP_RESPONSE_100 "Continue"
#define TXT_HTTP_RESPONSE_101 "Switching Protocols"
#define TXT_HTTP_RESPONSE_102 "Processing"
#define TXT_HTTP_RESPONSE_103 "Early Hints"
// 2xx - Successful Responses
#define TXT_HTTP_RESPONSE_200 "OK"
#define TXT_HTTP_RESPONSE_201 "Created"
#define TXT_HTTP_RESPONSE_202 "Accepted"
#define TXT_HTTP_RESPONSE_203 "Non-Authoritative Information"
#define TXT_HTTP_RESPONSE_204 "No Content"
#define TXT_HTTP_RESPONSE_205 "Reset Content"
#define TXT_HTTP_RESPONSE_206 "Partial Content"
#define TXT_HTTP_RESPONSE_207 "Multi-Status"
#define TXT_HTTP_RESPONSE_208 "Already Reported"
#define TXT_HTTP_RESPONSE_226 "IM Used"
// 3xx - Redirection Responses
#define TXT_HTTP_RESPONSE_300 "Multiple Choices"
#define TXT_HTTP_RESPONSE_301 "Moved Permanently"
#define TXT_HTTP_RESPONSE_302 "Found"
#define TXT_HTTP_RESPONSE_303 "See Other"
#define TXT_HTTP_RESPONSE_304 "Not Modified"
#define TXT_HTTP_RESPONSE_305 "Use Proxy"
#define TXT_HTTP_RESPONSE_307 "Temporary Redirect"
#define TXT_HTTP_RESPONSE_308 "Permanent Redirect"
// 4xx - Client Error Responses
#define TXT_HTTP_RESPONSE_400 "Bad Request"
#define TXT_HTTP_RESPONSE_401 "Unauthorized"
#define TXT_HTTP_RESPONSE_402 "Payment Required"
#define TXT_HTTP_RESPONSE_403 "Forbidden"
#define TXT_HTTP_RESPONSE_404 "Not Found"
#define TXT_HTTP_RESPONSE_405 "Method Not Allowed"
#define TXT_HTTP_RESPONSE_406 "Not Acceptable"
#define TXT_HTTP_RESPONSE_407 "Proxy Authentication Required"
#define TXT_HTTP_RESPONSE_408 "Request Timeout"
#define TXT_HTTP_RESPONSE_409 "Conflict"
#define TXT_HTTP_RESPONSE_410 "Gone"
#define TXT_HTTP_RESPONSE_411 "Length Required"
#define TXT_HTTP_RESPONSE_412 "Precondition Failed"
#define TXT_HTTP_RESPONSE_413 "Content Too Large"
#define TXT_HTTP_RESPONSE_414 "URI Too Long"
#define TXT_HTTP_RESPONSE_415 "Unsupported Media Type"
#define TXT_HTTP_RESPONSE_416 "Range Not Satisfiable"
#define TXT_HTTP_RESPONSE_417 "Expectation Failed"
#define TXT_HTTP_RESPONSE_418 "I'm a teapot"
#define TXT_HTTP_RESPONSE_421 "Misdirected Request"
#define TXT_HTTP_RESPONSE_422 "Unprocessable Content"
#define TXT_HTTP_RESPONSE_423 "Locked"
#define TXT_HTTP_RESPONSE_424 "Failed Dependency"
#define TXT_HTTP_RESPONSE_425 "Too Early"
#define TXT_HTTP_RESPONSE_426 "Upgrade Required"
#define TXT_HTTP_RESPONSE_428 "Precondition Required"
#define TXT_HTTP_RESPONSE_429 "Too Many Requests"
#define TXT_HTTP_RESPONSE_431 "Request Header Fields Too Large"
#define TXT_HTTP_RESPONSE_451 "Unavailable For Legal Reasons"
// 5xx - Server Error Responses
#define TXT_HTTP_RESPONSE_500 "Internal Server Error"
#define TXT_HTTP_RESPONSE_501 "Not Implemented"
#define TXT_HTTP_RESPONSE_502 "Bad Gateway"
#define TXT_HTTP_RESPONSE_503 "Service Unavailable"
#define TXT_HTTP_RESPONSE_504 "Gateway Timeout"
#define TXT_HTTP_RESPONSE_505 "HTTP Version Not Supported"
#define TXT_HTTP_RESPONSE_506 "Variant Also Negotiates"
#define TXT_HTTP_RESPONSE_507 "Insufficient Storage"
#define TXT_HTTP_RESPONSE_508 "Loop Detected"
#define TXT_HTTP_RESPONSE_510 "Not Extended"
#define TXT_HTTP_RESPONSE_511 "Network Authentication Required"

// ARDUINOJSON DESERIALIZATION ERROR CODES
#define TXT_DESERIALIZATION_ERROR_OK "Deserialization OK"
#define TXT_DESERIALIZATION_ERROR_EMPTY_INPUT "Deserialization Empty Input"
#define TXT_DESERIALIZATION_ERROR_INCOMPLETE_INPUT "Deserialization Incomplete Input"
#define TXT_DESERIALIZATION_ERROR_INVALID_INPUT "Deserialization Invalid Input"
#define TXT_DESERIALIZATION_ERROR_NO_MEMORY "Deserialization No Memory"
#define TXT_DESERIALIZATION_ERROR_TOO_DEEP "Deserialization Too Deep"

// WIFI STATUS
#define TXT_WL_NO_SHIELD "No Shield"
#define TXT_WL_IDLE_STATUS "Idle"
#define TXT_WL_NO_SSID_AVAIL "No SSID Available"
#define TXT_WL_SCAN_COMPLETED "Scan Complete"
#define TXT_WL_CONNECTED "Connected"
#define TXT_WL_CONNECT_FAILED "Connection Failed"
#define TXT_WL_CONNECTION_LOST "Connection Lost"
#define TXT_WL_DISCONNECTED "Disconnected"
