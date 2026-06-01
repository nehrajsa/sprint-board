#include "sprint_config.h"
#include "constants.h"
#include <ArduinoJson.h>

#ifdef ENABLE_STORAGE
#include <Preferences.h>
#endif

SprintConfig sprintConfig;

void SprintConfig::load()
{
#ifdef ENABLE_STORAGE
  Preferences prefs;
  if (!prefs.begin("sprint", true))
    return;

  String datesJson = prefs.getString("dates", "[]");
  milestoneDate = prefs.getString("milestone", "");
  prefs.end();

  sprintDates.clear();

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, datesJson);
  if (!error && doc.is<JsonArray>())
  {
    for (JsonVariant v : doc.as<JsonArray>())
    {
      String d = v.as<String>();
      if (d.length() == 10)
        sprintDates.push_back(d);
    }
  }
#endif
}

void SprintConfig::save() const
{
#ifdef ENABLE_STORAGE
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (const auto &d : sprintDates)
    arr.add(d);

  String datesJson;
  serializeJson(doc, datesJson);

  Preferences prefs;
  if (!prefs.begin("sprint", false))
    return;
  prefs.putString("dates", datesJson);
  prefs.putString("milestone", milestoneDate);
  prefs.end();
#endif
}

String SprintConfig::toJson() const
{
  JsonDocument doc;
  JsonArray arr = doc["sprintDates"].to<JsonArray>();
  for (const auto &d : sprintDates)
    arr.add(d);
  doc["milestoneDate"] = milestoneDate;

  String out;
  serializeJson(doc, out);
  return out;
}

bool SprintConfig::fromJson(const String &json)
{
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error)
    return false;

  sprintDates.clear();
  if (doc["sprintDates"].is<JsonArray>())
  {
    for (JsonVariant v : doc["sprintDates"].as<JsonArray>())
    {
      String d = v.as<String>();
      if (d.length() == 10)
        sprintDates.push_back(d);
    }
  }

  if (doc["milestoneDate"].is<String>())
  {
    String m = doc["milestoneDate"].as<String>();
    milestoneDate = (m.length() == 10) ? m : "";
  }
  else
  {
    milestoneDate = "";
  }

  return true;
}
