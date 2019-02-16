#include <EEPROM.h>

#ifndef SETTINGS_H
#define SETTINGS_H

#define HOST_NAME "SERVOCLOCK"
#define WIFI_SSID "SET_SSID_HERE"
#define WIFI_PWD "SET_PWD_HERE"

#define EEPROM_WIFI_SIZE 512
#define EEPROM_MAGIC_VAL "CLK1"

class Settings 
{
  public:

    char timeserver[64];
    uint8_t timezone;

    Settings() 
    {
      // Defaults
      String("time.nist.gov").toCharArray(timeserver, 64);
      timezone = 1;
    };

    void load() {
      EEPROM.begin(EEPROM_WIFI_SIZE);

      // Verify magic;
      for (int i = 0; i < 4; i++)
        if (EEPROM.read(i) != EEPROM_MAGIC_VAL[i])
          return;

      // time server
      for (int i = 4; i < 68; i++)
        timeserver[i-4] = EEPROM.read(i);

      timezone = EEPROM.read(68);

      EEPROM.end();
    }

    void save() {
      EEPROM.begin(EEPROM_WIFI_SIZE);

      // Write magic
      for (int i = 0; i < 4; i++)
        EEPROM.write(i, EEPROM_MAGIC_VAL[i]);

      for (int i = 4; i < 68; i++)
        EEPROM.write(i, timeserver[i-4]);

      EEPROM.write(68, timezone);

      EEPROM.commit();
      EEPROM.end();
    }
};

#endif
