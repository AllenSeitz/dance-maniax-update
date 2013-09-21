// gameplayMode.cpp implements the main gameplay loop for DMX
// source file created by Allen Seitz 7/18/2009
// 12-21-09: major refactoring to move all rendering and proceedurally generated graphics to gameplayRendering

#include "common.h"

#include "dwi_read.h"
#include "xsq_read.h"

#include "gameStateManager.h"
#include "inputManager.h"
#include "scoreManager.h"
#include "gameplayRendering.h"
#include "particleSprites.h"
#include "specialEffects.h"
#include "videoManager.h"

extern int* songIDs;
extern std::string* songTitles;
extern std::string* songArtists;
extern std::string* movieScripts;

extern bool isTestingChart;
extern int testChartSongID;
extern int testChartLevel;

//////////////////////////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////////////////////////
// timing windows
#define EARLY_MARVELLOUS 24
#define EARLY_PERFECT    48
#define EARLY_GREAT      120
#define EARLY_GOOD       150
//#define EARLY_BAD        248

#define LATE_MARVELLOUS  24
#define LATE_PERFECT     48
#define LATE_GREAT       120
#define LATE_GOOD        150
//#define LATE_BAD         248

#define JUMP_WINDOW      100
#define HOLD_WINDOW      500

// how long it takes a BPM_CHANGE event to smoothly animate
const int BPM_UPDATE_LENGTH = 400;

// for the between-stage banner animation
const UTIME BANNER_ANIM_LENGTH = 1500;

const UTIME RETIRE_TIMEOUT = 30000;

// macros
#define ISNOTE(x) (x == TAP || x == JUMP)


//////////////////////////////////////////////////////////////////////////////
// Program Variables
//////////////////////////////////////////////////////////////////////////////
// graphics
extern RenderingManager rm;
extern GameStateManager gs;
extern VideoManager vm;
extern ScoreManager sm;
extern EffectsManager em;
extern unsigned long int frameCounter;
extern unsigned long int totalGameTime;

bool gameplayInitialized = false;
extern BITMAP* m_banner1;
extern BITMAP* m_banner2;

// song transition
bool isMidTransition = false;
UTIME songTransitionTime = 0;
int rememberedCurrentStage = 1;

// input
bool autoplay = false;
bool useAssistClap = false;
bool debugCheats = false;
extern InputManager im;
int retireTimer = 0; // for ending the game early when there is a lack of input

// full combo
int fullComboAnimStep = 0; // 0 = not started, 1 = started
int fullComboAnimTimer = 0;
bool fullComboP1 = false;
bool fullComboP2 = false;

// announcer
int announcerPlusPoints = 0;
int announcerMinusPoints = 0;
int announcerQuipCycle = 0;
int announcerLastCheckTotal = 0;
int announcerTargetSpeak = 0; // how many "announcer points" are scored before he makes a comment

// songlist - for unlocks
extern SongEntry* songs;

SAMPLE* assistClap = NULL;
SAMPLE* shockSound = NULL;


//////////////////////////////////////////////////////////////////////////////
// function declarations
//////////////////////////////////////////////////////////////////////////////
void doStepZoneLogic(UTIME dt, int player);
// precondition: dt > 0
// postcondition: this function blinks the stepzone and handles other stepzone animations

void doChartLogic(UTIME dt, int player);
// precondition: dt > 0, a frame of logic has run since the last call
// postcondition: a frame of input logic will be run

int findNextNoteLate(int player, int column);
int findNextNoteEarly(int player, int column);
// precondition: currentChart is initialized, the column is 0-9
// postcondition: returns a note index if successful, or -1

void scoreNote(int player, int judgement, int column);
// precondition: the current game state is valid
// postcondition: updates the combo, lifebar, and score

void loadNextSong();
// precondition: gs.player[] has a setlist setlist, the chart data is loaded
// postcondition: reloads the audio and resets certain variables

void arrangeChart(std::vector<struct ARROW> *chart, std::vector<struct FREEZE> *holds, char type, bool isDoubles);
// precondition: see description of arguments at function declaration
// postcondition: if type != 0 then the chart and holds will be modified

int checkForExtraStages();
// precondition: the game is between stages, and at least gs.numStagesPerCredit have been cleared
// postcondition: modifies the song choices and returns 0-2, the number of bonus songs awarded

//////////////////////////////////////////////////////////////////////////////
// function implementations
//////////////////////////////////////////////////////////////////////////////
void firstGameplayLoop()
{
#ifdef DMXDEBUG
	debugCheats = true;
	autoplay = true;
#endif
	if ( isTestingChart )
	{
		autoplay = true;
		useAssistClap = true;
	}

	if ( isTestingChart )
	{
		gs.currentStage = 0; // loop!
	}

	// reset the player's stats
	for ( int p = 0; p < 2; p++ )
	{
		gs.player[p].nextStage();
		if ( gs.currentStage == 0 )
		{
			gs.player[p].displayCombo = 0;
			gs.player[p].comboColor = 0;
			announcerQuipCycle = rand()%4;
			retireTimer = fullComboAnimTimer = fullComboAnimStep = 0;
			fullComboP1 = fullComboP2 = false;
		}
	}
	rememberedCurrentStage = gs.currentStage; // this won't change during the transition. for most of the song it will remain the same
	announcerPlusPoints = 0;
	announcerMinusPoints = 0;
	announcerLastCheckTotal = 0;
	announcerTargetSpeak = 0;

	// load certain sound effects only during the first run of the game
	if ( !gameplayInitialized )
	{
		assistClap = load_sample("data/sfx/clap.wav");
		shockSound = load_sample("data/sfx/shock.wav");
		assistClap->priority = 100;
		shockSound->priority = 100;
	}

	// DEBUG: if booting directly into gameplay mode, set stuff
	if ( gs.player[0].stagesPlayed[0] <= 0 || isTestingChart )
	{
		TRACE("DEBUG START IN GAME MODE");
		gs.player[0].stagesPlayed[0] = gs.player[0].stagesPlayed[1] = gs.player[0].stagesPlayed[2] = testChartSongID;
		gs.player[0].stagesLevels[0] = gs.player[0].stagesLevels[1] = gs.player[0].stagesLevels[2] = testChartLevel;
		gs.player[1].stagesPlayed[0] = gs.player[1].stagesPlayed[1] = gs.player[1].stagesPlayed[2] = testChartSongID;
		gs.player[1].stagesLevels[0] = gs.player[1].stagesLevels[1] = gs.player[1].stagesLevels[2] = testChartLevel;
		if ( testChartLevel >= DOUBLE_MILD )
		{
			gs.isDoubles = true;
			gs.isVersus = false;
		}
		else
		{
			gs.isDoubles = false;
			gs.isVersus = true;
		}
	}
	clear_keybuf(); // also for debug

	loadNextSong();
	gameplayInitialized = true;

	im.setCooldownTime(67); // 1/15th of a second
}

void mainGameplayLoop(UTIME dt)
{
	int p = 0;
	for ( p = 0; p < (gs.isVersus ? 2 : 1); p++ )
	{
		doStepZoneLogic(dt, p);
		doChartLogic(dt, p);

		// process BPM gimmicks
		SUBTRACT_TO_ZERO(gs.player[p].stopLength, dt);
		SUBTRACT_TO_ZERO(gs.player[p].bpmUpdateTimer, dt);
		gs.player[p].scrollRate = WEIGHTED_AVERAGE(gs.player[p].scrollRate, gs.player[p].newScrollRate, gs.player[p].bpmUpdateTimer, BPM_UPDATE_LENGTH);
	}

	SUBTRACT_TO_ZERO(songTransitionTime, dt);

	// do full combo anim
	if ( fullComboAnimStep > 0 )
	{
		fullComboAnimTimer += dt;
		fullComboAnimStep = fullComboAnimTimer/1000 + 1;
		if ( fullComboAnimTimer >= 4000 )
		{
			fullComboAnimStep = fullComboAnimTimer = 0;
			fullComboP1 = fullComboP2 = false;
		}
	}

	updateParticles(dt);
	renderGameplay();

	gs.player[0].timeElapsed += dt;
	gs.player[1].timeElapsed += dt;
	gs.player[0].judgementTime += dt;
	gs.player[1].judgementTime += dt;

	// implement the "retire" timer (game was abandoned)
	if ( !autoplay )
	{
		retireTimer += dt;
	}
	for ( int i = 0; i < 8; i++ )
	{
		if ( im.getReleaseLength(i) < retireTimer )
		{
			retireTimer = 0;
		}
	}
	if ( retireTimer > 20000 )
	{
		gs.g_currentGameMode = RESULTS;
		gs.g_gameModeTransition = 1;
		sm.savePlayersToDisk();
		em.announcerQuip(86); // say "I can't wait anymore"
		return;
	}

	// check for the player changing their speed-mod at the start of the song
	//if ( gs.player[0].timeElapsed < 10000 )
	{
		if ( gs.player[0].speedMod > 10 && im.getKeyState(MENU_LEFT_1P) == JUST_DOWN )
		{
			gs.player[0].speedMod -= 5;
		}
		if ( gs.player[0].speedMod < 80 && im.getKeyState(MENU_RIGHT_1P) == JUST_DOWN )
		{
			gs.player[0].speedMod += 5;
		}
		if ( gs.player[1].speedMod > 10 && im.getKeyState(MENU_LEFT_2P) == JUST_DOWN )
		{
			gs.player[1].speedMod -= 5;
		}
		if ( gs.player[1].speedMod < 80 && im.getKeyState(MENU_RIGHT_2P) == JUST_DOWN )
		{
			gs.player[1].speedMod += 5;
		}
	}

	// update the per-column judgements and the "step zone resize" effect when a panel is newly hit (DDR only)
	for ( int i = 0; i < 10; i++ )
	for ( p = 0; p < (gs.isVersus ? 2 : 1); p++ )
	{
		gs.player[p].columnJudgeTime[i] += dt;

		if ( im.getKeyState(i) == JUST_DOWN ) // only matters for DDR
		{
			gs.player[p].stepZoneResizeTimers[i] = RESIZE_TIME;
		}
	}

	// check for the announcer making a comment
	if ( announcerPlusPoints + announcerMinusPoints > announcerLastCheckTotal )
	{
		announcerLastCheckTotal = announcerPlusPoints + announcerMinusPoints;

		if ( announcerLastCheckTotal > announcerTargetSpeak )
		{
			// index 0 and 2 are "long", index 1 and 3 are "short"
			static int GUY_PLAYING_PERFECT[4] = { 107, 132,  99, 140 };
			static int GUY_PLAYING_GREAT[4]   = {  65, 117,  98, 119 };
			static int GUY_PLAYING_UNWELL[4]  = { 102, 144, 111, 146 };
			static int GUY_PLAYING_GIVEUP[4]  = {  93, 127,  94, 128 };
			bool talked = false;

			if ( announcerPlusPoints > 0 && announcerMinusPoints == 0 )
			{
				talked = em.announcerQuipChance(GUY_PLAYING_PERFECT[announcerQuipCycle], 10);
			}
			else if ( announcerPlusPoints > announcerMinusPoints*12 )
			{
				talked = em.announcerQuipChance(GUY_PLAYING_GREAT[announcerQuipCycle], 10);
			}
			else if ( announcerPlusPoints > announcerMinusPoints )
			{
				talked = em.announcerQuipChance(GUY_PLAYING_UNWELL[announcerQuipCycle], 10);
			}
			else if ( announcerPlusPoints < announcerMinusPoints )
			{
				talked = em.announcerQuipChance(GUY_PLAYING_GIVEUP[announcerQuipCycle], 10);
			}

			if ( talked )
			{
				announcerLastCheckTotal = announcerPlusPoints = announcerMinusPoints = 0;
				announcerQuipCycle = (announcerQuipCycle + 1) % 4;
			}
		}
	}

	// check for the game ending suddenly due ot the battery lifebar
	if ( gs.player[0].useBattery && gs.player[0].lifebarLives <= 0 )
	{
		if ( !gs.isVersus || (gs.player[1].useBattery && gs.player[1].lifebarLives <= 0) )
		{
			gs.g_currentGameMode = FAILURE;
			gs.g_gameModeTransition = 1;
			gs.killSong();
			sm.savePlayersToDisk();
			em.playSample(SFX_FAILURE_WHOOSH);
			em.announcerQuip(GUY_STAGE_FAILED);
			return;
		}
	}

	// these should only work in debug mode
	while (keypressed() == TRUE && (debugCheats || isTestingChart) )
	{
		int k = readkey() >> 8;

		if ( k == KEY_O ) // restart the song
		{
			gs.g_currentGameMode = GAMEPLAY;
			gs.g_gameModeTransition = 1;
		}
		if ( k == KEY_F6 )
		{
			gs.player[0].useBattery = true;
			gs.player[0].lifebarLives = 4;
			if ( gs.isVersus )
			{
				gs.player[1].useBattery = true;
				gs.player[1].lifebarLives = 4;
			}
		}
		if ( k == KEY_F7 )
		{
			gs.player[0].useBattery = false;
			if ( gs.isVersus )
			{
				gs.player[1].useBattery = false;
			}
		}

		if ( k == KEY_PRTSCR )
		{
			rm.screenshot();
		}

		if ( k == KEY_F10 )
		{
			autoplay = !autoplay;
		}
		if ( k == KEY_F11 )
		{
			useAssistClap = !useAssistClap;
		}
		if ( k == KEY_M )
		{
			gs.player[0].danceManiaxMode = !gs.player[0].danceManiaxMode;
		}
		if ( k == KEY_COMMA )
		{
			gs.player[0].drummaniaMode = !gs.player[0].drummaniaMode;
		}
		if ( k == KEY_OPENBRACE && fullComboAnimTimer == 0 )
		{
			gs.setSongPosition(-5000);
		}
		if ( k == KEY_CLOSEBRACE && fullComboAnimTimer == 0 )
		{
			gs.setSongPosition(+5000);
		}
		if ( k == KEY_K )
		{
			gs.player[0].shockAnimTimer = SHOCK_ANIM_TIME;
			playSFXOnce(shockSound);
		}
	}
}

void doStepZoneLogic(UTIME dt, int p)
{
	gs.player[p].stepZoneBeatTimer += dt;
	SUBTRACT_TO_ZERO(gs.player[p].stepZoneBlinkTimer, dt);
	SUBTRACT_TO_ZERO(gs.player[p].shockAnimTimer, dt);
	for ( int i = 0; i < 10; i++ )
	{
		SUBTRACT_TO_ZERO(gs.player[p].stepZoneResizeTimers[i], dt);
		SUBTRACT_TO_ZERO(gs.player[p].laneFlareTimers[i], dt);
	}
	for ( int i = 0; i < 4; i++ )
	{
		SUBTRACT_TO_ZERO(gs.player[p].drummaniaCombo[i], dt);
	}

	if ( gs.player[p].stepZoneBeatTimer > gs.player[p].stepZoneTimePerBeat )
	{
		gs.player[p].stepZoneBeatTimer -= gs.player[p].stepZoneTimePerBeat;
		gs.player[p].stepZoneBlinkTimer = gs.player[p].stepZoneTimePerBeat / 4;
		gs.player[p].colorCycle = gs.player[p].colorCycle == 3 ? 0 : gs.player[p].colorCycle + 1;
	}
	//al_trace("%d\n", stepZoneBlinkTimer);
}

void doChartLogic(UTIME dt, int p)
{
	unsigned int n = gs.player[p].currentNote;

	// step through the chart until the next non-current note is found
	while ( gs.player[p].currentChart.size() > 0 && gs.player[p].currentChart[n].timing < gs.player[p].timeElapsed )
	{
		al_trace("time elapsed is %d\r\n", gs.player[p].timeElapsed);
		// mark notes as hit
		if ( autoplay )
		{
			if ( ISNOTE(gs.player[p].currentChart[n].type) )
			{
				scoreNote(p, PERFECT, gs.player[p].currentChart[n].columns[0]);
				gs.player[p].currentChart[n].judgement = PERFECT;
				gs.player[p].laneFlareTimers[gs.player[p].currentChart[n].columns[0]] = HIT_FLASH_DISPLAY_TIME;
				gs.player[p].laneFlareColors[gs.player[p].currentChart[n].columns[0]] = 0;
			}
		}
		if ( useAssistClap && assistClap && ISNOTE(gs.player[p].currentChart[n].type) )
		{
			playSFXOnce(assistClap); 
		}
		if ( gs.player[p].currentChart[n].type == BPM_CHANGE )
		{
			gs.player[p].bpmUpdateTimer = BPM_UPDATE_LENGTH;
			gs.player[p].newScrollRate = gs.player[p].currentChart[n].color;
		}
		if ( gs.player[p].currentChart[n].type == SCROLL_STOP )
		{
			gs.player[p].stopLength = gs.player[p].currentChart[n].color;
			gs.player[p].stopTime = gs.player[p].timeElapsed;
		}
		if ( gs.player[p].currentChart[n].type == NEW_SECTION )
		{
			// should not happen in DMX
		}
		if ( p == 0 && gs.player[p].currentChart[n].type == END_SONG ) // NOT A BUG! Only P1's chart ends the song (could be weird in versus mode)
		{
			int bestStatus = MAX(sm.player[0].currentSet[gs.currentStage].calculateStatus(), sm.player[1].currentSet[gs.currentStage].calculateStatus());
			TRACE("Best Clear Status: %d (%d %d)", bestStatus, sm.player[0].currentSet[gs.currentStage].status, sm.player[1].currentSet[gs.currentStage].status);

			gs.currentStage++;
			isMidTransition = false; // prevent a crash with the fast-foward debug key

			int numBonusStages = 0;
			if ( gs.currentStage >= gs.numSongsPerSet )
			{
				numBonusStages = checkForExtraStages();
			}

			bool shortList = gs.player[0].stagesPlayed[gs.currentStage] <= 0;				// true when, for any reason, not enough songs were picked to fill the setlist
			if ( (gs.currentStage >= gs.numSongsPerSet + numBonusStages) || shortList )		// if failing were possible in DMX, make sure that the bestStatus is at least CLEARED
			{
				if ( fullComboAnimStep > 0 ) // let the full combo animation play out on the way to the results
				{
					gs.currentStage--;
					return;
				}
				gs.g_currentGameMode = RESULTS;
				gs.g_gameModeTransition = 1;
				sm.savePlayersToDisk();
			}
			else
			{
				//loadNextSong(); // this goes off-sync by stage 2, restart the mode instead
				gs.g_currentGameMode = GAMEPLAY;
				gs.g_gameModeTransition = 1;
			}
			return;
		}
		n++;

		++gs.player[p].currentNote;

		if ( gs.player[p].currentNote == (int)gs.player[p].currentChart.size() )
		{
			gs.player[p].currentNote = (int)gs.player[p].currentChart.size() - 1;
			break;
		}
	}

	// check for late misses
	n = gs.player[p].lastLateNote;
	while ( (int)n < gs.player[p].currentNote )
	{
		// time to advance?
		if ( gs.player[p].currentChart[n].timing + LATE_GOOD < gs.player[p].timeElapsed )
		{
			if ( gs.player[p].currentChart[n].judgement == UNSET )
			{
				if ( gs.player[p].currentChart[n].type == TAP || gs.player[p].currentChart[n].type == JUMP )
				{
					scoreNote(p, MISS, gs.player[p].currentChart[n].columns[0]);

					// Drummania and IIDX don't have 'jumps', each note is separate, hmm....
					if ( gs.player[p].drummaniaMode && gs.player[p].currentChart[n].columns[1] != -1 )
					{
						gs.player[p].columnJudgement[gs.player[p].currentChart[n].columns[1]] = MISS;
						gs.player[p].columnJudgeTime[gs.player[p].currentChart[n].columns[1]] = 0;
					}
					gs.player[p].currentChart[n].judgement = MISS;
				}

				// shock arrows use a slightly smaller late window
				if ( gs.player[p].currentChart[n].type == SHOCK && (gs.player[p].currentChart[n].timing + LATE_GOOD < gs.player[p].timeElapsed) )
				{
					gs.player[p].currentChart[n].judgement = OK;
					scoreNote(p, OK, -1);
					gs.player[p].displayCombo++;
					gs.player[p].currentSongCombo++;
				}
			}
			gs.player[p].lastLateNote++;
		}
		n++;
	}

	// check for player input
	if ( !autoplay )
	{
		for (int i = 0; i < 10; i++ ) // each column
		{
			int lateNote = findNextNoteLate(p, i);
			int earlyNote = findNextNoteEarly(p, i);
			int diff = 0, closestNote = -1;

			// which is closer, the next early note, or the next late note?
			if ( lateNote == -1 && earlyNote != -1 )
			{
				closestNote = earlyNote;
				diff = gs.player[p].currentChart[earlyNote].timing - gs.player[p].timeElapsed;
			}
			else if ( lateNote != -1 && earlyNote == -1 )
			{
				closestNote = lateNote;
				diff = gs.player[p].timeElapsed - gs.player[p].currentChart[lateNote].timing;
			}
			else if ( lateNote != -1 && earlyNote != -1 )
			{
				int late = gs.player[p].timeElapsed - gs.player[p].currentChart[lateNote].timing;
				int early = gs.player[p].currentChart[earlyNote].timing - gs.player[p].timeElapsed;
				closestNote = late < early ? lateNote : earlyNote; // HELL: reverse this condition
				diff = MIN(late, early); // HELL: change this to a MAX as well
			}

			if ( closestNote == -1 )
			{
				continue;
			}

			if ( im.getKeyState(i) == JUST_DOWN )
			{
				//char debugJudges[11] = "?MPGDBMon!";
				int judgement = 0;

				// check the time difference against the late or early windows
				if ( gs.player[p].currentChart[closestNote].timing > gs.player[p].timeElapsed )
				{
					if ( diff < EARLY_MARVELLOUS )
						judgement = MARVELLOUS;
					else if ( diff < EARLY_PERFECT )
						judgement = PERFECT;
					else if ( diff < EARLY_GREAT )
						judgement = GREAT;
					else if ( diff < EARLY_GOOD )
						judgement = GOOD;
					//else if ( diff < EARLY_BAD )
					//	judgement = BAD;			

					//al_trace("[%d] EARLY %c: %ld\r\n", closestNote, debugJudges[judgement], diff);
				}
				else
				{
					if ( diff < EARLY_MARVELLOUS )
						judgement = MARVELLOUS;
					else if ( diff < EARLY_PERFECT )
						judgement = PERFECT;
					else if ( diff < EARLY_GREAT )
						judgement = GREAT;
					else if ( diff < EARLY_GOOD )
						judgement = GOOD;
					//else if ( diff < EARLY_BAD )
					//	judgement = BAD;

					//al_trace("[%d] LATE %c: %ld\r\n", closestNote, debugJudges[judgement], diff);
				}

				// jumps need both arrows to be pressed (NOTE: unless a special exception is made, 'jumps' in DMX charts are read as separate singles!)
				int col1 = gs.player[p].currentChart[closestNote].columns[0];
				int col2 = gs.player[p].currentChart[closestNote].columns[1];
				if ( gs.player[p].currentChart[closestNote].type == JUMP )
				{
					if ( !im.isKeyDown(col1) || !im.isKeyDown(col2) || im.getHoldLength(col1) > JUMP_WINDOW || im.getHoldLength(col2) > JUMP_WINDOW )
					{
						continue; // failed to hit the two keys simultaneously
					}
				}

				if ( gs.player[p].currentChart[closestNote].type == SHOCK )
				{
					// oops! did the player tap a shock arrow?
					if ( im.isKeyInUse(i) && judgement >= MARVELLOUS && judgement <= GOOD && gs.player[p].currentChart[closestNote].judgement == UNSET )
					{
						scoreNote(p, NG, -1);
						gs.player[p].currentChart[closestNote].judgement = NG;
						gs.player[p].shockAnimTimer = SHOCK_ANIM_TIME;
						playSFXOnce(shockSound);
					}
				}
				else
				{
					scoreNote(p,judgement, col1);
					if ( judgement == MARVELLOUS || judgement == PERFECT || judgement == GREAT )
					{
						gs.player[p].laneFlareTimers[col1] = HIT_FLASH_DISPLAY_TIME;
						gs.player[p].laneFlareColors[col1] = judgement == MARVELLOUS ? 0 : 1;
						if ( col2 != -1 )
						{
							gs.player[p].laneFlareTimers[col2] = HIT_FLASH_DISPLAY_TIME;
							gs.player[p].laneFlareColors[col2] = judgement == MARVELLOUS ? 0 : 1;
						}
					}
					gs.player[p].currentChart[closestNote].judgement = judgement;
				}
			}
			else if ( im.getKeyState(i) == HELD_DOWN )
			{
				if ( gs.player[p].currentChart[closestNote].type == SHOCK )
				{
					// oops! did the player hold a shock arrow
					if ( im.isKeyInUse(i) && im.getHoldLength(i) > JUMP_WINDOW && gs.player[p].currentChart[closestNote].judgement == UNSET )
					{
						scoreNote(p, NG, -1);
						gs.player[p].currentChart[closestNote].judgement = NG;
						gs.player[p].shockAnimTimer = SHOCK_ANIM_TIME;
						playSFXOnce(shockSound);
					}
				}
			}
		}
	}

	// process freeze arrow logic
	n = gs.player[p].currentFreeze;
	while ( n < (int)gs.player[p].freezeArrows.size() )
	{
		int col1 = gs.player[p].freezeArrows[n].columns[0];
		int col2 = gs.player[p].freezeArrows[n].columns[1];

		// first, is this hold graded? if so no further work needs to be done on it
		if ( gs.player[p].freezeArrows[n].judgement == OK || gs.player[p].freezeArrows[n].judgement == NG )
		{
			UTIME tailTime = 3000 + MAX(gs.player[p].freezeArrows[n].endTime1, gs.player[p].freezeArrows[n].endTime2);
			bool holdIsAncientHistory = (gs.player[p].freezeArrows[n].judgement == NG && tailTime < gs.player[p].timeElapsed) || gs.player[p].freezeArrows[n].judgement == OK;

			if ( (int)n == gs.player[p].currentFreeze && holdIsAncientHistory )
			{
				gs.player[p].currentFreeze++;
			}
			n++;
			continue;
		}

		// did the player start this hold?
		if ( gs.player[p].freezeArrows[n].startTime < gs.player[p].timeElapsed && im.isKeyDown(col1) && (col2 == -1 || im.isKeyDown(col2)) )
		{
			gs.player[p].freezeArrows[n].isHeld = 1;
		}

		// next, would this hold melt? update the melt time
		if ( gs.player[p].freezeArrows[n].startTime < gs.player[p].timeElapsed || gs.player[p].freezeArrows[n].isHeld == 1 )
		{
			// update flashing (holding) animation
			if ( gs.player[p].freezeArrows[n].isHeld == 1 )
			{
				gs.player[p].laneFlareTimers[col1] = HIT_FLASH_DISPLAY_TIME;
				gs.player[p].laneFlareColors[col1] = 2; //(gs.player[p].timeElapsed / 50) % 2 + 1;
			}
			
			// autoplay needs this to render properly
			if ( autoplay )
			{
				gs.player[p].freezeArrows[n].isHeld = 1;
			}

			if ( (!im.isKeyDown(col1) || gs.player[p].freezeArrows[n].isHeld != 1 ) && !autoplay )
			{
				gs.player[p].freezeArrows[n].meltTime1 += dt;
			}
			else if ( gs.player[p].freezeArrows[n].isHeld == 1 )
			{
				gs.player[p].freezeArrows[n].meltTime1 = 0;
			}

			if ( col2 != -1 )
			{
				// update flashing (holding) animation on column 2
				if ( gs.player[p].freezeArrows[n].isHeld == 1 )
				{
					gs.player[p].laneFlareTimers[col2] = HIT_FLASH_DISPLAY_TIME;
					gs.player[p].laneFlareColors[col2] = 2; //(gs.player[p].timeElapsed / 50) % 2 + 1;
				}

				if ( (!im.isKeyDown(col2) || gs.player[p].freezeArrows[n].isHeld != 1 ) && !autoplay )
				{
					gs.player[p].freezeArrows[n].meltTime2 += dt;
				}
				else if ( gs.player[p].freezeArrows[n].isHeld == 1 )
				{
					gs.player[p].freezeArrows[n].meltTime2 = 0;
				}
			}

			// did it completely melt?
			if ( gs.player[p].freezeArrows[n].meltTime1 > HOLD_WINDOW || gs.player[p].freezeArrows[n].meltTime2 > HOLD_WINDOW )
			{
				gs.player[p].freezeArrows[n].judgement = NG;
				scoreNote(p, NG, gs.player[p].freezeArrows[n].columns[0]);
				gs.player[p].freezeArrows[n].isHeld = 0;

				gs.player[p].laneFlareColors[col1] = 0;
				if ( col2 != -1 )
				{
					gs.player[p].laneFlareColors[col2] = 0;
				}
			}
		}

		// next, is this hold completed OK?
		if ( gs.player[p].freezeArrows[n].endTime1 <= gs.player[p].timeElapsed && ( gs.player[p].freezeArrows[n].endTime2 == -1 || gs.player[p].freezeArrows[n].endTime2 <= gs.player[p].timeElapsed ) )
		{
			// but for really short hold notes, if you never even began to hold them, then they're NG
			if ( gs.player[p].freezeArrows[n].isHeld == 0 )
			{
				gs.player[p].freezeArrows[n].judgement = NG;
				scoreNote(p, NG, gs.player[p].freezeArrows[n].columns[0]);
			}
			else
			{
				gs.player[p].freezeArrows[n].judgement = OK;
				scoreNote(p, OK, gs.player[p].freezeArrows[n].columns[0]);

				gs.player[p].laneFlareTimers[col1] = HIT_FLASH_DISPLAY_TIME;
				gs.player[p].laneFlareColors[col1] = 4;
				if ( col2 != -1 )
				{
					gs.player[p].laneFlareTimers[col2] = HIT_FLASH_DISPLAY_TIME;
					gs.player[p].laneFlareColors[col2] = 4;
				}
			}
			gs.player[p].freezeArrows[n].isHeld = 0;	
		}

		// finally, are we done looping through freeze arrows in the early+late window?
		if ( gs.player[p].freezeArrows[n].startTime >= gs.player[p].timeElapsed + EARLY_GOOD )
		{
			break;
		}
		n++;
	}
}

int findNextNoteLate(int p, int column)
{
	unsigned int n = gs.player[p].currentNote - 1;
	UTIME endTime = gs.player[p].timeElapsed - LATE_GOOD;

	if ( gs.player[p].currentNote == 0 || gs.player[p].timeElapsed < LATE_GOOD )
	{
		return -1; // prevent a crash
	}

	while ( n >= 0 && gs.player[p].currentChart[n].timing > endTime )
	{
		if ( ISNOTE(gs.player[p].currentChart[n].type) && gs.player[p].currentChart[n].judgement == UNSET )
		{
			if ( gs.player[p].currentChart[n].columns[0] == column || gs.player[p].currentChart[n].columns[1] == column )
			{
				return n;
			}
		}
		if ( gs.player[p].currentChart[n].type == SHOCK )
		{
			return n;
		}

		if ( n == 0 )
		{
			break;
		}
		else
		{
			n--;
		}
	}
	return -1;
}

int findNextNoteEarly(int p, int column)
{
	unsigned int n = gs.player[p].currentNote;
	UTIME endTime = gs.player[p].timeElapsed + EARLY_GOOD;

	while ( n < (int)gs.player[p].currentChart.size() && gs.player[p].currentChart[n].timing < endTime )
	{
		if ( ISNOTE(gs.player[p].currentChart[n].type) && gs.player[p].currentChart[n].judgement == UNSET )
		{
			if ( gs.player[p].currentChart[n].columns[0] == column || gs.player[p].currentChart[n].columns[1] == column )
			{
				return n;
			}
		}
		if ( gs.player[p].currentChart[n].type == SHOCK )
		{
			return n;
		}
		n++;
	}
	return -1;
}

void scoreNote(int p, int judgement, int column)
{
	// set the judgement display
	if ( (gs.player[p].drummaniaMode || judgement == OK || judgement == NG) && column != -1 )
	{
		gs.player[p].columnJudgement[column] = judgement;
		gs.player[p].columnJudgeTime[column] = 0;
	}
	else
	{
		gs.player[p].lastJudgement = judgement;
		gs.player[p].judgementTime = 0;
	}

	// tally the judgement
	switch (judgement)
	{
	case MARVELLOUS:
	case PERFECT:
	case OK:
		sm.player[p].currentSet[gs.currentStage].perfects += 1; break;
	case GREAT:
		sm.player[p].currentSet[gs.currentStage].greats += 1; break;
	case GOOD:
		sm.player[p].currentSet[gs.currentStage].goods += 1; break;
	//case BAD:
	case MISS:
	case NG:
		sm.player[p].currentSet[gs.currentStage].misses += 1; break;
	default:
		TRACE("Strange judgement encountered: %d", judgement);
	}
	sm.player[p].currentSet[gs.currentStage].calculateGrade();
	sm.player[p].currentSet[gs.currentStage].calculatePoints();
	gs.player[p].lifebarPercent = sm.player[p].currentSet[gs.currentStage].getScore()/1000; // yes really

	// how does this judgement affect the combo?
	if ( judgement == MISS || judgement == NG )
	{
		if ( gs.player[p].displayCombo >= 500 )
		{
			em.announcerQuip(126); // ouch! your combos end here
			announcerLastCheckTotal = 0;
		}
		gs.player[p].displayCombo = 0;
		gs.player[p].currentSongCombo = 0;
		gs.player[p].comboColor = COMBO_MISS;
		announcerMinusPoints++;
		if ( gs.player[p].useBattery && gs.player[p].lifebarLives > 0 )
		{
			SUBTRACT_TO_ZERO(gs.player[p].lifebarLives, 1);
			em.playSample(SFX_BATTERY_ZAP);
		}
	}
	if ( judgement == MARVELLOUS || judgement == PERFECT || judgement == GREAT || judgement == GOOD )
	{
		gs.player[p].currentSongCombo++;
		gs.player[p].displayCombo++;
		announcerPlusPoints++;

		// what happens to the drummania combo animation?
		gs.player[p].drummaniaCombo[0] = DRUMMANIA_COMBO_BOUNCE_TIME;
		if ( gs.player[p].displayCombo % 10 == 0 )
		{
			gs.player[p].drummaniaCombo[1] = DRUMMANIA_COMBO_BOUNCE_TIME;
		}
		if ( gs.player[p].displayCombo % 100 == 0 )
		{
			gs.player[p].drummaniaCombo[2] = DRUMMANIA_COMBO_BOUNCE_TIME;
			em.announceCombo(gs.player[p].displayCombo);
		}
		if ( gs.player[p].displayCombo % 1000 == 0 )
		{
			gs.player[p].drummaniaCombo[3] = DRUMMANIA_COMBO_BOUNCE_TIME;
		}
	}

	// update the max combo (NOTE: this needs to be tracked different from the display combo)
	if ( gs.player[p].currentSongMaxCombo < gs.player[p].currentSongCombo )
	{
		gs.player[p].currentSongMaxCombo = gs.player[p].currentSongCombo;
	}
	if ( sm.player[p].currentSet[gs.currentStage].maxCombo < gs.player[p].currentSongMaxCombo )
	{
		sm.player[p].currentSet[gs.currentStage].maxCombo = gs.player[p].currentSongMaxCombo;
	}

	// set the combo color
	if ( judgement == MARVELLOUS && gs.player[p].displayCombo == 1 )
	{
		gs.player[p].comboColor = COMBO_MARVELOUS;
	}
	if ( judgement == PERFECT && (gs.player[p].comboColor == COMBO_MARVELOUS || gs.player[p].displayCombo == 1) )
	{
		gs.player[p].comboColor = COMBO_PERFECT;
	}
	if ( judgement == GREAT && (gs.player[p].comboColor == COMBO_MARVELOUS || gs.player[p].comboColor == COMBO_PERFECT || gs.player[p].displayCombo == 1) )
	{
		gs.player[p].comboColor = COMBO_GREAT;
	}

	// did a full combo happen? start the animation
	int totalSteps = sm.player[p].currentSet[rememberedCurrentStage].perfects + sm.player[p].currentSet[rememberedCurrentStage].greats + sm.player[p].currentSet[rememberedCurrentStage].goods;
	if ( totalSteps == sm.player[p].currentSet[rememberedCurrentStage].maxPoints/2 ) // works for holds, since OK == perfect and NG == miss
	{
		em.playSample(SFX_FULL_COMBO_SPLASH);
		fullComboAnimStep = 1;
		fullComboAnimTimer = 0;

		int comboType = 0;
		if ( sm.player[p].currentSet[rememberedCurrentStage].goods == 0 )
		{
			comboType = 1;
		}
		if ( sm.player[p].currentSet[rememberedCurrentStage].goods == 0 && sm.player[p].currentSet[rememberedCurrentStage].greats == 0 )
		{
			comboType = 2;
		}

		if ( p == 0 )
		{
			fullComboP1 = true;
			createFullComboParticles(0, comboType);
		}
		else
		{
			fullComboP2 = true;
			createFullComboParticles(1, comboType);
		}
	}
}

void loadNextSong()
{
	int p1maxscore = 0, p2maxscore = 0;

	if ( gs.isVersus )
	{
		if ( isTestingChart )
		{
			//p1maxscore = readDMXSQ(&gs.player[0].currentChart, &gs.player[0].freezeArrows, gs.player[0].stagesPlayed[gs.currentStage], gs.player[0].stagesLevels[gs.currentStage]);
			p1maxscore = readXSQ(&gs.player[0].currentChart, &gs.player[0].freezeArrows, gs.player[0].stagesPlayed[gs.currentStage], gs.player[0].stagesLevels[gs.currentStage]);
		}
		else
		{
			p1maxscore = readDWI(&gs.player[0].currentChart, &gs.player[0].freezeArrows, gs.player[0].stagesPlayed[gs.currentStage], gs.player[0].stagesLevels[gs.currentStage]);
		}
		p2maxscore = readDWI2P(&gs.player[1].currentChart, &gs.player[1].freezeArrows, gs.player[1].stagesPlayed[gs.currentStage], gs.player[1].stagesLevels[gs.currentStage]);
	}
	else if ( gs.isSingles() )
	{
		if ( gs.player[0].centerLeft )
		{
			p1maxscore = readDWI(&gs.player[0].currentChart, &gs.player[0].freezeArrows, gs.player[0].stagesPlayed[gs.currentStage], gs.player[0].stagesLevels[gs.currentStage]);
		}
		else if ( gs.player[0].centerRight )
		{
			p1maxscore = readDWI2P(&gs.player[0].currentChart, &gs.player[0].freezeArrows, gs.player[0].stagesPlayed[gs.currentStage], gs.player[0].stagesLevels[gs.currentStage]);
		}
		else
		{
			p1maxscore = readDWICenter(&gs.player[0].currentChart, &gs.player[0].freezeArrows, gs.player[0].stagesPlayed[gs.currentStage], gs.player[0].stagesLevels[gs.currentStage]);
		}
	}
	else
	{
		p1maxscore = readDWI(&gs.player[0].currentChart, &gs.player[0].freezeArrows, gs.player[0].stagesPlayed[gs.currentStage], gs.player[0].stagesLevels[gs.currentStage]);
	}

	for ( int p = 0; p < (gs.isVersus ? 2 : 1); p++ )
	{
		gs.player[p].nextStage();

		sm.player[p].currentSet[gs.currentStage].resetData();
		sm.player[p].currentSet[gs.currentStage].time = time(NULL);
		sm.player[p].currentSet[gs.currentStage].status = STATUS_NONE;
		sm.player[p].currentSet[gs.currentStage].songID = gs.player[p].stagesPlayed[gs.currentStage];
		sm.player[p].currentSet[gs.currentStage].chartID = gs.player[p].stagesLevels[gs.currentStage];
		sm.player[p].currentSet[gs.currentStage].grade = GRADE_NONE;
		sm.player[p].currentSet[gs.currentStage].maxPoints = p == 0 ? p1maxscore : p2maxscore;
		if ( gs.currentStage != 0 )
		{
			sm.player[p].currentSet[gs.currentStage - 1].calculateStatus();
		}

		if ( gs.player[p].arrangeModifier > 0 )
		{
			arrangeChart(&gs.player[p].currentChart, &gs.player[p].freezeArrows, gs.player[p].arrangeModifier, gs.isDoubles);
		}
	}

	gs.loadSong(gs.player[0].stagesPlayed[gs.currentStage]);
	vm.loadScript(movieScripts[songID_to_listID(gs.player[0].stagesPlayed[gs.currentStage])].c_str()); // I love the "])]" on this line!!!
	gs.playSong();
	vm.play();
	isMidTransition = true;
	songTransitionTime = BANNER_ANIM_LENGTH;

	announcerTargetSpeak = (p1maxscore + p2maxscore)/7; // speak approximately 7 times per song (although it is both random and dependant on other factors)
}

// chart - list of tap notes
// holds - list of hold notes
// type  - 0 = no change, 1 = mirror (horizontal), 2 = upside down (v-mirror), 3 = shuffle
// isDoubles - matters for some types
void arrangeChart(std::vector<struct ARROW> *chart, std::vector<struct FREEZE> *holds, char type, bool isDoubles)
{
	char arrangeMatrix[4][2][8] = { 
		{ {0,1,2,3,4,5,6,7}, {0,1,2,3,4,5,6,7} }, // original chart
		{ {3,2,1,0,3,2,1,0}, {7,6,5,4,3,2,1,0} }, // mirror
		{ {1,0,3,2,1,0,3,2}, {1,0,3,2,5,4,7,6} }, // upside down
		{ {2,1,0,3,2,1,0,3}, {0,1,2,3,4,5,6,7} }, // one shuffle pattern
	};
	char doubles = isDoubles ? 1 : 0;

	for ( std::vector<struct ARROW>::iterator c = chart->begin(); c != chart->end(); c++ )
	{
		for ( int i = 0; i < 4; i++ )
		{
			if ( c->columns[i] >= 0 && c->columns[i] <= 7 ) // -1 means no note here (very important for triples
			{
				c->columns[i] = arrangeMatrix[type][doubles][c->columns[i]];
			}
		}
	}

	UNUSED(holds); // modifying the head of the freezes automatically moves the holds
}

int checkForExtraStages()
{
	bool awardedExtra = false;
	bool awardedEncore = false;

	awardedExtra = gs.player[0].stagesPlayed[gs.numSongsPerSet] > 0; // extra stage was already awarded
	awardedEncore = gs.player[0].stagesPlayed[gs.numSongsPerSet+1] > 0; // encore stage was already awarded

	if ( !awardedExtra )
	{
		// basic extra stage: are your combined scores above 900,000 per stage?
		int sumP1 = 0, sumP2 = 0;
		int levelP1 = 0, levelP2 = 0;
		for ( int i = 0; i < gs.numSongsPerSet; i++ )
		{
			sumP1 += sm.player[0].currentSet[i].getScore();
			sumP2 += sm.player[1].currentSet[i].getScore();
			levelP1 += sm.player[0].currentSet[i].chartID;
			levelP2 += sm.player[1].currentSet[i].chartID;
		}
		sumP1 /= gs.numSongsPerSet;
		sumP2 /= gs.numSongsPerSet;
		levelP1 /= gs.numSongsPerSet;
		levelP2 /= gs.numSongsPerSet;
		if ( levelP1 % SINGLE_ANOTHER == SINGLE_ANOTHER )
		{
			levelP1 -= 1;
		}
		if ( levelP2 % SINGLE_ANOTHER == SINGLE_ANOTHER )
		{
			levelP2 -= 1;
		}

		// check for (126) "Jet World + Drop Out + Paranoia Max type 2" -OR- (303) POSSESSION if versus
		if ( sumP1 >= 900000 || sumP2 >= 900000 )
		{
			awardedExtra = true;

			// automatically set up for song 126 - Nonstop Megamix 1
			int songToPlay = 126;

			int numFullComboP1 = sm.player[0].getNumStars(levelP1, STATUS_FULLCOMBO);
			int numFullComboP2 = sm.player[1].getNumStars(levelP2, STATUS_FULLCOMBO);

			if ( (numFullComboP1 >= 15 && sm.player[0].getStatusOnSong(303, levelP1) >= STATUS_FULLCOMBO) || (numFullComboP2 >= 15 && sm.player[1].getStatusOnSong(303, levelP2) >= STATUS_FULLCOMBO) )
			{
				songToPlay = 324;
				if ( songs[ songID_to_listID(324) ].version <= 0 )
				{
					songToPlay = 301; // TODO: finish Elemental Creation
				}
			}
			else if ( (numFullComboP1 >= 5 && sm.player[0].getStatusOnSong(126, levelP1) >= STATUS_FULLCOMBO) || (numFullComboP2 >= 5 && sm.player[1].getStatusOnSong(126, levelP2) >= STATUS_FULLCOMBO) )
			{
				songToPlay = 303;
			}

			// make it happen
			gs.player[0].stagesPlayed[gs.numSongsPerSet] = songToPlay;
			gs.player[0].stagesLevels[gs.numSongsPerSet] = levelP1;
			gs.player[0].useBattery = true;
			gs.player[0].lifebarLives = 4;
			if ( gs.isVersus )
			{
				gs.player[1].stagesPlayed[gs.numSongsPerSet] = songToPlay;
				gs.player[1].stagesLevels[gs.numSongsPerSet] = levelP2;
				gs.player[1].useBattery = true;
				gs.player[1].lifebarLives = 4;
			}

			if ( songToPlay != 126 )
			{
				em.announcerQuip(GUY_EARN_SPECIAL_EXTRA);
			}
			else
			{
				em.announcerQuip(GUY_EARN_EXTRA);
			}
		}
	}

	// check for permanently unlocking an extra stage by scoring 90% while on the extra stage
	if ( awardedExtra )
	{
		int songIndex = songID_to_listID(gs.player[0].stagesPlayed[gs.numSongsPerSet]);
		if ( songIndex == -1 )
		{
			songIndexError(gs.player[0].stagesPlayed[gs.numSongsPerSet]);
		}
		if ( awardedExtra && songs[songIndex].unlockFlag == UNLOCK_METHOD_EXTRA_STAGE )
		{
			if ( sm.player[0].currentSet[gs.numSongsPerSet].getScore() >= 900000 )
			{
				sm.player[0].currentSet[gs.numSongsPerSet].unlockStatus = 1;
			}
			if ( sm.player[1].currentSet[gs.numSongsPerSet].getScore() >= 900000 )
			{
				sm.player[0].currentSet[gs.numSongsPerSet].unlockStatus = 1;
			}
		}
	}

	return 0 + (awardedExtra) + (awardedEncore);
}