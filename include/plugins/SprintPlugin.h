#pragma once
#include "PluginManager.h"
#include "sprint_config.h"
#include "timing.h"

class SprintPlugin : public Plugin
{
private:
  NonBlockingDelay timer;
  int lastSprintDays = -999;
  int lastMilestoneDays = -999;

  int computeDaysLeft(const String &dateStr) const;
  int nearestSprintDays() const;
  int milestoneDays() const;
  void drawNumber(int n, int yOffset);
  void redraw();

public:
  void setup() override;
  void loop() override;
  void teardown() override;
  const char *getName() const override;
};
