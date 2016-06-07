// version.h implements functions for telling the compile tile version of the program
// source file created by Allen Seitz 6/6/2016

#ifndef _DMXVERSION_H_
#define _DMXVERSION_H_

class VersionManager
{
public:
	void initialize();
	// precondition: call once at the start of the program
	// postcondition: the public data members are now set correctly

private:
	time_t convert_DATE(char const *time);
	// precondition: called from initializeVersionString()
	// postcondition: returns the __DATE__ in seconds since the epoch

public:
	char versionString[128];
	time_t currentVersionInSeconds;
	long currentVersionYear;
	long currentVersionMonth;
	long currentVersionDay;
};

#endif