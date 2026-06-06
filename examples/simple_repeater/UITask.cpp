#include "UITask.h"
#include <Arduino.h>
#include <helpers/CommonCLI.h>

#ifndef USER_BTN_PRESSED
#define USER_BTN_PRESSED LOW
#endif

#define AUTO_OFF_MILLIS      20000  // 20 seconds
#define BOOT_SCREEN_MILLIS   4000   // 4 seconds

// 4x4 open-ring degree symbol (XBM, LSB-first per row)
static const uint8_t degree_xbm[] PROGMEM = {0x06, 0x09, 0x09, 0x06};

// 6x8 satellite dish symbol (XBM, LSB-first per row)
static const uint8_t sat_xbm[] PROGMEM = {0x1E, 0x12, 0x3F, 0x3F, 0x3F, 0x3F, 0x12, 0x1E};

// 'meshcore', 128x13px
static const uint8_t meshcore_logo [] PROGMEM = {
    0x3c, 0x01, 0xe3, 0xff, 0xc7, 0xff, 0x8f, 0x03, 0x87, 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 
    0x3c, 0x03, 0xe3, 0xff, 0xc7, 0xff, 0x8e, 0x03, 0x8f, 0xfe, 0x3f, 0xfe, 0x1f, 0xff, 0x1f, 0xfe, 
    0x3e, 0x03, 0xc3, 0xff, 0x8f, 0xff, 0x0e, 0x07, 0x8f, 0xfe, 0x7f, 0xfe, 0x1f, 0xff, 0x1f, 0xfc, 
    0x3e, 0x07, 0xc7, 0x80, 0x0e, 0x00, 0x0e, 0x07, 0x9e, 0x00, 0x78, 0x0e, 0x3c, 0x0f, 0x1c, 0x00, 
    0x3e, 0x0f, 0xc7, 0x80, 0x1e, 0x00, 0x0e, 0x07, 0x1e, 0x00, 0x70, 0x0e, 0x38, 0x0f, 0x3c, 0x00, 
    0x7f, 0x0f, 0xc7, 0xfe, 0x1f, 0xfc, 0x1f, 0xff, 0x1c, 0x00, 0x70, 0x0e, 0x38, 0x0e, 0x3f, 0xf8, 
    0x7f, 0x1f, 0xc7, 0xfe, 0x0f, 0xff, 0x1f, 0xff, 0x1c, 0x00, 0xf0, 0x0e, 0x38, 0x0e, 0x3f, 0xf8, 
    0x7f, 0x3f, 0xc7, 0xfe, 0x0f, 0xff, 0x1f, 0xff, 0x1c, 0x00, 0xf0, 0x1e, 0x3f, 0xfe, 0x3f, 0xf0, 
    0x77, 0x3b, 0x87, 0x00, 0x00, 0x07, 0x1c, 0x0f, 0x3c, 0x00, 0xe0, 0x1c, 0x7f, 0xfc, 0x38, 0x00, 
    0x77, 0xfb, 0x8f, 0x00, 0x00, 0x07, 0x1c, 0x0f, 0x3c, 0x00, 0xe0, 0x1c, 0x7f, 0xf8, 0x38, 0x00, 
    0x73, 0xf3, 0x8f, 0xff, 0x0f, 0xff, 0x1c, 0x0e, 0x3f, 0xf8, 0xff, 0xfc, 0x70, 0x78, 0x7f, 0xf8, 
    0xe3, 0xe3, 0x8f, 0xff, 0x1f, 0xfe, 0x3c, 0x0e, 0x3f, 0xf8, 0xff, 0xfc, 0x70, 0x3c, 0x7f, 0xf8, 
    0xe3, 0xe3, 0x8f, 0xff, 0x1f, 0xfc, 0x3c, 0x0e, 0x1f, 0xf8, 0xff, 0xf8, 0x70, 0x3c, 0x7f, 0xf8, 
};

void UITask::begin(NodePrefs* node_prefs, const char* build_date, const char* firmware_version) {
  _prevBtnState = HIGH;
  _auto_off = millis() + AUTO_OFF_MILLIS;
  _node_prefs = node_prefs;
  _display->turnOn();

  // strip off dash and commit hash by changing dash to null terminator
  // e.g: v1.2.3-abcdef -> v1.2.3
  char *version = strdup(firmware_version);
  char *dash = strchr(version, '-');
  if(dash){
    *dash = 0;
  }

  // v1.2.3 (1 Jan 2025)
  sprintf(_version_info, "%s (%s)", version, build_date);
}

static void fmtCount(char* buf, uint32_t n) {
  if (n < 1000) {
    sprintf(buf, "%u", n);
  } else if (n < 99500) {
    sprintf(buf, "%uk", (n + 500) / 1000);
  } else if (n < 950000) {
    sprintf(buf, ".%um", (n + 50000) / 100000);
  } else {
    uint32_t m = (n + 500000) / 1000000;
    if (m > 99) m = 99;
    sprintf(buf, "%um", m);
  }
}

void UITask::renderCurrScreen() {
  char tmp[80];
  if (millis() < BOOT_SCREEN_MILLIS) { // boot screen
    // meshcore logo
    _display->setColor(DisplayDriver::BLUE);
    int logoWidth = 128;
    _display->drawXbm((_display->width() - logoWidth) / 2, 3, meshcore_logo, logoWidth, 13);

    // version info
    _display->setColor(DisplayDriver::LIGHT);
    _display->setTextSize(1);
    uint16_t versionWidth = _display->getTextWidth(_version_info);
    _display->setCursor((_display->width() - versionWidth) / 2, 22);
    _display->print(_version_info);

    // node type
    const char* node_type = "< Repeater >";
    uint16_t typeWidth = _display->getTextWidth(node_type);
    _display->setCursor((_display->width() - typeWidth) / 2, 35);
    _display->print(node_type);
  } else {  // home screen
    // node name
    _display->setCursor(0, 0);
    _display->setTextSize(1);
    _display->setColor(DisplayDriver::GREEN);
    _display->print(_node_prefs->node_name);

    // separator line below node name
    _display->setColor(DisplayDriver::LIGHT);
    _display->fillRect(0, 9, 128, 1);

    if (millis() < _advert_until) {
      // centered "flood" / "advert" in the area below the separator (y=11..63)
      _display->setColor(DisplayDriver::LIGHT);
      const char* line1 = "flood";
      const char* line2 = "advert";
      uint16_t w1 = _display->getTextWidth(line1);
      uint16_t w2 = _display->getTextWidth(line2);
      _display->setCursor((_display->width() - w1) / 2, 29);
      _display->print(line1);
      _display->setCursor((_display->width() - w2) / 2, 38);
      _display->print(line2);
    } else {
#if ENV_INCLUDE_GPS
      if (_gps && _gps->isEnabled()) {
        _display->setCursor(0, 11);
        _display->setColor(DisplayDriver::YELLOW);
        if (_gps->isValid())
          sprintf(tmp, "GPS: fix %ld ", _gps->satellitesCount());
        else
          sprintf(tmp, "GPS: no fix %ld ", _gps->satellitesCount());
        _display->print(tmp);
        int cx = strlen(tmp) * 6;
        _display->drawXbm(cx, 11, sat_xbm, 6, 8);
        if (_gps->isValid() && _sensors && _sensors->gps_blur_digits > 0) {
          sprintf(tmp, " fz:%d", _sensors->gps_blur_digits);
          _display->setCursor(cx + 6, 11);
          _display->print(tmp);
        }
      }
#endif

      // freq / sf
      _display->setCursor(0, 20);
      _display->setColor(DisplayDriver::YELLOW);
      sprintf(tmp, "FREQ: %06.3f SF%d", _node_prefs->freq, _node_prefs->sf);
      _display->print(tmp);

      // bw / cr
      _display->setCursor(0, 29);
      sprintf(tmp, "BW: %03.2f CR: %d", _node_prefs->bw, _node_prefs->cr);
      _display->print(tmp);

      // temp / humidity
      if (_sensors && _sensors->has_environment) {
        char prefix[32];
        sprintf(prefix, "bme280 0x%02X %d", _sensors->env_sensor_addr, (int)_sensors->node_temp_c);
        _display->setCursor(0, 38);
        _display->setColor(DisplayDriver::LIGHT);
        _display->print(prefix);
        int cx = strlen(prefix) * 6;
        _display->drawXbm(cx + 1, 38, degree_xbm, 4, 4);
        sprintf(tmp, "C %d%%", (int)_sensors->node_humidity);
        _display->setCursor(cx + 6, 38);
        _display->print(tmp);
      }

      // radio signal: rssi / snr / noise floor
      if (_stats) {
        _display->setCursor(0, 47);
        _display->setColor(DisplayDriver::LIGHT);
        sprintf(tmp, "rs:%-4d sn:%-3d f:%-4d",
                _stats->last_rssi,
                (int)(_stats->last_snr_x4 / 4),
                _stats->noise_floor);
        _display->print(tmp);

        // packet counts: recv / sent / dups
        _display->setCursor(0, 56);
        char rx[4], tx[4], dp[4];
        fmtCount(rx, _stats->n_recv);
        fmtCount(tx, _stats->n_sent);
        fmtCount(dp, _stats->n_dups);
        sprintf(tmp, "rx:%s tx:%s dp:%s", rx, tx, dp);
        _display->print(tmp);
      }
    }
  }
}

void UITask::loop() {
#ifdef PIN_USER_BTN
  if (millis() >= _next_read) {
    int btnState = digitalRead(PIN_USER_BTN);
    if (btnState != _prevBtnState) {
      if (btnState == USER_BTN_PRESSED) {  // falling edge
        if (!_display->isOn()) _display->turnOn();
        _auto_off = millis() + AUTO_OFF_MILLIS;
        if (_press_count == 0) {
          _press_count = 1;
          _first_press_at = millis();
        } else if ((unsigned long)(millis() - _first_press_at) <= 2000) {
          _press_count = 0;
          _advert_until = millis() + 5000;
          _auto_off = millis() + AUTO_OFF_MILLIS;
          if (_on_long_press) _on_long_press();
        } else {
          _press_count = 1;
          _first_press_at = millis();
        }
      }
      _prevBtnState = btnState;
    }
    if (_press_count == 1 && (unsigned long)(millis() - _first_press_at) > 2000) {
      _press_count = 0;
    }
    _next_read = millis() + 50;  // 20 reads per second
  }
#endif

  if (_display->isOn()) {
    if (millis() >= _next_refresh) {
      _display->startFrame();
      renderCurrScreen();
      _display->endFrame();

      _next_refresh = millis() + 1000;   // refresh every second
    }
    if (millis() > _auto_off) {
      _display->turnOff();
    }
  }
}
