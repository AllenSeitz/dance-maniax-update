// menuMode.cpp implements the mode select screen
// source file created by Allen Seitz 2/8/2012

#include "common.h"
#include "GameStateManager.h"
#include "inputManager.h"
#include "bookManager.h"
#include "lightsManager.h"
#include "scoreManager.h"
#include "songwheelMode.h"

extern GameStateManager gs;
extern RenderingManager rm;
extern InputManager im;
extern EffectsManager em;
extern LightsManager lm;
extern ScoreManager sm;
extern BookManager bm;

extern unsigned long int frameCounter;
extern unsigned long int totalGameTime;

extern int NUM_SONGS;
extern int NUM_COURSES; 

extern UTIME timeRemaining;
extern void renderTimeRemaining(int xc, int yc);
extern void playTimeLowSFX(UTIME dt);


//////////////////////////////////////////////////////////////////////////////
// Assets
//////////////////////////////////////////////////////////////////////////////
BITMAP* m_menuBG = NULL;
BITMAP* m_displayMachine[3];
BITMAP* m_modeSelect;
BITMAP* m_mainModes;
BITMAP* m_mainPlayers;
BITMAP* m_modRect;
BITMAP* m_modHeadings;
BITMAP* m_mods[4];
BITMAP* m_hazard;

BITMAP* m_startButton;
BITMAP* m_triangles;


//////////////////////////////////////////////////////////////////////////////
// Variables
//////////////////////////////////////////////////////////////////////////////
int currentRow = 0; // 0 = players, 1 = mode, 2-5 = modifiers, 6 = done/wait
int p1row = 0;
int p2row = 0; // they can separate in versus play as long as each is > 1
int playersChoice = 0; // 0 = singles, 1 = doubles, 2 = versus
int modeChoice = 1; // 0 = course select, 1 = music select, 2 = special
int mods[2][4];

UTIME blinkTimer = 0;
UTIME introTimer = 0;
UTIME modeMovingTimer = 0;
int modesY = 118;
int modesDirection = 0; // -1 = up, 1 = down, 0 = stationary

int hazardCount[2] = { 0, 0 }; // for the hidden hazard code

const int MOD_LIMITS[4] = { 6, 2, 2, 2 }; // how many options are in each row
const int SPEED_MOD_EFFECTS[10] = { 10, 15, 20, 25, 30, 50, 80, 5, 3 };

const int MODE_Y_TOP = 118;
const int MODE_Y_LOW = 416;
const int MODE_UNFOLD_TIME = 500;

// these are used to render the blinking words - they reference x-coordinates in each asset
const int MOD_BREAKS[4][8] = 
{
	{ 0, 40, 80, 122, 164, 206, 248, 290 },
	{ 0, 80, 215, 290 },
	{ 0, 51, 145, 290 },
	//{ 0, 41, 120, 201, 290 } // off hidden sudden stealth
	{ 0, 128, 226, 290 }, // center left right
};

//////////////////////////////////////////////////////////////////////////////
// Functions
//////////////////////////////////////////////////////////////////////////////
void renderMenuLoop();
// precondition: called only once per frame after the logic is complete
// postcondition: rm.m_backbuf contains an image of the current game state

void renderMods(int x, int y, int which, int selection, int currentRow);
// precondition: which and currentRow are 0-3, selection is within range for this mod
// postcondition: renders a bunch of stuff to the lower area

void endMenuMode();
// postcondition: calls for bookkeeping updates and sets up the next game mode

void firstMenuLoop()
{
	// load assets
	if ( m_menuBG == NULL )
	{
		m_menuBG = loadImage("DATA/menus/bg_mode.tga");
		m_displayMachine[0] = loadImage("DATA/menus/disp_singles.tga");
		m_displayMachine[1] = loadImage("DATA/menus/disp_doubles.tga");
		m_displayMachine[2] = loadImage("DATA/menus/disp_versus.tga");
		m_modeSelect = loadImage("DATA/menus/main_mode_select.tga");
		m_mainModes = loadImage("DATA/menus/main_modes.tga");
		m_mainPlayers = loadImage("DATA/menus/main_players.tga");
		m_modRect = loadImage("DATA/menus/main_rect.tga");
		m_modHeadings = loadImage("DATA/menus/mods_types.tga");
		m_mods[0] = loadImage("DATA/menus/mods_speed.tga");
		m_mods[1] = loadImage("DATA/menus/mods_reverse.tga");
		m_mods[2] = loadImage("DATA/menus/mods_mirror.tga");
		m_mods[3] = loadImage("DATA/menus/mods_appear.tga");
		m_hazard = loadImage("DATA/menus/hazard_flash.png");
	}
	if ( m_startButton == NULL )
	{
		m_startButton = loadImage("DATA/menus/start.tga");
		m_triangles = loadImage("DATA/menus/triangles.tga");
	}

	if ( gs.g_currentGameMode == PLAYERSELECT )
	{
		blinkTimer = introTimer = 0;
		timeRemaining = 15999;
		currentRow = 0;
		playersChoice = 0;
		if ( gs.leftPlayerPresent && gs.rightPlayerPresent ) // you both pressed start during the attract/caution, so here you go
		{
			playersChoice = 2;
		}

		modesY = MODE_Y_TOP;
		modesDirection = 0;

		gs.loadSong(BGM_MODE);
		gs.playSong();
		lm.loadLampProgram("menu_0.txt");

		gs.player[0].resetAll();
		gs.player[1].resetAll();
		sm.player[0].resetData();
		sm.player[1].resetData();
	}
	else // MAINMENU
	{
		playersChoice = 0;
		if ( gs.isDoubles )
		{
			playersChoice = 1;
		}
		if ( gs.isVersus )
		{
			playersChoice = 2;
		}
		currentRow = 1;
		timeRemaining = 30999;
		lm.loadLampProgram("menu_1.txt");

		modesY = MODE_Y_LOW;
		modesDirection = -1;
	}

	modeChoice = 1;
	p1row = 0;
	p2row = 0;
	mods[0][0] = mods[0][1] = mods[0][2] = mods[0][3] = 0;
	mods[1][0] = mods[1][1] = mods[1][2] = mods[1][3] = 0;
	if ( gs.isSingles() )
	{
		mods[0][3] = gs.leftPlayerPresent ? 1 : 2; // pick "LEFT" or "RIGHT" over "CENTER" as the default
	}
	modeMovingTimer = 0;
	hazardCount[0] = hazardCount[1] = 0;

	im.setCooldownTime(0);
}

void mainMenuLoop(UTIME dt)
{
	blinkTimer = (blinkTimer + dt) % 250;
	introTimer += dt;
	if ( !gs.isEventMode )
	{
		SUBTRACT_TO_ZERO(timeRemaining, dt);
	}
	playTimeLowSFX(dt);

	// process the unfolding animation
	if ( modesDirection != 0 )
	{
		modeMovingTimer += dt;
		
		if ( modesDirection > 0 )
		{
			modesY = getValueFromRange(MODE_Y_TOP, MODE_Y_LOW, modeMovingTimer*100/MODE_UNFOLD_TIME);
			if ( modeMovingTimer >= MODE_UNFOLD_TIME )
			{
				modesY = MODE_Y_LOW;
				modesDirection = 0;
				gs.g_currentGameMode = LOGIN;
				gs.g_gameModeTransition = 1;
				return;
			}
		}
		else
		{
			modesY = getValueFromRange(MODE_Y_LOW, MODE_Y_TOP, modeMovingTimer*100/MODE_UNFOLD_TIME);
			if ( modeMovingTimer >= MODE_UNFOLD_TIME )
			{
				modesY = MODE_Y_TOP;
				modesDirection = 0;
			}
		}
	}

	// debug testing
	if ( key[KEY_SPACE] )
	{
		timeRemaining = 5000;
	}

	bool noDouble = !gs.isFreeplay && !gs.isDoublePremium && gs.numCoins < gs.numCoinsPerCredit*2;
	bool noVersus = !gs.isFreeplay && !gs.isVersusPremium && gs.numCoins < gs.numCoinsPerCredit*2;
	bool forcedVersus = gs.leftPlayerPresent && gs.rightPlayerPresent;
	int numModRows = gs.isDoubles || gs.isVersus ? 3 : 4; // only show the last modifier (which side) for singles play

	// check input
	if ( currentRow == 0 && modesDirection == 0 )
	{
		// here the triangle buttons choose between single/double/versus
		if ( forcedVersus )
		{
			// if we're in forced versus mode then the triangles don't matter
			if ( im.getKeyState(MENU_LEFT_1P) == JUST_DOWN || im.getKeyState(MENU_LEFT_2P) == JUST_DOWN || im.getKeyState(MENU_RIGHT_1P) == JUST_DOWN || im.getKeyState(MENU_RIGHT_2P) == JUST_DOWN )
			{
				em.playSample(SFX_MENU_STUCK);
			}
		}
		else
		{
			// select single/double/versus except skip double or versus if either is locked out due to a lack of credits
			if ( im.getKeyState(MENU_LEFT_1P) == JUST_DOWN || im.getKeyState(MENU_LEFT_2P) == JUST_DOWN )
			{
				int originalSelection = playersChoice;
				playersChoice = playersChoice == 0 ? 0 : playersChoice - 1;
				if ( playersChoice == 1 && noDouble )
				{
					playersChoice = 0; // singles
				}

				em.playSample(originalSelection == playersChoice ? SFX_MENU_STUCK : SFX_MENU_MOVE);
			}
			if ( im.getKeyState(MENU_RIGHT_1P) == JUST_DOWN || im.getKeyState(MENU_RIGHT_2P) == JUST_DOWN )
			{
				int originalSelection = playersChoice;
				playersChoice = playersChoice == 2 ? 2 : playersChoice + 1;
				if ( playersChoice == 2 && noVersus )
				{
					playersChoice = 1; // doubles
				}
				if ( playersChoice == 1 && noDouble )
				{
					playersChoice = noVersus ? 0 : 2; // single or versus
				}

				em.playSample(originalSelection == playersChoice ? SFX_MENU_STUCK : SFX_MENU_MOVE);
			}
		}

		// pressing start will confirm the current selection and potentially add a second player
		if ( im.getKeyState(MENU_START_1P) == JUST_DOWN || im.getKeyState(MENU_START_2P) == JUST_DOWN )
		{
			bool leftPlayerAdding = !gs.leftPlayerPresent && im.getKeyState(MENU_START_1P) == JUST_DOWN;
			bool rightPlayerAdding = !gs.rightPlayerPresent && im.getKeyState(MENU_START_2P) == JUST_DOWN;
			bool enoughCreditsForVersus = gs.isVersusPremium || gs.isFreeplay || (gs.numCoins >= gs.numCoinsPerCredit*2);
			
			// second player added! versus mode is auto-confirmed
			if ( (leftPlayerAdding || rightPlayerAdding) && enoughCreditsForVersus )
			{
				gs.leftPlayerPresent = true;
				playersChoice = 2;
				em.playSample(SFX_CREDIT_BEGIN);
			}

			// sanity check
			if ( (playersChoice == 1 && noDouble) || (playersChoice == 2 && noVersus) )
			{
				em.playSample(SFX_MENU_STUCK);
				playersChoice = 0; // should be imposssible
			}
			else
			{
				// set the player type
				gs.isDoubles = gs.isSolo = gs.isVersus = false;
				if ( playersChoice == 1 ) gs.isDoubles = true;
				if ( playersChoice == 2 ) gs.isVersus = true;

				em.playSample(SFX_START_BUTTON);
				if ( playersChoice == 1 )
				{
					em.announcerQuipChance(71, 33);
				}

				if ( gs.allowLogins )
				{
					modesDirection = 1; // automatically starts the unfolding animation
				}
				else
				{
					currentRow = 1; // skip directly to the mode selection
				}
				return;
			}
		}
	}
	else if ( currentRow == 1 && modesDirection == 0 )
	{
		int originalSelection = modeChoice;
		if ( im.getKeyState(MENU_LEFT_1P) == JUST_DOWN || im.getKeyState(MENU_LEFT_2P) == JUST_DOWN )
		{
			modeChoice = modeChoice == 0 ? 0 : modeChoice - 1;
			em.playSample(modeChoice == originalSelection ? SFX_MENU_STUCK : SFX_MENU_MOVE);
		}
		if ( im.getKeyState(MENU_RIGHT_1P) == JUST_DOWN || im.getKeyState(MENU_RIGHT_2P) == JUST_DOWN )
		{
			modeChoice = modeChoice == 2 ? 2 : modeChoice + 1;
			em.playSample(modeChoice == originalSelection ? SFX_MENU_STUCK : SFX_MENU_MOVE);
		}

		if ( modeChoice == 2 ) // HACK: remove mission mode, TODO: implement new mode
		{
			em.playSample(SFX_DIFF);
			modeChoice = 1;
		}

		if ( im.getKeyState(MENU_START_1P) == JUST_DOWN || im.getKeyState(MENU_START_2P) == JUST_DOWN )
		{
			// now that Mode Select is separate from Player Select (and with Login Mode in between) do not allow reversal back to Player Select
			//if ( im.isKeyDown(MENU_LEFT_1P) && im.isKeyDown(MENU_RIGHT_1P) && im.isKeyDown(MENU_START_1P) )
			//{
			//	currentRow = 0; // backup a row
			//	em.playSample(SFX_MENU_SORT);
			//}
			//else if ( im.isKeyDown(MENU_LEFT_2P) && im.isKeyDown(MENU_RIGHT_2P) && im.isKeyDown(MENU_START_2P) )
			//{
			//	currentRow = 0; // backup a row
			//	em.playSample(SFX_MENU_SORT);
			//}			
			//else
			//{
				currentRow = 2; // this variable advances no further. time to do per-player modifiers!
				em.playSample(SFX_START_BUTTON);
			//}
		}
	}
	else if ( currentRow == 2 )
	{
		if ( playersChoice < 2 ) // one player playing alone
		{
			if ( p1row < numModRows )
			{
				if ( im.getKeyState(MENU_LEFT_1P) == JUST_DOWN || im.getKeyState(MENU_LEFT_2P) == JUST_DOWN )
				{
					mods[0][p1row] = mods[0][p1row] == 0 ? 0 : mods[0][p1row] - 1;
					em.playSample(SFX_MOD_MOVE);

					// 1P: hazard is 6 lefts of the speed mod
					if ( p1row == 0 && mods[0][p1row] == 0 )
					{
						hazardCount[0]++;
						if ( hazardCount[0] == 7 )
						{
							hazardCount[0] = 0;
							gs.player[0].useHazard = !gs.player[0].useHazard;
							if ( gs.player[0].useHazard )
							{
								em.playSample(10);
								em.announcerQuip(79);
							}
							else
							{
								em.playSample(18);
							}
						}
					}
				}
				if ( im.getKeyState(MENU_RIGHT_1P) == JUST_DOWN || im.getKeyState(MENU_RIGHT_2P) == JUST_DOWN )
				{
					mods[0][p1row] = mods[0][p1row] == MOD_LIMITS[p1row] ? MOD_LIMITS[p1row] : mods[0][p1row] + 1;
					em.playSample(SFX_MOD_MOVE);
					hazardCount[0] = 0;
				}
			}
			if ( im.getKeyState(MENU_START_1P) == JUST_DOWN || im.getKeyState(MENU_START_2P) == JUST_DOWN )
			{
				if ( (im.isKeyDown(MENU_LEFT_1P) && im.isKeyDown(MENU_RIGHT_1P) && im.isKeyDown(MENU_START_1P)) ||
						im.isKeyDown(MENU_LEFT_2P) && im.isKeyDown(MENU_RIGHT_2P) && im.isKeyDown(MENU_START_2P) )
				{
					if ( p1row == 0 )
					{
						currentRow = 1; // back up to the mode select
					}
					else
					{
						p1row--; // back up one row
					}
					em.playSample(SFX_MENU_SORT);
				}
				else
				{
					if ( p1row < numModRows )
					{
						em.playSample(SFX_START_BUTTON);
					}
					p1row = p1row == numModRows ? numModRows : p1row + 1; // done modifiers! advance to login
				}
			}
		}
		else // versus modifers! each player splits up and chooses mods separate
		{
			int keys[2][3] = { {MENU_LEFT_1P, MENU_RIGHT_1P, MENU_START_1P}, {MENU_LEFT_2P, MENU_RIGHT_2P, MENU_START_2P} };

			for ( int i = 0; i < 2; i++ )
			{
				int *prow = (i == 0 ? &p1row : &p2row);

				if ( *prow < numModRows )
				{
					if ( im.getKeyState(keys[i][0]) == JUST_DOWN ) // left // MAJOR ERROR HERE // TODO: remember what it was and fix it
					{
						mods[i][*prow] = mods[i][*prow] == 0 ? 0 : mods[i][*prow] - 1;
						em.playSample(SFX_MOD_MOVE);
					}
					if ( im.getKeyState(keys[i][1]) == JUST_DOWN ) // right // MAJOR ERROR HERE
					{
						mods[i][*prow] = mods[i][*prow] == MOD_LIMITS[*prow] ? MOD_LIMITS[*prow] : mods[i][*prow] + 1;
						em.playSample(SFX_MOD_MOVE);
					}
				}
				if ( im.getKeyState(keys[i][2]) == JUST_DOWN ) // start
				{
					if ( im.isKeyDown(keys[i][0]) && im.isKeyDown(keys[i][1]) && im.isKeyDown(keys[i][2]) )
					{
						if ( *prow == 0 )
						{
							currentRow = 1; // back up to the mode select
						}
						else
						{
							*prow = *prow - 1; // back up one row
						}
						em.playSample(SFX_MENU_SORT);
					}
					else
					{
						if ( *prow < numModRows )
						{
							em.playSample(SFX_START_BUTTON);
						}
						*prow = *prow == numModRows ? numModRows : *prow + 1; // done modifiers! wait for other player
					}
				}
			}			
		}
	}

	// ran out of time while choosing?
	if ( timeRemaining <= 0 )
	{
		if ( currentRow < 2 ) // nevermind, they left the game unattended
		{
			gs.g_currentGameMode = ATTRACT;
			gs.g_gameModeTransition = 1;
		}
		else // skip the mods - move on to next mode
		{
			p1row = p2row = 4;
		}
	}

	// are we done with the main menu?
	if ( (playersChoice < 2 && p1row >= numModRows) || (playersChoice == 2 && p1row >= numModRows && p2row >= numModRows) )
	{
		// set mode
		gs.currentGameType = modeChoice;

		// set modifiers
		for ( int side = 0; side < 2; side++ )
		{
			gs.player[side].speedMod = SPEED_MOD_EFFECTS[mods[side][0]];
			if ( mods[side][1] > 0 )
			{
				gs.player[side].reverseModifier = 0xFF;
				if ( mods[side][1] == 2 )
				{
					gs.player[side].reverseModifier = 0x99; // binary 10011001 for the blue columns
				}
			}
			gs.player[side].arrangeModifier = mods[side][2];

			/*
			if ( mods[side][3] == 1 )
			{
				gs.player[side].hiddenModifier = 0xFF;
			}
			if ( mods[side][3] == 2 )
			{
				gs.player[side].suddenModifier = 0xFF;
			}
			if ( mods[side][3] == 3 )
			{
				gs.player[side].stealthModifier = true;
			}
			*/
		}

		// the side option is only available when playing alone
		if ( gs.isSingles() )
		{
			if ( mods[0][3] == 1 )
			{
				gs.player[0].centerLeft = true;
			}
			if ( mods[0][3] == 2 )
			{
				gs.player[0].centerRight = true;
			}
		}

		// update bookkeeping, stats, progress to next determined screen
		endMenuMode();
	}

	// set lamps for players
	int numPlayers = gs.isVersus ? 2 : 1;
	for ( int pnum = 0; pnum < numPlayers; pnum++ )
	{
		int duration = sm.player[pnum].isLoggedIn ? 500 : 0; // duration of 0 turns the lamp off
		for ( int i = 0; i < 4; i++ )
		{
			lm.setOrbColor(pnum, i, 2, duration);
			if ( gs.isDoubles )
			{
				lm.setOrbColor(1, i, 2, duration); // also set the other side for doubles
			}
			if ( gs.rightPlayerPresent && !gs.leftPlayerPresent && gs.isSingles() )
			{
				lm.setOrbColor(0, i, 2, 0); // super special case for one player on the right: turn off the left and turn on the right
				lm.setOrbColor(1, i, 2, 500);
			}
		}
	}
	bool use1P = gs.isDoubles || gs.isVersus || gs.leftPlayerPresent;
	bool use2P = gs.isDoubles || gs.isVersus || gs.rightPlayerPresent;
	lm.setLamp(lampStart, use1P ? 100 : 0);
	lm.setLamp(lampLeft, use1P ? 100 : 0);
	lm.setLamp(lampRight, use1P ? 100 : 0);
	lm.setLamp(lampStart+1, use2P ? 100 : 0);
	lm.setLamp(lampLeft+1, use2P ? 100 : 0);
	lm.setLamp(lampRight+1, use2P ? 100 : 0);

	renderMenuLoop();
}

void renderMenuLoop()
{
	int sx = 0, i = 0, color = 0;

	blit(m_menuBG, rm.m_backbuf, 0, 0, 0, 0, 640, 480);
	draw_trans_sprite(rm.m_backbuf, m_modeSelect, 287, 45); 

	// render the sample machine
	if ( currentRow < 2 )
	{
		draw_trans_sprite(rm.m_backbuf, m_displayMachine[playersChoice], 20, 200);
	}

	// render the "body" of the unfolded login area
	rectfill(rm.m_backbuf, 0, MODE_Y_TOP, 640, modesY, makeacol(255, 170, 200, modesDirection == -1 ? 200 : 255 ));

	// render the blcak and pink stripes
	rectfill(rm.m_backbuf, 0, 83, 640, 83+33, makeacol(0, 0, 0, 255));
	rectfill(rm.m_backbuf, 0, modesY, 640, modesY+44, makeacol(208, 89, 160, 255));

	// see if whatever should blink, should blink
	char shouldBlink = blinkTimer >= 180 ? 1 : 0;

	// render SINGLE DOUBLE VERSUS
	for ( i = 0; i < 3; i++ )
	{
		if ( currentRow == 0 )
		{
			color = i == playersChoice && shouldBlink ? 0 : 1;
		}
		else
		{
			color = i == playersChoice ? 1 : 2;
		}

		// if there aren't enough credits for doubles or versus, disable those selections
		if ( i == 1 && !gs.isFreeplay && !gs.isDoublePremium && gs.numCoins < gs.numCoinsPerCredit*2 )
		{
			color = 2;
		}
		if ( i == 2 && !gs.isFreeplay && !gs.isVersusPremium && gs.numCoins < gs.numCoinsPerCredit*2 )
		{
			color = 2;
		}

		// if this is forced versus time then single and double should be darkened as well
		if ( (i == 0 || i == 1) && gs.leftPlayerPresent && gs.rightPlayerPresent )
		{
			color = 2;
		}

		sx = color * 114;
		masked_blit(m_mainPlayers, rm.m_backbuf, sx, i*32, i*114 + (i+1)*74, 84, 114, 32);
	}

	// render GAME MODE
	if ( modeChoice == 2 )
	{
		sx = currentRow != 1 ? 210 : shouldBlink ? 0 : 210;
		int extraY = currentRow == 1 ? (blinkTimer/75) : 0;
		masked_blit(m_mainModes, rm.m_backbuf, sx, 2*32, 215, (modesY-1) + extraY, 210, 32);
		renderArtistString("6 challenge tokens", 222, modesY+25, 200, 32);
	}
	else
	{
		//char numStagesDesc[] = "3 stages + ext";
		//numStagesDesc[0] = gs.numSongsPerSet + '0';
		char numSongsDesc[16] = "";
		char numCoursesDesc[16] = "";
		sprintf_s(numSongsDesc, "%d songs", NUM_SONGS);
		sprintf_s(numCoursesDesc, "%d courses", NUM_COURSES);

		sx = currentRow > 1 ? 420 : 210;
		if ( modeChoice == 0 )
		{
			sx = currentRow == 1 && shouldBlink ? 0 : 210;
		}
		masked_blit(m_mainModes, rm.m_backbuf, sx, 0*32,  40, modesY-1, 210, 32);
		renderArtistString(numCoursesDesc, 74+24, modesY+25, 200, 32);

		sx = currentRow > 1 ? 420 : 210;
		if ( modeChoice == 1 )
		{
			sx = currentRow == 1 && shouldBlink ? 0 : 210;
		}
		masked_blit(m_mainModes, rm.m_backbuf, sx, 1*32, 390, modesY-1, 210, 32);
		renderArtistString(numSongsDesc, 424+24, modesY+25, 200, 32);
	}

	// render the modifers
	if ( currentRow == 2 )
	{
		int blinkFrame = blinkTimer/84;
		if ( playersChoice < 2 ) // render single player mods
		{
			renderMods(172, 200, 0, mods[0][0], p1row);
			renderMods(172, 260, 1, mods[0][1], p1row);
			renderMods(172, 320, 2, mods[0][2], p1row);
			if ( gs.isSingles() )
			{
				renderMods(172, 380, 3, mods[0][3], p1row);
			}
			if ( gs.player[0].useHazard )
			{
				masked_blit(m_hazard, rm.m_backbuf, 0, blinkFrame*23, 44, 200, 84, 23);
			}
		}
		else // render one set of mods for each player
		{
			renderMods(12, 200, 0, mods[0][0], p1row);
			renderMods(12, 260, 1, mods[0][1], p1row);
			renderMods(12, 320, 2, mods[0][2], p1row);
			//renderMods(12, 380, 3, mods[0][3], p1row);
			renderMods(332, 200, 0, mods[1][0], p2row);
			renderMods(332, 260, 1, mods[1][1], p2row);
			renderMods(332, 320, 2, mods[1][2], p2row);
			//renderMods(332, 380, 3, mods[1][3], p2row);

			if ( gs.player[0].useHazard )
			{
				masked_blit(m_hazard, rm.m_backbuf, 0, blinkFrame*23, 44, 380, 84, 23);
			}
			if ( gs.player[1].useHazard )
			{
				masked_blit(m_hazard, rm.m_backbuf, 0, ((blinkFrame+1)%3)*23, 512, 380, 84, 23);
			}
		}
	}

	// render any potential "wait for other player" during versus mode
	if ( playersChoice == 2 )
	{
		int xoff = (timeRemaining%2000) < 1000 ? (timeRemaining%2000) / 50 : 20 - ((timeRemaining%2000 - 1000) / 50);
		if ( p1row >= 3 )
		{
			renderArtistString("Wait...", 12+xoff, 410, 180, 32);
		}
		if ( p2row >= 3 )
		{
			renderArtistString("Wait...", 332+xoff, 410, 180, 32);
		}
	}

	renderTimeRemaining();

	if ( introTimer < 250 )
	{
		rectfill(rm.m_backbuf, 0, 0, getValueFromRange(640, 0, introTimer*100/250), 480, makeacol(0,0,0,255));
		rm.dimScreen(getValueFromRange(100, 0, introTimer*100/250));
	}
}

void renderMods(int x, int y, int which, int selection, int currentRow)
{
	char shouldBlink = blinkTimer >= 180 ? 1 : 0;

	masked_blit(m_modHeadings, rm.m_backbuf, (which == currentRow ? 0 : 0), which*20, x, y-25, 80, 20);
	masked_blit(m_modRect, rm.m_backbuf, 0, (which == currentRow ? 25 : 0), x, y, 296, 25);
	
	for ( int i = 0; i <= MOD_LIMITS[which]; i++ )
	{
		int color = currentRow > which ? 2 : 0;
		if ( selection == i )
		{
			color = which == currentRow && shouldBlink ? 1 : 0;
		}

		masked_blit(m_mods[which], rm.m_backbuf, MOD_BREAKS[which][i], color*30, (x+2 + MOD_BREAKS[which][i]), y-3, MOD_BREAKS[which][i+1]-MOD_BREAKS[which][i], 30);
	}
}

void endMenuMode()
{
	if ( gs.currentGameType == MODE_NONSTOP )
	{
		gs.g_currentGameMode = NONSTOP;
	}
	else
	{
		gs.g_currentGameMode = SONGWHEEL;
		em.announcerQuipChance(GUY_SONGWHEEL, 100);
	}
	gs.g_gameModeTransition = 1;

	// update bookkeeping
	bm.logLogin(sm.player[0].isLoggedIn);
	if ( gs.isVersus )
	{
		bm.logLogin(sm.player[1].isLoggedIn);
		bm.logMode(VERSUS_PLAY);
	}
	else
	{
		bm.logMode(gs.isDoubles ? DOUBLES_PLAY : SINGLES_PLAY);
	}

	// update play counts
	if ( sm.player[0].isLoggedIn )
	{
		if ( gs.isDoubles )
		{
			sm.player[0].numPlaysDP++;
		}
		else
		{
			sm.player[0].numPlaysSP++;
		}
	}
	if ( sm.player[1].isLoggedIn )
	{
		sm.player[1].numPlaysSP++;
	}

	// subtract the credits used
	gs.numCoins -= gs.numCoinsPerCredit;
	if ( gs.isDoubles && !gs.isDoublePremium )
	{
		gs.numCoins -= gs.numCoinsPerCredit;
	}
	if ( gs.isVersus && !gs.isVersusPremium )
	{
		gs.numCoins -= gs.numCoinsPerCredit;
	}
	if ( gs.numCoins < 0 )
	{
		gs.numCoins = 0; // happens in debug mode
	}
}
