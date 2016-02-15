// inputManger.cpp implements a class that handles all types of user input
// file created by Allen Seitz 10/12/2009

#include "inputManager.h"
#include "gameStateManager.h"

extern GameStateManager gs;

// notice what these keybinds accomplish:
// 1P = {q,w} {az,sx} which mimicks the shape of the sensors on the machine
// 2P = play DDR and doubles with the arrow keys, because the 'special' blues are mapped
// while testing singles play, space bar makes triples and quads easier
// mapping the triangles to {g,h} is a convention that started on other 573 hacks
// test/service/coin are just whatever
static int defaultKeyMapping[NUM_INPUTS] = { KEY_SPACE, KEY_Q, KEY_W, KEY_SPACE,
									         KEY_SPACE, KEY_DOWN, KEY_UP, KEY_SPACE, 
											 KEY_E, KEY_D, // solo is probably not making a comeback
											 KEY_G, KEY_H, KEY_B, KEY_N, KEY_QUOTE, KEY_ENTER_PAD,
											 KEY_F1, KEY_F2, KEY_7,
											 KEY_A, KEY_Z, KEY_S, KEY_X, // 1P blue sensors
											 KEY_LEFT, KEY_C, KEY_RIGHT, KEY_V  // 2P blue sensors
};

// the minimaid has a keyboard mode which uses these alternate mappings that make sense for a JAMMA harness
static int minimaidKeyMapping[NUM_INPUTS] = { KEY_SPACE, KEY_S, KEY_U, KEY_SPACE, // 1P RED
									         KEY_SPACE, KEY_T, KEY_V, KEY_SPACE, // 2P RED
											 KEY_SPACE, KEY_SPACE, // unused
											 KEY_K, KEY_M, KEY_L, KEY_N, KEY_I, KEY_J, // menu buttons
											 KEY_B, KEY_A, KEY_D, // test service coin
											 KEY_Y, KEY_O, KEY_Q, KEY_1, // 1P blue sensors
											 KEY_P, KEY_Z, KEY_R, KEY_2  // 2P blue sensors
};

InputManager::InputManager()
{
	int i = 0;

	for ( i = 0; i < NUM_INPUTS; i++ )
	{
		m_panelStates[i] = HELD_UP;
		m_panelHoldLength[i] = 0;
		m_panelReleaseLength[i] = 0;
		m_ioBoardStates[i] = false;
	}

	for ( i = 0; i < NUM_INPUTS; i++ )
	{
		m_keyMapping[i] = defaultKeyMapping[i];
	}

	reverseRedSensorPolarity = false;
	reverseBlueSensorPolarity = false;
}

void InputManager::updateKeyStates(UTIME dt)
{	
	for ( int i = 0; i < NUM_INPUTS; i++ )
	{
		bool thisKeyIsPressed = key[m_keyMapping[i]] != 0 || m_ioBoardStates[i];

		// hax for a PPP sensor
		if ( (reverseRedSensorPolarity && isRedSensor(i)) || (reverseBlueSensorPolarity && isBlueSensor(i)) )
		{
			thisKeyIsPressed = !thisKeyIsPressed;
		}

		if ( thisKeyIsPressed || (i == MENU_START_1P && key[KEY_ENTER]) || (i == MENU_START_2P && key[KEY_SEMICOLON]) ) // keyboard testing requires alternate start button
		{
			if ( m_panelStates[i] < 1 ) // if the key was up
			{
				m_panelStates[i] = JUST_DOWN;
				m_panelHoldLength[i] = dt;
				m_panelReleaseLength[i] = 0;
			}
			else // key stayed held down
			{
				m_panelStates[i] = HELD_DOWN;
				m_panelHoldLength[i] = m_panelHoldLength[i] + dt > 30000 ? 30000 : m_panelHoldLength[i] + dt;
			}
		}
		else
		{
			if ( m_panelStates[i] > 0 ) // if the key was down
			{
				m_panelStates[i] = JUST_UP;
				m_panelHoldLength[i] = 0;
				m_panelReleaseLength[i] = dt;
			}
			else // key stayed 'held' up
			{
				m_panelStates[i] = HELD_UP;
				m_panelReleaseLength[i] = m_panelReleaseLength[i] + dt > 30000 ? 30000 : m_panelReleaseLength[i] + dt;
			}
		}
	}

	processDanceManiaxBlueSensors();
}

char InputManager::getKeyState(int column)
{
	ASSERT(column < NUM_INPUTS && column >= 0);

	// implement the cooldown timer here and in isKeyDown()
	if ( m_cooldownThreshold > 0 && column < 8 )
	{
		if ( getReleaseLength(column) > 0 && getReleaseLength(column) < m_cooldownThreshold )
		{
			return HELD_DOWN;
		}
	}

	return m_panelStates[column];
}

bool InputManager::isKeyDown(int column)
{
	ASSERT(column < NUM_INPUTS && column >= 0);

	// implement the cooldown timer here and in getKeyState()
	if ( m_cooldownThreshold > 0 && column < 8 )
	{
		if ( getShortestReleaseLength(column) > 0 && getShortestReleaseLength(column) < m_cooldownThreshold )
		{
			return HELD_DOWN;
		}
	}

	return m_panelStates[column] > 0;
}

// I think this function is only used for making sure that only active panels trigger shock arrows
bool InputManager::isKeyInUse(int column)
{
	ASSERT(column < NUM_INPUTS && column >= 0);

	if ( gs.isSolo && !gs.isDoubles )
	{
		return column < 6;
	}
	else if ( !gs.isSolo && gs.isDoubles )
	{
		return column != 1 && column != 4;
	}
	else if ( gs.isSolo && gs.isDoubles )
	{
		return true;
	}
	return column >= 6; // singles play assumes 2P
}

int InputManager::getHoldLength(int column)
{
	ASSERT(column < NUM_INPUTS && column >= 0);
	return m_panelHoldLength[column];
}

int InputManager::getReleaseLength(int column)
{
	ASSERT(column < NUM_INPUTS && column >= 0);
	return m_panelReleaseLength[column];
}

void InputManager::processInputFromExtio(unsigned char bytes[3])
{
	const int pins0[] = { MENU_TEST, MENU_SERVICE, MENU_START_1P, MENU_LEFT_1P, MENU_RIGHT_1P, MENU_START_2P, MENU_LEFT_2P, MENU_RIGHT_2P };
	const int pins1[] = { UL_1P, UR_1P, UL_2P, UR_2P, -1, -1, BLUE_SENSOR_1PL0, BLUE_SENSOR_1PL1 };
	const int pins2[] = { BLUE_SENSOR_1PR0, BLUE_SENSOR_1PR1, BLUE_SENSOR_2PL0, BLUE_SENSOR_2PL1, BLUE_SENSOR_2PR0, BLUE_SENSOR_2PR1, MENU_COIN, -2 };

	// check for an error
	if (bytes[0] == 'B' && bytes[1] == 'A' && bytes[2] == 'D' )
	{
		// according to my calculations, this can happen when the following inputs are pressed and NO others:
		// service + MENU_LEFT_2P + UL_1P + BLUE_L0_1P + BLUE_L0_2P + coin
		globalError(EXTIO_ERROR, "PLEASE WAIT FOR REBOOT");
		return;
	}

	//al_trace("GIVEN: %d %d %d\n", bytes[0], bytes[1], bytes[2]);
	for ( int i = 0; i < 8; i++ )
	{
		if ( pins0[i] > 0 )
		{
			m_ioBoardStates[pins0[i]] = bytes[0] & (1 << i) ? false : true;
		}
		if ( pins1[i] > 0 )
		{
			m_ioBoardStates[pins1[i]] = bytes[1] & (1 << i) ? false : true;
		}
		if ( pins2[i] > 0 )
		{
			m_ioBoardStates[pins2[i]] = bytes[2] & (1 << i) ? false : true;
		}
	}
}

void InputManager::switchToMinimaidKeys()
{
	for ( int i = 0; i < NUM_INPUTS; i++ )
	{
		m_keyMapping[i] = minimaidKeyMapping[i];
	}
}

void InputManager::processDanceManiaxBlueSensors()
{
	processDanceManiaxBlueSensorHelper(BLUE_SENSOR_1PL0, BLUE_SENSOR_1PL1, LL_1P);
	processDanceManiaxBlueSensorHelper(BLUE_SENSOR_1PR0, BLUE_SENSOR_1PR1, LR_1P);
	processDanceManiaxBlueSensorHelper(BLUE_SENSOR_2PL0, BLUE_SENSOR_2PL1, LL_2P);
	processDanceManiaxBlueSensorHelper(BLUE_SENSOR_2PR0, BLUE_SENSOR_2PR1, LR_2P);
}

void InputManager::processDanceManiaxBlueSensorHelper(int a, int b, int blue)
{
	if ( m_panelStates[a] == HELD_DOWN || m_panelStates[b] == HELD_DOWN )
	{
		m_panelStates[blue] = HELD_DOWN;
		m_panelHoldLength[blue] = MAX(m_panelHoldLength[a], m_panelHoldLength[b]);
		m_panelReleaseLength[blue] = 0;
	}
	else if ( m_panelStates[a] == JUST_DOWN || m_panelStates[b] == JUST_DOWN )
	{
		m_panelStates[blue] = JUST_DOWN;
		m_panelHoldLength[blue] = MAX(m_panelHoldLength[a], m_panelHoldLength[b]);
		m_panelReleaseLength[blue] = 0;
	}
	else
	{
		if ( m_panelStates[blue] > 1 ) // if the blue column was 'down' before
		{
			m_panelStates[blue] = JUST_UP;
			m_panelHoldLength[blue] = 0;
			m_panelReleaseLength[blue] = MIN(m_panelReleaseLength[a], m_panelReleaseLength[b]);
		}
		else // this blue column was already open
		{
			m_panelStates[blue] = HELD_UP;
			m_panelHoldLength[blue] = 0;
			m_panelReleaseLength[blue] = MIN(m_panelReleaseLength[a], m_panelReleaseLength[b]);
		}
	}
}

int InputManager::getShortestReleaseLength(int column)
{
	switch (column)
	{
	case LL_1P: return MIN(m_panelReleaseLength[BLUE_SENSOR_1PL0], m_panelReleaseLength[BLUE_SENSOR_1PL1]);
	case LR_1P: return MIN(m_panelReleaseLength[BLUE_SENSOR_1PR0], m_panelReleaseLength[BLUE_SENSOR_1PR1]);
	case LL_2P: return MIN(m_panelReleaseLength[BLUE_SENSOR_2PL0], m_panelReleaseLength[BLUE_SENSOR_2PL1]);
	case LR_2P: return MIN(m_panelReleaseLength[BLUE_SENSOR_2PR0], m_panelReleaseLength[BLUE_SENSOR_2PR1]);
	}
	return getReleaseLength(column);
}
