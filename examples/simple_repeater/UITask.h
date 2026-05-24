#pragma once

#include <helpers/ui/DisplayDriver.h>
#include <helpers/CommonCLI.h>
#include <helpers/SensorManager.h>

#if ENV_INCLUDE_GPS
#include <helpers/sensors/LocationProvider.h>
#endif

class UITask {
  DisplayDriver* _display;
  unsigned long _next_read, _next_refresh, _auto_off;
  int _prevBtnState;
  NodePrefs* _node_prefs;
  char _version_info[32];
  SensorManager* _sensors = nullptr;
#if ENV_INCLUDE_GPS
  LocationProvider* _gps = nullptr;
#endif

  void renderCurrScreen();
public:
  UITask(DisplayDriver& display) : _display(&display) { _next_read = _next_refresh = 0; }
  void begin(NodePrefs* node_prefs, const char* build_date, const char* firmware_version);
  void setSensors(SensorManager* s) { _sensors = s; }
#if ENV_INCLUDE_GPS
  void setGPS(LocationProvider* gps) { _gps = gps; }
#endif

  void loop();
};