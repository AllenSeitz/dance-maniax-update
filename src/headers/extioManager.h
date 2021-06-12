// extioManager.h implements programming the serial port for DMX
// source file created by Allen Seitz 11/14/2013

#ifndef _EXTIOMANAGER_H_
#define _EXTIOMANAGER_H_

#include "../headers/common.h"

class extioManager
{
public:
	extioManager::extioManager();

	void initialize();
	// precondition: called only once before the visible boot sequence begins
	// postcondition: ready for updateInitialize() to begin

	bool updateInitialize(UTIME dt);
	// precondition: called continuously during the boot loop, until the IO is ready
	// postcondition: hopefully, eventualy, isReady() will return true
	// returns: false when the software should give up on finding an IO board, true when it should continue to wait

	bool isReady();
	// returns: true when the initialization is complete and the hardware is fully usable

	void update(UTIME dt);
	// precondition: isReady() (checked) or else this call will do nothing
	// postcondition: updates the IO board, if necessary

	void updateLamps();
	// precondition: called only by the LightsManager whenever a lamp change happens, and not more than 20 times per second
	// postcondition: a packet has been sent to the IO board

	// TODO: functions for coin counter and lockout coil

private:
	int comPort;
	bool isConnected;
	bool isTalking;
	UTIME powerOnTime;
	int connectionState;

	bool attemptConnection(const char* port);
	// precondition: only called while in boot mode, and should be called once each loop
	// returns: returns false when this manager gives up on finding the extio, true otherwise

	//////////////////////////////////////////////////////////////////////////
	// LOW LEVEL SERIAL FUNCTIONS
	//////////////////////////////////////////////////////////////////////////
private:
	static const int PACKET_SIZE = 3;
	unsigned char inputBuffer[PACKET_SIZE];
	UTIME timeSinceLastLampUpdate;

	void* hSerial;
	COMSTAT status;
	DWORD errors;

	int ReadData(unsigned char *buffer, unsigned int nbChar);
	// precondition: isReady() and buffer can hold nbChar bytes
	// postcondition: reads up to nbChar bytes
	// returns: number of bytes actually read, or -1 if the port was empty

	bool WriteData(char *buffer, unsigned int nbChar);
	// precondition: isReady() and buffer contains at least nbChar bytes
	// postcondition: writes nbChar bytes to the port
	// returns: true on success or false on error

	bool isPacketReady();
	// precondition: isReady()
	// returns: true if there are 8 bytes or more waiting to be read
};

#endif