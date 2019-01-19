#ifndef RhaNtp_h
#define RhaNtp_h

#include <Arduino.h>
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WifiUdp.h>

#define NTP_PACKET_SIZE 48 // NTP time stamp is in the first 48 bytes of the message
#define NTP_LOCAL_PORT 2390
#define NTP_RESPONSE_TIME_MS 500

class RhaNtp
{
public:
	void init(IPAddress &timeServerIp, uint8_t offset);
	void loop();
	void updateTime();
	void setOffset(uint8_t offset);

	time_t localNow();

private:
	IPAddress timeServerIp;
	uint8_t offset;
	
	unsigned long nextNtpTimeUpdate;	// next time update required at...
	unsigned long expectedTimeResponse; // expect a udp packet by this time...

	byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

	WiFiUDP udp;

	//time_t requestTime();
	void sendNTPpacket();
};

#endif