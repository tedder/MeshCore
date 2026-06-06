#pragma once

#include <helpers/ui/DisplayDriver.h>
#include <helpers/CommonCLI.h>
#include <helpers/SensorManager.h>

#if ENV_INCLUDE_GPS
#include <helpers/sensors/LocationProvider.h>
#endif

struct UIStats {
  int16_t  last_rssi;
  int16_t  last_snr_x4;   // raw SNR * 4; divide by 4 for dB
  int16_t  noise_floor;
  uint32_t n_recv;
  uint32_t n_sent;
  uint32_t n_dups;
};

class UITask {
  DisplayDriver* _display;
  unsigned long _next_read, _next_refresh, _auto_off;
  int _prevBtnState;
  unsigned long _first_press_at = 0;
  uint8_t _press_count = 0;
  unsigned long _advert_until = 0;
  void (*_on_long_press)() = nullptr;
  NodePrefs* _node_prefs;
  char _version_info[32];
  SensorManager* _sensors = nullptr;
  UIStats* _stats = nullptr;
#if ENV_INCLUDE_GPS
  LocationProvider* _gps = nullptr;
#endif

  void renderCurrScreen();
public:
  UITask(DisplayDriver& display) : _display(&display) { _next_read = _next_refresh = 0; }
  void begin(NodePrefs* node_prefs, const char* build_date, const char* firmware_version);
  void setSensors(SensorManager* s) { _sensors = s; }
  void setStats(UIStats* s) { _stats = s; }
  void setLongPressCallback(void (*cb)()) { _on_long_press = cb; }
#if ENV_INCLUDE_GPS
  void setGPS(LocationProvider* gps) { _gps = gps; }
#endif

  void loop();
};