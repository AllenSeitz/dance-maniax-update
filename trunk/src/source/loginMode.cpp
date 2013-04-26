// loginMode.cpp implements the login/pass screen for Dance Maniax
// source file created 4/26/2012 by Allen Seitz

#include "common.h"
#include "bookManager.h"
#include "GameStateManager.h"
#include "inputManager.h"
#include "scoreManager.h"
#include "songwheelMode.h"

extern GameStateManager gs;
extern RenderingManager rm;
extern InputManager im;
extern ScoreManager sm;
extern EffectsManager em;
extern BookManager bm;

extern unsigned long int frameCounter;
extern unsigned long int totalGameTime;

extern UTIME timeRemaining;
extern void renderTimeRemaining(int xc, int yc);
extern void playTimeLowSFX(UTIME dt);


//////////////////////////////////////////////////////////////////////////////
// Assets
//////////////////////////////////////////////////////////////////////////////
extern BITMAP* m_menuBG;
extern BITMAP* m_mainModes;
extern BITMAP* m_mainPlayers;

extern BITMAP* m_startButton;
extern BITMAP* m_triangles;

BITMAP* m_loginSelect = NULL;
BITMAP* m_nameCursor[2][8];
BITMAP* m_cursorTopper = NULL;
BITMAP* m_nameHelp = NULL;
BITMAP* m_pinHelp = NULL;

//////////////////////////////////////////////////////////////////////////////
// Variables
//////////////////////////////////////////////////////////////////////////////
extern UTIME blinkTimer;
bool isUse[2] = { false, false };
bool isSkip[2] = { false, false };
bool isDone[2] = { false, false };

int  cursorPos[2] = {0,0};

UTIME waggleTimerP1 = 0; // wave the red sensors to pick a recent name
int  waggleIndexP1 = 0;
int  nameBounceP1 = 0;
UTIME waggleTimerP2 = 0;
int  waggleIndexP2 = 0;
int  nameBounceP2 = 0;

static char nameLetters[45] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!+.#@&~^";
static char prevNames[4][9] = { "", "", "", "" };
char tempNames[2][9] = { "", "" };
int pinNums[2][4] = { {-1,-1,-1,-1}, {-1,-1,-1,-1} };
int currentDigitP1 = 0;
int currentDigitP2 = 0;
char currentStatusP1 = 0; // see current login status enumeration below
char currentStatusP2 = 0;
UTIME messageTimer[2] = { 0, 0 };

const int MAX_CURSOR_POS = 44;
const int END_NAME = 43;
const int WAGGLE_TIME = 2750; // how long a waggle input takes (before it is considered not an accidental input)
const int WAGGLE_STEP = 500;
const int LOGIN_MESSAGE_TIME = 3000;
const int UNFOLDED_Y = 416;
#define LOGIN_SHADE_COLOR makeacol(124, 0, 120, 84)

// for the currentStatusP1 and P2 variables
#define LOGIN_CHOOSE_LOGIN 0
#define LOGIN_NAME_ENTRY 1
#define LOGIN_PIN_ENTRY 2
#define LOGIN_PIN_VERIFY 3
#define LOGIN_PIN_CORRECT 4
#define LOGIN_PIN_INCORRECT 5
#define LOGIN_WAITING 6
#define LOGIN_SYSTEM_ERROR 7

//////////////////////////////////////////////////////////////////////////////
// Functions
//////////////////////////////////////////////////////////////////////////////
void renderLoginLoop();
//
//

void renderPrompt(int x, bool isUse, bool isPass);
//
//

void endLoginMode();
//
//

void enterLetter(int player);
// precondition: player is 0-1, tempNames are initialized and null-terminated
// postcondition: uses cursorPosP1 or P2 to append a letter, or not

bool enterPinDigit(int player);
// precondition: player is 0-1, pinP1 and pinP2 are initialized to {-1,-1,-1,-1}
// postcondition: uses currentDigitP1 or P2 to append a digit
// returns: true if the pin is now complete, or false otherwise

void renderNameWithWhitespace(char string[9], int x, int y, int color);
// precondition: string[9] should be tempNames[]
// postcondition: just like renderNameString() but spaces are replaced with underscores

void renderNamePrompt(int x, char side);
//
//

void renderPinPrompt(int x, char side, bool isVerify);
//
//

void renderLoginMessage(int xcoord, char* string);
//
//

void saveRecentNames();
void loadRecentNames();
// precondition: none, but the file recent_players.bin is used
// postcondition: reads (if possible) or writes to that file up to 4 strings

int getIndexOfPrevName(char* name);
// precondition: name is 9 bytes long at most, prevNames[] are loaded
// postcondition: returns an index [3..0] where the name should be inserted


void firstLoginLoop()
{
	if ( m_loginSelect == NULL )
	{
		m_loginSelect = loadImage("DATA/menus/main_login_select.tga");
		m_cursorTopper = loadImage("DATA/menus/1p2p_topper.tga");
		m_nameHelp = loadImage("DATA/menus/name_help.tga");
		m_pinHelp = loadImage("DATA/menus/pin_help.tga");

		for ( int i = 0; i < 2; i++ )
		for ( int j = 0; j < 8; j++ )
		{
			char cursorFilename[] = "DATA/menus/1Pframe_0000.tga";
			cursorFilename[11] = i + '1';
			cursorFilename[22] = j + '0';
			m_nameCursor[i][j] = loadImage(cursorFilename);
		}
	}
	if ( m_startButton == NULL )
	{
		m_startButton = loadImage("DATA/menus/start.tga");
		m_triangles = loadImage("DATA/menus/triangles.tga");
	}

	blinkTimer = 0;
	timeRemaining = 10999; // time to choose login

	isUse[0] = isUse[1] = false;
	isSkip[0] = isSkip[1] = false;
	isDone[0] = isDone[1] = false;
	cursorPos[0] = END_NAME;//0;
	cursorPos[1] = END_NAME;//10;
	nameBounceP1 = nameBounceP2 = 0;
	messageTimer[0] = messageTimer[1] = 0;

	waggleTimerP1 = waggleIndexP1 = nameBounceP1 = 0;
	waggleTimerP2 = waggleIndexP2 = nameBounceP2 = 0;

	for ( int i = 0; i < 8; i++ )
	{
		tempNames[0][i] = tempNames[1][i] = 0;
		pinNums[0][i/2] = pinNums[1][i/2] = -1; // -1 for "no digit entered here", legal digits are 0-9
	}
	currentDigitP1 = currentDigitP2 = 0;
	currentStatusP1 = currentStatusP2 = LOGIN_CHOOSE_LOGIN;

	// ALWAYS do this so that players do not accidentally play as someone else
	sm.player[0].resetData();
	sm.player[1].resetData();
	sm.player[0].isLoggedIn = false;
	sm.player[1].isLoggedIn = false;
	memcpy(sm.player[0].displayName, "PLAYER 1", 8);
	memcpy(sm.player[1].displayName, "PLAYER 2", 8);

	loadRecentNames();
}

void mainLoginLoop(UTIME dt)
{
	blinkTimer += dt;
	blinkTimer %= 1000;
	if ( !gs.isEventMode )
	{
		SUBTRACT_TO_ZERO(timeRemaining, dt);
	}
	SUBTRACT_TO_ZERO(waggleTimerP1, dt);
	SUBTRACT_TO_ZERO(waggleTimerP2, dt);
	SUBTRACT_TO_ZERO(nameBounceP1, (int)dt);
	SUBTRACT_TO_ZERO(nameBounceP2, (int)dt);
	playTimeLowSFX(dt);

	// precalculate input - this is because the P2 controls also count as P1 controls during singles play
	bool isLeftPress[2], isRightPress[2], isStartPress[2], isLeftHeld[2], isRightHeld[2];
	isLeftPress[0] = im.getKeyState(MENU_LEFT_1P) == JUST_DOWN || (!gs.isVersus && im.getKeyState(MENU_LEFT_2P) == JUST_DOWN);
	isRightPress[0] = im.getKeyState(MENU_RIGHT_1P) == JUST_DOWN || (!gs.isVersus && im.getKeyState(MENU_RIGHT_2P) == JUST_DOWN);
	isStartPress[0] = im.getKeyState(MENU_START_1P) == JUST_DOWN || (!gs.isVersus && im.getKeyState(MENU_START_2P) == JUST_DOWN);
	isLeftPress[1] = gs.isVersus && im.getKeyState(MENU_LEFT_2P) == JUST_DOWN;
	isRightPress[1] = gs.isVersus && im.getKeyState(MENU_RIGHT_2P) == JUST_DOWN;
	isStartPress[1] = gs.isVersus && im.getKeyState(MENU_START_2P) == JUST_DOWN;
	isLeftHeld[0] = im.isKeyDown(MENU_LEFT_1P) || (!gs.isVersus && im.isKeyDown(MENU_LEFT_2P));
	isRightHeld[0] = im.isKeyDown(MENU_RIGHT_1P) || (!gs.isVersus && im.isKeyDown(MENU_RIGHT_2P));
	isLeftHeld[1] = gs.isVersus && im.isKeyDown(MENU_LEFT_2P);
	isRightHeld[1] = gs.isVersus && im.isKeyDown(MENU_RIGHT_2P);

	// debug testing
	if ( key[KEY_SPACE] )
	{
		timeRemaining = 5000;
	}

	// process input depending on the current sub-state of Login Mode
	if ( currentStatusP1 == LOGIN_CHOOSE_LOGIN ) // P1 and P2 must perform this step together
	{
		for ( int side = 0; side < 2; side++ )
		{
			if ( isLeftPress[side] )
			{
				isSkip[side] = false;
			}
			if ( isRightPress[side] )
			{
				isSkip[side] = true;
				em.playSample(SFX_NAME_ENTRY_BACKSPACE);
			}
			if ( isStartPress[side] )
			{
				isUse[side] = true;
				em.playSample(SFX_EAMUSE_DIAL);
			}
		}
	}
	else if ( currentStatusP1 == LOGIN_NAME_ENTRY ) // P1 and P2 must also pass name entry together
	{
		for ( int side = 0; side < 2; side++ )
		{
			if ( isUse[side] && !isDone[side] )
			{
				if ( isLeftPress[side] )
				{
					if ( isRightHeld[side] )
					{
						cursorPos[side] += 11; // left + right = down a row on the name entry screen
					}
					else
					{
						cursorPos[side] -= 1;
					}
					em.playSample(SFX_NAME_CURSOR_MOVE);
				}
				if ( isRightPress[side] )
				{
					if ( isLeftHeld[side] )
					{
						cursorPos[side] += 11; // left + right = down a row on the name entry screen
					}
					else
					{
						cursorPos[side] += 1;
					}
					em.playSample(SFX_NAME_CURSOR_MOVE);
				}
				if ( isStartPress[side] && isUse[side] && !isDone[side] )
				{
					if ( cursorPos[side] == END_NAME )
					{
						isDone[side] = true;
						if ( tempNames[side][0] == 0 )
						{
							isUse[side] = false; // entering a blank name = skip
						}
						em.playSample(SFX_START_BUTTON);
					}
					else
					{
						enterLetter(side);
					}
				}
			}
		}

		// P1 waggle controls
		bool hitAnyRedSensor = im.getKeyState(UL_1P) == JUST_DOWN || im.getKeyState(UR_1P) == JUST_DOWN;
		if ( !gs.isVersus )
		{
			hitAnyRedSensor = hitAnyRedSensor || im.getKeyState(UL_2P) == JUST_DOWN || im.getKeyState(UR_2P) == JUST_DOWN; // one player (center, doubles) can waggle anywhere
		}
		if ( hitAnyRedSensor && isUse[0] && !isDone[0] )
		{
			waggleTimerP1 += WAGGLE_STEP;
			nameBounceP1 = 100;
			if ( waggleTimerP1 > WAGGLE_TIME )
			{
				waggleTimerP1 = 0;
				memcpy_s(tempNames[0], 9, prevNames[waggleIndexP1], 9);
				waggleIndexP1 = waggleIndexP1 == 3 ? 0 : waggleIndexP1 + 1;
				cursorPos[0] = END_NAME;
				em.playSample(SFX_MENU_SORT);
			}
			else
			{
				em.playSample(SFX_NAME_CURSOR_WAGGLE);
			}
		}

		// P2 waggle controls are a litle easier
		if ( im.getKeyState(UL_2P) == JUST_DOWN || im.getKeyState(UR_2P) == JUST_DOWN ) // 2P waggle controls are separate from singles/doubles
		{
			if ( gs.isVersus && isUse[1] && !isDone[1] )
			{
				waggleTimerP2 += WAGGLE_STEP;
				nameBounceP2 = 100;
				if ( waggleTimerP2 > WAGGLE_TIME )
				{
					waggleTimerP2 = 0;
					memcpy_s(tempNames[1], 9, prevNames[waggleIndexP2], 9);
					waggleIndexP2 = waggleIndexP2 == 3 ? 0 : waggleIndexP2 + 1;
					cursorPos[1] = END_NAME;
					em.playSample(SFX_START_BUTTON);
				}
				else
				{
					em.playSample(SFX_NAME_CURSOR_WAGGLE);
				}
			}
		}

		cursorPos[0] = (cursorPos[0] + MAX_CURSOR_POS) % MAX_CURSOR_POS;
		cursorPos[1] = (cursorPos[1] + MAX_CURSOR_POS) % MAX_CURSOR_POS;
	}
	else // now P1 and P2 can be in different status
	{
		for ( int side = 0; side < 2; side++ )
		{
			int* pdigit = side == 0 ? &currentDigitP1 : &currentDigitP2;

			switch (side == 0 ? currentStatusP1 : currentStatusP2)
			{
			case LOGIN_PIN_ENTRY:
			case LOGIN_PIN_VERIFY:
				// scroll through pin digits
				if ( isLeftPress[side] && isUse[side] && !isDone[side] )
				{
					*pdigit = *pdigit == 0 ? 9 : *pdigit - 1;
					em.playSample(SFX_PIN_DIGIT);
				}
				if ( isRightPress[side] && isUse[side] && !isDone[side] )
				{
					*pdigit = *pdigit == 9 ? 0 : *pdigit + 1;
					em.playSample(SFX_PIN_DIGIT);
				}
				if ( isStartPress[side] && isUse[side] && !isDone[side] )
				{
					if ( enterPinDigit(side) )
					{
						isDone[side] = true;
					}
				}
				break;
			case LOGIN_PIN_CORRECT:
			case LOGIN_PIN_INCORRECT:
				// no input - just an animation
				break;
			case LOGIN_WAITING:
				break;
			}
		}
	}

	// do logic - check for the end of the mode
	if ( currentStatusP1 == 0 )
	{
		if ( timeRemaining <= 0 )
		{
			isSkip[0] = !isUse[0];
			isSkip[1] = !isUse[1];
		}

		bool isDecidedP1 = isUse[0] || isSkip[0];
		bool isDecidedP2 = isUse[1] || isSkip[1] || !gs.isVersus;

		if ( isDecidedP1 && isDecidedP2 )
		{
			timeRemaining = 60000;
			if ( isUse[0] || isUse[1] )
			{
				currentStatusP1 = currentStatusP2 = LOGIN_NAME_ENTRY;
			}
			else
			{
				endLoginMode();
			}
		}
	}
	else if ( currentStatusP1 == 1 ) // currently in name entry mode (with the alphabet visible)
	{
		if ( ((isUse[0] && isDone[0]) || !isUse[0]) && ((isUse[1] && isDone[1]) || !isUse[1]) ) // if each player that is in use, is done
		{
			if ( tempNames[0][0] == 0 || !isUse[0] )
			{
				isUse[0] = false; // P1 entered a blank name - treat it as a pass
				sm.player[0].resetData();
				currentStatusP1 = LOGIN_WAITING;
			}
			else
			{
				if ( sm.doesPlayerNameExist(tempNames[0]) )
				{
					if ( !sm.loadPlayerFromDisk(tempNames[0], 0) )
					{
						currentStatusP1 = LOGIN_SYSTEM_ERROR;
						em.playSample(SFX_FAILURE_ANIMATION);
					}
				}
				else
				{
					memcpy(sm.player[0].displayName, tempNames[0], 8);
				}
				currentStatusP1 = LOGIN_PIN_ENTRY;
			}

			if ( tempNames[1][0] == 0 || !isUse[1] )
			{
				isUse[1] = false; // P2 entered a blank name - treat it as a pass
				sm.player[1].resetData();
				currentStatusP2 = LOGIN_WAITING;
			}
			else
			{
				if ( sm.doesPlayerNameExist(tempNames[1]) )
				{
					if ( !sm.loadPlayerFromDisk(tempNames[1], 1) )
					{
						currentStatusP2 = LOGIN_SYSTEM_ERROR;
						em.playSample(SFX_FAILURE_ANIMATION);
					}
				}
				else
				{
					memcpy(sm.player[1].displayName, tempNames[1], 8);
				}
				currentStatusP2 = LOGIN_PIN_ENTRY;
			}

			isDone[0] = isDone[1] = false;
			if ( !isUse[0] && !isUse[1] )
			{
				endLoginMode();
			}		
		}
	}
	else // after the name entry screen P1 and P2 can each be doing their own thing
	{
		for ( int side = 0; side < 2; side++ )
		{
			char* pstatus = side == 0 ? &currentStatusP1 : &currentStatusP2;

			switch (*pstatus)
			{
			case LOGIN_PIN_ENTRY:
				if ( isDone[side] )
				{
					if ( sm.doesPlayerNameExist(tempNames[side]) )
					{
						if ( sm.isPinCorrect((pinNums[side][0]*1000)+(pinNums[side][1]*100)+(pinNums[side][2]*10)+(pinNums[side][3]), side) )
						{
							*pstatus = LOGIN_PIN_CORRECT;
							em.playSample(SFX_COURSE_ENTRY_CONFIRM);
							em.announcerQuipChance(GUY_LOGIN, 100);
						}
						else
						{
							*pstatus = LOGIN_PIN_INCORRECT;
							isDone[side] = false;
							em.playSample(SFX_FAILURE_ANIMATION);
						}
						messageTimer[side] = LOGIN_MESSAGE_TIME;
					}
					else
					{
						if ( pinNums[side][0] == pinNums[side][1] && pinNums[side][0] == pinNums[side][2] && pinNums[side][0] == pinNums[side][3] )
						{
							*pstatus = LOGIN_PIN_INCORRECT; // disallow pin numbers 0000, 1111, 2222, .... 9999
							isDone[side] = false;
							em.playSample(SFX_FAILURE_ANIMATION);
							messageTimer[side] = LOGIN_MESSAGE_TIME;
						}
						else
						{
							*pstatus = LOGIN_PIN_VERIFY;
							for ( int i = 0; i < 4; i++ )
							{
								sm.player[side].pinDigits[i] = (char)pinNums[side][i]; // now the verify loop will check against what you've entered
								pinNums[side][i] = -1; // erase them and retype them
							}
						}
					}
				}
				isDone[side] = false;
				break;
			case LOGIN_PIN_VERIFY:
				if ( isDone[side] )
				{
					if ( sm.isPinCorrect((pinNums[side][0]*1000)+(pinNums[side][1]*100)+(pinNums[side][2]*10)+(pinNums[side][3]), side) )
					{
						sm.player[side].isLoggedIn = true; // hooray! create a new save file
						*pstatus = LOGIN_PIN_CORRECT;
						em.playSample(SFX_CROWD_CHEERING);
						em.announcerQuip(GUY_EVERYBODY_WAITING);
					}
					else
					{
						*pstatus = LOGIN_PIN_INCORRECT;
						isDone[side] = false;
						em.playSample(SFX_FAILURE_ANIMATION);
					}
					messageTimer[side] = LOGIN_MESSAGE_TIME;
				}
				break;
			case LOGIN_PIN_CORRECT:
				SUBTRACT_TO_ZERO(messageTimer[side], dt);
				if ( messageTimer[side] <= 0 )
				{
					*pstatus = LOGIN_WAITING;
				}
				break;
			case LOGIN_PIN_INCORRECT:
				SUBTRACT_TO_ZERO(messageTimer[side], dt);
				if ( messageTimer[side] <= 0 )
				{
					*pstatus = LOGIN_PIN_ENTRY;
					for ( int i = 0; i < 4; i++ )
					{
						pinNums[side][i] = -1;
					}
				}
				break;
			case LOGIN_WAITING:
				// intentionally do nothing
				break;
			}
		}

		// are we done with the login mode entirely?
		if ( (!isUse[0] || currentStatusP1 == LOGIN_WAITING) && (!isUse[1] || currentStatusP2 == LOGIN_WAITING) )
		{
			endLoginMode();
		}
	}

	// ran out of time while entering the name or pin
	if ( timeRemaining <= 0 && currentStatusP1 >= LOGIN_NAME_ENTRY ) // this is safe even when P1 isn't using the login feature
	{
		if ( (isUse[0] && currentStatusP1 < LOGIN_PIN_CORRECT) || (isUse[1] && currentStatusP2 < LOGIN_PIN_CORRECT) )
		{
			gs.g_currentGameMode = ATTRACT; // I'll be nice and give you another shot
			gs.g_gameModeTransition = 1;
		}
	}

	renderLoginLoop();
}

void renderLoginLoop()
{
	int color = 0, sx = 0, i = 0;

	blit(m_menuBG, rm.m_backbuf, 0, 0, 0, 0, 640, 480);
	draw_trans_sprite(rm.m_backbuf, m_loginSelect, 287, 45); 

	rectfill(rm.m_backbuf, 0, 83, 640, 83+33, makeacol(0, 0, 0, 255));
	rectfill(rm.m_backbuf, 0, UNFOLDED_Y, 640, UNFOLDED_Y+44, makeacol(208, 89, 160, 255));
	rectfill(rm.m_backbuf, 0, 118, 640, UNFOLDED_Y-1, makeacol(255, 170, 200, 200));

	// render SINGLE DOUBLE VERSUS
	for ( i = 0; i < 3; i++ )
	{
		color = 2;
		if ( (i == 1 && gs.isDoubles) || (i == 2 && gs.isVersus) || (i == 0 && !gs.isVersus && !gs.isDoubles) )
		{
			color = 1;
		}
		sx = color * 114;
		masked_blit(m_mainPlayers, rm.m_backbuf, sx, i*32, i*114 + (i+1)*74, 84, 114, 32);
	}

	// render GAME MODE
	if ( gs.currentGameType == 2 )
	{
		masked_blit(m_mainModes, rm.m_backbuf, 210, 2*32, 215, UNFOLDED_Y-1, 210, 32);
		renderArtistString("6 challenge tokens", 222, UNFOLDED_Y+25, 200, 32);
	}
	else
	{
		char numStagesDesc[] = "3 stages + ext";
		numStagesDesc[0] = gs.numSongsPerSet + '0';
		sx = 420;
		//if ( gs.currentGameType == 0 ) // login BEFORE mode selection now
		//{
			sx = 210;
		//}
		masked_blit(m_mainModes, rm.m_backbuf, sx, 0*32,  40, UNFOLDED_Y-1, 210, 32);
		renderArtistString(numStagesDesc, 74, UNFOLDED_Y+25, 200, 32);

		sx = 420;
		//if ( gs.currentGameType == 1 ) // login BEFORE mode selection now
		//{
			sx = 210;
		//}
		masked_blit(m_mainModes, rm.m_backbuf, sx, 1*32, 390, UNFOLDED_Y-1, 210, 32);
		renderArtistString(numStagesDesc, 424, UNFOLDED_Y+25, 200, 32);
	}

	// RENDER LOWER LOGIN AREA
	if ( currentStatusP1 == LOGIN_CHOOSE_LOGIN )
	{
		renderBoldString("Press start to enter your name.", 140, 180, 400, false);
		renderArtistString("Entering your name allows you", 160, 215, 360, 20);
		renderArtistString("to save high scores.", 160, 235, 360, 20);

		if ( gs.isVersus )
		{
			renderPrompt(118, isUse[0], isSkip[0]);
			renderPrompt(380, isUse[1], isSkip[1]);
		}
		else
		{
			renderPrompt(248, isUse[0], isSkip[0]);
		}
		masked_blit(m_triangles, rm.m_backbuf, 32, 0, 246, 370, 32, 32);
		renderArtistString("to skip", 318, 375, 200, 32);
	}
	// render the alphabet - enter your name (10/20 - no longer displays an alphabet)
	else if ( currentStatusP1 == LOGIN_NAME_ENTRY )
	{
		int xcoord[2] = { 136, 388 };
		if ( !gs.isVersus )
		{
			xcoord[0] = 263; // center the menu for center and double play
		}
		for ( int side = 0; side < 2; side++ )
		{
			if ( isUse[side] )
			{
				renderNamePrompt(xcoord[side], side);
			}
		}

		draw_trans_sprite(rm.m_backbuf, m_nameHelp, 0, 384); 

		renderNameWithWhitespace(tempNames[0], 32, 128-(nameBounceP1/10), 1);
		if ( gs.isVersus )
		{
			renderNameWithWhitespace(tempNames[1], 352, 128-(nameBounceP2/10), 2);
		}

		// render the last 5 people who have logged in
		rectfill(rm.m_backbuf, 4, 190, 4+100, 190+150, LOGIN_SHADE_COLOR);
		renderBoldString("recent", 10, 230-37, 80, false);
		renderBoldString("visitors", 10, 260-37, 80, false);
		for ( i = 0; i < 4; i++ )
		{
			renderArtistString(prevNames[i], 10, 290-37 + 20*i, 128, 32);
		}
	}
	// render each player's modes separately from this point on
	else
	{
		int xcoord[2] = { 136, 388 };
		if ( !gs.isVersus )
		{
			xcoord[0] = 263; // center the menu for center and double play
		}

		draw_trans_sprite(rm.m_backbuf, m_pinHelp, 0, 384); 

		// render the names up top
		renderNameWithWhitespace(tempNames[0], 32, 128-(nameBounceP1/10), 1);
		if ( gs.isVersus )
		{
			renderNameWithWhitespace(tempNames[1], 352, 128-(nameBounceP2/10), 2);
		}

		for ( int side = 0; side < 2; side++ )
		{
			char* pstatus = side == 0 ? &currentStatusP1 : &currentStatusP2;
			if ( isUse[side] )
			{
				switch ( *pstatus )
				{
				case LOGIN_PIN_ENTRY:
				case LOGIN_PIN_VERIFY:
					renderPinPrompt(xcoord[side], side, *pstatus == LOGIN_PIN_VERIFY);
					break;
				case LOGIN_PIN_CORRECT:
					renderLoginMessage(xcoord[side], "LOGIN ACCEPTED");
					break;
				case LOGIN_PIN_INCORRECT:
					if ( pinNums[side][0] == pinNums[side][1] && pinNums[side][0] == pinNums[side][2] && pinNums[side][0] == pinNums[side][3] )
					{
						renderLoginMessage(xcoord[side], "PIN# DISALLOWED");
					}
					else
					{
						renderLoginMessage(xcoord[side], "INCORRECT LOGIN");
					}
					break;
				case LOGIN_SYSTEM_ERROR:
					// intentional no break
				case LOGIN_WAITING:
					renderLoginMessage(xcoord[side], "WAIT...");
					break;
				}
			}
		}
	}

	renderTimeRemaining();
	rm.flip();
}

void endLoginMode()
{
	gs.g_currentGameMode = MAINMENU;
	gs.g_gameModeTransition = 1;

	// count the number of credits played by either player
	for ( int side = 0; side < 2; side++ )
	{
		sm.player[side].isLoggedIn = false;
		if ( isUse[side] )
		{
			sm.player[side].isLoggedIn = true;
		}
	}

	// update and save the most recent player names
	for ( int player = 0; player < 2; player++ )
	{
		if ( isUse[player] )
		{
			for ( int i = getIndexOfPrevName(tempNames[player]); i > 0; i-- )
			{
				memcpy_s(prevNames[i], 9, prevNames[i-1], 9);
			}
			memcpy_s(prevNames[0], 9, tempNames[player], 9);
		}
	}
	saveRecentNames();
}

void renderPrompt(int x, bool isUse, bool isPass)
{
	int yoff = blinkTimer < 500 ? blinkTimer / 50 : 10 - ((blinkTimer-500) / 50);

	if ( isUse )
	{
		renderBoldString("  LOG IN!  ", x, 280+5, 400, false);
	}
	else if  ( isPass )
	{
		renderBoldString("  pass...  ", x, 280+5, 400, false);
	}
	else
	{
		renderBoldString("PRESS START", x, 280+yoff, 400, false);
		masked_blit(m_startButton, rm.m_backbuf, 0, 0, x, 310+yoff, 45, 32);
		renderTimeRemaining(x+70, 310+yoff);
	}
}

void renderNamePrompt(int x, char side)
{
	int y = 190;
	int frame = (blinkTimer/150) % 2;

	rectfill(rm.m_backbuf, x-10, y, x-10+220, y+150, LOGIN_SHADE_COLOR);

	renderBoldString("ENTER YOUR NAME", x, y+10, 400, false);
	if ( tempNames[side][0] == 0 )
	{
		renderBoldString("(or END to cancel)", x, y+40, 400, false);
	}
	else if ( sm.doesPlayerNameExist(tempNames[side]) )
	{
		renderBoldString("RETURNING PLAYER", x, y+40, 400, false, frame == 0 ? 1 : 2);
	}
	else
	{
		renderBoldString("NEW PLAYER", x, y+40, 400, false, 1);
	}

	bool isLeftMenuPressed = side == 0 ? im.isKeyDown(MENU_LEFT_1P) : im.isKeyDown(MENU_LEFT_2P);
	bool isRightMenuPressed = side == 0 ? im.isKeyDown(MENU_RIGHT_1P) : im.isKeyDown(MENU_RIGHT_2P);
	masked_blit(m_triangles, rm.m_backbuf, 0, (char)isLeftMenuPressed * 32, x, y+80, 32, 32);
	masked_blit(m_triangles, rm.m_backbuf, 32, (char)isRightMenuPressed * 32, x+98, y+80, 32, 32);

	// render current pin digit
	renderNameLetter(nameLetters[cursorPos[side]], x + 48, y+80, 0);

	// render whatever the player entered for a name
	//renderNameWithWhitespace(tempNames[side], x-64+1, 180, side+1);
}

void renderPinPrompt(int x, char side, bool isVerify)
{
	int y = 190;

	rectfill(rm.m_backbuf, x-10, y, x-10+220, y+150, LOGIN_SHADE_COLOR);

	if ( sm.doesPlayerNameExist(tempNames[side]) )
	{
		renderBoldString("RETURNING PLAYER", x, y+10, 400, false);
		renderBoldString("PIN ENTRY", x, y+40, 400, false);
	}
	else
	{
		if ( isVerify )
		{
			//renderBoldString("NEW PLAYER", x, y+10, 400, false);
			renderBoldString("VERIFY PIN", x+4, y+40, 400, false);
		}
		else
		{
			renderBoldString("NEW PLAYER", x, y+10, 400, false);
			renderBoldString("CREATE PIN", x+4, y+40, 400, false);
		}
	}

	bool isLeftMenuPressed = side == 0 ? im.isKeyDown(MENU_LEFT_1P) : im.isKeyDown(MENU_LEFT_2P);
	bool isRightMenuPressed = side == 0 ? im.isKeyDown(MENU_RIGHT_1P) : im.isKeyDown(MENU_RIGHT_2P);
	masked_blit(m_triangles, rm.m_backbuf, 0, (char)isLeftMenuPressed * 32, x, y+80, 32, 32);
	masked_blit(m_triangles, rm.m_backbuf, 32, (char)isRightMenuPressed * 32, x+98, y+80, 32, 32);

	// render current pin digit
	renderNameLetter((side == 0 ? currentDigitP1 : currentDigitP2) + '0', x + 48, y+80, 0);

	// render an asterisk for every entered pin digit
	for ( int i = 0; i < 4; i++ )
	{
		char num = pinNums[side][i] + '0';
		num = '*';
		if ( pinNums[side][i] != -1 )
		{
			renderNameLetter(num, x+1+(i*32), y+120, side+1);
		}
		else
		{
			renderNameLetter('_', x+1+(i*32), y+120, side+1);
		}
	}

	// render whatever the player entered for a name
	//renderNameWithWhitespace(tempNames[side], x-64+1, 180, side+1);
}

void renderLoginMessage(int xcoord, char* string)
{
	renderBoldString(string, xcoord, 240, 400, false);
}

void enterLetter(int player)
{
	int i = 0;
	for ( i = 0; i < 7; i++ )
	{
		if ( tempNames[player][i] == 0 )
		{
			break;
		}
	}

	char letter = nameLetters[cursorPos[player]];
	if ( letter == '~' )
	{
		if ( i == 0 || (i== 7 && tempNames[player][i] != 0) )
		{
			tempNames[player][i] = 0; // backspace works differently on the 1st and last letters
		}
		else
		{
			tempNames[player][i-1] = 0;
		}
		em.playSample(SFX_NAME_ENTRY_BACKSPACE);
	}
	else
	{
		tempNames[player][i] = letter; // the check for END is done before enterLetter() is called
		em.playSample(SFX_MENU_PICK2);
	}
}

bool enterPinDigit(int player)
{
	int i = 0;
	for ( i = 0; i < 4; i++ )
	{
		if ( pinNums[player][i] == -1 )
		{
			pinNums[player][i] = player == 0 ? currentDigitP1 : currentDigitP2;
			break;
		}
	}
	em.playSample(SFX_PIN_DIGIT);
	return i >= 3;
}

void renderNameWithWhitespace(char string[9], int x, int y, int color)
{
	for ( int i = 0; i < 8; i++ )
	{
		if ( string[i] == 0 )
		{
			renderNameLetter('_', x, y, color);
		}
		else
		{
			renderNameLetter(string[i], x, y, color);
		}
		x += 32;
	}
}

void saveRecentNames()
{
	FILE* fp = NULL;
	if ( fopen_s(&fp, "conf/recent_players.bin", "wb") != 0 )
	{
		return;
	}

	for ( int i = 0; i < 4; i++ )
	{
		fwrite(&prevNames[i], sizeof(char), 9, fp);
	}
	fclose(fp);
}

void resetPrevNames()
{
	prevNames[0][0] = 0;
	prevNames[1][0] = 0;
	prevNames[2][0] = 0;
	prevNames[3][0] = 0;
	saveRecentNames();
}

void loadRecentNames()
{
	FILE* fp = NULL;
	if ( fopen_s(&fp, "conf/recent_players.bin", "rb") != 0 )
	{
		return;
	}

	for ( int i = 0; i < 4; i++ )
	{
		if ( fread_s(&prevNames[i], 9, sizeof(char), 9, fp) < 9 )
		{
			// doesn't matter
		}
	}
	fclose(fp);
}

int getIndexOfPrevName(char* name)
{
	int i = 0;
	for ( i = 0; i < 3; i++ )
	{
		if ( _stricmp(prevNames[i], name) == 0 )
		{
			return i;
		}
	}
	return i;
}