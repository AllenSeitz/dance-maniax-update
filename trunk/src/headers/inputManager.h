// inputManger.h declares a class that handles all types of user input
// file created by Allen Seitz 10/12/2009

#ifndef _INPUTMANAGER_H_
#define _INPUTMANAGER_H_

#include "common.h"

// key states for the panelStates array
#define HELD_UP 0
#define JUST_UP -1
#define HELD_DOWN 2
#define JUST_DOWN 1

// column IDs
#define LL_1P         0
#define UL_1P         1
#define UR_1P         2
#define LR_1P         3
#define MENU_LEFT_1P  10
#define MENU_RIGHT_1P 11
#define MENU_START_1P 14

#define LL_2P         4
#define UL_2P         5
#define UR_2P         6
#define LR_2P         7
#define MENU_LEFT_2P  12
#define MENU_RIGHT_2P 13
#define MENU_START_2P 15

#ifdef DMXDEBUG
	#define MENU_TEST 16
	#define MENU_SERVICE 17
#else // the j-pac is annoying
	#define MENU_TEST 17
	#define MENU_SERVICE 16
#endif
#define MENU_COIN 18

#define BLUE_SENSOR_1PL0 19
#define BLUE_SENSOR_1PL1 20
#define BLUE_SENSOR_1PR0 21
#define BLUE_SENSOR_1PR1 22
#define BLUE_SENSOR_2PL0 23
#define BLUE_SENSOR_2PL1 24
#define BLUE_SENSOR_2PR0 25
#define BLUE_SENSOR_2PR1 26

#define NUM_INPUTS 27

class InputManager
{
public:
	InputManager();

	void updateKeyStates(UTIME dt);
	// precondition: this is called every logic frame, regardless of game mode
	// postcondition: panelsDown and panelHoldLength are updated

	char getKeyState(int column);
	// precondition: column is [0..10], 0-9 = panels, 10 = start
	// postcondition: returns a panelState

	bool isKeyDown(int column);
	// precondition: updateKeyStates() has been called at least once
	// postcondition: returns true if the state is DOWN or JUST_DOWN

	bool isKeyInUse(int column);
	// precondition: isSolo and isDoubles are set
	// postcondition: returns true this column is used in the current mode

	int getHoldLength(int column);
	int getReleaseLength(int column);
	// precondition: updateKeyStates() has been called at least once
	// postcondition: returns a number of milliseconds, 0-30000 (capped)

	void setCooldownTime(int milliseconds) { m_cooldownThreshold = milliseconds; }
	// precondition: milliseconds should be [0..150] or bad things will happen
	// postcondition: if milliseconds > 0 then "ghosting" will be less of a problem

	void processInputFromExtio(unsigned char bits[3]);
	// precondition: both this class and the extioManager are initialized and ready
	// postcondition: panelsDown and panelHoldLength are updated
	// NOTE: the parameter is a specific array of 24 bits, directly from the IO board

	// these functions and bools are for reversing the on/off logic for a PPP controller
	bool isRedSensor(int i)
	{
		return i == UL_1P || i == UR_1P || i == UL_2P || i == UR_2P;
	}
	bool isBlueSensor(int i)
	{
		return i >= 19 && i <= 26; // BLUE_SENSOR_1PL0 = 19, BLUE_SENSOR_2PR1 = 26
	}
	bool reverseRedSensorPolarity; // a hack for PPP sensors
	bool reverseBlueSensorPolarity;

protected:
	char m_panelStates[NUM_INPUTS];
	int m_panelHoldLength[NUM_INPUTS];
	int m_panelReleaseLength[NUM_INPUTS];

	int m_cooldownThreshold; // needed to prevent input flickering (as infrareds are likely to do)

	// for the extioManager to set
	bool m_ioBoardStates[NUM_INPUTS];

	// {qw,az,sx} = 1P, arrow keys for 2P, enter for 'start', see source file
	int m_keyMapping[NUM_INPUTS];

	void processDanceManiaxBlueSensors();
	// precondition: called only from updateKeyStates()
	// postcondition: reads the inputs from BLUE_SENSOR_* and alters the state of inputs L?_#P
	// NOTE: when installed on a machine, inputs L?_#P are not supposed to be wired. Use the blue sensors.

	void processDanceManiaxBlueSensorHelper(int a, int b, int blue);
	// postcondition: 'a' and 'b' inputs each control 'blue'
	// example parameters: BLUE_SENSOR_1PL0, BLUE_SENSOR_1PL1, LL_1P

	int getShortestReleaseLength(int column);
	// precondition: updateKeyStates() has been called at least once
	// postcondition: for blue columns, returns the lower of the two hold lengths
};

#endif // end include guard