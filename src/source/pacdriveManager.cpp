// pacdriveManager.h implements programming for the PacLED64
// source file created by Allen Seitz 12/15/2013
// NOTE: this implements lights control using a PacLED64, other boards are supported in other source files

#include "../headers/lightsManager.h"
#include "../headers/pacdriveManager.h"

#include "../lib/pacdrive/include/PacDrive.h"

extern LightsManager lm;

pacdriveManager::pacdriveManager()
{
	numDevicesFound = -1;
	indexOfDevice = -1;
	deviceReady = false;

	timeSinceLastLampUpdate = 0;
}

// precondition: called only once before the visible boot sequence begins
void pacdriveManager::initialize()
{
	numDevicesFound = PacInitialize();
}

// returns: false when the software should give up on finding an IO board, true when it should continue to wait (or succeeds)
bool pacdriveManager::updateInitialize(UTIME dt)
{
	// one of the devices plugged in had better be a PacLED64 (device type 3)
	for ( int i = 0; i < numDevicesFound; i++ )
	{
		if ( PacGetDeviceType(i) == 3 )
		{
			// hooray!
			indexOfDevice = i;
			deviceReady = true;
			return true;
		}
	}

	// this function only runs once
	return false;
}

bool pacdriveManager::isReady()
{
	return indexOfDevice >= 0 && deviceReady;
}

void pacdriveManager::update(UTIME dt)
{
	if ( !isReady() )
	{
		return;
	}
	timeSinceLastLampUpdate += dt;
}

void pacdriveManager::updateLamps()
{
	if ( timeSinceLastLampUpdate < 50 )
	{
		//al_trace("--STIFLED LAMP UPDATE-- too soon of an update.\r\n");
		return; // don't do that
	}
	timeSinceLastLampUpdate = 0;

	// the LED states are stored in these bitfields
	char upper = 0;
	char lowp1 = 0;
	char lowp2 = 0;
	char menu = 0;
	char spot = 0;
	
	// these 'pins' (lamps) belong to the corresponding bitfields, in order low-bit to high-bit
	int upperPins[8] = { lampRed0, lampBlue0, lampRed1, lampBlue1, lampRed0+1, lampBlue0+1, lampRed1+1, lampBlue1+1 };
	int lowerPins[8] = { lampRed2a, lampBlue2a, lampRed2b, lampBlue2b, lampRed3a, lampBlue3a, lampRed3b, lampBlue3b };
	int menuPins[8] = { lampRight, lampStart, lampLeft, lampRight+1, lampStart+1, lampLeft+1, 0, 0 };
	int spotPins[8] = { spotlightC, spotlightB, spotlightA, 0, 0, 0, 0, 0 };

	// pack them into bits
	for ( int i = 0; i < 8; i++ )
	{
		upper |= lm.getLamp(upperPins[i]) ? 1 << i : 0;
		lowp1 |= lm.getLamp(lowerPins[i]) ? 1 << i : 0;
		lowp2 |= lm.getLamp(lowerPins[i]+1) ? 1 << i : 0; // because the p2 version of every pin is the p1 index plus one
		if ( menuPins[i] != 0 )
		{
			menu |= lm.getLamp(menuPins[i]) ? 1 << i : 0;
		}
		if ( spotPins[i] != 0 )
		{
			spot |= lm.getLamp(spotPins[i]) ? 1 << i : 0;
		}
	}

	// tell the PacLED64
	Pac64SetLEDStates(indexOfDevice, 1, upper);
	Pac64SetLEDStates(indexOfDevice, 2, lowp1);
	Pac64SetLEDStates(indexOfDevice, 3, lowp2);
	Pac64SetLEDStates(indexOfDevice, 4, menu);
	Pac64SetLEDStates(indexOfDevice, 5, spot);
}