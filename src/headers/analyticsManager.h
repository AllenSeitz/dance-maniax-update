// analyticsManager.h implements online usage tracking for Dance Maniax
// source file created by Allen Seitz 5/30/2016
// v1 - implements Google Analytics

#ifndef _ANALYTICSMANAGER_H_
#define _ANALYTICSMANAGER_H_

#include "../headers/common.h"

#define TRACK_CAT_DEBUG "Debug"
#define TRACK_CAT_SYSTEM "System"
#define TRACK_CAT_ERROR "Error"
#define TRACK_CAT_CONFIG "Configuration"
#define TRACK_CAT_ACTIVITY "Activity"

// debug
#define TRACK_EV_TEST "TestEvent"

// activity
#define TRACK_EV_BOOT "AppStart"
#define TRACK_EV_GAMESTART "GameStart"
#define TRACK_EV_PICKSONG "PickSong"

// error
#define TRACK_EV_ERROR "Error"

class AnalyticsManager
{
public:
	void initialize();
	// precondition: called once on program start if tracking is enabled
	// postcondition: future calls to logEvent() will make post rquests to google-analytics.com

	void logEvent(const char* eventCategory, const char* eventType, const char* eventLabel = "", int eventValue = -1);
	// precondition: none, although this function intentionally does nothing if initialize() is not called first
	// postcondition: makes a post request to google-analytics.com and does not check the result

private:
	void getOrMakeClientID();
	// precondition: loads client_id.bin if it exists, or creates it if it does not
	// postcondition: clientID is initialized

	void logEventToServer(const char* serverURL, const char* eventCategory, const char* eventType, const char* eventLabel = "", int eventValue = -1);
	//
	//

	char versionString[32]; // set to the date the program was compiled "YYMMDD"
	bool initialized;
	char clientID[17]; // this install's random 16 byte client id, created at rnadom if it does not exist
};

#endif