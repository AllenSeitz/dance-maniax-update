// gameplayRendering.h contains declarations used in rendering the game loop
// source file created by Allen Seitz 12-20-09

#ifndef _GAMEPLAYRENDERING_H_
#define _GAMEPLAYRENDERING_H_

#include "../headers/common.h"

//////////////////////////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////////////////////////
#define JUDGEMENT_DISPLAY_TIME 600
#define HIT_FLASH_DISPLAY_TIME 300
#define SHOCK_ANIM_TIME 750

#define DRUMMANIA_COMBO_BOUNCE_TIME 250

// these are specifically for DMX
#define JUDGEMENT_Y 222
#define REVERSE_JUDGEMENT_Y 331

// these are for the step zone animation when a panel is pressed
#define RESIZE_TIME 100
#define GET_RESIZE(time) (100-(25*time/RESIZE_TIME))

static int columnX[5][10] = 
{
	{ 0, 8, 1, 2, 9, 3, 4, 5, 6, 7 }, // doubles (and singles 1P)
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }, // solo
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }, // solo + doubles
	{ 4, 5, 6, 7, 8, 9, 0, 1, 2, 3 }, // singles (2P)
	{ 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 }, // "column" modifer
};

//////////////////////////////////////////////////////////////////////////////
// Constants - specifically for proceedurally generated graphics
//////////////////////////////////////////////////////////////////////////////
#define END_SECTION_COLOR1 makeacol(255, 32, 32, 255)
#define END_SECTION_COLOR2 makeacol(196, 32, 32, 255)
#define BPM_CHANGE_COLOR1 makeacol(32, 255, 32, 255)
#define BPM_CHANGE_COLOR2 makeacol(32, 196, 32, 255)
#define SCROLL_STOP_COLOR1 makeacol(32, 32, 255, 255)
#define SCROLL_STOP_COLOR2 makeacol(32, 32, 196, 255)
#define COLOR_WHITE makeacol(255, 255, 255, 255)
#define SHOCK_OUTLINE_COLOR 0x88EEFF

static int arrowColors[9][3] = 
{
	{ 255, 128, 128 },  // red(ish)
	{ 128, 128, 255 },  // blue
	{ 164, 255, 255 },  // green
	{ 255, 255, 164 },  // yellow
	{ 255, 128, 128 },  // repeat of the first color (for blending)
	{ 196, 255, 96  },  // the freeze arrow color
	{ 196, 255, 96  },  // the freeze arrow color (for blending)
	{ 100, 100, 255 },  // the shock arrow color
	{ 100, 100, 255 },  // the shock arrow color (for blending)
};

static int drummaniaColors[7][3] =
{
	{ 114, 121, 255 },  // hi hat
	{ 255, 229,  38 },  // snare
	{ 255,   0,  80 },  // bass
	{   0, 255,  33 },  // low tom
	{ 255,   0,   0 },  // hi tom
	{ 127, 201, 255 },  // cymbal
	{ 255, 255, 255 },  // the shock arrow color
};

static int beatmaniaColors[3][3] = 
{
	{ 240, 240, 240 },  // white
	{ 130, 130, 255 },  // blue
	{ 255, 255, 255 },  // the shock arrow color
};

static int flareColors[3][3] =
{
	{ 255, 255, 255 },   // white
	{ 255, 239, 128 },   // yellow
	{ 128, 255, 140 },   // green
};

static int dmxHoldColors[5][4][3] =
{
	{
		{ 255, 216,   0 },  // red notes
		{  20,  20,  60 },
		{ 255,  38,   0 },
		{ 255, 176, 173 },
	},
	{
		{ 255, 216,   0 },  // blue notes
		{  60,  20,  20 },
		{   0,  38, 255 },
		{  76, 255, 231 },
	},
	{
		{ 255, 216,   0 },  // gold notes (not used)
		{  60,  60,  20 },
		{ 255, 255,  38 },
		{ 255, 231,  76 },
	},
	{
		{ 255, 255, 196 },  // being held
		{  60,  60,  60 },
		{ 255, 255,  80 },
		{ 180, 255, 100 },
	},
	{
		{ 128, 128, 128 },  // melted
		{  60,  20,  20 },
		{   0,  38,   0 },
		{ 128, 128, 128 },
	},
};

// a constant in the blending function for the arrow colors
#define ARROW_BRIGHTNESS 164


//////////////////////////////////////////////////////////////////////////////
// General Functions
//////////////////////////////////////////////////////////////////////////////
int getNumColumns();
int getColumnOffsetX_DDR(int column);
int getColumnOffsetX_DM(int column);
int getColumnOffsetX_BM(int column);
int getColumnOffsetX_DMX(int column);

int getLeftmostColumnX_DMX(int player);

int getCenterOfLanesX(int player);

void loadGameplayGraphics();
// precondition: called only once when the program starts
// postcondition: allocates a bunch of memory that isn't freed until the program quits


//////////////////////////////////////////////////////////////////////////////
// Primary Functions - these call the mode-specific functions
//////////////////////////////////////////////////////////////////////////////
void renderGameplay();
// precondition: called only once per frame after the logic is complete
// postcondition: rm.m_backbuf contains a fresh image of the current game state

void renderStageDisplay();
// precondition: gs.currentStage is 0-6 or the game is in the demo loop
// postcondition: renders a bitmap, top center, telling the player the stage number

void renderGrade(int grade, int x, int y);
// precondition: grade is a valid grade enumeration
// postcondition: the letter grade is rendered to the screen

void renderSpeedMod(int scrollRate, int speedMod, int x, int y);
//
//

void renderLifebar(int player);
//
//

void renderFullComboAnim(int x, int time, int step, bool isPerfect);
//
//

void renderEndSongMarker(int x1, int x2, int y);
void renderBPMMarker(int x1, int x2, int y, int bpm);
void renderTempoStopMarker(int x1, int x2, int y, int len);
//
//


//////////////////////////////////////////////////////////////////////////////
// Mode-Specific Functions
//////////////////////////////////////////////////////////////////////////////
int  getStepZonePos_DDR(char player);
void renderStepZone_DDR(int player);
void renderFlaresAndFlashes_DDR(int player);
void renderStepZonePiece_DDR(BITMAP* src, int x, int y, int scale);

// a generic "renderNote" function for debugging
void renderDebugNote(struct ARROW n, int x, int y);

// helper functions for renderChart()
void renderDDRChart(int player);
void renderDDRNote(int player, struct ARROW n, int x, int y);
void renderDDRArrow(int player, int column, int color, int judgement, int x, int y); // called only by renderDDRNote()
char renderDDRHoldNote(int player, struct FREEZE f, UTIME time, long pausedTime);    // returns HOLD_PASSED_BY, HOLD_RENDERED, or HOLD_TOO_EARLY
void renderDDRHoldNoteBody(int column, int x, int topy, int bot, int color );        // called only by renderDDRHoldNote()

void renderDDRJudgement(int judgement, int time, int x, int y);
void renderDDRCombo(int combo, int time, int centerX, int color);
void renderDDRComboNum(int num, int size, int color, int x, int y);            // called only by renderDDRCombo()

void renderDMChart(int player);
void renderDMNote(int player, struct ARROW n, int x, int y);
void renderDMChip(int player, int column, int color, int judgement, int x, int y);
char renderDMHoldNote(struct FREEZE f, UTIME time, long pausedTime);
void renderColumnJudgements(int player);
void renderDMCombo(int combo, int player);
void renderDMComboNum(int num, int x, int y);                                  // called only by renderDMCombo()

void renderDMXChart(int player);
void renderDMXNote(int player, struct ARROW n, int y);
void renderDMXArrow(int player, int column, int color, int judgement, int x, int y);       // called only by renderDMXNote()
char renderDMXHoldNote(int player, struct FREEZE f, UTIME time, long pausedTime);          // returns HOLD_PASSED_BY, HOLD_RENDERED, or HOLD_TOO_EARLY
void renderDMXHoldNoteBody(int column, int x, int topy, int bot, int color, int player);
void renderDMXCombo(int combo, int time, int centerX, int color);
void renderDMXJudgement(int judgement, int time, int x, int y);

void renderOverFrameDMX();
void renderUnderFrameDMX();
void renderStepZoneDMX(int player);
void renderStepLaneDMX(int x, int bgColor, int outlineColor);
void renderDMXFlaresAndFlashes(int player);
void renderBatteryLivesDMX(int player);

#endif // end include guard