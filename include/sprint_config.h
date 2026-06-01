#pragma once
#include <Arduino.h>
#include <vector>

class SprintConfig
{
public:
  std::vector<String> sprintDates;
  String milestoneDate;

  void load();
  void save() const;
  String toJson() const;
  bool fromJson(const String &json);
};

extern SprintConfig sprintConfig;
