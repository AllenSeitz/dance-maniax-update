// analyticsManager.cpp implements online usage tracking for Dance Maniax
// source file created by Allen Seitz 5/30/2016
// v1 - implements Google Analytics

#include "analyticsManager.h"
#include "versionManager.h"

#include "curl.h"

extern VersionManager version;

#define CLIENT_ID_FILENAME "conf/client_id.bin"

#define TRACKING_ID "UA-78516566-1"
#define GOOGLE_BASE_URL "www.google-analytics.com/collect"
#define KATZE_BASE_URL "saveasdotdmx.katzepower.com/ghettoanalytics.php"
#define TRACKING_APP_NAME "Dance%20Maniax%20Update"

int isDebugMode = 0;

void AnalyticsManager::initialize()
{
	curl_global_init(CURL_GLOBAL_ALL);
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
	logEventToServer(KATZE_BASE_URL, eventCategory, eventType, eventLabel, eventValue);
}

void AnalyticsManager::logEventToServer(const char* serverURL, const char* eventCategory, const char* eventType, const char* eventLabel, int eventValue)
{
	CURL* curl = curl_easy_init();
	CURLcode res;

	// construct the postfields using much string math
	char postfields[512] = "";
	sprintf_s(postfields, 512, "v=%d&tid=%s&an=%s&cid=%s&t=%s&ec=%s&ea=%s&debug=%d", 1, TRACKING_ID, TRACKING_APP_NAME, clientID, "event", eventCategory, eventType, isDebugMode);
	if ( eventLabel[0] != 0 )
	{
		char* tempEncode = curl_easy_escape(curl, eventLabel, strlen(eventLabel));
		strcat_s(postfields, 512, "&el=");
		strcat_s(postfields, 512, tempEncode);
		curl_free(tempEncode);
	}
	if ( eventValue != -1 )
	{
		char tempEncode[128] = "";
		_itoa_s(eventValue, tempEncode, 64, 10);
		strcat_s(postfields, 512, "&ev=");
		strcat_s(postfields, 512, tempEncode);
	}

	// post the data using curl
	if ( curl )
	{
		// post to Google Analytics first
	    curl_easy_setopt(curl, CURLOPT_URL, serverURL);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields); 
		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
		{
			al_trace("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
		
	    curl_easy_cleanup(curl);
	}
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