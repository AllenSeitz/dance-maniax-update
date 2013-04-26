// scoreManager.h implements a class to record scores for Dance Maniax
// source file created by Allen Seitz 2/12/2012

#ifndef _SCOREMANAGER_H_
#define _SCOREMANAGER_H_

#include "common.h"

extern int NUM_SONGS;

struct SONG_RECORD
{
	int  songID;		//
	int  chartID;		// for example, SINGLE_WILD
	int  perfects;		//
	int  greats;		//
	int  goods;			//
	int  misses;		//
	int  maxCombo;		// this is saved as each song is played
	int  points;        // dance points
	int  maxPoints;		// this is the maximum dance points (max score is 1 mil)
	char status;		// for example, STATUS_NONE, STATUS_CLEAR
	char grade;			// for example, GRADE_A
	long time;			// when this score was set (seconds since Jan 1, 1970)
	char unlockStatus;  // normally 0, but could be nonzero for an unlock

	SONG_RECORD::SONG_RECORD()
	{
		resetData();
	}

	void SONG_RECORD::resetData()
	{
		songID = chartID = 0;
		perfects = greats = goods = misses = 0;
		maxCombo = points = maxPoints = status = 0;
		grade = 0;
		time = 0;
		unlockStatus = 0;
	}

	int SONG_RECORD::getDancePoints()
	{
		return perfects*2 + greats;
	}

	int SONG_RECORD::getScore()
	{
		if ( maxPoints == 0 )
		{
			if ( getDancePoints() != 0 )
			{
				return 5; // this indicates a serious error in the data permanence, and player data may be lost!
			}
			return 0;
		}
		return (getDancePoints() * 100000 / maxPoints) * 10;
	}

	int SONG_RECORD::calculateStatus()
	{
		if ( maxPoints == 0 )
		{
			status = STATUS_NONE; // can happen, for example 2P during a single player game
		}
		else if ( misses == 0 )
		{
			status = STATUS_FULLCOMBO;
			if ( greats == 0 )
			{
				status = STATUS_PERFECT;
			}
		}
		else if ( getScore() >= 700000 )
		{
			status = STATUS_CLEARED;
		}
		else
		{
			status = STATUS_FAILED;
		}

		return status;
	}

	int SONG_RECORD::calculateGrade()
	{
		if ( maxPoints == 0 )
		{
			status = GRADE_NONE; // can happen, for example 2P during a single player game
		}
		if ( getScore() >= 990000 )
		{
			grade = GRADE_AAA;
		}
		else if ( getScore() >= 950000 )
		{
			grade = GRADE_AA;
		}
		else if ( getScore() >= 900000 )
		{
			grade = GRADE_A;
		}
		else if ( getScore() >= 800000 )
		{
			grade = GRADE_B;
		}
		else if ( getScore() >= 700000 )
		{
			grade = GRADE_C;
		}
		else if ( getScore() >= 600000 )
		{
			grade = GRADE_D;
		}
		else
		{
			grade = GRADE_E;
		}

		return grade;
	}

	int calculatePoints()
	{
		return points = perfects*2 + greats;
	}
};

struct PLAYER_DATA
{
	bool isLoggedIn;
	char displayName[8];
	char pinDigits[4];
	SONG_RECORD currentSet[7];		   // these are the songs which have been played during just this credit
	SONG_RECORD** allTime; // these are the records, all-time, for this player (or blank)
	long numPlaysSP;
	long numPlaysDP;

	PLAYER_DATA::PLAYER_DATA()
	{
		allTime = NULL;
	}

	void PLAYER_DATA::allocateMemory()
	{
		if ( allTime == NULL )
		{
			allTime = (SONG_RECORD **)malloc(NUM_SONGS * sizeof(SONG_RECORD));

			for ( int i = 0; i < NUM_SONGS; i++ )
			{
				allTime[i] = (SONG_RECORD *)malloc(NUM_SONGS * 6);
			}
		}
	}

	void PLAYER_DATA::resetData()
	{
		static char chartTypes[6] = { SINGLE_MILD, SINGLE_WILD, SINGLE_ANOTHER, DOUBLE_MILD, DOUBLE_WILD, DOUBLE_ANOTHER };
		isLoggedIn = false;
		displayName[0] = 0;

		for ( int i = 0; i < 7; i++ )
		{
			currentSet[i].resetData();
		}

		for ( int i = 0; i < NUM_SONGS; i++ )
		for ( int chart = 0; chart < 6; chart++ )
		{
			allTime[i][chart].resetData();
			allTime[i][chart].songID = listID_to_songID(i);
			allTime[i][chart].chartID = chartTypes[chart];
		}

		numPlaysSP = numPlaysDP = 0;
	}
};

class ScoreManager
{
public:
	ScoreManager::ScoreManager();

	void resetData();
	// postcondition: zeros out both players

	bool loadPlayerFromDisk(char* name, char side);
	// precondition: name is 1-8 letters [0-9,A-Z], side is 0-1
	// postcondition: loads the player if it exists and sets isLoggedIn to true on success

	void savePlayersToDisk();
	// postcondition: for each player, if isLoggedIn is true, creates a new file on disk

	bool doesPlayerNameExist(char* name);
	// precondition: name is 8 or less in length and contains only symbols allowed in filenames
	// postcondition: returns true if PLAYERS/name.prefs exists

	bool isPinCorrect(int pin, int side);
	// precondition: pin is [0..9999]
	// postcondition: returns true only if a player is loaded and this is their pin

private:
	void mergeCurrentScores(PLAYER_DATA &p, int side);
	// postcondition: reconciles player[side].currentSet with player[side].allTime (basically)

	void savePlayerToDisk(PLAYER_DATA &p);
	// postcondition: writes or rewrites players/NAME.score on disk

public:
	PLAYER_DATA player[2];
};

#endif