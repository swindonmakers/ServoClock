#include <EEPROM.h>

#ifndef SETTINGS_H
#define SETTINGS_H

#define HOST_NAME "SERVOCLOCK"
#define WIFI_SSID "SET_SSID_HERE"
#define WIFI_PWD "SET_PWD_HERE"

#define EEPROM_WIFI_SIZE 512
#define EEPROM_MAGIC_VAL "CLK2"

#define N_SERVOS 28

class Settings 
{
  public:

    char timeserver[64];
    uint8_t timezone;
    uint8_t servoTrims[N_SERVOS];

    Settings() 
    {
      // Defaults
      String("time.nist.gov").toCharArray(timeserver, 64);
      timezone = 1;
      for (int i=0; i<N_SERVOS; i++)
        servoTrims[i] = 0;
    };

    void load() {
      EEPROM.begin(EEPROM_WIFI_SIZE);

      // Verify magic;
      for (int i = 0; i < 4; i++)
        if (EEPROM.read(i) != EEPROM_MAGIC_VAL[i])
          return;

      for (int i = 4; i < 68; i++)
        timeserver[i-4] = EEPROM.read(i);

      timezone = EEPROM.read(68);

      for (int i = 0; i < N_SERVOS; i++)
        servoTrims[i] = EEPROM.read(69+i);

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

      for (int i = 0; i < N_SERVOS; i++)
        EEPROM.write(69+i, servoTrims[i]);

      EEPROM.commit();
      EEPROM.end();
    }
};

#endif
