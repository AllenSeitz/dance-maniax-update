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

	return acioDetected;
}