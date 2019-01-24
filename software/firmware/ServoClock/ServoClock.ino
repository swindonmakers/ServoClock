/*
    Using Arduino ESP8266 Core v2.4.2
    Flash as
     - Board: NodeMCU 1.0 (12E Module)
     - Debug: None
     - Cpu : 80Mhz
     - Flash Size: 4M (3M SPIFFS)
     - IwIP Variant v2 Lower Memory
*/

#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <RhaNtp.h>
#include <RemoteDebug.h>
#include <TimeLib.h>
#include <Tiny5940.h>
#include <WiFiUdp.h>

#include "Settings.h"
#include "Characters.h"

RemoteDebug Debug;
Settings settings;
RhaNtp ntp;
Tiny5940 tlc;

#define DEBUG_INTERVAL_MS 1000
static unsigned long lastDebug = 0; // Last time debug data was output from the mainloop
static int lastMinute = 0; // Last minute of the hour that we updated the clock display

///
/// Called by the time libraries when a timesync is due
/// signal the ntp library to update the time
///
time_t requestTime()
{
    DEBUG_D("requestTime()\n");
	ntp.updateTime();
	return 0;
}

///
/// Returns a nicely formatted date & time String
///
String formatTime(time_t t) {
    char temp[20];
    snprintf(temp, 20, "%d/%d/%d %d:%02d:%02d", day(t), month(t), year(t), hour(t), minute(t), second(t));
    return String(temp);
}

///
/// Called by Debug to process commands received over telnet
///
void processRemoteDebugCmd() {
	String cmd = Debug.getLastCommand();

    if (cmd.startsWith("set_timezone")) {
        uint8_t offset = cmd.substring(cmd.length() - 1).toInt();
        ntp.setOffset(offset);
        settings.timezone = offset;
        settings.save();
        DEBUG("Timezone set to %d\n", offset);
        DEBUG("Utc time is: %s\n", formatTime(now()).c_str());
        DEBUG("Local time is: %s\n", formatTime(ntp.localNow()).c_str());

    } else if (cmd.startsWith("get_time")) {
        DEBUG("Utc time is: %s\n", formatTime(now()).c_str());
        DEBUG("Local time is: %s\n", formatTime(ntp.localNow()).c_str());

    } else if (cmd.startsWith("set_digit")) {
        int n = cmd.substring(10, 11).toInt();
        char c = cmd.substring(12, 13)[0];
        DEBUG("Setting digit %d to %c", n, c);
        setDigit(n, getClockDigit(c));
    }
}

CLOCKDIGIT getClockDigit(char digit) {
    switch(digit) {
        case ' ': return SPACE;
        case '0': return CD0;
        case '1': return CD1;
        case '2': return CD2;
        case '3': return CD3;
        case '4': return CD4;
        case '5': return CD5;
        case '6': return CD6;
        case '7': return CD7;
        case '8': return CD8;
        case '9': return CD9;
    }

    return SPACE;
}

///
/// Set the n'th digit (0,1,2,3) on the display to show <digit> from Characters.h
///
void setDigit(int n, CLOCKDIGIT digit) {
    byte mask = 128;
    int servoNumber = 0;
    int pos = 0;

    DEBUG_D("Set digit %d to %c\n", n, digit.digit);

    for (int i=0; i<7; i++) {
        servoNumber = 7*n + i;

        // work out if this segment be on or off
        pos = !((digit.positions & mask) == mask);
        if (i == 2 || i == 4) // servos 2 & 4 work the other way round to the rest
            pos = !pos;
        pos *= 90;

        DEBUG_D("Setting Servo %d to %d\n", servoNumber, pos);
        tlc.setServo(servoNumber, pos);
        mask = mask >> 1; 
    }
}

///
/// Standard Arduino setup code - runs at startup
///
void setup() {
    Serial.begin(115200);
    Serial.println("ServoClock");

    Serial.println("Load settings");
    settings.load();

    Serial.println("Init servos");
    tlc.init(D0, D1, D2);

    Serial.print("Connecting to Wifi");
    WiFi.begin(WIFI_SSID, WIFI_PWD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");

    Serial.printf("Connected to %s, IP %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

    String hostNameWifi = HOST_NAME;
    hostNameWifi.concat(".local");
    WiFi.hostname(hostNameWifi);

    if (MDNS.begin(HOST_NAME)) {
        Serial.printf("* MDNS responder started. Hostname -> %s\n", HOST_NAME);
        MDNS.addService("telnet", "tcp", 23);
    }

    // Setup telnet debug library
    Debug.begin(HOST_NAME);
    Debug.setResetCmdEnabled(true);
    Debug.setSerialEnabled(true);
    String rdbCmds = "set_timezone n\n";
    rdbCmds.concat("get_time\n");
    rdbCmds.concat("set_digit n c\n");
    Debug.setResetCmdEnabled(true);
    Debug.setHelpProjectsCmds(rdbCmds);
    Debug.setCallBackProjectCmds(&processRemoteDebugCmd);

    // Setup time library to get time via ntp
    IPAddress timeServerIP;
    WiFi.hostByName(settings.timeserver, timeServerIP);
    ntp.init(timeServerIP, settings.timezone);
    setSyncProvider(requestTime);
    setSyncInterval(60 * 60); // every hour
}

///
/// Standard Arduino loop, runs continuously
///
void loop()
{
    // Do some housekeeping tasks required by libraries
    Debug.handle();
    ntp.loop();
    tlc.update(); // TODO: maybe only call this when we know there are servo updates pending

    if (millis() - lastDebug > DEBUG_INTERVAL_MS) {
        lastDebug = millis();

        // Put any regular debugging in here using DEBUG()
        // output goes to serial and telnet
        //DEBUG("Mainloop\n");
    }

    if (timeStatus() != timeNotSet) {
        // Time is set, do some actions like update the display
        time_t t = ntp.localNow();
        
        if (minute(t) != lastMinute) {
            // Update display only once per minute
            lastMinute = minute(t);

            DEBUG("Updating clock at: %s\n", formatTime(t).c_str());
            // TODO: update clock display
            // tlc.setServo(servoNumber, position);
            // tlc.setServo(0, 90);
            // etc...

        }
    }
}