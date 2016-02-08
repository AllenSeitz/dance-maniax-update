// minimaidManager.h implements HID support for a certain custom IO board
// source file created by Allen Seitz 02/07/2016

#ifndef _MINIMAIDMANAGER_H_
#define _MINIMAIDMANAGER_H_

#include "hidapi.h"

#include "common.h"

class minimaidManager
{
public:
	minimaidManager::minimaidManager();

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

private:
	hid_device* handle;

	time_t powerOnTime;
	time_t timeSinceLastLampUpdate;
};

#endif