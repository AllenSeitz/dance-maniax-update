// gameStateManager.h declares a "class" that manages the gameplay state for the gameplayMode, gameplayRendering, and gameplayDirector objects
// source file created by Allen Seitz 12/20/09

#ifndef _GAMESTATEVARS_H_
#define _GAMESTATEVARS_H_

#include "common.h"
#include <vector>

#include "fmod.h"				// an audio library used for playing keysounds

/*
#ifdef __cplusplus
extern "C" {
#endif
//#include "logg.h"               // an addon for ogg support
#ifdef __cplusplus
}
#endif
//*/

#define MSETTING_FILENAME "conf/msetting.dat"

#define MODE_NONSTOP 0
#define MODE_FREE 1
#define MODE_MISSION 2

class GameStateManager
{
public:

	// program flow data
	int g_currentGameMode;
	int g_gameModeTransition;
	int g_errorCode;
	char g_errorInfo[256];

	// the current BGM
	FSOUND_SAMPLE* currentSongSample;
	int     currentSong;
	int     currentSongLength;
	int     currentSongChannel; // used by FMOD hardware functions
	int     bgmGap;             // how many ms early or late to start the bgm

	// non-player game state data
	bool isSolo;                // should not be used in this program
	bool isDoubles; 
	bool isVersus;
	int  currentStage;
	int  numCoins;
	int  currentGameType;		// 0 = nonstop, 1 = free, 2 = mission

	// operator data
	bool isInitialized;
	int  numSongsPerSet;
	int  numCoinsPerCredit;
	bool isFreeplay;
	bool isVersusPremium;
	bool isDoublePremium;
	bool isEventMode;

	class PLAYER
	{
	public:
		// chart data
		std::vector<struct ARROW> currentChart;
		std::vector<struct FREEZE> freezeArrows;
		int   currentNote;        // the index of the closest note that hasn't passed
		int   lastLateNote;       // the index of the last note that hasn't been scored
		int   currentFreeze;      // the index of the next freeze arrow to render
		UTIME timeElapsed;        // the current time position into the currentChart

		// scroll rate
		int   scrollRate;         // the current BPM
		int   newScrollRate;      // what the BPM is changing to
		UTIME stopTime;           // the timeElapsed that the tempo stop happened at
		UTIME stopLength;         // the milliseconds that the current tempo stop lasts for
		UTIME bpmUpdateTimer;     // allows the BPM to change smoothly from scrollRate to newScrollRate
		int   speedMod;           // the current speed mod times 10 (for 1.5, 2.5, etc)

		// animation timers and states (mostly timers)
		UTIME stepZoneBeatTimer;
		UTIME stepZoneBlinkTimer;
		UTIME stepZoneTimePerBeat;
		bool  everyOtherBeat;            // DMX needs to blink only every other beat
		UTIME stepZoneResizeTimers[10];
		int   colorCycle;                // [0..3], rotates the arrows between the four colors
		int   judgementTime;             // time since the last judgement was registered
		int   lastJudgement;             // what the most recent Judgement was
		int   columnJudgeTime[10];
		int   columnJudgement[10];
		int   laneFlareColors[10];
		UTIME laneFlareTimers[10];
		UTIME shockAnimTimer;            // greater than 0 while the shock anim is playing
		UTIME drummaniaCombo[4];         // each digit of the combo bounces independently

		// player's score
		int displayCombo;
		int comboColor;
		int currentSongCombo;
		int currentSongMaxCombo;

		// lifebar related
		int  lifebarPercent;			// a number from 0 (empty) to 1000 (full)
		int  lifebarLives;				// when using the battery, lives lift
		bool useBattery;				// switches the lifebar mode

		// special trick rendering modes
		bool drummaniaMode;             // pretty much switches the combo and enables "column judgements"
		bool danceManiaxMode;           // added for Dance Maniax (Yes, this is technically a modifier. Without it the game renders DDR!)

		// modifiers
		bool darkModifier;              // hides the step zone
		unsigned char reverseModifier;  // 8 bits for which columns are on reverse
		unsigned char suddenModifier;   // 8 bits for which columns are on sudden
		unsigned char hiddenModifier;   // 8 bits for which columns are on hidden
		bool stealthModifier;           // special rendering mode
		char arrangeModifier;           // 1 = mirror, 2 = upside-down

		// segment choosing logic
		int stagesPlayed[7];			// stages picked by the player
		int stagesLevels[7];

		PLAYER::PLAYER()
		{
			resetAll();
			resetStages();
		}

		void nextStage()
		{
			currentNote = lastLateNote = currentFreeze = 0;
			timeElapsed = 0;

			stopTime = 0;
			stopLength = 0;
			bpmUpdateTimer = 0;

			for ( int i = 0; i < 10; i++ )
			{
				stepZoneResizeTimers[i] = 0;
				columnJudgeTime[i] = 0;
				columnJudgement[i] = 0;
				laneFlareColors[i] = 0;
				laneFlareTimers[i] = 0;
			}

			// animation timers and states (mostly timers)
			stepZoneBeatTimer = 0;
			stepZoneBlinkTimer = 0;
			stepZoneTimePerBeat = 400;
			colorCycle = 0;
			judgementTime = 0;
			lastJudgement = 0;
			shockAnimTimer = 0;
			drummaniaCombo[0] = drummaniaCombo[1] = drummaniaCombo[2] = drummaniaCombo[3] = 0;

			//comboColor = 1;
			currentSongCombo = 0;
			currentSongMaxCombo = 0;

			// lifebar related
			lifebarPercent = 0;
		}

		void resetAll()
		{
			nextStage();

			// scroll rate
			scrollRate = 150;
			newScrollRate = 150;
			speedMod = 10;

			// player's score
			displayCombo = 0;
			comboColor = 1;

			// lifebar related
			lifebarLives = 4;
			useBattery = false;

			// tricky Variables
			drummaniaMode = false;
			danceManiaxMode = true;
			darkModifier = false;
			reverseModifier = 0;
			suddenModifier = 0;
			hiddenModifier = 0;
			stealthModifier = false;
			arrangeModifier = 0;
		}

		void resetStages()
		{
			for ( int i = 0; i < 7; i++ )
			{
				stagesPlayed[i] = -1;
				stagesLevels[i] = 0;
			}
		}

		bool isColumnReversed(char col)
		{
			if ( col == -1 )
			{
				return false;
			}
			return (reverseModifier & (1 << col)) > 0;
		}
	};

	// per-player data
	PLAYER player[2];

	GameStateManager::GameStateManager()
	{
		player[0].resetAll();
		player[0].resetStages();
		player[1].resetAll();
		player[1].resetStages();

		// program flow data
		g_currentGameMode = FIRST_GAME_MODE;
		g_gameModeTransition = 1;
		g_errorCode = 0;

#ifdef CHART_RECORDING
		g_currentGameMode = RECORDING;
#endif

		// directly audio related
		currentSong = 0;
		currentSongSample = NULL;
		currentSongLength = -1;
		currentSongChannel = -1;

		// global game state
		isSolo = false;
		isDoubles = false;
		isVersus = false;
		currentStage = 0;
		numCoins = 0;
		currentGameType = 1;
		if ( g_currentGameMode == SONGWHEEL )
		{
			//isVersus = true; // for debugging - otherwise this cannot happen
		}

		// operator settings
		isInitialized = false;
		numSongsPerSet = DEFAULT_SONGS_PER_SET;
		numCoinsPerCredit = DEFAULT_COINS_PER_CREDIT;
		isFreeplay = false;
		isVersusPremium = false;
		isDoublePremium = false;
		isEventMode = false;
	}

	void loadSong(int songID)
	{
		char filename[] = "DATA/audio/112.mp3";
		filename[11] = (songID/100 % 10) + '0';
		filename[12] = (songID/10 % 10) + '0';
		filename[13] = (songID % 10) + '0';

		killSong();
		currentSongSample = FSOUND_Sample_Load(FSOUND_UNMANAGED, filename, FSOUND_NORMAL | FSOUND_MPEGACCURATE, 0, 0);
		if ( currentSongSample == NULL )
		{
			// try again as an ogg
			filename[15] = 'o';
			filename[16] = 'g';
			filename[17] = 'g';
			currentSongSample = FSOUND_Sample_Load(FSOUND_UNMANAGED, filename, FSOUND_NORMAL, 0, 0);

			if ( currentSongSample == NULL )
			{
				globalError(UNABLE_TO_LOAD_AUDIO, filename);
			}
		}

		currentSong = songID;
		currentSongLength = 0; // TODO: figure out how to calculate the length of a stream? or just look it up
	}

	void killSong()
	{
		if ( currentSongSample != NULL )

		{
			FSOUND_Sample_Free(currentSongSample);
			currentSongChannel = -1;
			currentSong = -1;
		}
		currentSongSample = NULL;
	}

	void playSong()
	{
		currentSongChannel = FSOUND_PlaySound(FSOUND_FREE, currentSongSample);
		FSOUND_SetVolume(currentSongChannel, DONT_WANNA_HEAR_IT ? 0 : 128); // '64' leaves room to grow/shrink (turn up other volume controls)
		FSOUND_SetLoopMode(currentSongChannel, currentSong < 100 ? FSOUND_LOOP_NORMAL : FSOUND_LOOP_OFF); // menu music should loop
	}

	void pauseSong()
	{
		FSOUND_StopSound(currentSongChannel);
	}

	void setSongPosition(int offset)
	{
		unsigned int numSamples = FSOUND_GetCurrentPosition(currentSongChannel);
		if ( numSamples == 0 )
		{
			return;
		}

		unsigned int samplesPerSec = numSamples/player[0].timeElapsed;

		player[0].timeElapsed += offset;
		player[1].timeElapsed += offset;
		FSOUND_SetCurrentPosition(currentSongChannel, samplesPerSec*player[0].timeElapsed);
	}

	void setSongVolume(unsigned char vol)
	{
		FSOUND_SetVolume(currentSongChannel, (int)vol); // volume is 0-255, the int is misleading
	}

	bool isSongPlaying()
	{
		if ( currentSongChannel == -1 )
		{
			return false;
		}
		return FSOUND_IsPlaying(currentSongChannel) != 0;
	}

	void loadOperatorSettings()
	{
		FILE* fp = NULL;
		int n = 0;

		// does bookkeeping exist?
		if ( (fp = safeLoadFile(MSETTING_FILENAME)) == NULL )
		{
			return; // failure to load operator settings requires them to be reset manually for safety
		}

		// read a short version number
		int vnum = checkFileVersion(fp, "DMXM");
		if ( vnum != CURRENT_VERSION_NUMBER )
		{
			TRACE("Wrong msettings version number.");
			return; // later when multiple versions exist, ideas for converting them
		}

		// okay, read everything
		fread(&numSongsPerSet, sizeof(long), 1, fp);
		fread(&numCoinsPerCredit, sizeof(long), 1, fp);
		fread(&n, sizeof(long), 1, fp);
		isFreeplay = n != 0;
		fread(&n, sizeof(long), 1, fp);
		isVersusPremium = n != 0;
		fread(&n, sizeof(long), 1, fp);
		isDoublePremium = n != 0;

		isInitialized = true;
		fclose(fp);
	}

	void saveOperatorSettings()
	{
		FILE* fp = NULL;
		int n = 0;

		if ( fopen_s(&fp, MSETTING_FILENAME, "wb") != 0 )
		{
			TRACE("UNABLE TO SAVE BOOKKEEPING");
			return; // this is bad
		}

		// write the version
		fprintf(fp, "DMXM");
		int vnum = CURRENT_VERSION_NUMBER;
		fwrite(&vnum, sizeof(long), 1, fp);
		fwrite(&numSongsPerSet, sizeof(long), 1, fp);
		fwrite(&numCoinsPerCredit, sizeof(long), 1, fp);
		n = isFreeplay ? 1 : 0;
		fwrite(&n, sizeof(long), 1, fp);
		n = isVersusPremium ? 1 : 0;
		fwrite(&n, sizeof(long), 1, fp);
		n = isDoublePremium ? 1 : 0;
		fwrite(&n, sizeof(long), 1, fp);

		safeCloseFile(fp, MSETTING_FILENAME);
	}

	const char* getErrorString()
	{
		switch (g_errorCode)
		{
		case 3000: return "UNSPECIFIC NVRAM ERROR";
		case UNSET_MACHINE_SETTINGS: return "MACHINE SETTINGS NEED TO BE SET BEFORE FIRST USE.";
		case BOOKKEEPING_NOT_FOUND: return "BOOKKEEPING DATA NOT FOUND.";
		case CLEARED_MACHINE_SETTINGS: return "THE MACHINE SETTINGS HAVE BEEN CLEARED.";
		case 4000: return "UNSPECIFIC SONG DATA ERROR";
		case SONG_DATA_INCOMPLETE: return "SONG DATA INCOMPLETE";
		case ERROR_DECODING_VIDEO: return "VIDEO DECODE ERROR";
		case UNABLE_TO_LOAD_AUDIO: return "AUDIO DECODE ERROR";
		case INVALID_SONG_ID: return "INVALID SONG ID";
		case 5000: return "UNSPECIFIC PLAYER DATA ERROR";
		case PLAYER_PREFS_LOST: return "PLAYER RECORD LOST";
		case PLAYER_SCORES_LOST: return "PLAYER SCORE DATA LOST";
		case 6000: return "UNSPECIFIC UPDATE ERROR";
		case UPDATE_FAILED: return "FAILED TO START UPDATE PROCESS";
		}
		return "(no error description)";
	}
};

#endif // end include guard
