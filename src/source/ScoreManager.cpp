// scoreManager.cpp implements a class to record scores for Dance Maniax
// source file created by Allen Seitz 2/12/2012

#include <stdio.h>
#include <string>

#include "scoreManager.h"

#include "GameStateManager.h"
extern GameStateManager gs;

ScoreManager::ScoreManager()
{
}

void ScoreManager::resetData()
{
	player[0].allocateMemory();
	player[1].allocateMemory();
	player[0].resetData();
	player[1].resetData();
}

bool ScoreManager::loadPlayerFromDisk(char* name, char side)
{
	FILE* fp = NULL;
	struct PLAYER_DATA& p = side == 0 ? player[0] : player[1];

	char baseFilename[64] = "PLAYERS/";
	char prefsFilename[64] = "";
	char scoreFilename[64] = "";

	// calculate the filenames
	for ( int i = 0; i < 8; i++ )
	{
		baseFilename[8+i] = name[i]; //can't strcat because it isn't null terminated
	}
	strcat_s(prefsFilename, 64, baseFilename);
	strcat_s(prefsFilename, 64, ".prefs");
	strcat_s(scoreFilename, 64, baseFilename);
	strcat_s(scoreFilename, 64, ".score");

	// load the preferences
	if ( (fp = safeLoadFile(prefsFilename)) == NULL )
	{
#ifdef _DEBUG
		allegro_message("Error reading: %s", prefsFilename);
#endif
		globalError(PLAYER_PREFS_LOST, prefsFilename);
		return false;
	}

	// read a short version number
	int vnum = checkFileVersion(fp, "DMXp");
	if ( vnum != CURRENT_PLAYER_VERSION_NUMBER )
	{
		globalError(PLAYER_PREFS_LOST, prefsFilename);
	}

	fread(&p.displayName, sizeof(char), 8, fp);
	fread(&p.pinDigits, sizeof(char), 4, fp);
	fread(&p.numPlaysSP, sizeof(long), 1, fp);
	fread(&p.numPlaysDP, sizeof(long), 1, fp);

	fclose(fp);

	// then read all the scores to the scores file
	if ( (fp = safeLoadFile(scoreFilename)) == NULL )
	{
#ifdef _DEBUG
		allegro_message("Error reading: %s", scoreFilename);
#endif
		globalError(PLAYER_SCORES_LOST, scoreFilename);
		return false;
	}

	vnum = checkFileVersion(fp, "DMXs");
	if ( vnum != CURRENT_SCORE_VERSION_NUMBER )
	{
		if ( vnum == 1 )
		{
			return loadPlayerFromDisk_v1(fp, side);
		}
		else
		{
			globalError(PLAYER_SCORES_LOST, scoreFilename);
		}
	}

	for ( int i = 0; i < NUM_SONGS; i++ ) // loop through each song
	for ( int j = 0; j < 6; j++ )         // loop through each chart
	{
		SONG_RECORD temp;
		if ( fread(&temp.songID, sizeof(int), 1, fp) == 0 || fread(&temp.chartID, sizeof(int), 1, fp) == 0 )
		{
			i = NUM_SONGS; // file ended short. use the default blank scores for any new songs added to the DB
			j = 6;
			break;
		}
		fread(&temp.grade, sizeof(char), 1, fp);
		fread(&temp.status, sizeof(char), 1, fp);
		fread(&temp.time, sizeof(long), 1, fp);
		fread(&temp.maxCombo, sizeof(int), 1, fp);
		fread(&temp.points, sizeof(int), 1, fp);
		fread(&temp.maxPoints, sizeof(int), 1, fp);
		fread(&temp.perfects, sizeof(int), 1, fp);
		fread(&temp.greats, sizeof(int), 1, fp);
		fread(&temp.goods, sizeof(int), 1, fp);
		fread(&temp.misses, sizeof(int), 1, fp);
		fread(&temp.unlockStatus, sizeof(char), 1, fp);

		int songIndex = songID_to_listID(temp.songID);
		int chartIndex = getChartIndexFromType(temp.chartID);

		temp.grade = temp.calculateGrade(); // to update old 98% = 'AA' scores to the new 98% = 'AAA' cutoff

		if ( songIndex >= 0 && chartIndex >= 0 ) // otherwise would indicate a song which was cut
		{
			p.allTime[songIndex][chartIndex] = temp;
		}
		else
		{
			TRACE("Unable to use high score for song %d chart %d.", temp.songID, temp.chartID);
		}
	}

	fclose(fp);
	return true;
}

bool ScoreManager::loadPlayerFromDisk_v1(FILE* fp, char side)
{
	struct PLAYER_DATA& p = side == 0 ? player[0] : player[1];

	for ( int i = 0; i < NUM_SONGS; i++ ) // loop through each song
	for ( int j = 0; j < 6; j++ )         // loop through each chart
	{
		SONG_RECORD temp;
		if ( fread(&temp.songID, sizeof(int), 1, fp) == 0 || fread(&temp.chartID, sizeof(int), 1, fp) == 0 )
		{
			i = NUM_SONGS; // file ended short. use the default blank scores for any new songs added to the DB
			j = 6;
			break;
		}
		fread(&temp.grade, sizeof(char), 1, fp);
		fread(&temp.status, sizeof(char), 1, fp);
		fread(&temp.time, sizeof(long), 1, fp);
		fread(&temp.maxCombo, sizeof(int), 1, fp);
		fread(&temp.points, sizeof(int), 1, fp);
		fread(&temp.maxPoints, sizeof(int), 1, fp);
		fread(&temp.perfects, sizeof(int), 1, fp);
		fread(&temp.greats, sizeof(int), 1, fp);
		fread(&temp.goods, sizeof(int), 1, fp);
		fread(&temp.misses, sizeof(int), 1, fp);
		fread(&temp.unlockStatus, sizeof(char), 1, fp);

		int songIndex = songID_to_listID(temp.songID);
		int chartIndex = getChartIndexFromType(temp.chartID);

		if ( songIndex >= 0 && chartIndex >= 0 ) // otherwise would indicate a song which was cut
		{
			// all that changed between v1 and v2 was that I changed the full combo status to be more specific
			if ( temp.status == 4 ) // OLD STATUS_PERFECT
			{
				temp.status = STATUS_FULL_PERFECT_COMBO;
			}
			else if ( temp.status == 3 ) // OLD STATUS_FULL_COMBO
			{
				if ( temp.goods == 0 )
				{
					temp.status = STATUS_FULL_GREAT_COMBO;
				}
				else
				{
					temp.status = STATUS_FULL_GOOD_COMBO;
				}
			}

			// just to be sure
			temp.grade = temp.calculateGrade();

			p.allTime[songIndex][chartIndex] = temp;
		}
	}

	fclose(fp);
	return true;
}

void ScoreManager::savePlayersToDisk()
{
	for ( int side = 0; side < 2; side++ )
	{
		if ( player[side].isLoggedIn )
		{
			mergeCurrentScores(player[side], side);
			savePlayerToDisk(player[side]);
		}
	}
}

void ScoreManager::mergeCurrentScores(PLAYER_DATA &p, int side)
{
	UNUSED(side);
	for ( int i = 0; i < 7; i++ )
	{
		if ( p.currentSet[i].songID < 100 )
		{
			continue;
		}

		int songindex = songID_to_listID(p.currentSet[i].songID);
		int chartindex = getChartIndexFromType(p.currentSet[i].chartID);
	
		if ( songindex == -1 || chartindex == -1 )
		{
			continue; // it may be possible to play non-existant songs (somehow)
		}

		// compare status, grade, maxcombo, and score separately
		bool changedSomething = false;
		if ( p.allTime[songindex][chartindex].status < p.currentSet[i].status )
		{
			p.allTime[songindex][chartindex].status = p.currentSet[i].status;
			changedSomething = true;
		}
		if ( p.allTime[songindex][chartindex].grade < p.currentSet[i].grade )
		{
			p.allTime[songindex][chartindex].grade = p.currentSet[i].grade;
			changedSomething = true;
		}
		if ( p.allTime[songindex][chartindex].maxCombo < p.currentSet[i].maxCombo )
		{
			p.allTime[songindex][chartindex].maxCombo = p.currentSet[i].maxCombo;
			changedSomething = true;
		}
		if ( p.allTime[songindex][chartindex].points < p.currentSet[i].points || p.allTime[songindex][chartindex].maxPoints != p.currentSet[i].maxPoints )
		{
			p.allTime[songindex][chartindex].maxPoints = p.currentSet[i].maxPoints;
			p.allTime[songindex][chartindex].points = p.currentSet[i].points;
			p.allTime[songindex][chartindex].perfects = p.currentSet[i].perfects;
			p.allTime[songindex][chartindex].greats = p.currentSet[i].greats;
			p.allTime[songindex][chartindex].goods = p.currentSet[i].goods;
			p.allTime[songindex][chartindex].misses = p.currentSet[i].misses;
			changedSomething = true;
		}
		if ( changedSomething )
		{
			p.allTime[songindex][chartindex].time = time(NULL);
		}
		
		// updating the unlocks doesn't update the timestamp (though that probably happened)
		if ( p.allTime[songindex][chartindex].unlockStatus < p.currentSet[i].unlockStatus )
		{
			p.allTime[songindex][chartindex].unlockStatus = p.currentSet[i].unlockStatus;
		}
	}
}

bool ScoreManager::doesPlayerNameExist(char* name)
{
	FILE* fp = NULL;
	char filename[64] = "PLAYERS/";
	strcat_s(filename, 64, name);
	strcat_s(filename, 64, ".prefs");
	if ( fopen_s(&fp, filename, "rb") != 0 ) // only care if the player exists, not if the file is corrupt
	{
		return false;
	}
	fclose(fp);
	return true;
}

bool ScoreManager::isPinCorrect(int pin, int side)
{
	if ( player[side].pinDigits[3] == pin % 10 &&
		player[side].pinDigits[2] == (pin/10) % 10 &&
		player[side].pinDigits[1] == (pin/100) % 10 && 
		player[side].pinDigits[0] == (pin/1000) % 10 )
	{
		return true;
	}
	return false;
}

void ScoreManager::savePlayerToDisk(PLAYER_DATA &p)
{
	FILE* fp = NULL;
	char baseFilename[64] = "PLAYERS/";
	char prefsFilename[64] = "";
	char scoreFilename[64] = "";

	// calculate the filenames
	for ( int i = 0; i < 8; i++ )
	{
		baseFilename[8+i] = p.displayName[i]; //can't strcat because it isn't null terminated
	}
	strcat_s(prefsFilename, 64, baseFilename);
	strcat_s(prefsFilename, 64, ".prefs");
	strcat_s(scoreFilename, 64, baseFilename);
	strcat_s(scoreFilename, 64, ".score");

	// first wirte the name, pin, and general stats to the prefs file
	if ( fopen_s(&fp, prefsFilename, "wb") != 0 )
	{
		allegro_message("Error saving: %s", prefsFilename);
		return;
	}

	fprintf(fp, "DMXp");
	int vnum = CURRENT_PLAYER_VERSION_NUMBER;
	fwrite(&vnum, sizeof(long), 1, fp);

	fwrite(&p.displayName, sizeof(char), 8, fp);
	fwrite(&p.pinDigits, sizeof(char), 4, fp);
	fwrite(&p.numPlaysSP, sizeof(long), 1, fp);
	fwrite(&p.numPlaysDP, sizeof(long), 1, fp);

	safeCloseFile(fp, prefsFilename);

	// then write all the scores to the scores file
	if ( fopen_s(&fp, scoreFilename, "wb") != 0 )
	{
		allegro_message("Error saving: %s", scoreFilename);
		return;
	}

	fprintf(fp, "DMXs");
	vnum = CURRENT_SCORE_VERSION_NUMBER;
	fwrite(&vnum, sizeof(long), 1, fp);

	for ( int i = 0; i < NUM_SONGS; i++ ) // loop through each song
	for ( int j = 0; j < 6; j++ )         // loop through each chart
	{
		fwrite(&p.allTime[i][j].songID, sizeof(int), 1, fp);
		fwrite(&p.allTime[i][j].chartID, sizeof(int), 1, fp);
		fwrite(&p.allTime[i][j].grade, sizeof(char), 1, fp);
		fwrite(&p.allTime[i][j].status, sizeof(char), 1, fp);
		fwrite(&p.allTime[i][j].time, sizeof(long), 1, fp);
		fwrite(&p.allTime[i][j].maxCombo, sizeof(int), 1, fp);
		fwrite(&p.allTime[i][j].points, sizeof(int), 1, fp);
		fwrite(&p.allTime[i][j].maxPoints, sizeof(int), 1, fp);
		fwrite(&p.allTime[i][j].perfects, sizeof(int), 1, fp);
		fwrite(&p.allTime[i][j].greats, sizeof(int), 1, fp);
		fwrite(&p.allTime[i][j].goods, sizeof(int), 1, fp);
		fwrite(&p.allTime[i][j].misses, sizeof(int), 1, fp);
		fwrite(&p.allTime[i][j].unlockStatus, sizeof(char), 1, fp);
	}

	safeCloseFile(fp, scoreFilename);
}
