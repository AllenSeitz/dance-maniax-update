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
//   /  C B A B C  \    //
//   |-------------|    //
//   |             |    //
//   |             |    //
//   |             |    //
// (2)_(3)_____(2)_(3)  //
//   |             |    //
//   |             |    //
//                      //

class LightsManager
{
public:

	bool initialize();
	// postcondition: returns false if there is an error

	void update(UTIME dt);
	// precondition: initialize() has been called
	// postcondition: communicates with the IO board and updates the state of the lights

	void setLamp(PinACIO which, int milliseconds);
	void setLamp(int which, int milliseconds) { return setLamp( (PinACIO)which, milliseconds ); }
	void addLamp(PinACIO which, int milliseconds);
	void addLamp(int which, int milliseconds) { return addLamp( (PinACIO)which, milliseconds ); }
	// precondition: which is an output pin (checked)
	// postcondition: sets or adds to the duration of this lamp

	bool getLamp(PinACIO which);
	bool getLamp(int which) { return getLamp( (PinACIO)which ); }
	// returns: true if the duration for this pin is > 0

	void lampOff(PinACIO which);
	// precondition: which is an output pin (checked)
	// postcondition: turns the lamp off NOW and also sets any remaining duration to 0

	void setAll(int milliseconds);
	// postcondition: calls setLamp() for every lamp

	void pulseOdometer();
	// postcondition: sends the only accepted output on this pin

	void setGroupColor(int group, int color, int milliseconds);
	// precondition: group is 0-11, and color is 0=red, 1=blue, 2=purple
	// postcondition: for all LEDs on this orb, the duration becomes either the given duration or zero

	void setOrbColor(int player, int orb, int color, int milliseconds);
	// precondition: playeris 0-1, orb is 0-3, and color is 0=red, 1=blue, 2=purple
	// postcondition: for all LEDs on this orb, the duration becomes either the given duration or zero

	void renderDebugOutput(BITMAP* surface);
	// precondition: surface is one of the backbuffers
	// postcondition: renders a simulated lights display in the top right corner

private:
	int getLampDebugColor(int ledset);
	// precondition: only to be used by renderDebugOutput()
	// returns: an rgb color value

	bool isOutputPin(PinACIO which);
	// returns: true if this pin is intended to be set by this class

private:
	bool acioDetected;
	int duration[54];
	int spotDurations[3];

	BITMAP* debug;
};

#endif