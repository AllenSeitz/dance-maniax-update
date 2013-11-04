// lightsManager.h declares a class which manages the led and spot lights
// source file created by Allen Seitz 10/26/2013

#ifndef _LIGHTSMANAGER_H_
#define _LIGHTSMANAGER_H_

#include "common.h"
#include <vector>

// Lamp Numbering

//                      //
// (0) (1)_____(0) (1)  //
//   \             /    //
//   /  A B C B A  \    //
//   |-------------|    //
//   |             |    //
//   |             |    //
//   |             |    //
// (2)_(3)_____(2)_(3)  //
//   |             |    //
//   |             |    //
//                      //

// used for all input and output pins. also used to distinguish lamps for the LightsManager
enum PinACIO
{
	// input pins
	test = 2,
	service = 3,
	coin = 4,

	// output pins
	spotlightA = 5, // spotlights 1 + 5, always together
	spotlightB = 6, // spotlights 2 + 4, always together
	spotlightC = 7, // the spotlight in the middle

	// more output pins
	odometer = 8,
	lampStartP1 = 9,
	lampMenuP1 = 10,
	lampStartP2 = 11,
	lampMenuP2 = 12,
	selfTest = 13,

	// input pins - add one for 2P
	buttonStart = 20,
	buttonMenuLeft = 22,
	buttonMenuRight = 24,
	sensorRed0 = 26,      // left
	sensorRed1 = 28,      // right
	sensorBlueL0 = 30,    // straight down
	sensorBlueL1 = 32,    // angled down
	sensorBlueR0 = 34,    // straight down
	sensorBlueR1 = 36,    // angled down
	
	// output pins - add one for 2P
	lampRed0 = 38,  // upper left
	lampRed1 = 40,  // upper right
	lampRed2 = 42,  // lower left
	lampRed3 = 44,  // lower right
	lampBlue0 = 46, // upper left
	lampBlue1 = 48, // upper right
	lampBlue2 = 50, // lower left
	lampBlue3 = 52, // lower right
};

// used for making sure that output signals are only sent on output pins
#define NUM_OUTPUT_PINS 15
static const PinACIO outputPins[NUM_OUTPUT_PINS] = {	
										spotlightA, spotlightB, spotlightC, 
										lampStartP1, lampMenuP1, lampStartP2, lampMenuP2,
										lampRed0, lampRed1, lampRed2, lampRed3,
										lampBlue0, lampBlue1, lampBlue2, lampBlue3 };

class LightsManager
{
public:

	bool initialize();
	// postcondition: returns false if there is an error

	void update(UTIME dt);
	// precondition: initialize() has been called
	// postcondition: communicates with the IO board and updates the state of the lights

	void setLamp(PinACIO which, int milliseconds);
	void addLamp(PinACIO which, int milliseconds);
	// precondition: which is an output pin (checked)
	// postcondition: sets or adds to the duration of this lamp

	bool getLamp(PinACIO which);
	// returns: true if the duration for this pin is > 0

	void lampOff(PinACIO which);
	// precondition: which is an output pin (checked)
	// postcondition: turns the lamp off NOW and also sets any remaining duration to 0

	void allOff();
	// postcondition: calls lampOff() for every lamp

	void pulseOdometer();
	// postcondition: sends the only accepted output on this pin

	void setOrbColor(int player, int orb, int color, int milliseconds);
	// precondition: player is 0-1, orb is 0-3, and color is 0=red, 1=blue, 2=purple
	// postcondition: for all LEDs on this orb, the duration becomes either the given duration or zero

	void renderDebugOutput(BITMAP* surface);
	// precondition: surface is one of the backbuffers
	// postcondition: renders a simulated lights display in the top right corner

private:
	bool isOutputPin(PinACIO which);
	// returns: true if this pin is intended to be set by this class

private:
	bool acioDetected;
	int duration[53];

	BITMAP* debug;
};

#endif