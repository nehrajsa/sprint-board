#include "constants.h"
#ifdef ENABLE_SERVER
#include "http_time_sync.h"
#include "config.h"
#include "secrets.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <sys/time.h>

bool syncTimeFromHTTP()
{
  String tz = config.getIanaTimezone();
  String url = "https://world-time-api3.p.rapidapi.com/timezone/" + tz;

  Serial.printf("[Time] HTTP fallback: GET %s\n", url.c_str());

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, url);
  http.setTimeout(8000);
  http.addHeader("X-RapidAPI-Key", RAPIDAPI_KEY);
  http.addHeader("X-RapidAPI-Host", "world-time-api3.p.rapidapi.com");

  int code = http.GET();
  if (code != 200)
  {
    Serial.printf("[Time] HTTP fallback failed: HTTP %d\n", code);
    http.end();
    return false;
  }

  String body = http.getString();
  http.end();

  JsonDocument doc;
  if (deserializeJson(doc, body) != DeserializationError::Ok)
  {
    Serial.println("[Time] HTTP fallback: JSON parse error");
    return false;
  }

  long unixtime = doc["unixtime"].as<long>();
  if (unixtime < 1577836800L)
  {
    Serial.printf("[Time] HTTP fallback: implausible timestamp %ld\n", unixtime);
    return false;
  }

  struct timeval tv = {(time_t)unixtime, 0};
  settimeofday(&tv, nullptr);
  setenv("TZ", config.getTzInfo().c_str(), 1);
  tzset();
  Serial.printf("[Time] HTTP fallback synced (%s): %ld\n", tz.c_str(), unixtime);
  return true;
}
#endif
