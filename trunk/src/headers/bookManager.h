// bookManager.h implements bookkeeping for Dance Maniax
// source file created by Allen Seitz 3/6/2012

#ifndef _BOOKMANAGER_H_
#define _BOOKMANAGER_H_

#include "common.h"

struct DAILY_BOOKKEEP
{
	long int uptime;     // total power-on time counted no matter what (in ms)
	long int playtime;   // counted while in certain game modes (in ms)

	long int coins;      // coin switch hits which resulted in a credit
	long int services;   // service button hits which resulted in a credit

	long int logins;     // player logins that day (including first time players)
	long int nonLogins;  // games played where the player opted out of login

	long int spGames;    // singles play
	long int dpGames;    // doubles play
	long int vpGames;    // versus  play
	long int mpGames;    // mission play

	DAILY_BOOKKEEP::DAILY_BOOKKEEP()
	{
		reset();
	}

	void DAILY_BOOKKEEP::reset()
	{
		uptime = playtime = 0;
		coins = services = 0;
		logins = nonLogins = 0;
		spGames = dpGames = vpGames = mpGames = 0;
	}
};

class BookManager
{
public:
	BookManager();

	void resetData();
	// postcondition: sets all bookkeeping values to zero and all times to 1/1/70

private:
	void loadData(); // called once by the constructor and never again
	// postcondition: if bkeep.dat exists on disk, loads the previous values from it

	void saveData(); // called periodically by the logTime() function
	// postcondition: writes current values to bkeep.dat (including newly initialized values)

public:
	void logCoin(bool wasService);
	void logTime(int ms, int mode);
	void logLogin(bool wasAnonymous);
	void logMode(int mode);
	// postcondition: logs whatever, however

	void forceBookkeepingSave() { saveData(); }
	// precondition: only to be called from firstOperatorLoop(), for debugging purposes

	void nukeBookkeeping() { resetData(); saveData(); loadData(); saveData(); }
	// precondition: to be called from exactly one place in the operator menu

	DAILY_BOOKKEEP getAllTime(int day) { return allTime[day]; }
	DAILY_BOOKKEEP getThisWeek(int day) { return thisWeek[day]; }

private:
	DAILY_BOOKKEEP allTime[7];
	DAILY_BOOKKEEP thisWeek[7];

	int lastPowerOnMonth;
	int lastPowerOnDay;
	int lastPowerOnYear;
	int currentDay; // 0-6, used as an index into thisWeek[]

	int timeSinceLastSave; // incremented during logTime(), calls saveData() during next mode transition

	long countDaysIn(int year, int month, int day);
	// precondition: year is >= 1900, month and day are also legal dates
	// postcondition: returns the number of days since 1/1/1900
};

#endif