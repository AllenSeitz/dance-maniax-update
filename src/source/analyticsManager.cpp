// analyticsManager.cpp implements online usage tracking for Dance Maniax
// source file created by Allen Seitz 5/30/2016
// v1 - implements Google Analytics
// v2 - both analytics service has ve shut down in the last 10 years, so do nothing

#include "../headers/analyticsManager.h"
#include "../headers/versionManager.h"

extern VersionManager version;

#define CLIENT_ID_FILENAME "conf/client_id.bin"

#define TRACKING_ID "UA-78516566-1"
#define GOOGLE_BASE_URL "www.google-analytics.com/collect"
#define TRACKING_APP_NAME "Dance%20Maniax%20Update"

int isDebugMode = 0;

void AnalyticsManager::initialize()
{
	getOrMakeClientID();
	initialized = true;

	// calculate the version string once and use it in every event
	versionString[0] = 0;
	strcpy_s(versionString, 32, "&av=001122");
	versionString[4] = version.currentVersionYear / 10 + '0';
	versionString[5] = version.currentVersionYear % 10 + '0';
	versionString[6] = version.currentVersionMonth / 10 + '0';
	versionString[7] = version.currentVersionMonth % 10 + '0';
	versionString[8] = version.currentVersionDay / 10 + '0';
	versionString[9] = version.currentVersionDay % 10 + '0';

#ifdef DMXDEBUG
	isDebugMode = 1;
	logEvent(TRACK_CAT_DEBUG, TRACK_EV_TEST);
	//initialized = false;
#endif
	logEvent(TRACK_CAT_ACTIVITY, TRACK_EV_BOOT);
}

void AnalyticsManager::logEvent(const char* eventCategory, const char* eventType, const char* eventLabel, int eventValue)
{
	if ( !initialized )
	{
		return; // this is also how "notracking" is implemented
	}

	logEventToServer(GOOGLE_BASE_URL, eventCategory, eventType, eventLabel, eventValue);
}

void AnalyticsManager::logEventToServer(const char* serverURL, const char* eventCategory, const char* eventType, const char* eventLabel, int eventValue)
{
	// construct the postfields using much string math
	char postfields[512] = "";

	sprintf_s(postfields, 512, "v=%d&tid=%s&an=%s&cid=%s&t=%s&ec=%s&ea=%s&debug=%d", 1, TRACKING_ID, TRACKING_APP_NAME, clientID, "event", eventCategory, eventType, isDebugMode);

	// post the data using curl
}

void AnalyticsManager::getOrMakeClientID()
{
	static char validCharacters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"; // characters used in random client IDs

	FILE* fp = NULL;
	strcpy_s(clientID, 17, "UNINITIALIZED123");

	if ( !fileExists(CLIENT_ID_FILENAME) )
	{
		if ( fopen_s(&fp, CLIENT_ID_FILENAME, "wt") != 0 )
		{
			TRACE("UNABLE TO SAVE CLIENT_ID");
			return;
		}

		// generate a new, random user id
		for ( int i = 0; i < 16; i++ )
		{
			int temp = validCharacters[rand()%36];
			fputc(temp, fp);
		}
		fclose(fp);
	}

	// okay now open it and read from it
	if ( fopen_s(&fp, CLIENT_ID_FILENAME, "rt") != 0 )
	{
		return; // or not
	}

	fscanf_s(fp, "%s", clientID, 17);

	fclose(fp);
}