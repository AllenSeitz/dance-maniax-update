// version.cpp implements a function for telling the compile tile version of the program
// source file created by Allen Seitz 6/6/2016

#include "common.h"
#include "versionManager.h"

void VersionManager::initialize()
{
	versionString[0] = 0;
	strcpy_s(versionString, 128, __DATE__);

	currentVersionInSeconds = convert_DATE(__DATE__);
}

time_t VersionManager::convert_DATE(char const *time)
{ 
    char s_month[5] = "";
    int month, day, year;
    struct tm t = {0};
    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

    sscanf_s(time, "%s %d %d", s_month, 5, &day, &year);

    month = (strstr(month_names, s_month)-month_names)/3;

    currentVersionMonth = t.tm_mon = month + 1;
    currentVersionDay = t.tm_mday = day;
    currentVersionYear = t.tm_year = year - 2000;
    t.tm_isdst = -1;

    return mktime(&t);
}