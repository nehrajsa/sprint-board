#include "plugins/SprintPlugin.h"

void SprintPlugin::setup()
{
  Screen.clear();
  lastSprintDays = -999;
  lastMilestoneDays = -999;
  timer.forceReady();
}

int SprintPlugin::computeDaysLeft(const String &dateStr) const
{
  if (dateStr.length() < 10)
    return -1;

  struct tm nowTm;
  if (!getLocalTime(&nowTm, 0))
    return -1; // NTP not synced yet (non-blocking check)

  struct tm today = nowTm;
  today.tm_hour = 0;
  today.tm_min = 0;
  today.tm_sec = 0;
  today.tm_isdst = -1;
  time_t todayT = mktime(&today);

  struct tm target = {};
  target.tm_year = dateStr.substring(0, 4).toInt() - 1900;
  target.tm_mon = dateStr.substring(5, 7).toInt() - 1;
  target.tm_mday = dateStr.substring(8, 10).toInt();
  target.tm_isdst = -1;
  time_t targetT = mktime(&target);

  return (int)((targetT - todayT) / 86400L);
}

int SprintPlugin::nearestSprintDays() const
{
  if (sprintConfig.sprintDates.empty())
    return -1;

  int minFuture = 9999;
  bool hasFuture = false;

  for (const auto &date : sprintConfig.sprintDates)
  {
    int days = computeDaysLeft(date);
    if (days == -1)
      return -1; // NTP not synced
    if (days >= 0 && days < minFuture)
    {
      minFuture = days;
      hasFuture = true;
    }
  }

  return hasFuture ? minFuture : 0;
}

int SprintPlugin::milestoneDays() const
{
  if (sprintConfig.milestoneDate.length() < 10)
    return -1;
  int d = computeDaysLeft(sprintConfig.milestoneDate);
  if (d == -1)
    return -1; // NTP not synced
  return (d >= 0) ? d : 0;
}

void SprintPlugin::drawNumber(int n, int yOffset)
{
  if (n < 0)
    return;
  if (n < 10)
  {
    Screen.drawBigNumbers(4, yOffset, {n});
  }
  else if (n < 100)
  {
    Screen.drawBigNumbers(0, yOffset, {n / 10, n % 10});
  }
  else
  {
    int cap = min(n, 999);
    Screen.drawNumbers(1, yOffset, {cap / 100, (cap % 100) / 10, cap % 10});
  }
}

void SprintPlugin::redraw()
{
  Screen.clear();

  // If dates are configured but NTP hasn't synced yet, show 3 dim dots
  struct tm dummy;
  bool ntpSynced = getLocalTime(&dummy, 0);
  bool hasDates = !sprintConfig.sprintDates.empty() ||
                  sprintConfig.milestoneDate.length() >= 10;

  if (hasDates && !ntpSynced)
  {
    Screen.setPixel(5, 7, 1, 80);
    Screen.setPixel(8, 7, 1, 80);
    Screen.setPixel(11, 7, 1, 80);
    return;
  }

  drawNumber(lastSprintDays, 0);
  drawNumber(lastMilestoneDays, 9);
}

void SprintPlugin::loop()
{
  if (!timer.isReady(1000))
    return;

  int sprint = nearestSprintDays();
  int milestone = milestoneDays();

  if (sprint != lastSprintDays || milestone != lastMilestoneDays)
  {
    lastSprintDays = sprint;
    lastMilestoneDays = milestone;
    redraw();
  }
}

void SprintPlugin::teardown()
{
  Screen.clear();
}

const char *SprintPlugin::getName() const
{
  return "Sprint";
}
