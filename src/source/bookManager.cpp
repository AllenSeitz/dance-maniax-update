// bookManager.cpp implements bookkeeping for Dance Maniax
// source file created by Allen Seitz 3/6/2012

#include <time.h>
#include <stdio.h>

#include "bookManager.h"

#define BKEEP_FILENAME "conf/bkeep.dat"

BookManager::BookManager()
{
	resetData();
	loadData();
	saveData();
	timeSinceLastSave = 0;
}

void BookManager::resetData()
{
	for ( int i = 0; i < 7; i++ )
	{
		allTime[i].reset();
		thisWeek[i].reset();
	}

	lastPowerOnMonth = 1;
	lastPowerOnDay = 1;
	lastPowerOnYear = 1970;
	currentDay = 0;
	timeSinceLastSave = 0;
}

void BookManager::loadData()
{
	FILE* fp = NULL;
	tm timePtr;

	// get the current month/day/year
	time_t t = time(NULL);
	localtime_s(&timePtr, &t); // if this returns non-zero, something is horribly wrong
	int todayMonth = timePtr.tm_mon + 1;
	int todayDay = timePtr.tm_mday;
	int todayYear = timePtr.tm_year + 1900;

	// which day of the week is today? (0-6)
	currentDay = timePtr.tm_wday;

	// does bookkeeping exist?
	if ( (fp = safeLoadFile(BKEEP_FILENAME)) == NULL )
	{
		return; // saveData() will handle this next
	}

	// read a short version number
	int vnum = checkFileVersion(fp, "DMXB");
	if ( vnum != CURRENT_VERSION_NUMBER )
	{
		TRACE("Wrong bookkeeping version number.");
		return; // later when multiple versions exist, ideas for converting them
	}

	// okay, read everything
	fread(&lastPowerOnMonth, sizeof(long), 1, fp);
	fread(&lastPowerOnDay, sizeof(long), 1, fp);
	fread(&lastPowerOnYear, sizeof(long), 1, fp);
	for ( int i = 0; i < 7; i++ )
	{
		fread(&thisWeek[i].uptime, sizeof(long), 1, fp);
		fread(&thisWeek[i].playtime, sizeof(long), 1, fp);
		fread(&thisWeek[i].coins, sizeof(long), 1, fp);
		fread(&thisWeek[i].services, sizeof(long), 1, fp);
		fread(&thisWeek[i].logins, sizeof(long), 1, fp);
		fread(&thisWeek[i].nonLogins, sizeof(long), 1, fp);
		fread(&thisWeek[i].spGames, sizeof(long), 1, fp);
		fread(&thisWeek[i].dpGames, sizeof(long), 1, fp);
		fread(&thisWeek[i].vpGames, sizeof(long), 1, fp);
		fread(&thisWeek[i].mpGames, sizeof(long), 1, fp);

		fread(&allTime[i].uptime, sizeof(long), 1, fp);
		fread(&allTime[i].playtime, sizeof(long), 1, fp);
		fread(&allTime[i].coins, sizeof(long), 1, fp);
		fread(&allTime[i].services, sizeof(long), 1, fp);
		fread(&allTime[i].logins, sizeof(long), 1, fp);
		fread(&allTime[i].nonLogins, sizeof(long), 1, fp);
		fread(&allTime[i].spGames, sizeof(long), 1, fp);
		fread(&allTime[i].dpGames, sizeof(long), 1, fp);
		fread(&allTime[i].vpGames, sizeof(long), 1, fp);
		fread(&allTime[i].mpGames, sizeof(long), 1, fp);
	}

	// now, based on the last power-on time, how much data is over a week old?
	long grandDaysLast = countDaysIn(lastPowerOnYear, lastPowerOnMonth, lastPowerOnDay);
	long grandDaysToday = countDaysIn(todayYear, todayMonth, todayDay);
	int difference = grandDaysToday - grandDaysLast;
	for ( int i = 0; i < 7; i++ )
	{
		int dayIndex = (currentDay - i + 7) % 7; // loop 0-6 "days backwards"
		if ( difference + i >= 7 )
		{
			thisWeek[dayIndex].reset();
		}
	}

	lastPowerOnMonth = todayMonth;
	lastPowerOnDay = todayDay;
	lastPowerOnYear = todayYear;

	fclose(fp);
}

void BookManager::saveData()
{
	FILE* fp = NULL;

	if ( fopen_s(&fp, BKEEP_FILENAME, "wb") != 0 )
	{
		TRACE("UNABLE TO SAVE BOOKKEEPING");
		return; // this is bad
	}

	// write the version and the last power-on time
	fprintf(fp, "DMXB");
	int vnum = CURRENT_VERSION_NUMBER;
	fwrite(&vnum, sizeof(long), 1, fp);
	fwrite(&lastPowerOnMonth, sizeof(long), 1, fp);
	fwrite(&lastPowerOnDay, sizeof(long), 1, fp);
	fwrite(&lastPowerOnYear, sizeof(long), 1, fp);

	// write all data
	for ( int i = 0; i < 7; i++ )
	{
		fwrite(&thisWeek[i].uptime, sizeof(long), 1, fp);
		fwrite(&thisWeek[i].playtime, sizeof(long), 1, fp);
		fwrite(&thisWeek[i].coins, sizeof(long), 1, fp);
		fwrite(&thisWeek[i].services, sizeof(long), 1, fp);
		fwrite(&thisWeek[i].logins, sizeof(long), 1, fp);
		fwrite(&thisWeek[i].nonLogins, sizeof(long), 1, fp);
		fwrite(&thisWeek[i].spGames, sizeof(long), 1, fp);
		fwrite(&thisWeek[i].dpGames, sizeof(long), 1, fp);
		fwrite(&thisWeek[i].vpGames, sizeof(long), 1, fp);
		fwrite(&thisWeek[i].mpGames, sizeof(long), 1, fp);

		fwrite(&allTime[i].uptime, sizeof(long), 1, fp);
		fwrite(&allTime[i].playtime, sizeof(long), 1, fp);
		fwrite(&allTime[i].coins, sizeof(long), 1, fp);
		fwrite(&allTime[i].services, sizeof(long), 1, fp);
		fwrite(&allTime[i].logins, sizeof(long), 1, fp);
		fwrite(&allTime[i].nonLogins, sizeof(long), 1, fp);
		fwrite(&allTime[i].spGames, sizeof(long), 1, fp);
		fwrite(&allTime[i].dpGames, sizeof(long), 1, fp);
		fwrite(&allTime[i].vpGames, sizeof(long), 1, fp);
		fwrite(&allTime[i].mpGames, sizeof(long), 1, fp);
	}

	//fclose(fp);
	safeCloseFile(fp, BKEEP_FILENAME);
}

void BookManager::logCoin(bool wasService)
{
	if ( wasService )
	{
		thisWeek[currentDay].services++;
		allTime[currentDay].services++;
	}
	else
	{
		thisWeek[currentDay].coins++;
		allTime[currentDay].coins++;
	}
}

void BookManager::logTime(int ms, int mode)
{
	thisWeek[currentDay].uptime += ms;
	allTime[currentDay].uptime += ms;
	timeSinceLastSave += ms;

	// is someone playing?
	if ( mode != SPLASH && mode != WARNING && mode != TESTMODE && mode != ATTRACT )
	{
		thisWeek[currentDay].playtime += ms;
		allTime[currentDay].playtime += ms;
	}
	// periodically write the bookkeeping to disk, but only if no one is playing
	else if ( timeSinceLastSave > 600000 )
	{
		saveData();
		timeSinceLastSave = 0;
	}
}

void BookManager::logLogin(bool wasAnonymous)
{
	if ( wasAnonymous )
	{
		thisWeek[currentDay].nonLogins++;
		allTime[currentDay].nonLogins++;
	}
	else
	{
		thisWeek[currentDay].logins++;
		allTime[currentDay].logins++;
	}
}

void BookManager::logMode(int mode)
{
	switch (mode)
	{
	case SINGLES_PLAY:
		thisWeek[currentDay].spGames++;
		allTime[currentDay].spGames++;
		break;
	case DOUBLES_PLAY:
		thisWeek[currentDay].dpGames++;
		allTime[currentDay].dpGames++;
		break;
	case VERSUS_PLAY:
		thisWeek[currentDay].vpGames++;
		allTime[currentDay].vpGames++;
		break;
	case MISSION_PLAY:
		thisWeek[currentDay].mpGames++;
		allTime[currentDay].mpGames++;
		break;
	}
}

// takes a specific date and returns the number of days since 1/1/1900
long BookManager::countDaysIn(int year, int month, int day)
{
	static int dpm[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	int days = ((year-1900) * 365) + ((year-1900)/4);
	for ( int m = 0; m < 12; m++ )
	{
		if ( m < month-1 )
		{
			days += dpm[m];
		}
	}
	days += day-1;

	return days;
}