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
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <RhaNtp.h>
#include <RemoteDebug.h>
#include <TimeLib.h>
#include <Tiny5940.h>
#include <WiFiUdp.h>
#include <FS.h>

#include "Settings.h"
#include "Characters.h"

RemoteDebug Debug;
ESP8266WebServer server(80);
Settings settings;
RhaNtp ntp;
Tiny5940 tlc;

#define DEBUG_INTERVAL_MS 1000
static unsigned long lastDebug = 0; // Last time debug data was output from the mainloop
static int lastMinute = 0; // Last minute of the hour that we updated the clock display

static unsigned long holdUntil = 0; // millis() time to hold the current display until
static unsigned long holdTimeMs = 60 * 1000; // time to keep a (non-time) message on display

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

    } else if (cmd.startsWith("set_holdtime")) {
        int n = cmd.substring(13).toInt();
        if (n > 600) // max 10 mins?
            n = 600;
        holdTimeMs = n * 1000;
        DEBUG("Hold time set to: %d secs\n", n);

    } else if (cmd.startsWith("set_digit")) {
        int n = cmd.substring(10, 11).toInt();
        char c = cmd.substring(12, 13)[0];
        DEBUG("Setting digit %d to %c", n, c);
        setDigit(n, getClockDigit(c));
        tlc.update();
        holdDisplay();

    } else if (cmd.startsWith("show")) {
        String msg = cmd.substring(5);
        showMessage(msg);

    } else if (cmd.startsWith("time")) {
        displayTime();
    
    } else if (cmd.startsWith("get_trims")) {
        for (int i = 0; i < N_SERVOS; i++)
            DEBUG("servo:%d, trim:%d\n", i, settings.servoTrims[i]);
    
    } else if (cmd.startsWith("set_trim")) {
        int servoNumber = cmd.substring(9, cmd.indexOf('=')).toInt();
        int trim = cmd.substring(cmd.indexOf('=') + 1).toInt();
        DEBUG("Trim servo %d by %d\n", servoNumber, trim);
        settings.servoTrims[servoNumber] = trim;
        settings.save();
        
    }
}

///
/// web handler for /showMessage?msg=HiHi
///
void handleMessage() {
    if (server.hasArg("msg")) {
        showMessage(server.arg("msg"));
        server.send(200, "text/plain", "ok");
    
    } else {
        server.send(500, "text/plain", "err - no msg");
    }
}

///
/// returns the conent type for a file
///
String getContentType(String filename) {
    if (filename.endsWith(".htm")) {
        return "text/html";
    } else if (filename.endsWith(".html")) {
        return "text/html";
    } else if (filename.endsWith(".css")) {
        return "text/css";
    } else if (filename.endsWith(".png")) {
        return "image/png";
    }
    return "text/plain";
}

///
/// deal with serving files out from SPIFFS internal flash filesystem
///
bool handleFileRead(String path) {
  if (path.endsWith("/")) {
    path += "index.htm";
  }
  String contentType = getContentType(path);
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

///
/// hold current display for the set time to stop time updates overriding it
///
void holdDisplay() {
    holdUntil = millis() + holdTimeMs;
}

///
/// Returns the CLOCKDIGIT for the given char
/// so the servo positions can be set.
///
CLOCKDIGIT getClockDigit(char digit) {
    // Search all known digits for a match
    for (int i=0; i<N_DIGITS; i++)
        if (allDigits[i].digit == digit)
            return allDigits[i];
    
    // Default return if nothing found
    return CDSPACE;
}

///
/// Returns the CLOCKDIGIT for the given int
///
CLOCKDIGIT getClockDigit(int n) {
    switch (n) {
        case 0: return CD0;
        case 1: return CD1;
        case 2: return CD2;
        case 3: return CD3;
        case 4: return CD4;
        case 5: return CD5;
        case 6: return CD6;
        case 7: return CD7;
        case 8: return CD8;
        case 9: return CD9;

        default: return CDSPACE;
    }
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

        // apply servo offsets
        pos += settings.servoTrims[servoNumber];

        DEBUG_D("Setting Servo %d to %d\n", servoNumber, pos);
        tlc.setServo(servoNumber, pos);
        mask = mask >> 1; 
    }
}

///
/// puts the clock in time display mode and shows the current time
///
void displayTime() {
    holdUntil = 0;
    time_t t = ntp.localNow();

    DEBUG("Updating clock at: %s\n", formatTime(t).c_str());

    if (hour(t) < 10)
        setDigit(0, CDSPACE);
    else if (hour(t) < 20)
        setDigit(0, CD1);
    else
        setDigit(0, CD2);
    
    setDigit(1, getClockDigit(hour(t) % 10));
    setDigit(2, getClockDigit((minute(t) - (minute(t) % 10)) / 10));
    setDigit(3, getClockDigit(minute(t) % 10));

    tlc.update();
}

///
/// display a custom message on the clock
///
void showMessage(String msg) {
    msg.toUpperCase();
    DEBUG("Show message: %s\n", msg.c_str());
    for (uint8_t i=0; i<4; i++) {
        if (i < msg.length())
            setDigit(i, getClockDigit(msg[i]));
        else
            setDigit(i, CDSPACE);
    }
    tlc.update();
    holdDisplay();
}

///
/// show count of makerspace members
///
void showMembers() {
    // TODO:
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
    tlc.setAllServo(45);
    delay(50); // wait for long enough that update wont abort
    tlc.update();

    Serial.print("Connecting to Wifi");
    WiFi.begin(WIFI_SSID, WIFI_PWD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");

    Serial.printf("Connected to %s, IP %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

    WiFi.hostname(HOST_NAME);

    if (MDNS.begin(HOST_NAME)) {
        Serial.printf("* MDNS responder started. Hostname -> %s\n", HOST_NAME);
        MDNS.addService("telnet", "tcp", 23);
        MDNS.addService("http", "tcp", 80);
    }

    // Setup telnet debug library
    Debug.begin(HOST_NAME);
    Debug.setResetCmdEnabled(true);
    Debug.setSerialEnabled(true);
    String rdbCmds = "set_timezone <n> - set timezone offset to <n>\r\n";
    rdbCmds.concat("get_time - output current time\r\n");
    rdbCmds.concat("set_digit <n> <c> - set clock digit <n> to char <c>\r\n");
    rdbCmds.concat("set_holdtime <n> - set custom message display time to <n> secs (max 600)\r\n");
    rdbCmds.concat("get_trims - show the current servo trim values\r\n");
    rdbCmds.concat("set_trim <n>=<t> - set the trim for servo <n> to <t> (note: trim wont be applied until next time the servos move)\r\n");
    rdbCmds.concat("time - display time\r\n");
    rdbCmds.concat("show <msg> - display <msg> on the clock (max 4 chars)\r\n");
    Debug.setHelpProjectsCmds(rdbCmds);
    Debug.setResetCmdEnabled(true);
    Debug.setCallBackProjectCmds(&processRemoteDebugCmd);

    // Setup time library to get time via ntp
    IPAddress timeServerIP;
    WiFi.hostByName(settings.timeserver, timeServerIP);
    ntp.init(timeServerIP, settings.timezone);
    setSyncProvider(requestTime);
    setSyncInterval(60 * 60); // every hour

    // Setup webserver
    server.on("/time", []() { displayTime(); server.send(200, "text/plain", "ok"); });
    server.on("/members", []() { showMembers(); server.send(200, "text/plain", "ok"); });
    server.on("/message", handleMessage);
    server.onNotFound( []() { 
        if (!handleFileRead(server.uri()))
            server.send ( 404, "text/plain", "page not found" ); 
        });
    server.begin();
}

///
/// Standard Arduino loop, runs continuously
///
void loop()
{
    // Do some housekeeping tasks required by libraries
    Debug.handle();
    ntp.loop();
    MDNS.update();
    server.handleClient();

    if (millis() - lastDebug > DEBUG_INTERVAL_MS) {
        lastDebug = millis();

        // Put any regular debugging in here using DEBUG()
        // output goes to serial and telnet
        //DEBUG("Mainloop\n");
    }

    if (timeStatus() != timeNotSet) {
        // Time is set, do some actions like update the display
        time_t t = ntp.localNow();
        
        // Update the display with the current time if the minute has changed
        // and we aren't holding a message
        if (minute(t) != lastMinute && millis() > holdUntil) {
            // Update display only once per minute
            lastMinute = minute(t);
            holdUntil = 0;
            displayTime();
        }
    
    } else if (millis() > 30000) {
        // Blank the display
        for (int i=0; i<4; i++)
            setDigit(i, CDSPACE);
        tlc.update();
    }
}
