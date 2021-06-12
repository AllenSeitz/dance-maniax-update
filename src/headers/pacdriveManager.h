// pacdriveManager.h implements programming for the PacLED64
// source file created by Allen Seitz 12/15/2013

#ifndef _PACDRIVEMANAGER_H_
#define _PACDRIVEMANAGER_H_

#include "../headers/common.h"

// meh always use the 32 bit version, it doesn't matter, and makes distibution so much easier
//#ifdef _WIN64
//#pragma comment(lib,"PacDrive64.lib")
//#else
#pragma comment(lib,"PacDrive32.lib")
//#endif

class pacdriveManager
{
public:
	pacdriveManager::pacdriveManager();

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
	int numDevicesFound;
	int indexOfDevice;
	bool deviceReady;

	unsigned long timeSinceLastLampUpdate;
};

#endif