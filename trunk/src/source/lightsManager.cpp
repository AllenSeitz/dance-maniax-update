// lightsManager.cpp implements a class to control lamp input in DMX
// source file created by Allen Seitz 10/26/2013

#include <stdio.h>
#include <string>

#include "lightsManager.h"

bool LightsManager::initialize()
{
	acioDetected = false;

	// the IO board (Arduino Mega) has 53 pins on it
	for ( int i = 0; i < 53; i++ ) 
	{
		duration[i] = 0;
	}

	debug = create_bitmap_ex(32, 100, 100);

	return acioDetected;
}

void LightsManager::update(UTIME dt)
{
	// update current durations
	for ( int i = 0; i < 53; i++ ) 
	{
		if ( duration[i] > 0 )
		{
			SUBTRACT_TO_ZERO(duration[i], (int)dt);
		}
	}

	// update the IO board
}

void LightsManager::setLamp(PinACIO which, int milliseconds)
{
	if ( isOutputPin(which) )
	{
		duration[(int)which] = milliseconds;
	}
}

void LightsManager::addLamp(PinACIO which, int milliseconds)
{
	if ( isOutputPin(which) )
	{
		duration[(int)which] += milliseconds;
	}
}

bool LightsManager::getLamp(PinACIO which)
{
	if ( !isOutputPin(which) )
	{
		al_trace("WARNING: getLamp() called on a pin which was not allocated for a lamp");
	}
	return duration[which] != 0; // so that -1 returns true as well
}

void LightsManager::lampOff(PinACIO which)
{
	setLamp(which, 0);
}

void LightsManager::allOff()
{
	for ( int i = 0; i < NUM_OUTPUT_PINS; i++ )
	{
		lampOff(outputPins[i]);
	}
}

void LightsManager::pulseOdometer()
{
	// probably a setDuration which a short, tested, duration
}

void LightsManager::setOrbColor(int player, int orb, int color, int duration)
{

}
// precondition: player is 0-1, orb is 0-3, and color is 0=red, 1=blue, 2=purple
// postcondition: for all LEDs on this orb, the duration becomes either the given duration or zero

void LightsManager::renderDebugOutput(BITMAP* surface)
{
	clear_to_color(debug, 0);

	textout_centre(debug, font, "LAMPS", 50, 50, makeacol(255,255,255,255));

	blit(debug, surface, 0, 0, SCREEN_WIDTH-100, 0, 100, 100);
}

bool LightsManager::isOutputPin(PinACIO which)
{
	for ( int i = 0; i < NUM_OUTPUT_PINS; i++ )
	{
		if ( which == outputPins[i] )
		{
			return true;
		}
	}
	return false;
}