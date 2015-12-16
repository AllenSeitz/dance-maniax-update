// lightsManager.cpp implements a class to control lamp input in DMX
// source file created by Allen Seitz 10/26/2013

#include <stdio.h>
#include <string>
#include <algorithm>

#include "lightsManager.h"
#include "extioManager.h"
#include "pacdriveManager.h"

extern extioManager extio;
extern pacdriveManager pacio;

bool LightsManager::initialize()
{
	acioDetected = false;

	// the IO board (Arduino Mega) has 54 pins on it
	for ( int i = 0; i < 54; i++ ) 
	{
		duration[i] = 0;
	}
	spotDurations[0] = spotDurations[1] = spotDurations[2] = 0;

	currentLampProgramTime = 0;
	currentLampProgramIndex = 0;
	loopLampProgram = false;

	debug = create_bitmap_ex(32, 100, 100);

	return acioDetected;
}

void LightsManager::update(UTIME dt)
{
	// update current durations
	for ( int i = 0; i < 54; i++ ) 
	{
		if ( duration[i] > 0 )
		{
			SUBTRACT_TO_ZERO(duration[i], (int)dt);
		}
	}
	for ( int j = 0; j < 3; j++ )
	{
		if ( spotDurations[j] > 0 )
		{
			SUBTRACT_TO_ZERO(spotDurations[j], (int)dt);
		}
	}

	updateLampProgram(dt);

	// update the IO board, whichever one is plugged in
	extio.updateLamps();
	pacio.updateLamps();
}

void LightsManager::setLamp(PinACIO which, int milliseconds)
{
	if ( which >= spotlightA && which <= spotlightC )
	{
		spotDurations[ (int)which - (int)spotlightA ] = milliseconds;
	}
	if ( isOutputPin(which) )
	{
		int checkedIndex = (int)which; // avoids a warning
		if ( checkedIndex < 54 )
		{
			duration[checkedIndex] = milliseconds;
		}
	}
}

void LightsManager::addLamp(PinACIO which, int milliseconds)
{
	if ( which >= spotlightA && which <= spotlightC )
	{
		spotDurations[ (int)which - (int)spotlightA ] += milliseconds;
	}
	if ( isOutputPin(which) )
	{
		duration[(int)which] += milliseconds;
	}
}

bool LightsManager::getLamp(PinACIO which)
{
	if ( !isOutputPin(which) )
	{
		al_trace("WARNING: getLamp() called on a pin which was not allocated for a lamp\n");
	}
	if ( which >= spotlightA && which <= spotlightC )  // there's only three of these
	{
		return spotDurations[ (int)which - (int)spotlightA ] != 0;
	}
	return duration[which] != 0; // so that -1 returns true as well
}

void LightsManager::lampOff(PinACIO which)
{
	setLamp(which, 0);
}

void LightsManager::setAll(int milliseconds)
{
	setLamp(lampLeft, milliseconds);
	setLamp(lampStart, milliseconds);
	setLamp(lampRight, milliseconds);
	setLamp((int)lampLeft + 1, milliseconds);
	setLamp((int)lampStart + 1, milliseconds);
	setLamp((int)lampRight + 1, milliseconds);

	for ( int p = 0; p < 2; p++ )
	{
		setLamp( (PinACIO)( (int)lampRed0 + p), milliseconds );
		setLamp( (PinACIO)( (int)lampRed1 + p), milliseconds );
		setLamp( (PinACIO)( (int)lampRed2a + p), milliseconds );
		setLamp( (PinACIO)( (int)lampRed2b + p), milliseconds );
		setLamp( (PinACIO)( (int)lampRed3a + p), milliseconds );
		setLamp( (PinACIO)( (int)lampRed3b + p), milliseconds );

		setLamp( (PinACIO)( (int)lampBlue0 + p), milliseconds );
		setLamp( (PinACIO)( (int)lampBlue1 + p), milliseconds );
		setLamp( (PinACIO)( (int)lampBlue2a + p), milliseconds );
		setLamp( (PinACIO)( (int)lampBlue2b + p), milliseconds );
		setLamp( (PinACIO)( (int)lampBlue3a + p), milliseconds );
		setLamp( (PinACIO)( (int)lampBlue3b + p), milliseconds );
	}

	setLamp(spotlightA, milliseconds);
	setLamp(spotlightB, milliseconds);
	setLamp(spotlightC, milliseconds);
}

void LightsManager::pulseOdometer()
{
	// probably a setDuration which a short, tested, duration
}

// precondition: player is 0-1, group is 0-11, and color is 0=red, 1=blue, 2=purple
// postcondition: for all LEDs on this orb, the duration becomes either the given duration or zero
void LightsManager::setGroupColor(int group, int color, int milliseconds)
{
	ASSERT(group < 12 && group >= 0);

	int red = color == 0 || color == 2 ? milliseconds : 0;
	int blue = color == 1 || color == 2 ? milliseconds : 0;

	switch (group)
	{
	case 0:
		setLamp(lampRed0, red);
		setLamp(lampBlue0, blue);
		break;
	case 1:
		setLamp(lampRed1, red);
		setLamp(lampBlue1, blue);
		break;
	case 2:
		setLamp((int)lampRed0 + 1, red);
		setLamp((int)lampBlue0 + 1, blue);
		break;
	case 3:
		setLamp((int)lampRed1 + 1, red);
		setLamp((int)lampBlue1 + 1, blue);
		break;
	case 4:
		setLamp(lampRed2a, red);
		setLamp(lampBlue2a, blue);
		break;
	case 5:
		setLamp(lampRed2b, red);
		setLamp(lampBlue2b, blue);
		break;
	case 6:
		setLamp((int)lampRed3a, red);
		setLamp((int)lampBlue3a, blue);
		break;
	case 7:
		setLamp((int)lampRed3b, red);
		setLamp((int)lampBlue3b, blue);
		break;
	case 8:
		setLamp((int)lampRed2a + 1, red);
		setLamp((int)lampBlue2a + 1, blue);
		break;
	case 9:
		setLamp((int)lampRed2b + 1, red);
		setLamp((int)lampBlue2b + 1, blue);
		break;
	case 10:
		setLamp((int)lampRed3a + 1, red);
		setLamp((int)lampBlue3a + 1, blue);
		break;
	case 11:
		setLamp((int)lampRed3b + 1, red);
		setLamp((int)lampBlue3b + 1, blue);
		break;
	}
}

void LightsManager::setOrbColor(int player, int orb, int color, int milliseconds)
{
	ASSERT(player < 2 && orb < 4 && player >= 0 && orb >= 0);
	switch (orb)
	{
	case 0:
		setGroupColor(0 + player*2, color, milliseconds);
		break;
	case 1:
		setGroupColor(1 + player*2, color, milliseconds);
		break;
	case 2:
		setGroupColor(4 + player*4, color, milliseconds);
		setGroupColor(5 + player*4, color, milliseconds);
		break;
	case 3:
		setGroupColor(6 + player*4, color, milliseconds);
		setGroupColor(7 + player*4, color, milliseconds);
		break;
	}
}

void LightsManager::loadLampProgram(const char* filename)
{
	char temp[512] = "data/lights/";
	FILE* fp = NULL;

	stopLampProgram();

	// open the file
	strcat_s(temp, 256, filename);
	fopen_s(&fp, temp, "rt");
	if ( fp == NULL )
	{
		globalError(MISSING_LAMP_SCRIPT, filename);
		return;
	}

	// read lines until EOF
	fgets(temp, 512, fp);
	while ( temp[0] != 0 )
	{
		if ( temp[0] == '#' || temp[0] == '@' || temp[0] == '\n' || temp[0] == '\r' )
		{
			// comment
			if ( strcmp(temp, "@loop\n") == 0 )
			{
				loopLampProgram = true;
			}
		}
		else
		{
			struct LAMP_STEP lamp;
			sscanf_s(temp, "%d, %d, %d, %d, %c", &lamp.timing, &lamp.group, &lamp.addDuration, &lamp.color, &lamp.option);
			lampProgram.push_back(lamp);
		}

		// next loop
		if ( fgets(temp, 512, fp) == NULL )
		{
			break;
		}
	}

	sort(lampProgram.begin(), lampProgram.end(), LAMP_STEP::sortNoteFunction);

	fclose(fp);
}

void LightsManager::updateLampProgram(UTIME dt)
{
	if ( lampProgram.size() == 0 )
	{
		return;
	}
	currentLampProgramTime += dt;

	while ( currentLampProgramIndex < (int)lampProgram.size() && lampProgram[currentLampProgramIndex].timing <= currentLampProgramTime )
	{
		// perform the instruction and move on to the next element
		if ( lampProgram[currentLampProgramIndex].group >= 13 && lampProgram[currentLampProgramIndex].group <= 15 )
		{
			addLamp(lampProgram[currentLampProgramIndex].group + 100, lampProgram[currentLampProgramIndex].addDuration); // spotlight
		}
		if ( lampProgram[currentLampProgramIndex].group < 12 )
		{
			setGroupColor(lampProgram[currentLampProgramIndex].group, lampProgram[currentLampProgramIndex].color, lampProgram[currentLampProgramIndex].addDuration);
		}
		currentLampProgramIndex++;
	}

	// restart the lamp sequence or end it
	if ( currentLampProgramIndex >= (int)lampProgram.size() )
	{
		if ( loopLampProgram )
		{
			currentLampProgramIndex = 0;
			currentLampProgramTime = 0;
		}
		else
		{
			stopLampProgram();
		}
	}
}

void LightsManager::stopLampProgram()
{
	lampProgram.clear();
	currentLampProgramTime = 0;
	currentLampProgramIndex = 0;
	loopLampProgram = false;
}

void LightsManager::renderDebugOutput(BITMAP* surface)
{
	clear_to_color(debug, 0);

	// the four orbs on top
	textout_centre(debug, font, "U", 15, 10, getLampDebugColor(0));
	textout_centre(debug, font, "U", 15,  5, getLampDebugColor(0));

	textout_centre(debug, font, "U", 35, 15, getLampDebugColor(1));
	textout_centre(debug, font, "U", 35, 10, getLampDebugColor(1));

	textout_centre(debug, font, "U", 65, 15, getLampDebugColor(2));
	textout_centre(debug, font, "U", 65, 10, getLampDebugColor(2));

	textout_centre(debug, font, "U", 85, 10, getLampDebugColor(3));
	textout_centre(debug, font, "U", 85,  5, getLampDebugColor(3));

	// the four orbs below
	textout_centre(debug, font, "U", 15, 85, getLampDebugColor(5));
	textout_centre(debug, font, "U", 15, 80, getLampDebugColor(4));

	textout_centre(debug, font, "U", 35, 90, getLampDebugColor(7));
	textout_centre(debug, font, "U", 35, 85, getLampDebugColor(6));

	textout_centre(debug, font, "U", 65, 90, getLampDebugColor(9));
	textout_centre(debug, font, "U", 65, 85, getLampDebugColor(8));

	textout_centre(debug, font, "U", 85, 85, getLampDebugColor(11));
	textout_centre(debug, font, "U", 85, 80, getLampDebugColor(10));

	// spot lamps
	textout_centre(debug, font, "O", 20, 34, getLamp(spotlightC) ? makeacol32(150, 64, 255, 255) : DARKGRAY);
	textout_centre(debug, font, "O", 35, 32, getLamp(spotlightB) ? makeacol32(255, 0, 150, 255) : DARKGRAY);
	textout_centre(debug, font, "O", 50, 30, getLamp(spotlightA) ? YELLOW : DARKGRAY);
	textout_centre(debug, font, "O", 65, 32, getLamp(spotlightB) ? makeacol32(255, 0, 150, 255) : DARKGRAY);
	textout_centre(debug, font, "O", 80, 34, getLamp(spotlightC) ? makeacol32(150, 64, 255, 255) : DARKGRAY);

	// menu lamps
	textout_centre(debug, font, "<", 20, 60, getLamp(lampLeft) ? RED : DARKGRAY);
	textout_centre(debug, font, "=", 24, 60, getLamp(lampStart) ? RED : DARKGRAY);
	textout_centre(debug, font, ">", 30, 60, getLamp(lampRight) ? RED : DARKGRAY);
	textout_centre(debug, font, "<", 70, 60, getLamp((int)lampLeft + 1) ? RED : DARKGRAY);
	textout_centre(debug, font, "=", 74, 60, getLamp((int)lampStart + 1) ? RED : DARKGRAY);
	textout_centre(debug, font, ">", 80, 60, getLamp((int)lampRight + 1) ? RED : DARKGRAY);

	blit(debug, surface, 0, 0, SCREEN_WIDTH-100, 0, 100, 100);
}

int LightsManager::getLampDebugColor(int ledset)
{
	bool red = false, blue = false;

	switch (ledset) // follows the lamp test
	{
	case 0:
		red = getLamp(lampRed0);
		blue = getLamp(lampBlue0);
		break;
	case 1:
		red = getLamp(lampRed1);
		blue = getLamp(lampBlue1);
		break;
	case 2:
		red = getLamp((int)lampRed0 + 1);
		blue = getLamp((int)lampBlue0 + 1);
		break;
	case 3:
		red = getLamp((int)lampRed1 + 1);
		blue = getLamp((int)lampBlue1 + 1);
		break;
	case 4:
		red = getLamp((int)lampRed2a);
		blue = getLamp((int)lampBlue2a);
		break;
	case 5:
		red = getLamp((int)lampRed2b);
		blue = getLamp((int)lampBlue2b);
		break;
	case 6:
		red = getLamp((int)lampRed3a);
		blue = getLamp((int)lampBlue3a);
		break;
	case 7:
		red = getLamp((int)lampRed3b);
		blue = getLamp((int)lampBlue3b);
		break;
	case 8:
		red = getLamp((int)lampRed2a + 1);
		blue = getLamp((int)lampBlue2a + 1);
		break;
	case 9:
		red = getLamp((int)lampRed2b + 1);
		blue = getLamp((int)lampBlue2b + 1);
		break;
	case 10:
		red = getLamp((int)lampRed3a + 1);
		blue = getLamp((int)lampBlue3a + 1);
		break;
	case 11:
		red = getLamp((int)lampRed3b + 1);
		blue = getLamp((int)lampBlue3b + 1);
		break;
	}

	if ( red && blue )
	{
		return PURPLE;
	}
	else if ( red )
	{
		return RED;
	}
	else if ( blue )
	{
		return BLUE;
	}
	return DARKGRAY;
}

bool LightsManager::isOutputPin(PinACIO which)
{
	if ( which >= coinCounter && which <= 53 ) // the number of digital pins on the Arduino Mega
	{
		return true;
	}
	if ( which == spotlightA || which == spotlightB || which == spotlightC )
	{
		return true;
	}
	return false;
}