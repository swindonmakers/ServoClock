#include "RhaNtp.h"

void RhaNtp::init(IPAddress &timeServerIp, uint8_t offset)
{
	this->timeServerIp = timeServerIp;
	this->offset = offset;

	udp.begin(NTP_LOCAL_PORT);

}

void RhaNtp::setOffset(uint8_t offset)
{
	this->offset = offset;
}

void RhaNtp::updateTime()
{
	nextNtpTimeUpdate = millis();
}

void RhaNtp::loop()
{
	if (nextNtpTimeUpdate != 0 && millis() > nextNtpTimeUpdate) {
		// Request NTP time packet
		while (udp.parsePacket() > 0); // discard any previously received packets

		sendNTPpacket();
		nextNtpTimeUpdate = 0;
		expectedTimeResponse = millis() + NTP_RESPONSE_TIME_MS;
	}

	if (expectedTimeResponse != 0) {
		if (udp.parsePacket()) {
			// have Ntp response set time
			udp.read(packetBuffer, NTP_PACKET_SIZE);
			// Extract seconds portion.
			unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
			unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
			unsigned long secSince1900 = highWord << 16 | lowWord;
			udp.flush();
			time_t t = secSince1900 - 2208988800UL;
			setTime(t);
			expectedTimeResponse = 0;
		} else if (millis() > expectedTimeResponse) {
			// Ntp Fail, try again
			expectedTimeResponse = 0;
			nextNtpTimeUpdate = millis();
		}
	}

  yield();
}

// send an NTP request to the time server at the given address
void RhaNtp::sendNTPpacket()
{
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011;   // LI, Version, Mode
	packetBuffer[1] = 0;     // Stratum, or type of clock
	packetBuffer[2] = 6;     // Polling Interval
	packetBuffer[3] = 0xEC;  // Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12]  = 49;
	packetBuffer[13]  = 0x4E;
	packetBuffer[14]  = 49;
	packetBuffer[15]  = 52;

	// all NTP fields have been given values, now
	// you can send a packet requesting a timestamp:
	udp.beginPacket(timeServerIp, 123); //NTP requests are to port 123
	udp.write(packetBuffer, NTP_PACKET_SIZE);
	udp.endPacket();
}

time_t RhaNtp::localNow()
{
	return now() + offset * SECS_PER_HOUR;
}