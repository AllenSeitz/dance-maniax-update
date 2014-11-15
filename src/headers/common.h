// common.h declares some common stuff that I need in most source files
// file created by Allen Seitz 7/18/2009

#ifndef _COMMONDMXH_H_
#define _COMMONDMXH_H_

#pragma warning(disable : 4312)
#include "allegro.h"			// allegro library
#include "winalleg.h"
#include "loadpng.h"            // extra functions for PNG support

#include "mmsystem.h"			// high resolution Windows specific timers

#include "fmod.h"				// an audio library used for playing keysounds
#pragma warning(default : 4312)

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <string>

#define UTIME unsigned long int

#define UNUSED(var) (void)(var)

// use this while developing the game to disable most types of audio
#ifdef DMXDEBUG
#define DONT_WANNA_HEAR_IT 0
#else // and use this to ensure that release mode is never accidentally built without audio
#define DONT_WANNA_HEAR_IT 0
#endif

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define CURRENT_SETTING_VERSION_NUMBER 2
#define CURRENT_BKEEP_VERSION_NUMBER 1
#define CURRENT_HITLIST_VERSION_NUMBER 1
#define CURRENT_SCORE_VERSION_NUMBER 2
#define CURRENT_PLAYER_VERSION_NUMBER 1

// for the operator menu
#define WHITE makeacol(255,255,255, 255)
#define RED makeacol(255,32,32, 255)
#define GREEN makeacol(100,255,100, 255)
#define BLUE makeacol(100,100,255, 255)
#define PURPLE makeacol(255,64,255, 255)
#define YELLOW makeacol(255,255,64, 255)
#define DARKGRAY makeacol(96,96,96, 255)
#define GET_ON_OFF(b) (b ? "ON" : "OFF")
#define GET_ON_COLOR(b) (b ? RED : WHITE)
#define GET_LAMP_STRING(n) (n == 1 ? "RED" : n == 2 ? "BLUE" : n == 3 ? "PURPLE" : "OFF")
#define GET_LAMP_COLOR(n) (n == 1 ? RED : n == 2 ? BLUE : n == 3 ? PURPLE : WHITE)

#define DEFAULT_SONGS_PER_SET 3
#define DEFAULT_COINS_PER_CREDIT 2

// generally useful functions
#define SUBTRACT_TO_ZERO(x,y) (x = y > x ? 0 : x - y)
#define WEIGHTED_AVERAGE(a,b,weight,denominator) ( ((a*weight) + (b*(denominator-weight))) / denominator )
#define BPM_TO_MSEC(bpm) (60000.0/bpm)
#define MSEC_TO_BPM(msec) (60000.0/msec)

int pickRandomInt(int argc, ...);
int getValueFromRange(int min, int max, int percent);

void addLeadingZeros(char* buffer, int desiredLength);
// precondition: buffer is a null-terminated string and is at least desiredLength in size
// postcondition: moves the string down and adds '0' chars. For example "1234" -> "0001234"

// used by the lightning effect, other places
struct AL_POINT
{
	int x;
	int y;

	AL_POINT() { x = y = 0; }
};

// game modes
#define SPLASH 0
#define WARNING 1
#define SONGWHEEL 2
#define GAMEPLAY 3
#define RECORDING 4
#define MAINMENU 5
#define RESULTS 6
#define TESTMODE 7
#define ATTRACT 8
#define LOGIN 9
#define GAMEOVER 10
#define ERRORMODE 11
#define VIDEOTEST 12
#define CAUTIONMODE 13
#define NONSTOP 14
#define FAILURE 15
#define PLAYERSELECT 16
#define VOTEMODE 17
#define BOOTMODE 18
#ifdef DMXDEBUG
	#define FIRST_GAME_MODE BOOTMODE
	//#define FIRST_GAME_MODE SONGWHEEL
	//#define FIRST_GAME_MODE GAMEPLAY
	//#define FIRST_GAME_MODE PLAYERSELECT
	//#define FIRST_GAME_MODE ATTRACT
#else
	#define FIRST_GAME_MODE BOOTMODE
#endif

// play modes, for logging with bookkeeping
#define SINGLES_PLAY 1
#define DOUBLES_PLAY 2
#define VERSUS_PLAY 3
#define MISSION_PLAY 4

// global error codes used by error mode
#define UNSET_MACHINE_SETTINGS   3001
#define BOOKKEEPING_NOT_FOUND    3002
#define CLEARED_MACHINE_SETTINGS 3003
#define SONG_DATA_INCOMPLETE     4001
#define ERROR_DECODING_VIDEO     4002
#define UNABLE_TO_LOAD_AUDIO     4004
#define INVALID_SONG_ID          4003
#define BAD_NONSTOP_DATA         4005
#define MISSING_VIDEO_SCRIPT     4006
#define PLAYER_PREFS_LOST        5001
#define PLAYER_SCORES_LOST       5002
#define UPDATE_FAILED            6001
#define EXTIO_ERROR              7001

void globalError(long errorCode, const char* errorInfo);
void songIndexError(int id);

// implement safe asset loading
BITMAP* loadImage(const char* filename);

// implement a weak file backup system
FILE* safeLoadFile(char* filename);
void safeCloseFile(FILE* fp, char* filename);
char calculateChecksum(char* filename); // calculates the checksum for any file and should return 0 on any legit file
bool fileExists(char* filename);
void makeBackupFile(char* filename);

int checkFileVersion(FILE* fp, char* expected);


//////////////////////////////////////////////////////////////////////////////
// generic rendering functions, including text and debug rendering
//////////////////////////////////////////////////////////////////////////////

// implement some debug rendering
void renderWhiteLetter(char letter, int x, int y);
void renderWhiteString(const char* string, int x, int y);
void renderWhiteNumber(int number, int x, int y);

// implement real text rendering; colors 0-3 = white, green, pink, blue
void renderTextString(const char* string, int x, int y, int width, int height);
void renderBoldString(const unsigned char* string, int x, int y, int maxWidth, bool fixedWidth, int color = 0);
void renderBoldString(const char* string, int x, int y, int maxWidth, bool fixedWidth, int color = 0);
void renderArtistString(const unsigned char* string, int x, int y, int width, int height);
void renderArtistString(const char* string, int x, int y, int width, int height);
void renderScoreString(const char* string, int x, int y, int width, int height);
void renderScoreNumber(int number, int x, int y, int requiredDigits);
void renderNameString(const char* string, int x, int y, int color);
void renderNameLetter(char letter, int x, int y, int color);

// implement double buffering
class RenderingManager
{
public:
	void Initialize();

	BITMAP* m_backbuf;
	BITMAP* m_backbuf1;
	BITMAP* m_backbuf2;

	BITMAP* m_whiteFont;
	BITMAP* m_textFont;
	BITMAP* m_boldFont[4];
	BITMAP* m_artistFont;
	BITMAP* m_scoreFont;
	BITMAP* m_nameFont[3];

	BITMAP* m_temp64; // used for rendering effects on arrows

	bool useAlphaLanes;

	int currentPage;

	void flip();
	void screenshot();
	void renderWipeAnim(int frame); // frame must be 0-14
	void dimScreen(int percent); // must be 0-100
};

void replaceColor(BITMAP* bmp, long col1, long col2);
// precondition: bmp is 32 bit
// postcondition: any instances of col1 are replaced by col2


//////////////////////////////////////////////////////////////////////////////
// sound effects
//////////////////////////////////////////////////////////////////////////////
#define TOTAL_NUM_SFX 160
class EffectsManager
{
public:
	void initialize();

	void playSample(int which);
	void announcerQuip(int which);
	bool announcerQuipChance(int which, int percent);
	
	SAMPLE* basic_sfx[TOTAL_NUM_SFX];
	SAMPLE* currentAnnouncer;

	EffectsManager::EffectsManager()
	{
		currentAnnouncer = 0;
	}

	void announceCombo(int combo)
	{
		static int comboGroups[10][4] = 
		{
			{ 134, 134, 134, 134 }, // 100 "CHARISMA!"
			{ 142, 142, 145, 145 },
			{ 137, 137, 141, 141 },
			{ 124, 124, 124, 124 }, // 400 "wow lots of combos"
			{ 147, 147, 149, 149 },
			{ 125, 125, 125, 125 }, // 600 "the way of the combo is still continuing"
			{ 147, 147, 124, 124 },
			{ 149, 149, 125, 125 },
			{ 148, 148, 148, 148 }, // 900 "everybody is getting hot"
			{ 147, 134, 124, 125 }, // combo fever
		};
		int hundred = MIN(9, (combo/100)-1);
		announcerQuip( pickRandomInt(4, comboGroups[hundred][0], comboGroups[hundred][1], comboGroups[hundred][2], comboGroups[hundred][3]) );
	}
};

#define SFX_CREDIT_BEGIN 2
#define SFX_START_BUTTON 3
#define SFX_MENU_MOVE 4
#define SFX_MENU_STUCK 5
#define SFX_MENU_PICK 6
#define SFX_NONE 7
#define SFX_EAMUSE_DIAL 8
#define SFX_CROWD_HAPPY 9
#define SFX_CROWD_SCARED 10
#define SFX_MENU_PICK2 11
#define SFX_DIFF 12 // a bit of a 'ting'
#define SFX_MOD_MOVE 14
#define SFX_FORCEFUL_SELECTION 15
#define SFX_TIMER_LOW 16
#define SFX_SOUND_CHECK1 17
#define SFX_SOUND_CHECK2 18
#define SFX_SONGWHEEL_MOVE 19
#define SFX_SONGWHEEL_PICK 20
#define SFX_SONGWHEEL_APPEAR 21
#define SFX_RESULTS_APPEAR 22
#define SFX_NAME_CURSOR_MOVE 23
#define SFX_NAME_CURSOR_WAGGLE 24
#define SFX_FAILURE_WHOOSH 25
#define SFX_GAME_OVER_EXPLOSION 27
#define SFX_FAILURE_ANIMATION 29
#define SFX_DMX2ND_APPEND 30
#define SFX_MENU_SORT 31
#define SFX_COURSE_APPEAR 32
#define SFX_COURSE_PREVIEW_LOAD 33
#define SFX_PIN_DIGIT 34
#define SFX_NAME_ENTRY_BACKSPACE 35
#define SFX_COURSE_ENTRY_CONFIRM 36
#define SFX_LOUD_BELL 37
#define SFX_CROWD_CHEERING 39
#define SFX_CROWD_BOOING 40
#define SFX_FULL_COMBO_SPLASH 157
#define SFX_BATTERY_ZAP 158
#define SFX_BATTERY_RECOVER 159
#define SFX_DIFFICULTY_MOVE SFX_MENU_MOVE
#define SFX_CREDIT SFX_SONGWHEEL_PICK
// be sure to update TOTAL_NUM_SFX above

#define BGM_TITLE 0
#define BGM_DEMO 1
#define BGM_MODE 2
#define BGM_RESULT 3
#define BGM_GAMEOVER 4

#define GUY_GAME_BEGIN pickRandomInt(2, 45, 114)
#define GUY_NONSTOP_BEGIN pickRandomInt(2, 47, 113)
#define GUY_EVERYBODY_WAITING 84
#define GUY_LOGIN pickRandomInt(4, 42, 46, 49, 84)
#define GUY_SONGWHEEL pickRandomInt(2, 80, 87)

#define GUY_RESULT_S pickRandomInt(2, 100, 101)
#define GUY_RESULT_B pickRandomInt(2, 97, 104)
#define GUY_RESULT_C pickRandomInt(2, 103, 108)
#define GUY_RESULT_D pickRandomInt(2, 91, 95)

#define GUY_EARN_EXTRA pickRandomInt(2, 115, 85) // one more time, made just for you, 
#define GUY_EARN_SPECIAL_EXTRA pickRandomInt(2, 82, 81) // wow youre a real dancer, can you do this, 
#define GUY_STAGE_FAILED pickRandomInt(2, 66, 67)
#define GUY_STAGE_HAZARD_FAILED pickRandomInt(2, 146, 131)
#define GUY_SPECIAL_GAMEOVER pickRandomInt(2, 110, 138)

#define GUY_MILD 73
#define GUY_WILD 74
#define GUY_MANIAX 77

//////////////////////////////////////////////////////////////////////////////
// implement the chart structure
//////////////////////////////////////////////////////////////////////////////
enum NOTE_TYPE
{
	TAP			= 1,
    JUMP        = 2,
	//HOLD_START  = 3,   // columns == which columns to hold
	//HOLD_END    = 4,   // columns == which columns to release
	SHOCK       = 5,   
	NEW_SECTION = 6,   // new banner/gimmick time
	END_SONG    = 7,   // new song segment time
	BPM_CHANGE  = 8,   // color = new BPM instead
	SCROLL_STOP = 9,   // color = stop length instead
};

// used while recording new charts
//#define CHART_RECORDING

// the charts being played are made out of these two structs
struct ARROW
{
	UTIME          timing;
	int            type;       // one of the enumerated note types
	int            color;      // 0-3, a color offset for the animation
	char           columns[4]; // which columns this note appears in
	char           judgement;  // the judgement earned for this arrow

	ARROW()
	{
		timing = type = color = judgement = 0;
		columns[0] = columns[1] = columns[2] = columns[3] = -1;
	}
};

struct FREEZE
{
	UTIME          startTime;
	UTIME          endTime1;
	UTIME          endTime2;
	char           columns[2];
	UTIME          meltTime1; // counts milliseconds while this isn't held
	UTIME          meltTime2; // counts milliseconds while this isn't held
	char           isHeld;    // set to 1 while this freeze is being held
	char           judgement; // the judgement earned for this arrow

	FREEZE()
	{
		startTime = 0;
		endTime1 = endTime2 = 0;
		columns[0] = columns[1] = -1;
		meltTime1 = meltTime2 = isHeld = 0;
		judgement = 0;
	}
};

#define HOLD_PASSED_BY -1
#define HOLD_RENDERED 0
#define HOLD_TOO_EARLY 1

#define HOLD_UNTOUCHED 0
#define HOLD_BRIGHT 1
#define HOLD_MELT 2

#define HOLD_DMX_RED 0
#define HOLD_DMX_BLUE 1
#define HOLD_DMX_GOLD 2
#define HOLD_DMX_BRIGHT 3
#define HOLD_DMX_MELT 4

int calculateArrowColor(unsigned long timing, int timePerBeat);
// precondition: timePerBeat is not 0
// returns: 0 = 1/4 beat, 1 = 1/8th beat, 2 = 1/16th beat, else 3


//////////////////////////////////////////////////////////////////////////////
// song list
//////////////////////////////////////////////////////////////////////////////
struct SongEntry
{
	long songID;
	int  minBPM;
	int  maxBPM;
	char mildSingle;
	char wildSingle;
	char maniaxSingle;
	char mildDouble;
	char wildDouble;
	char maniaxDouble;
	char version;     // for sorting
	int numPlays;     // the only non-static data in this structure
	char unlockFlag;
	bool isNew;

	SongEntry::SongEntry()
	{
		songID = 0;
		isNew = false;
	}

	void SongEntry::initialize(long id, int min, int max, char ms, char ws, char xs, char md, char wd, char xd, char v)
	{
		songID = id;
		minBPM = min;
		maxBPM = max;
		mildSingle = ms;
		wildSingle = ws;
		maniaxSingle = xs;
		mildDouble = md;
		wildDouble = wd;
		maniaxDouble = xd;
		version = v;
		numPlays = 0;
		unlockFlag = 0;
	}
};

#define SINGLE_MILD 0
#define SINGLE_WILD 1
#define SINGLE_ANOTHER 2 // wanted to have "LAZY" and "CRAZY", but there's no need
#define DOUBLE_MILD 10
#define DOUBLE_WILD 11
#define DOUBLE_ANOTHER 12 

#define UNLOCK_METHOD_NONE 0
#define UNLOCK_METHOD_EXTRA_STAGE 1
#define UNLOCK_METHOD_CLEAR 2
#define UNLOCK_METHOD_SPECIAL 3
#define UNLOCK_METHOD_ROULETTE 4

int songID_to_listID(int songID);
int listID_to_songID(int index);
// precondition: the index or songID is valid
// postcondition: returns the matching value. For example: 101 <-> 0 and 102 <-> 1

int getChartIndexFromType(int type);
// precondition: type is one of the above enumerations
// postcondition: returns an index 0-6 that is used a few places


//////////////////////////////////////////////////////////////////////////////
// sound effects
//////////////////////////////////////////////////////////////////////////////
void playSFXOnce(SAMPLE* sample);
// precondition: sample is not NULL
// postcondition: calls an Allegro function to mix the audio automatically

int getSampleLength(SAMPLE* sample);
// precondition: sample is not NULL
// postcondition: returns the length of the sample in seconds


//////////////////////////////////////////////////////////////////////////////
// gameplay elements
//////////////////////////////////////////////////////////////////////////////

// normal judgements
#define MARVELLOUS 1
#define PERFECT    2
#define GREAT      3
#define GOOD       4
//#define BAD        5
#define MISS       6
#define OK         7
#define NG         8
#define IGNORED    9
#define UNSET      0

// different combo colors
#define COMBO_SHADOW    0
#define COMBO_GREAT     1
#define COMBO_PERFECT   2
#define COMBO_MARVELOUS 3
#define COMBO_MISS      4


//////////////////////////////////////////////////////////////////////////////
// score related
//////////////////////////////////////////////////////////////////////////////
#define GRADE_NONE 0
#define GRADE_F 1
#define GRADE_E 2
#define GRADE_D 3
#define GRADE_C 4
#define GRADE_B 5
#define GRADE_A 6
#define GRADE_AA 7
#define GRADE_AAA 8

#define STATUS_NONE 0
#define STATUS_FAILED 1
#define STATUS_CLEARED 2
#define STATUS_FULL_GOOD_COMBO 3
#define STATUS_FULL_GREAT_COMBO 4
#define STATUS_FULL_PERFECT_COMBO 5


//////////////////////////////////////////////////////////////////////////////
// IO board related
//////////////////////////////////////////////////////////////////////////////

// used only while debugging to speed up the IO test
#define TRUSTED_COM_PORT 6

// used for all input and output pins. also used to distinguish lamps for the LightsManager
enum PinACIO
{
	// input pins
	buttonStartP1 = 2,
	buttonLeftP1 = 3,
	buttonRightP1 = 4,
	buttonStartP2 = 5,
	buttonLeftP2 = 6,
	buttonRightP2 = 7,

	// red input pins
	sensorLeftRedP1 = 8,
	sensorRightRedP1 = 9,
	sensorLeftRedP2 = 10,
	sensorRightRedP2 = 11,
	diagnostic = 12,           // not hooked up / reserved for potential hardware test
	internalLED = 13,          // not hooked up

	// blue input pins
	sensorBlueLeft0P1 = 14,    // straight down
	sensorBlueLeft1P1 = 15,    // angled down
	sensorBlueRight0P1 = 16,   // straight down
	sensorBlueRight1P1 = 17,   // angled down
	sensorBlueLeft0P2 = 18,    // straight down
	sensorBlueLeft1P2 = 19,    // angled down
	sensorBlueRight0P2 = 20,   // straight down
	sensorBlueRight1P2 = 21,   // angled down

	// lamp output pins - 2P uses the odd numbers (add one)
	coinCounter = 22,          // coin OUTPUT
	coinLockout = 23,          // coin OUTPUT
	lampLeft = 24,
	lampRight = 26,
	lampStart = 28,
	lampRed0 = 30,
	lampRed1 = 32,
	lampRed2a = 34,
	lampRed2b = 36,
	lampRed3a = 38,
	lampRed3b = 40,
	lampBlue0 = 42,
	lampBlue1 = 44,
	lampBlue2a = 46,
	lampBlue2b = 48,
	lampBlue3a = 50,
	lampBlue3b = 52,	

	// analog input pins
	vgaInputRed = 100,
	vgaInputGreen = 101,
	vgaInputBlue = 102,
	vgaInputSync = 103,
	vgaInputGround = 104,
	test = 105,                // could be on a digital pin, but it fits here
	service = 106,             // could be on a digital pin, but it fits here
	coin = 107,                // could be on a digital pin, but it fits here

	// analog output pins
	vgaOutputRed = 108,
	vgaOutputGreen = 109,
	vgaOutputBlue = 110,
	vgaOutputSync = 111,
	vgaOutputGround = 112,
	spotlightA = 113,          // the spotlight in the middle (yellow)
	spotlightB = 114,          // spotlights 2 + 4, always together (pink)
	spotlightC = 115,          // spotlights 1 + 5, always together (blue)
};

struct LAMP_STEP
{
	UTIME     timing;
	int       group;        // 0-11 signals a group number, or 113-115 signals a spotlight
	int	      addDuration;  // how much time to add to this lamp
	int       color;        // 0=red, 1=blue, 2=purple, ignored for spotlights
	char      option;       // dunno yet

	LAMP_STEP()
	{
		timing = group = addDuration = color = option = 0;
	}

	static bool sortNoteFunction(struct LAMP_STEP a, struct LAMP_STEP b)
	{
		if ( a.timing == b.timing )
		{
			return a.group < b.group;
		}
		return a.timing < b.timing;
	}
};

#endif // end include guard