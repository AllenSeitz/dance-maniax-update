// main.cpp defines the main entry point for DMX
// file created by Allen Seitz on 7/18/2009
// April 26, 2013: cleaned up many source files and moved into an svn repo
// November 29, 2015: cleaned up a few source files and moved into a git repo

#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <string>

#include "hidapi.h"

#include "common.h"
#include "analyticsManager.h"
#include "bookManager.h"
#include "downloadManager.h"
#include "extioManager.h"
#include "inputManager.h"
#include "gameStateManager.h"
#include "lightsManager.h"
#include "pacdriveManager.h"
#include "minimaidManager.h"
#include "scoreManager.h"
#include "versionManager.h"
#include "videoManager.h"


//////////////////////////////////////////////////////////////////////////////
// Song List
//////////////////////////////////////////////////////////////////////////////
SongEntry* songs; // used globally
int NUM_SONGS = 0;
int NUM_COURSES = 0;
int NUM_PLAYABLE_SONGS = 0;
int* songIDs;
std::string* songTitles;
std::string* songArtists;
std::string* movieScripts;

#define HITLIST_FILENAME "conf/hitlist.dat"
#define SONGDB_FILENAME "DATA/song_db.csv"
#define UPSCRIPT_FILENAME "update.bat"
#define UNZIP_FILENAME "unzip.exe"

int initializeSonglist();
int loadSongDB();

void savePlayersHitList();
void loadPlayersHitList();


//////////////////////////////////////////////////////////////////////////////
// Game Logic
//////////////////////////////////////////////////////////////////////////////
volatile UTIME last_utime = 0;	// the last time a frame ran
unsigned long int frameCounter = 0;
unsigned long int totalGameTime = 0;
int fpsLastFrame = 0;
int fpsHappyThreshold = 50;
int testMenuMainIndex = 0;
int testMenuSubIndex = 0;
bool allsongsDebug = false;
bool isTestingChart = false;
int testChartSongID = 101;
int testChartLevel = SINGLE_MILD;
bool SHOW_LAMPS = true;
bool forceMinimaid = false;
bool minimaidLightsOnly = false;
bool held_printscreen = false;
bool beginInitialInstall = false;
bool redownloadManifest = true;

BITMAP** m_banners; // used globally
BITMAP* m_caution;
BITMAP* m_failure;
UTIME cautionTimer = 0;
UTIME failureTimer = 0;
UTIME hideCreditsTimer = 0;
UTIME totalBootTime = 0;
UTIME bootStepTime = 0;
int currentBootStep = 0;
bool updateInProgress = false;
UTIME updateCloseTimer = 0;

BookManager bm;
RenderingManager rm;
InputManager im;
GameStateManager gs;
ScoreManager sm;
VideoManager vm;
EffectsManager em;
LightsManager lm;
extioManager extio;
pacdriveManager pacio;
minimaidManager minimaidio;
DownloadManager dm;
AnalyticsManager am;
VersionManager version;

void firstSplashLoop();
void mainSplashLoop(UTIME dt);

void firstWarningLoop();
void mainWarningLoop(UTIME dt);

void firstFailureLoop();
void mainFailureLoop(UTIME dt);

extern void firstAttractLoop();
extern void mainAttractLoop(UTIME dt);

void firstErrorLoop();
void mainErrorLoop(UTIME dt);

void firstOperatorLoop();
void mainOperatorLoop(UTIME dt);

void firstBootLoop();
void mainBootLoop(UTIME dt);

extern void firstSongwheelLoop();
extern void mainSongwheelLoop(UTIME dt);
extern void killPreviewClip();

extern void firstGameplayLoop();
extern void mainGameplayLoop(UTIME dt);
extern void loadGameplayGraphics();

extern void firstMenuLoop();
extern void mainMenuLoop(UTIME dt);

extern void firstLoginLoop();
extern void mainLoginLoop(UTIME dt);
extern void resetPrevNames();

extern void firstNonstopLoop();
extern void mainNonstopLoop(UTIME dt);

extern void firstResultsLoop();
extern void mainResultsLoop(UTIME dt);

extern void firstGameoverLoop();
extern void mainGameoverLoop(UTIME dt);

extern void firstVoteLoop();
extern void mainVoteLoop(UTIME dt);

void firstVideoTestLoop();
void mainVideoTestLoop(UTIME dt);

void firstCautionLoop();
void mainCautionLoop(UTIME dt);

void firstUpdateLoop();
void mainUpdateLoop(UTIME dt);
void alternateMainUpdateLoop();

void renderCreditsDisplay();
bool checkForUpdates();
void giveUpAndRestart();
void renderDebugOverlay();

// operator menus
void renderInputTest();
void renderLampTest();
void renderScreenTest();
void renderColorTest();
void renderCoinOptions();
void renderGameOptions();
void renderBookkeeping(int temp);
void renderSoundOptions();
void renderDataOptions();


//////////////////////////////////////////////////////////////////////////////
// Main Program
//////////////////////////////////////////////////////////////////////////////
int main()
{
	// setup the Allegro library
	allegro_init();
	srand(time(0));

	// keyboard initialization
	if ( install_keyboard() != 0 )
	{
		allegro_message("Keyboard Initialization Failed!");
		return EXIT_FAILURE;
	}
	key_led_flag = 0;

	// set a refresh rate BEFORE the display is created
	if ( fileExists("refreshrate") )
	{
		FILE* rfile = NULL;
		fopen_s(&rfile, "refreshrate", "rt");
		if ( rfile )
		{
			int rate = 0;
			fscanf_s(rfile, "%d", &rate);
			fclose(rfile);
			request_refresh_rate(rate);
		}
	}

	// timer initalization
	if ( timeBeginPeriod(1) != TIMERR_NOERROR )
	{
		allegro_message("Unable to initialize timer.");
		return EXIT_FAILURE;
	}
	last_utime = timeGetTime();

	//* initialize the sound playback
	int digienabled = detect_digi_driver(DIGI_AUTODETECT);		// check sample software
	int midienabled = detect_midi_driver(MIDI_NONE);			// not needed
	digienabled = digienabled > 8 ? 8 : digienabled;			// using more voices lowers quality
	midienabled = midienabled > 8 ? 8 : midienabled;			// using more voices lowers quality
	if ( digienabled == 0 )
	{
		allegro_message("Digital Initalization Failed!");
		return EXIT_FAILURE;
	}
	reserve_voices(digienabled, midienabled);
	if ( install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL) == -1 )
	{
		allegro_message("Audio Initalization Failed!");
		return EXIT_FAILURE;
	}
	//*/

	//* initialize... yet more sound playback!
    if (!FSOUND_Init(44100, 64, 0))
    {
		allegro_message("FMOD failed to initialize: %d", FSOUND_GetError());
		return EXIT_FAILURE;
    }
	//*/

	// initialize graphics resources
	set_color_depth(32);
	int windowOrFullscreen = GFX_AUTODETECT_WINDOWED;
	if ( fileExists("fullscreen") )
	{
		windowOrFullscreen = GFX_AUTODETECT_FULLSCREEN;
	}
	if ( set_gfx_mode(windowOrFullscreen, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0) != 0 )
	{
		allegro_message("Video Initalization Failed!");
		return EXIT_FAILURE;
	}

	set_color_conversion(COLORCONV_MOST|COLORCONV_KEEP_TRANS);
	set_display_switch_mode(SWITCH_BACKGROUND); //SWITCH_PAUSE
	text_mode(-1);
	set_alpha_blender(); // the game assumes the graphics are left in this mode
	register_bitmap_file_type("png", load_png, NULL);
	version.initialize();

	// initialize the HID (Human Interface Device) library, used by the minimaid IO board
	if ( hid_init() != 0 )
	{
		printf("Unable to initialize the HID library.\n");
	}

	// hax because sometimes players don't want to login and unlock the extra stages
	if ( fileExists("allsongs") )
	{
		allsongsDebug = true;
	}

	// give players an option to disable tracking
	if ( !fileExists("notracking") )
	{
		am.initialize();
	}

	if ( !fileExists("initial_install_complete.txt") )
	{
		beginInitialInstall = true; // a special flag that prevents gameplay and just downloads files
		NUM_SONGS = 0;

		// do this once at the very start
		if ( !fileExists("DATA/etc/white_font.bmp") )
		{
			system("mkdir backup");
			system("mkdir conf");
			system("mkdir update");
			system("mkdir PLAYERS");
			system("mkdir DATA");
		}
		rm.Initialize(true); // flag to skip loading fonts while the initial install is happening
		checkForUpdates();
	}
	else
	{
		// normal initialization happens when the game is properly installed
		rm.Initialize(false);
		em.initialize();
		lm.initialize();
		vm.initialize();

		loadGameplayGraphics();	// these stay loaded the ENTIRE program

		if ( initializeSonglist() == -1 )
		{
			allegro_message("Error loading song database.");
			return 0;
		}
		sm.resetData();
	}

	im.updateKeyStates(1);
	extio.initialize();
	minimaidio.initialize();
	pacio.initialize();

	// check options
	if ( fileExists("revpolarityred") )
	{
		im.reverseRedSensorPolarity = true; // temporary for testing
	}
	if ( fileExists("revpolarityblue") )
	{
		im.reverseBlueSensorPolarity = true; // temporary for testing
	}
	if ( fileExists("testchart.txt") )
	{
		isTestingChart = true;
		char tempch[3] = "sm";

		FILE* tfp = NULL;
		fopen_s(&tfp, "testchart.txt", "rt");
		fscanf_s(tfp, "%d", &testChartSongID);
		fscanf_s(tfp, " %c", &tempch[0]);
		fscanf_s(tfp, "%c", &tempch[1]);
		fclose(tfp);

		if ( tempch[1] == 'm' )
		{
			testChartLevel = SINGLE_MILD;
		}
		if ( tempch[1] == 'w' )
		{
			testChartLevel = SINGLE_WILD;
		}
		if ( tempch[1] == 'x' )
		{
			testChartLevel = SINGLE_ANOTHER;
		}
		if ( tempch[0] == 'd' )
		{
			testChartLevel += 10; // doubles
		}

		gs.g_currentGameMode = GAMEPLAY; // launch directly into the chart test mode
	}
	if ( fileExists("nomenutimer") )
	{
		gs.isEventMode = true;	// override this (intentionally unsaved) setting after the normal settings are loaded
	}
	if ( fileExists("forceminimaid") )
	{
		forceMinimaid = true; // reboot until we find one
	}
	if ( fileExists("minimaidLightsOnly") )
	{
		minimaidLightsOnly = true; // do not use the minimaid for input
	}

	// booting while holding service down? delete (overwrite) machine settings
	if ( !beginInitialInstall )
	{
		if ( im.isKeyDown(MENU_SERVICE) ) // doesn't seem to work in visual studio
		{
			globalError(CLEARED_MACHINE_SETTINGS, "");
		}
		else
		{
			gs.loadOperatorSettings();
			if ( !gs.isInitialized )
			{
				globalError(UNSET_MACHINE_SETTINGS, "PLEASE REBOOT WHILE HOLDING SERVICE");
			}
		}
	}

	// main game loop
	while ( !key[KEY_ESC] )
	{
		UTIME time = timeGetTime();

		if ( time > last_utime )
		{
			UTIME dt = time - last_utime;
			//dt *= 1000; // FOR SOAK TESTING, CAUSES HILLARIOUS THINGS TO HAPPEN
			last_utime = time;
			totalGameTime += dt;

			// calculate the framerate
			if ( totalGameTime >= 1000 )
			{
				fpsLastFrame = frameCounter / (totalGameTime/1000);
				//al_trace("FPS: %d\r\n", fpsLastFrame);
			}

			// update the core classes and check for lag
			im.updateKeyStates(dt);
			bm.logTime(dt, gs.g_currentGameMode);
			vm.update(dt);
			lm.update(dt);
			extio.update(dt);
			minimaidio.update(dt);
			pacio.update(dt);

			if ( gs.g_gameModeTransition == 2 )
			{
				gs.g_gameModeTransition = 0;
				al_trace("%ld msec were eaten by load times.\r\n", dt);
				continue;
			}

			// check for the three menu buttons which work in all modes
			if ( im.getKeyState(MENU_TEST) == JUST_DOWN && gs.g_currentGameMode != TESTMODE )
			{
				gs.g_gameModeTransition = 1;
				gs.g_currentGameMode = TESTMODE;
				gs.killSong();
			}
			if ( im.getKeyState(MENU_SERVICE) == JUST_DOWN || im.getKeyState(MENU_COIN) == JUST_DOWN )
			{
				if ( gs.g_currentGameMode != TESTMODE )
				{
					gs.numCoins++;
					bm.logCoin(im.getKeyState(MENU_SERVICE) == JUST_DOWN);
				}
				em.playSample(SFX_CREDIT);
			}

			// switch between modes
			if ( gs.g_gameModeTransition == 1 )
			{
				gs.g_gameModeTransition = 2;
				frameCounter = 0;
				totalGameTime = 0;
				killPreviewClip(); // mainly for pressing TEST during the songwheel
				hideCreditsTimer = 2000;
				lm.stopLampProgram();
				lm.setAll(0);

				// do the first frame different from the rest
				switch (gs.g_currentGameMode)
				{
				case SPLASH:
					firstSplashLoop();
					break;
				case WARNING:
					firstWarningLoop();
					break;
				case SONGWHEEL:
					firstSongwheelLoop();
					break;
				case GAMEPLAY:
					firstGameplayLoop();
					break;
				case RECORDING:
					globalError(0, "Invalid game mode?");
					//firstRecordingLoop();
					break;
				case PLAYERSELECT:
				case MAINMENU:
					firstMenuLoop();
					break;
				case RESULTS:
					firstResultsLoop();
					break;
				case TESTMODE:
					firstOperatorLoop();
					break;
				case LOGIN:
					firstLoginLoop();
					break;
				case GAMEOVER:
					firstGameoverLoop();
					break;
				case ATTRACT:
					savePlayersHitList();
					firstAttractLoop();
					break;
				case ERRORMODE:
					firstErrorLoop();
					break;
				case VIDEOTEST:
					firstVideoTestLoop();
					break;
				case CAUTIONMODE:
					firstCautionLoop();
					break;
				case NONSTOP:
					firstNonstopLoop();
					break;
				case FAILURE:
					firstFailureLoop();
					break;
				case VOTEMODE:
					firstVoteLoop();
					break;
				case BOOTMODE:
					firstBootLoop();
					break;
				case UPDATEMODE:
					firstUpdateLoop();
					break;
				default:
					break;
				}
			}
			else
			{
				// process one frame
				switch (gs.g_currentGameMode)
				{
				case SPLASH:
					mainSplashLoop(dt);
					break;
				case WARNING:
					mainWarningLoop(dt);
					break;
				case SONGWHEEL:
					mainSongwheelLoop(dt);
					break;
				case GAMEPLAY:
					mainGameplayLoop(dt);
					break;
				case RECORDING:
					//mainRecordingLoop(dt);
					break;
				case PLAYERSELECT:
				case MAINMENU:
					mainMenuLoop(dt);
					break;
				case RESULTS:
					mainResultsLoop(dt);
					break;
				case TESTMODE:
					mainOperatorLoop(dt);
					break;				
				case LOGIN:
					mainLoginLoop(dt);
					break;				
				case GAMEOVER:
					mainGameoverLoop(dt);
					break;				
				case ATTRACT:
					mainAttractLoop(dt);
					break;				
				case ERRORMODE:
					mainErrorLoop(dt);
					break;
				case VIDEOTEST:
					mainVideoTestLoop(dt);
					break;
				case CAUTIONMODE:
					mainCautionLoop(dt);
					break;
				case NONSTOP:
					mainNonstopLoop(dt);
					break;
				case FAILURE:
					mainFailureLoop(dt);
					break;
				case VOTEMODE:
					mainVoteLoop(dt);
					break;
				case BOOTMODE:
					mainBootLoop(dt);
					break;
				case UPDATEMODE:
					mainUpdateLoop(dt);
					break;
 				default:
					break;
				}
				++frameCounter;

				// render the credits display
				if ( gs.g_currentGameMode != TESTMODE && hideCreditsTimer == 0 )
				{
					renderCreditsDisplay();
				}
				else if ( gs.g_currentGameMode != TESTMODE )
				{
					if ( gs.g_currentGameMode != GAMEPLAY )
					{
						rectfill(rm.m_backbuf, 0, 460, 640, 480, 0); // burn-in protection
					}
					blit(rm.m_backbuf, screen, 0, 460, 0, 460, 640, 20);
				}
				SUBTRACT_TO_ZERO(hideCreditsTimer, dt);

#ifdef DMXDEBUG
				if ( gs.g_currentGameMode != TESTMODE || key[KEY_F8] )
				{
					renderDebugOverlay();
				}
				if ( key[KEY_8] )
				{
					SHOW_LAMPS = !SHOW_LAMPS;
				}
#endif
				rm.flip();

				// allow screenshots anywhere, even in release mode
				if ( key[KEY_PRTSCR] && !held_printscreen )
				{
					held_printscreen = true;
					rm.screenshot();
				}
				else
				{
					held_printscreen = false;
				}
			}
		}

		// share the processor
		//rest(0);
	}

	// clean up
	gs.killSong();
	hid_exit();

	return EXIT_SUCCESS;
}
END_OF_MAIN();	

void renderDebugOverlay()
{
	if ( beginInitialInstall )
	{
		return;
	}

	//* FPS display
	if ( totalGameTime > 1000 )
	{
		//al_trace("FPS = %d (%d %ld)\n", (int)(frameCounter/((float)timeElapsed/1000.0f)), frameCounter, timeElapsed);
		renderWhiteNumber(frameCounter / (totalGameTime/1000), 0, 0);
		renderWhiteString("DEBUG MODE", 0, 12);
	}
	//*/

	if ( SHOW_LAMPS )
	{
		lm.renderDebugOutput(rm.m_backbuf);
	}
}

void firstSplashLoop()
{
}

void mainSplashLoop(UTIME dt)
{
	UNUSED(dt);
}

void firstWarningLoop()
{
}

void mainWarningLoop(UTIME dt)
{
	UNUSED(dt);
}

void firstErrorLoop()
{
}

void mainErrorLoop(UTIME dt)
{
	static UTIME accumulate = 0;
	accumulate += dt;
	
	int redTimer = accumulate % 2000;
	int bordercolor = makecol(getValueFromRange(111,222, redTimer*100/2000), 0, 0);

	solid_mode();
	rectfill(rm.m_backbuf, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	for ( int i = 0; i < 10; i++ )
	{
		rect(rm.m_backbuf, 20+i, 20+i, SCREEN_WIDTH-20-i, SCREEN_HEIGHT-20-i, bordercolor);
	}
	set_alpha_blender(); // the game assumes that the graphics are left in this mode

	textprintf_centre(rm.m_backbuf, font, 320, 70, makecol(255,255,255), "ERROR %d", gs.g_errorCode);
	textprintf_centre(rm.m_backbuf, font, 320, 120, makecol(255,255,255), gs.getErrorString());
	textprintf_centre(rm.m_backbuf, font, 320, 170, makecol(255,255,255), gs.g_errorInfo);
	
	textprintf_centre(rm.m_backbuf, font, 320, 300, makecol(255,255,255), "PLEASE PRESS TEST BUTTON");
	textprintf_centre(rm.m_backbuf, font, 320, 410, makecol(255,255,255), "time = %d", accumulate/1000);

	// nevermind? no one is coming to the rescue? just reboot after 10 minutes
	if ( accumulate/1000 > 600 ) 
	{
		giveUpAndRestart();
		accumulate = 600000 - 10000;
	}
}

void firstFailureLoop()
{
	failureTimer = 0;
	if ( m_failure == NULL )
	{
		m_failure = loadImage("DATA/attract/failure.png");
	}
}

void mainFailureLoop(UTIME dt)
{
	// play this sound effect once right as the timer reaches this threshold
	if ( failureTimer < SCREEN_HEIGHT && failureTimer+dt >= SCREEN_HEIGHT )
	{
		em.playSample(SFX_FAILURE_ANIMATION);
	}

	failureTimer += dt;

	// implement the black "wipe down"
	if ( failureTimer < SCREEN_HEIGHT ) // typically 480? and 480ms?
	{
		rectfill(rm.m_backbuf, 0, 0, SCREEN_WIDTH, failureTimer, 0);
	}
	else
	{
		clear_to_color(rm.m_backbuf, 0);
	}

	// render the digital "FAILED_"
	int frame = 7;
	if ( failureTimer < 1000+SCREEN_HEIGHT )
	{
		frame = MIN(7, getValueFromRange(0, 7, (failureTimer-SCREEN_HEIGHT)*100/(1000-SCREEN_HEIGHT)));
	}
	if ( failureTimer > SCREEN_HEIGHT )
	{
		blit(m_failure, rm.m_backbuf, 0, frame*30, (SCREEN_WIDTH-148)/2, (SCREEN_HEIGHT-30)/2, 176, 30);
	}

	// done forever?
	if ( failureTimer > 4000 )
	{
		gs.g_currentGameMode = RESULTS;
		gs.g_gameModeTransition = 1;
	}
}

// this function ultimately renders the bottom twenty pixels
void renderCreditsDisplay()
{
	if ( beginInitialInstall )
	{
		return;
	}

	if ( gs.g_currentGameMode != GAMEPLAY )
	{
		rectfill(rm.m_backbuf, 0, 460, 640, 480, makecol(0,0,0));
	}

	// for burn-in protection, move this around (nevermind, done another way)
	int x = 375;
	int y = 464; // default position

	if ( gs.isFreeplay )
	{
		renderWhiteString(" FREE PLAY ", x-110, y);
	}
	else
	{
		renderWhiteString("CREDIT(S): ", x-110, y);

		int numGames = gs.numCoins / gs.numCoinsPerCredit;
		int numFraction = gs.numCoins % gs.numCoinsPerCredit;
		renderWhiteNumber(numGames, x, y);
		x = x + 24 + (numGames >= 10 ? 12 : 0);
		if ( numFraction > 0 )
		{
			renderWhiteNumber(numFraction, x, y);
			x = x + 12 + (numFraction >= 10 ? 12 : 0);
			renderWhiteString("/", x, y);
			renderWhiteNumber(gs.numCoinsPerCredit, x+12, y);
		}
	}
}

// update.bat is exepected to relaunch this program. The point is that this program may be modified.
bool checkForUpdates()
{
	bool startedDownload = true;
	if ( !fileExists(MANIFEST_FILENAME) || redownloadManifest )
	{
		dm.downloadFile(MANIFEST_FILENAME, MANIFEST_FILENAME);
	}
	else if ( !fileExists(UPSCRIPT_FILENAME) )
	{
		dm.downloadFile(UPSCRIPT_FILENAME, UPSCRIPT_FILENAME);
	}
	else if ( !fileExists(UNZIP_FILENAME) )
	{
		dm.downloadFile(UNZIP_FILENAME, UNZIP_FILENAME);
	}
	else if ( dm.doWeNeedAnyUpdates().length() != 0 )
	{
		dm.downloadFile(dm.doWeNeedAnyUpdates().c_str(), dm.doWeNeedAnyUpdates().c_str());
	}
	else
	{
		startedDownload = false;
	}

	// always enter the update mode, either to show a progress bar or to say "up to date"
	gs.g_currentGameMode = UPDATEMODE;
	gs.g_gameModeTransition = 1;
	updateInProgress = startedDownload;

	return startedDownload;
}

// This is used in the event of a fatal error.
void giveUpAndRestart()
{
	if ( _execl("dmx.exe", "dmx.exe", NULL) == -1 )
	{
		globalError(REBOOT_FAILED, "please reboot the machine");
		em.playSample(SFX_LOUD_BELL);
	}
}

void firstOperatorLoop()
{
	bm.forceBookkeepingSave();
	testMenuMainIndex = 0;
	testMenuSubIndex = -1;
	gs.killSong();
	sm.player[0].resetData();
	sm.player[1].resetData();
	sm.player[0].isLoggedIn = false;
	sm.player[1].isLoggedIn = false;
	gs.numCoins = 0; // might as well, just in case this needs to be done
	im.setCooldownTime(0);
}

#define NUM_OP_MENU_CHOICES 11

void mainOperatorLoop(UTIME dt)
{
	static int tempSelection = 0;
	UNUSED(dt);
	clear_to_color(rm.m_backbuf, 0);

	if ( testMenuSubIndex == -1 ) // in the main menu
	{
		textprintf_centre(rm.m_backbuf, font, 320, 50, WHITE, "TEST MENU");
		textprintf(rm.m_backbuf, font, 50, 100, testMenuMainIndex == 0 ? RED : WHITE, "INPUT  TEST");
		textprintf(rm.m_backbuf, font, 50, 120, testMenuMainIndex == 1 ? RED : WHITE, "LAMP   TEST");
		textprintf(rm.m_backbuf, font, 50, 140, testMenuMainIndex == 2 ? RED : WHITE, "SCREEN TEST");
		textprintf(rm.m_backbuf, font, 50, 160, testMenuMainIndex == 3 ? RED : WHITE, "COLOR  TEST");
		textprintf(rm.m_backbuf, font, 50, 180, testMenuMainIndex == 4 ? RED : WHITE, "COIN   OPTIONS");
		textprintf(rm.m_backbuf, font, 50, 200, testMenuMainIndex == 5 ? RED : WHITE, "GAME   OPTIONS");
		textprintf(rm.m_backbuf, font, 50, 220, testMenuMainIndex == 6 ? RED : WHITE, "SOUND  OPTIONS");
		textprintf(rm.m_backbuf, font, 50, 240, testMenuMainIndex == 7 ? RED : WHITE, "BOOKKEEPING");
		textprintf(rm.m_backbuf, font, 50, 260, testMenuMainIndex == 8 ? RED : WHITE, "DATA OPTIONS");
		textprintf(rm.m_backbuf, font, 50, 300, testMenuMainIndex == 9 ? RED : WHITE, "ALL FACTORY DEFAULTS");
		textprintf(rm.m_backbuf, font, 50, 320, testMenuMainIndex == 10 ? RED : WHITE, "GAME MODE");

		lm.setAll(0);

		if ( im.getKeyState(MENU_START_1P) == JUST_DOWN || im.getKeyState(MENU_TEST) == JUST_DOWN )
		{
			testMenuSubIndex = 0; // enter that submenu
			tempSelection = 0;
		}
		if ( im.getKeyState(MENU_RIGHT_1P) == JUST_DOWN || im.getKeyState(MENU_SERVICE) == JUST_DOWN )
		{
			testMenuMainIndex = (testMenuMainIndex + 1) % NUM_OP_MENU_CHOICES;
		}
		if ( im.getKeyState(MENU_LEFT_1P) == JUST_DOWN )
		{
			testMenuMainIndex = (testMenuMainIndex - 1 + NUM_OP_MENU_CHOICES) % NUM_OP_MENU_CHOICES;
		}
	}
	else
	{
		switch (testMenuMainIndex)
		{
		case 0: // input test
			renderInputTest();
			if ( im.isKeyDown(MENU_START_1P) && im.isKeyDown(MENU_START_2P) )
			{
				testMenuMainIndex = 0;
				testMenuSubIndex = -1;
			}
			break;
		case 1: // lamp test
			renderLampTest();
			if ( im.getKeyState(MENU_START_1P) == JUST_DOWN )
			{
				testMenuMainIndex = 0;
				testMenuSubIndex = -1;
			}
			if ( im.getKeyState(MENU_RIGHT_1P) == JUST_DOWN || im.getKeyState(MENU_SERVICE) == JUST_DOWN )
			{
				testMenuSubIndex = (testMenuSubIndex + 1) % 46;
			}
			if ( im.getKeyState(MENU_LEFT_1P) == JUST_DOWN )
			{
				testMenuSubIndex = (testMenuSubIndex - 1 + 46) % 46;
			}

			// turn lamps on or off
			if ( testMenuSubIndex == 0 && im.isKeyDown(MENU_START_2P) )
			{
				lm.setAll(500);
			}
			else
			{
				for ( int i = 0; i < 12; i++ )
				{
					if ( (testMenuSubIndex >= 1 && testMenuSubIndex <= 36) && (testMenuSubIndex-1)/3 == i )
					{
						lm.setGroupColor(i, (testMenuSubIndex-1)%3, -1);
						if ( i == 9 )
						{
							al_trace("setting group 9 to color=%d\n", (testMenuSubIndex-1)%3);
						}
					}
					else
					{
						lm.setGroupColor(i, 2, 0);
					}
				}
				lm.setLamp(lampLeft, testMenuSubIndex == 37 ? -1 : 0);
				lm.setLamp(lampStart, testMenuSubIndex == 38 ? -1 : 0);
				lm.setLamp(lampRight, testMenuSubIndex == 39 ? -1 : 0);
				lm.setLamp(spotlightA, testMenuSubIndex == 40 ? -1 : 0);
				lm.setLamp(spotlightB, testMenuSubIndex == 41 ? -1 : 0);
				lm.setLamp(spotlightC, testMenuSubIndex == 42 ? -1 : 0);
				lm.setLamp((int)lampLeft + 1, testMenuSubIndex == 43 ? -1 : 0);
				lm.setLamp((int)lampStart + 1, testMenuSubIndex == 44 ? -1 : 0);
				lm.setLamp((int)lampRight + 1, testMenuSubIndex == 45 ? -1 : 0);
			}

			break;
		case 2: // screen test
			renderScreenTest();
			if ( im.getKeyState(MENU_START_1P) == JUST_DOWN )
			{
				testMenuMainIndex = 0;
				testMenuSubIndex = -1;
			}
			break;
		case 3: // color test
			renderColorTest();
			if ( im.getKeyState(MENU_START_1P) == JUST_DOWN )
			{
				testMenuMainIndex = 0;
				testMenuSubIndex = -1;
			}
			break;
		case 4: // coin options
			renderCoinOptions();
			if ( im.getKeyState(MENU_START_1P) == JUST_DOWN )
			{
				if ( testMenuSubIndex == 4 )
				{
					gs.isFreeplay = false;
					gs.numCoinsPerCredit = DEFAULT_COINS_PER_CREDIT;
					gs.isVersusPremium = false;
					gs.isDoublePremium = false;
				}
				if ( testMenuSubIndex == 5 )
				{
					testMenuMainIndex = 0;
					testMenuSubIndex = -1;
				}
			}
			if ( im.getKeyState(MENU_RIGHT_1P) == JUST_DOWN || im.getKeyState(MENU_SERVICE) == JUST_DOWN )
			{
				testMenuSubIndex = (testMenuSubIndex + 1) % 6;
			}
			if ( im.getKeyState(MENU_LEFT_1P) == JUST_DOWN )
			{
				testMenuSubIndex = (testMenuSubIndex - 1 + 6) % 6;
			}
			if ( im.getKeyState(MENU_RIGHT_2P) == JUST_DOWN || im.getKeyState(MENU_LEFT_2P) == JUST_DOWN )
			{
				switch (testMenuSubIndex)
				{
				case 0:
					gs.isFreeplay = !gs.isFreeplay;
					break;
				case 1:
					if ( im.getKeyState(MENU_LEFT_2P) == JUST_DOWN && gs.numCoinsPerCredit > 1 )
					{
						gs.numCoinsPerCredit -= 1;
					}
					if ( im.getKeyState(MENU_RIGHT_2P) == JUST_DOWN && gs.numCoinsPerCredit < 16 )
					{
						gs.numCoinsPerCredit += 1;
					}
					break;
				case 2:
					gs.isVersusPremium = !gs.isVersusPremium;
					break;
				case 3:
					gs.isDoublePremium = !gs.isDoublePremium;
					break;
				}
			}
			break;
		case 5: // game options
			renderGameOptions();
			if ( im.getKeyState(MENU_START_1P) == JUST_DOWN )
			{
				if ( testMenuSubIndex == 3 )
				{
					gs.numSongsPerSet = DEFAULT_SONGS_PER_SET;
				}
				if ( testMenuSubIndex == 4 )
				{
					testMenuMainIndex = 0;
					testMenuSubIndex = -1;
				}
			}
			if ( im.getKeyState(MENU_RIGHT_1P) == JUST_DOWN || im.getKeyState(MENU_SERVICE) == JUST_DOWN )
			{
				testMenuSubIndex = (testMenuSubIndex + 1) % 5;
			}
			if ( im.getKeyState(MENU_LEFT_1P) == JUST_DOWN )
			{
				testMenuSubIndex = (testMenuSubIndex - 1 + 5) % 5;
			}
			if ( im.getKeyState(MENU_RIGHT_2P) == JUST_DOWN || im.getKeyState(MENU_LEFT_2P) == JUST_DOWN )
			{
				switch (testMenuSubIndex)
				{
				case 0:
					if ( im.getKeyState(MENU_LEFT_2P) == JUST_DOWN && gs.numSongsPerSet > 2 )
					{
						gs.numSongsPerSet -= 1;
					}
					if ( im.getKeyState(MENU_RIGHT_2P) == JUST_DOWN && gs.numSongsPerSet < 5 )
					{
						gs.numSongsPerSet += 1;
					}
					break;
				case 1:
					gs.isEventMode = !gs.isEventMode;
					break;
				case 2:
					gs.isFreestyleMode = !gs.isFreestyleMode;
					gs.player[0].resetAll();
					break;
				}
			}
			break;
		case 6: // sound options
			renderSoundOptions();
			if ( im.isKeyDown(MENU_START_1P) )
			{
				if ( testMenuSubIndex == 3 )
				{
					// TODO: set sound in attract mode to default
				}
				if ( testMenuSubIndex == 4 )
				{
					testMenuMainIndex = 0;
					testMenuSubIndex = -1;
				}
			}
			if ( im.getKeyState(MENU_RIGHT_1P) == JUST_DOWN || im.getKeyState(MENU_SERVICE) == JUST_DOWN )
			{
				testMenuSubIndex = (testMenuSubIndex + 1) % 5;
			}
			if ( im.getKeyState(MENU_LEFT_1P) == JUST_DOWN )
			{
				testMenuSubIndex = (testMenuSubIndex - 1 + 5) % 5;
			}
			if ( im.getKeyState(MENU_RIGHT_2P) == JUST_DOWN )
			{
				if ( testMenuSubIndex == 0 )
				{
					// TODO: cycle through sound in attract options
				}
			}
			if ( im.getKeyState(MENU_LEFT_2P) == JUST_DOWN )
			{
				if ( testMenuSubIndex == 0 )
				{
					// TODO: cycle through sound in attract options
				}
			}

			break;
		case 7: // bookkeeping
			renderBookkeeping(tempSelection);
			if ( im.getKeyState(MENU_START_1P) == JUST_DOWN )
			{
				if ( testMenuSubIndex == 3 )
				{
					testMenuSubIndex = 0;
					if ( tempSelection == 1 )
					{
						bm.nukeBookkeeping();
					}
				}
				else
				{
					testMenuMainIndex = 0;
					testMenuSubIndex = -1;
				}
			}
			if ( im.getKeyState(MENU_START_2P) == JUST_DOWN && testMenuSubIndex != 3 )
			{
				testMenuSubIndex = 3;
				tempSelection = 0;
			}
			if ( im.getKeyState(MENU_RIGHT_1P) == JUST_DOWN && testMenuSubIndex != 3 )
			{
				testMenuSubIndex = (testMenuSubIndex + 1) % 3; // use testMenuSubIndex for each page (4th page is delete confirmation page)
			}
			if ( im.getKeyState(MENU_LEFT_1P) == JUST_DOWN && testMenuSubIndex != 3 )
			{
				testMenuSubIndex = (testMenuSubIndex - 1 + 3) % 3;
			}
			if ( im.getKeyState(MENU_RIGHT_2P) == JUST_DOWN && testMenuSubIndex == 3 )
			{
				tempSelection = 1; // use tempSelection for the yes/no prompt
			}
			if ( im.getKeyState(MENU_LEFT_2P) == JUST_DOWN && testMenuSubIndex == 3 )
			{
				tempSelection = 0;
			}
			break;
		case 8: // data options
			renderDataOptions();

			if ( im.isKeyDown(MENU_START_1P) )
			{
				if ( testMenuSubIndex == 0 ) // disallow name entry
				{
				}
				if ( testMenuSubIndex == 1 ) // reset player's hits chart
				{
					if ( im.isKeyDown(MENU_START_2P) )
					{
						for ( int i = 0; i < NUM_SONGS; i++ )
						{
							songs[i].numPlays = 0;
						}
					}
				}
				if ( testMenuSubIndex == 2 ) // reset last logins
				{
					if ( im.isKeyDown(MENU_START_2P) )
					{
						resetPrevNames();
					}
				}
				if ( testMenuSubIndex == 3 ) // delete player
				{
				}
				if ( testMenuSubIndex == 4 ) // change player pin
				{
				}
				if ( testMenuSubIndex == 5 ) // update software
				{
					checkForUpdates();
				}
				if ( testMenuSubIndex == 6 )
				{
					testMenuMainIndex = 0;
					testMenuSubIndex = -1;
				}
			}
			if ( im.getKeyState(MENU_RIGHT_1P) == JUST_DOWN || im.getKeyState(MENU_SERVICE) == JUST_DOWN )
			{
				testMenuSubIndex = (testMenuSubIndex + 1) % 7;
			}
			if ( im.getKeyState(MENU_LEFT_1P) == JUST_DOWN )
			{
				testMenuSubIndex = (testMenuSubIndex - 1 + 7) % 7;
			}
			if ( im.getKeyState(MENU_LEFT_2P) == JUST_DOWN || im.getKeyState(MENU_RIGHT_2P) == JUST_DOWN )
			{
				if ( testMenuSubIndex == 0 )
				{
					gs.allowLogins = !gs.allowLogins;
				}
			}

			break;
		case 9: // factory defaults
			testMenuMainIndex = 0;
			testMenuSubIndex = -1;
			break;
		case 10: // quit to game mode
			gs.g_gameModeTransition = 1;
			gs.g_currentGameMode = ATTRACT;
			gs.saveOperatorSettings();
			im.updateKeyStates(1); // so that 1P start is not "just down" in the attract loop
			break;
		}
	}

	if ( !(testMenuSubIndex != -1 && testMenuMainIndex == 2) ) // the screen test needs the whole screen
	{
		rectfill(rm.m_backbuf, 0, 460, 640, 480, makecol(0,0,0)); // burn-in protection
	}
}

void firstVideoTestLoop()
{
	vm.reset();
	vm.loadScript(movieScripts[0].c_str());
	vm.play();
}

void mainVideoTestLoop(UTIME dt)
{
	static int videoIndex = 0;

	UNUSED(dt);
	clear_to_color(rm.m_backbuf, 0);
	vm.renderToSurface(rm.m_backbuf, 100, 100);
	renderWhiteString("MOVIE TEST", 10, 10);
	renderWhiteString(movieScripts[videoIndex].c_str(), 10, 30);

	if ( im.getKeyState(MENU_START_2P) == JUST_DOWN )
	{
		vm.reset();
		vm.play();
	}
	if ( videoIndex > 0 && im.getKeyState(MENU_LEFT_1P) == JUST_DOWN )
	{
		videoIndex--;
		vm.loadScript(movieScripts[videoIndex].c_str());
		vm.play();
	}
	if ( videoIndex < NUM_SONGS-1 && im.getKeyState(MENU_RIGHT_1P) == JUST_DOWN )
	{
		videoIndex++;
		vm.loadScript(movieScripts[videoIndex].c_str());
		vm.play();
	}
}

void firstCautionLoop()
{
	if ( m_caution == NULL )
	{
		m_caution = loadImage("DATA/menus/caution.png");
	}
	lm.loadLampProgram("caution.txt");
	cautionTimer = 0;
}

void mainCautionLoop(UTIME dt)
{
	blit(m_caution, rm.m_backbuf, 0, 0, 0, 0, 640, 480);
	cautionTimer += dt;
	if ( cautionTimer < 500 )
	{
		rm.dimScreen(getValueFromRange(100, 0, cautionTimer*100/500));
	}
	if ( cautionTimer >= 2500 )
	{
		rm.dimScreen(getValueFromRange(0, 100, (cautionTimer-2500)*100/500));
	}
	if ( cautionTimer >= 3000 )
	{
		gs.g_gameModeTransition = 1;
		gs.g_currentGameMode = PLAYERSELECT;
	}

	// allow a second player to add in?
	bool enoughCreditsForVersus = gs.isVersusPremium || gs.isFreeplay || (gs.numCoins >= gs.numCoinsPerCredit*2);
	bool leftPlayerAdding = !gs.leftPlayerPresent && im.getKeyState(MENU_START_1P) == JUST_DOWN;
	bool rightPlayerAdding = !gs.rightPlayerPresent && im.getKeyState(MENU_START_2P) == JUST_DOWN;
	if ( leftPlayerAdding && enoughCreditsForVersus )
	{
		gs.leftPlayerPresent = true;
		em.playSample(SFX_CREDIT_BEGIN);
	}
	if ( rightPlayerAdding && enoughCreditsForVersus )
	{
		gs.rightPlayerPresent = true;
		em.playSample(SFX_CREDIT_BEGIN);
	}
}

void firstUpdateLoop()
{
	updateCloseTimer = 0;
}

void mainUpdateLoop(UTIME dt)
{
	clear_to_color(rm.m_backbuf, 0);

	textprintf_centre(rm.m_backbuf, font, 320, 50, WHITE, "DanceManiax System Update");
	if ( updateInProgress )
	{
		textprintf(rm.m_backbuf, font, 50, 140, WHITE, "FILE: %s", dm.getCurrentDownloadFilename().c_str());
		textprintf(rm.m_backbuf, font, 50, 160, WHITE, "PROGRESS: %d", dm.getCurrentDownloadProgress());
		textprintf(rm.m_backbuf, font, 50, 180, WHITE, "TIME ELAPSED: %ld", frameCounter);
	}
	else
	{
		textprintf(rm.m_backbuf, font, 50, 140, WHITE, "ALL FILES UP TO DATE!");
	}

	// is the update done?
	if ( dm.isDownloadComplete() && updateInProgress )
	{
		al_trace("DOWNLOADED FILE %s\n", dm.getCurrentDownloadFilename().c_str());

		// if the update was a zip then decompress it
		if ( dm.getCurrentDownloadFilename().find(".zip") == -1 )
		{
			dm.resetState();
			redownloadManifest = false;

			// non-zipped files are left as-is in the root folder. since this is only for "update.bat" and "unzip.exe" we should immediately check for a zip
			if ( !checkForUpdates() )
			{
				updateInProgress = false;
				redownloadManifest = true; // let the user try again without needing to restart the program
				em.playSample(SFX_LOUD_BELL);
			}
		}
		else
		{
			// a zip file was downloaded - decompress it (potentially overwriting this exe) and restart
			if ( _execl("update.bat", dm.getCurrentDownloadFilename().c_str(), NULL) == -1 )
			{
				al_trace("DOWNLOAD ERROR: unable to run update.bat\n");
				globalError(UPDATE_FAILED, "please reboot the machine");
			}
		}
	}

	// when it's time to leave, hang around for 3 seconds so the text can be read, then leave
	if ( !updateInProgress )
	{
		updateCloseTimer += dt;
		if ( updateCloseTimer > 3000 )
		{
			gs.g_gameModeTransition = 1;
			gs.g_currentGameMode = TESTMODE;				
		}
	}
}

// only called while there is definitely a file download in progress
void alternateMainUpdateLoop()
{
	static long framesElapsed = 0;
	framesElapsed++;
	clear_to_color(rm.m_backbuf, 0);

	textprintf_centre(rm.m_backbuf, font, 320, 50, WHITE, "DanceManiax System Update %s", beginInitialInstall ? " - Install In Progress" : " - Update Program");
	textprintf(rm.m_backbuf, font, 50, 140, WHITE, "FILE: %s", dm.getCurrentDownloadFilename().c_str());
	textprintf(rm.m_backbuf, font, 50, 160, WHITE, "PROGRESS: %d%%", dm.getCurrentDownloadProgress());
	textprintf(rm.m_backbuf, font, 50, 180, WHITE, "TIME ELAPSED: %ld", framesElapsed);
}

void firstBootLoop()
{
	totalBootTime = 0;
	bootStepTime = 0;
	currentBootStep = 0;
}

void mainBootLoop(UTIME dt)
{
	static char results[9][5] = { ".", "..", "...", "....", "OK", "BAD", "SKIP", "PAC", "MINI" };
	int dotdotdot = (bootStepTime / 333) % 3;

	// which board type to render?
	int boardType = 6; // "SKIP"
	if ( extio.isReady() )
	{
		boardType = 4; // "OK"
	}
	if ( pacio.isReady() )
	{
		boardType = 7; // "PAC"
	}
	if ( minimaidio.isReady() )
	{
		boardType = 8; // "MINI"
	}

	bootStepTime += dt;
	if ( currentBootStep <= 2 )
	{
		totalBootTime += dt;
	}

	clear_to_color(rm.m_backbuf, 0);
	textprintf_centre(rm.m_backbuf, font, 320, 50, WHITE, "DanceManiax System Startup");
	textprintf(rm.m_backbuf, font, 50, 140, WHITE, "I/O   CHECK:");
	textprintf(rm.m_backbuf, font, 50, 160, WHITE, "DATA  CHECK:");
	textprintf(rm.m_backbuf, font, 50, 180, WHITE, "SOUND CHECK:");
	textprintf(rm.m_backbuf, font, 50, 410, WHITE, "This program reports non-personal usage data to Google Analytics.");
	textprintf(rm.m_backbuf, font, 50, 420, WHITE, "You may disable tracking by creating a file named \"notracking\".");

	if ( currentBootStep == 0 )
	{
		if ( extio.updateInitialize(dt) == false && pacio.updateInitialize(dt) == false && minimaidio.updateInitialize(dt) == false )
		{
			textprintf(rm.m_backbuf, font, 154, 140, RED, results[5]); // hard fail
			bootStepTime = 0;
			currentBootStep = 1;

			// if the forceMinimaid flag is set then keep retrying until this type of io board appears
			if ( forceMinimaid && boardType != 8 )
			{
				minimaidio.initialize();
				currentBootStep = 0;
			}
		}
		else
		{
			textprintf(rm.m_backbuf, font, 154, 140, WHITE, results[dotdotdot]);

			if ( extio.isReady() || pacio.isReady() || minimaidio.isReady() )
			{
				bootStepTime = 0;
				currentBootStep = 1;
				if ( minimaidio.isReady() && !minimaidLightsOnly )
				{
					im.switchToMinimaidKeys();
				}
			}	
		}
	}
	else if ( currentBootStep == 1 )
	{
		textprintf(rm.m_backbuf, font, 154, 140, boardType == 6 ? WHITE : GREEN, results[boardType]);
		textprintf(rm.m_backbuf, font, 154, 160, WHITE, results[dotdotdot]);

		if ( bootStepTime >= 3000 )
		{
			bootStepTime = 0;
			currentBootStep = 2;
		}
	}
	else if ( currentBootStep == 2 )
	{
		textprintf(rm.m_backbuf, font, 154, 140, boardType == 6 ? WHITE : GREEN, results[boardType]);
		textprintf(rm.m_backbuf, font, 154, 160, GREEN, results[4]);
		textprintf(rm.m_backbuf, font, 154, 180, WHITE, results[dotdotdot]);

		if ( bootStepTime >= 3000 )
		{
			bootStepTime = 0;
			currentBootStep = 3;
		}
	}
	else
	{
		if ( boardType != 6 )
		{
			textprintf(rm.m_backbuf, font, 154, 140, (bootStepTime/75) %2 == 0 && bootStepTime < 2500 ? WHITE : GREEN, results[boardType]);
		}
		else
		{
			textprintf(rm.m_backbuf, font, 154, 140, WHITE, results[6]);
		}
		textprintf(rm.m_backbuf, font, 154, 160, (bootStepTime/75) %2 == 0 && bootStepTime < 2500 ? WHITE : GREEN, results[4]);
		textprintf(rm.m_backbuf, font, 154, 180, (bootStepTime/75) %2 == 0 && bootStepTime < 2500 ? WHITE : GREEN, results[4]);

		if ( bootStepTime >= 3000 )
		{
			gs.g_gameModeTransition = 1;
			gs.g_currentGameMode = ATTRACT;
		}
	}

	textprintf(rm.m_backbuf, font, 50, 350, WHITE, "TIME ELAPSED: %ld", totalBootTime);
}

int initializeSonglist()
{
	if ( loadSongDB() == -1 )
	{
		return -1; // aw crap
	}
	
	// load banners
	m_banners = (BITMAP **)malloc( sizeof(BITMAP) * NUM_SONGS);
	char bfilename[] = "DATA/banners/0000.png";
	for ( int i = 0; i < NUM_SONGS; i++ )
	{
		int c = songs[i].songID;
		bfilename[13] = (c/1000)%10 + '0';
		bfilename[14] = (c/100)%10 + '0';
		bfilename[15] = (c/10)%10 + '0';
		bfilename[16] = c%10 + '0';
		m_banners[i] = load_bitmap(bfilename, NULL);
		if ( m_banners[i] == NULL )
		{
			m_banners[i] = create_bitmap(256,256);
			clear_to_color(m_banners[i], 0);
			textprintf_centre(m_banners[i], font, 128, 128, 0xFFFFFF, "BANNER %d", c);
		}
	}

	loadPlayersHitList();
	return 0;
}

int loadSongDB()
{
	FILE* fp = NULL;

	if ( fopen_s(&fp, SONGDB_FILENAME, "rt") != 0 )
	{
		allegro_message("Unable to find DATA/song_db.csv");
		return -1;
	}

	// the first line of the file is: DMXDB,1,103,April 6th 2013
	char sanityCheck[32] = "";
	int verNum = 0;
	char date[32] = "";
	fscanf_s(fp, "%[^,],%[^,],%d,%d ", &sanityCheck, 8, &date, 32, &NUM_SONGS, &verNum);
	if ( _stricmp(sanityCheck, "DMXDB") != 0 )
	{
		allegro_message("Unexpected file contents in song_db.csv - not a song_db file");
		return -1;
	}
	if ( verNum != 2 )
	{
		allegro_message("Unexpected version number in song_db.csv: found %d expected 2", verNum);
		return -1;
	}

	fscanf_s(fp, "%[,] ", &sanityCheck, 32); // get rid of those commas

	songIDs = (int *)malloc(sizeof(int) * NUM_SONGS);
	songTitles = new std::string[NUM_SONGS];
	songArtists = new std::string[NUM_SONGS];
	movieScripts = new std::string[NUM_SONGS];
	songs = (SongEntry *)malloc(sizeof(SongEntry) * NUM_SONGS);
	NUM_COURSES = 0;

	// now read a bunch of lines that look like:
	// obsolete version 1 // -> 101,BROKEN MY HEART,NAOKI feat.PAULA TERRY,brok.seq,4,0,7,0,6,0,6,0,1,0,0
	// version 2 // -> 101,BROKEN MY HEART,NAOKI feat.PAULA TERRY,brok.seq,4,0,7,0,0,0,6,0,6,0,0,0,1,0,0
	for ( int i = 0; i < NUM_SONGS+1; i++ )
	{
		int id = 0;
		char name[64] = "";
		char artist[64] = "";
		int minbpm = 0, maxbpm = 0;
		char movie[64] = "";
		int sm = 0, smnotes = 0;
		int sw = 0, swnotes = 0;
		int sx = 0, sxnotes = 0;
		int dm = 0, dmnotes = 0;
		int dw = 0, dwnotes = 0;
		int dx = 0, dxnotes = 0;
		int version = 0;
		int flag = 0;
		int isNew = 0;

		fscanf_s(fp, "%d,%[^,],%[^,],%d,%d,%[^,],%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d ", &id, &name, 64, &artist, 64, &minbpm, &maxbpm, &movie, 64, &sm, &smnotes, &sw, &swnotes, &sx, &sxnotes, &dm, &dmnotes, &dw, &dwnotes, &dx, &dxnotes, &version, &flag, &isNew);

		if ( i == NUM_SONGS )
		{
			if ( id == 9999 )
			{
				break; // good! this was another sanity check
			}
			allegro_message("The declared number of songs in song_db.csv might be incorrect. Check for errors.");
			return -1; // the declared NUM_SONGS was incorrect
		}

		if ( id == 9999 )
		{
			allegro_message("The declared number of songs in song_db.csv is too high. (Expected: %d, found: %d)", NUM_SONGS, i-1);
		}
		if ( id < 100 || id > 400 )
		{
			allegro_message("Unexpected file contents in song_db.csv - check line %d. (SongID = %d)", i-1, id);
			return -1;
		}
		songIDs[i] = id;
		songTitles[i].assign(name, strlen(name));
		songArtists[i].assign(artist, strlen(artist));
		movieScripts[i].assign(movie, strlen(movie));
		songs[i].initialize(id, minbpm, maxbpm, sm, sw, sx, dm, dw, dx, version); // BROKEN MY HEART
		songs[i].specialFlag = flag;
		songs[i].isNew = isNew == 1;

		if ( version == 101 )
		{
			NUM_COURSES++;
		}
		else if ( version > 0 && version <= 5 )
		{
			NUM_PLAYABLE_SONGS++;
		}
		else if ( version < 0 || version > 5 )
		{
			allegro_message("Unexpected file contents in song_db.csv - check line %d. (Version = %d)", i-1, version);
		}

		// do some datamining? sure whatev's
		/*
		#include "xsq_read.h"
		if ( version > 0 && version < 5 )
		{
			smnotes = readXSQ(&gs.player[0].currentChart, &gs.player[0].freezeArrows, id, SINGLE_MILD)/2;
			swnotes = readXSQ(&gs.player[0].currentChart, &gs.player[0].freezeArrows, id, SINGLE_WILD)/2;
			dmnotes = readXSQ(&gs.player[0].currentChart, &gs.player[0].freezeArrows, id, DOUBLE_MILD)/2;
			dwnotes = readXSQ(&gs.player[0].currentChart, &gs.player[0].freezeArrows, id, DOUBLE_WILD)/2;
			al_trace("%d, %d, %d, %d, %d, %d, %d, %d, %d\r\n", id, sm, smnotes, sw, swnotes, dm, dmnotes, dw, dwnotes);
		}
		//*/
	}

	fclose(fp);

	return 0;
}

void savePlayersHitList()
{
	FILE* fp = NULL;

	if ( fopen_s(&fp, HITLIST_FILENAME, "wb") != 0 )
	{
		TRACE("UNABLE TO SAVE PLAYERS BEST LIST");
		return;
	}

	// write the version
	fprintf(fp, "DMXH");
	int vnum = CURRENT_HITLIST_VERSION_NUMBER;
	fwrite(&vnum, sizeof(long), 1, fp);
	
	// write everything out
	for ( int i = 0; i < NUM_SONGS; i++ )
	{
		fwrite(&songs[i].songID, sizeof(int), 1, fp);
		fwrite(&songs[i].numPlays, sizeof(int), 1, fp);
	}
	safeCloseFile(fp, HITLIST_FILENAME);
}

void loadPlayersHitList()
{
	FILE* fp = NULL;
	int n = 0;

	// does file exist?
	if ( (fp = safeLoadFile(HITLIST_FILENAME)) == NULL )
	{
		return; // no big deal
	}

	// check the version
	int vnum = checkFileVersion(fp, "DMXH");
	if ( vnum != CURRENT_HITLIST_VERSION_NUMBER )
	{
		TRACE("Wrong hitlist version number.");
		return; // later when multiple versions exist, ideas for converting them
	}

	// read a bunch of (songid, playcount) pairs until there are none left
	while ( fread(&n, sizeof(int), 1, fp) > 0 )
	{
		int listindex = songID_to_listID(n);
		fread(&n, sizeof(int), 1, fp);
		if ( listindex >= 0 )
		{
			songs[listindex].numPlays = n;
		}
	}
	fclose(fp);
}

void renderInputTest()
{
	textprintf_centre(rm.m_backbuf, font, 320, 50, WHITE, "INPUT TEST");
	textprintf(rm.m_backbuf, font, 50, 100, GREEN, "1P LEFT SIDE");
	textprintf(rm.m_backbuf, font, 50, 120, WHITE, "TOP L: ");
	textprintf(rm.m_backbuf, font, 50, 140, WHITE, "TOP R: ");
	textprintf(rm.m_backbuf, font, 50, 160, WHITE, "LOW L: ");
	textprintf(rm.m_backbuf, font, 50, 180, WHITE, "LOW R: ");
	textprintf(rm.m_backbuf, font, 50, 200, WHITE, "MENU LEFT : ");
	textprintf(rm.m_backbuf, font, 50, 220, WHITE, "MENU RIGHT: ");
	textprintf(rm.m_backbuf, font, 50, 240, WHITE, "MENU START: ");
	textprintf(rm.m_backbuf, font, 106, 120, GET_ON_COLOR(im.isKeyDown(UL_1P)), GET_ON_OFF(im.isKeyDown(UL_1P)));
	textprintf(rm.m_backbuf, font, 106, 140, GET_ON_COLOR(im.isKeyDown(UR_1P)), GET_ON_OFF(im.isKeyDown(UR_1P)));
	textprintf(rm.m_backbuf, font, 106, 160, GET_ON_COLOR(im.isKeyDown(BLUE_SENSOR_1PL0)), GET_ON_OFF(im.isKeyDown(BLUE_SENSOR_1PL0)));
	textprintf(rm.m_backbuf, font, 106, 180, GET_ON_COLOR(im.isKeyDown(BLUE_SENSOR_1PR0)), GET_ON_OFF(im.isKeyDown(BLUE_SENSOR_1PR0)));
	textprintf(rm.m_backbuf, font, 138, 160, GET_ON_COLOR(im.isKeyDown(BLUE_SENSOR_1PL1)), GET_ON_OFF(im.isKeyDown(BLUE_SENSOR_1PL1)));
	textprintf(rm.m_backbuf, font, 138, 180, GET_ON_COLOR(im.isKeyDown(BLUE_SENSOR_1PR1)), GET_ON_OFF(im.isKeyDown(BLUE_SENSOR_1PR1)));
	textprintf(rm.m_backbuf, font, 144, 200, GET_ON_COLOR(im.isKeyDown(MENU_LEFT_1P)), GET_ON_OFF(im.isKeyDown(MENU_LEFT_1P)));
	textprintf(rm.m_backbuf, font, 144, 220, GET_ON_COLOR(im.isKeyDown(MENU_RIGHT_1P)), GET_ON_OFF(im.isKeyDown(MENU_RIGHT_1P)));
	textprintf(rm.m_backbuf, font, 144, 240, GET_ON_COLOR(im.isKeyDown(MENU_START_1P)), GET_ON_OFF(im.isKeyDown(MENU_START_1P)));

	textprintf(rm.m_backbuf, font, 400, 100, GREEN, "2P RIGHT SIDE");
	textprintf(rm.m_backbuf, font, 400, 120, WHITE, "TOP L: ");
	textprintf(rm.m_backbuf, font, 400, 140, WHITE, "TOP R: ");
	textprintf(rm.m_backbuf, font, 400, 160, WHITE, "LOW L: ");
	textprintf(rm.m_backbuf, font, 400, 180, WHITE, "LOW R: ");
	textprintf(rm.m_backbuf, font, 400, 200, WHITE, "MENU LEFT : ");
	textprintf(rm.m_backbuf, font, 400, 220, WHITE, "MENU RIGHT: ");
	textprintf(rm.m_backbuf, font, 400, 240, WHITE, "MENU START: ");
	textprintf(rm.m_backbuf, font, 456, 120, GET_ON_COLOR(im.isKeyDown(UL_2P)), GET_ON_OFF(im.isKeyDown(UL_2P)));
	textprintf(rm.m_backbuf, font, 456, 140, GET_ON_COLOR(im.isKeyDown(UR_2P)), GET_ON_OFF(im.isKeyDown(UR_2P)));
	textprintf(rm.m_backbuf, font, 456, 160, GET_ON_COLOR(im.isKeyDown(BLUE_SENSOR_2PL0)), GET_ON_OFF(im.isKeyDown(BLUE_SENSOR_2PL0)));
	textprintf(rm.m_backbuf, font, 456, 180, GET_ON_COLOR(im.isKeyDown(BLUE_SENSOR_2PR0)), GET_ON_OFF(im.isKeyDown(BLUE_SENSOR_2PR0)));
	textprintf(rm.m_backbuf, font, 488, 160, GET_ON_COLOR(im.isKeyDown(BLUE_SENSOR_2PL1)), GET_ON_OFF(im.isKeyDown(BLUE_SENSOR_2PL1)));
	textprintf(rm.m_backbuf, font, 488, 180, GET_ON_COLOR(im.isKeyDown(BLUE_SENSOR_2PR1)), GET_ON_OFF(im.isKeyDown(BLUE_SENSOR_2PR1)));
	textprintf(rm.m_backbuf, font, 494, 200, GET_ON_COLOR(im.isKeyDown(MENU_LEFT_2P)), GET_ON_OFF(im.isKeyDown(MENU_LEFT_2P)));
	textprintf(rm.m_backbuf, font, 494, 220, GET_ON_COLOR(im.isKeyDown(MENU_RIGHT_2P)), GET_ON_OFF(im.isKeyDown(MENU_RIGHT_2P)));
	textprintf(rm.m_backbuf, font, 494, 240, GET_ON_COLOR(im.isKeyDown(MENU_START_2P)), GET_ON_OFF(im.isKeyDown(MENU_START_2P)));

	textprintf(rm.m_backbuf, font, 250, 300, WHITE, "   TEST: ");
	textprintf(rm.m_backbuf, font, 250, 320, WHITE, "SERVICE: ");
	textprintf(rm.m_backbuf, font, 250, 340, WHITE, "   COIN: ");
	textprintf(rm.m_backbuf, font, 330, 300, GET_ON_COLOR(im.isKeyDown(MENU_TEST)), GET_ON_OFF(im.isKeyDown(MENU_TEST)));
	textprintf(rm.m_backbuf, font, 330, 320, GET_ON_COLOR(im.isKeyDown(MENU_SERVICE)), GET_ON_OFF(im.isKeyDown(MENU_SERVICE)));
	textprintf(rm.m_backbuf, font, 330, 340, GET_ON_COLOR(im.isKeyDown(MENU_COIN)), GET_ON_OFF(im.isKeyDown(MENU_COIN)));

	textprintf(rm.m_backbuf, font, 50, 440, WHITE, "1P START + 2P START = exit input test");
}

void renderLampTest()
{
	textprintf_centre(rm.m_backbuf, font, 320, 50, WHITE, "LAMP TEST");
	textprintf_centre(rm.m_backbuf, font, 320, 70, testMenuSubIndex == 0 ? RED : WHITE, "ALL LAMPS OFF");

	textprintf(rm.m_backbuf, font, 40, 100, GREEN, "<1P - TOP LEFT>");
	textprintf(rm.m_backbuf, font, 40, 120, testMenuSubIndex == 1 ? RED : WHITE, "RED");
	textprintf(rm.m_backbuf, font, 40, 140, testMenuSubIndex == 2 ? RED : WHITE, "BLUE");
	textprintf(rm.m_backbuf, font, 40, 160, testMenuSubIndex == 3 ? RED : WHITE, "PURPLE");

	textprintf(rm.m_backbuf, font, 190, 100, GREEN, "<1P - TOP RIGHT>");
	textprintf(rm.m_backbuf, font, 190, 120, testMenuSubIndex == 4 ? RED : WHITE, "RED");
	textprintf(rm.m_backbuf, font, 190, 140, testMenuSubIndex == 5 ? RED : WHITE, "BLUE");
	textprintf(rm.m_backbuf, font, 190, 160, testMenuSubIndex == 6 ? RED : WHITE, "PURPLE");

	textprintf(rm.m_backbuf, font, 340, 100, GREEN, "<2P - TOP LEFT>");
	textprintf(rm.m_backbuf, font, 340, 120, testMenuSubIndex == 7 ? RED : WHITE, "RED");
	textprintf(rm.m_backbuf, font, 340, 140, testMenuSubIndex == 8 ? RED : WHITE, "BLUE");
	textprintf(rm.m_backbuf, font, 340, 160, testMenuSubIndex == 9 ? RED : WHITE, "PURPLE");

	textprintf(rm.m_backbuf, font, 490, 100, GREEN, "<2P - TOP RIGHT>");
	textprintf(rm.m_backbuf, font, 490, 120, testMenuSubIndex == 10 ? RED : WHITE, "RED");
	textprintf(rm.m_backbuf, font, 490, 140, testMenuSubIndex == 11 ? RED : WHITE, "BLUE");
	textprintf(rm.m_backbuf, font, 490, 160, testMenuSubIndex == 12 ? RED : WHITE, "PURPLE");

	textprintf(rm.m_backbuf, font, 40, 200, GREEN, "<1P - LOW LEFT>");
	textprintf(rm.m_backbuf, font, 40, 220, testMenuSubIndex == 13 ? RED : WHITE, "RED");
	textprintf(rm.m_backbuf, font, 40, 240, testMenuSubIndex == 14 ? RED : WHITE, "BLUE");
	textprintf(rm.m_backbuf, font, 40, 260, testMenuSubIndex == 15 ? RED : WHITE, "PURPLE");
	textprintf(rm.m_backbuf, font, 40, 280, testMenuSubIndex == 16 ? RED : WHITE, "RED");
	textprintf(rm.m_backbuf, font, 40, 300, testMenuSubIndex == 17 ? RED : WHITE, "BLUE");
	textprintf(rm.m_backbuf, font, 40, 320, testMenuSubIndex == 18 ? RED : WHITE, "PURPLE");

	textprintf(rm.m_backbuf, font, 190, 200, GREEN, "<1P - LOW RIGHT>");
	textprintf(rm.m_backbuf, font, 190, 220, testMenuSubIndex == 19 ? RED : WHITE, "RED");
	textprintf(rm.m_backbuf, font, 190, 240, testMenuSubIndex == 20 ? RED : WHITE, "BLUE");
	textprintf(rm.m_backbuf, font, 190, 260, testMenuSubIndex == 21 ? RED : WHITE, "PURPLE");
	textprintf(rm.m_backbuf, font, 190, 280, testMenuSubIndex == 22 ? RED : WHITE, "RED");
	textprintf(rm.m_backbuf, font, 190, 300, testMenuSubIndex == 23 ? RED : WHITE, "BLUE");
	textprintf(rm.m_backbuf, font, 190, 320, testMenuSubIndex == 24 ? RED : WHITE, "PURPLE");

	textprintf(rm.m_backbuf, font, 340, 200, GREEN, "<2P - LOW LEFT>");
	textprintf(rm.m_backbuf, font, 340, 220, testMenuSubIndex == 25 ? RED : WHITE, "RED");
	textprintf(rm.m_backbuf, font, 340, 240, testMenuSubIndex == 26 ? RED : WHITE, "BLUE");
	textprintf(rm.m_backbuf, font, 340, 260, testMenuSubIndex == 27 ? RED : WHITE, "PURPLE");
	textprintf(rm.m_backbuf, font, 340, 280, testMenuSubIndex == 28 ? RED : WHITE, "RED");
	textprintf(rm.m_backbuf, font, 340, 300, testMenuSubIndex == 29 ? RED : WHITE, "BLUE");
	textprintf(rm.m_backbuf, font, 340, 320, testMenuSubIndex == 30 ? RED : WHITE, "PURPLE");

	textprintf(rm.m_backbuf, font, 490, 200, GREEN, "<2P - LOW RIGHT>");
	textprintf(rm.m_backbuf, font, 490, 220, testMenuSubIndex == 31 ? RED : WHITE, "RED");
	textprintf(rm.m_backbuf, font, 490, 240, testMenuSubIndex == 32 ? RED : WHITE, "BLUE");
	textprintf(rm.m_backbuf, font, 490, 260, testMenuSubIndex == 33 ? RED : WHITE, "PURPLE");
	textprintf(rm.m_backbuf, font, 490, 280, testMenuSubIndex == 34 ? RED : WHITE, "RED");
	textprintf(rm.m_backbuf, font, 490, 300, testMenuSubIndex == 35 ? RED : WHITE, "BLUE");
	textprintf(rm.m_backbuf, font, 490, 320, testMenuSubIndex == 36 ? RED : WHITE, "PURPLE");

	textprintf(rm.m_backbuf, font, 90, 340, GREEN, "<1P - MENU>");
	textprintf(rm.m_backbuf, font, 90, 360, testMenuSubIndex == 37 ? RED : WHITE, "LEFT");
	textprintf(rm.m_backbuf, font, 90, 380, testMenuSubIndex == 38 ? RED : WHITE, "START");
	textprintf(rm.m_backbuf, font, 90, 400, testMenuSubIndex == 39 ? RED : WHITE, "RIGHT");

	textprintf(rm.m_backbuf, font, 265, 340, GREEN, "<SPOT LIGHT>");
	textprintf(rm.m_backbuf, font, 265, 360, testMenuSubIndex == 40 ? RED : WHITE, "YELLOW");
	textprintf(rm.m_backbuf, font, 265, 380, testMenuSubIndex == 41 ? RED : WHITE, "PINK");
	textprintf(rm.m_backbuf, font, 265, 400, testMenuSubIndex == 42 ? RED : WHITE, "BLUE");

	textprintf(rm.m_backbuf, font, 440, 340, GREEN, "<2P - MENU>");
	textprintf(rm.m_backbuf, font, 440, 360, testMenuSubIndex == 43 ? RED : WHITE, "LEFT");
	textprintf(rm.m_backbuf, font, 440, 380, testMenuSubIndex == 44 ? RED : WHITE, "START");
	textprintf(rm.m_backbuf, font, 440, 400, testMenuSubIndex == 45 ? RED : WHITE, "RIGHT");

	textprintf(rm.m_backbuf, font, 50, 420, makecol(196, 255, 255), "PRESS 1P LEFT / RIGHT = change target lamp");
	textprintf(rm.m_backbuf, font, 50, 440, makecol(196, 255, 255), "PRESS 1P START BUTTON = confirm selection");

#ifdef DMXDEBUG
	if (DMXDEBUG)
	{
		lm.renderDebugOutput(rm.m_backbuf);
	}
#endif
}

void renderScreenTest()
{
	// every 20 pixels, gridlines
	solid_mode();
	for ( int x = 0; x < SCREEN_WIDTH; x += 20 )
	{
		line(rm.m_backbuf, x, 0, x, SCREEN_HEIGHT, x == SCREEN_WIDTH/2 ? RED : WHITE);
		line(rm.m_backbuf, x+1, 0, x+1, SCREEN_HEIGHT, x == SCREEN_WIDTH/2 ? RED : WHITE);
	}
	for ( int y = 0; y < SCREEN_HEIGHT; y += 20 )
	{
		line(rm.m_backbuf, 0, y, SCREEN_WIDTH, y, y == SCREEN_HEIGHT/2 ? RED : WHITE);
		line(rm.m_backbuf, 0, y+1, SCREEN_WIDTH, y+1, y == SCREEN_HEIGHT/2 ? RED : WHITE);
	}

	rect(rm.m_backbuf, 0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, makecol(255, 0, 0));
	rect(rm.m_backbuf, 1, 1, SCREEN_WIDTH-2, SCREEN_HEIGHT-2, makecol(255, 0, 0));
	rect(rm.m_backbuf, 2, 2, SCREEN_WIDTH-3, SCREEN_HEIGHT-3, makecol(255, 0, 0));
	rect(rm.m_backbuf, 3, 3, SCREEN_WIDTH-4, SCREEN_HEIGHT-4, makecol(255, 0, 0));
	set_alpha_blender(); // the game assumes the graphics are left in this mode

	textprintf(rm.m_backbuf, font, 44, 448, makecol(196, 255, 255), "1P START = exit screen test");
	blit(rm.m_backbuf, screen, 0, 0, 0, 0, 640, 480);
}

void renderColorTest()
{
	textprintf_centre(rm.m_backbuf, font, 320, 50, WHITE, "COLOR TEST");

	for ( int i = 0; i < 20; i++ )
	{
		int p = 255 * i / 20;
		rectfill(rm.m_backbuf, i*32, 100, (i+1)*32, 200, makecol(p, p, p));
	}
	textprintf_centre(rm.m_backbuf, font, 320, 220, WHITE, "YOU SHOULD SEE 20 DISTINCT SHADES OF GREY");

	solid_mode();
	for ( int c = 0; c < 512; c++ )
	{
		line(rm.m_backbuf, 64+c, 270, 64+c, 290, makecol(c/2, 0, 0));
		line(rm.m_backbuf, 64+c, 290, 64+c, 310, makecol(0, c/2, 0));
		line(rm.m_backbuf, 64+c, 310, 64+c, 330, makecol(0, 0, c/2));
		line(rm.m_backbuf, 64+c, 330, 64+c, 350, makecol(c/2, 0, c/2));
		line(rm.m_backbuf, 64+c, 350, 64+c, 370, makecol(c/2, c/2, 0));
		line(rm.m_backbuf, 64+c, 370, 64+c, 390, makecol(0, c/2, c/2));
	}
	line(rm.m_backbuf, 64, 250, 64, 410, WHITE);
	line(rm.m_backbuf, 576, 250, 576, 410, WHITE);
	textprintf(rm.m_backbuf, font, 582, 276, WHITE, "RED");
	textprintf(rm.m_backbuf, font, 582, 296, WHITE, "GREEN");
	textprintf(rm.m_backbuf, font, 582, 316, WHITE, "BLUE");
	textprintf(rm.m_backbuf, font, 582, 336, WHITE, "PINK");
	textprintf(rm.m_backbuf, font, 582, 356, WHITE, "YELLOW");
	textprintf(rm.m_backbuf, font, 582, 376, WHITE, "CYAN");
	set_alpha_blender(); // the game assumes the graphics are left in this mode

	textprintf(rm.m_backbuf, font, 50, 440, makecol(196, 255, 255), "1P START = exit color test");
}

void renderCoinOptions()
{
	textprintf_centre(rm.m_backbuf, font, 320, 50, WHITE, "COIN OPTIONS");

	textprintf(rm.m_backbuf, font, 50, 100, testMenuSubIndex == 0 ? RED : WHITE, "FREE PLAY");
	textprintf(rm.m_backbuf, font, 50, 130, testMenuSubIndex == 1 ? RED : WHITE, "COINS PER CREDIT");
	textprintf(rm.m_backbuf, font, 50, 160, testMenuSubIndex == 2 ? RED : WHITE, "VERSUS PREMIUM");
	textprintf(rm.m_backbuf, font, 50, 190, testMenuSubIndex == 3 ? RED : WHITE, "DOUBLE PREMIUM");
	textprintf(rm.m_backbuf, font, 50, 310, testMenuSubIndex == 4 ? RED : WHITE, "FACTORY SETTINGS");
	textprintf(rm.m_backbuf, font, 50, 340, testMenuSubIndex == 5 ? RED : WHITE, "SAVE AND EXIT");

	textprintf(rm.m_backbuf, font, 236, 100, gs.isFreeplay ? RED : GREEN, GET_ON_OFF(gs.isFreeplay));
	textprintf(rm.m_backbuf, font, 236, 130, gs.numCoinsPerCredit == DEFAULT_COINS_PER_CREDIT ? GREEN : RED, "%d", gs.numCoinsPerCredit );
	textprintf(rm.m_backbuf, font, 236, 160, gs.isVersusPremium ? RED : GREEN, GET_ON_OFF(gs.isVersusPremium));
	textprintf(rm.m_backbuf, font, 236, 190, gs.isDoublePremium ? RED : GREEN, GET_ON_OFF(gs.isDoublePremium));

	textprintf(rm.m_backbuf, font, 50, 400, makecol(196, 255, 255), "PRESS 1P LEFT / RIGHT = select item");
	textprintf(rm.m_backbuf, font, 50, 420, makecol(196, 255, 255), "PRESS 2P LEFT / RIGHT = modify setting");
	textprintf(rm.m_backbuf, font, 50, 440, makecol(196, 255, 255), "PRESS 1P START BUTTON = confirm selection");
}

void renderGameOptions()
{
	textprintf_centre(rm.m_backbuf, font, 320, 50, WHITE, "GAME OPTIONS");

	textprintf(rm.m_backbuf, font, 50, 100, testMenuSubIndex == 0 ? RED : WHITE, "SONGS PER CREDIT");
	textprintf(rm.m_backbuf, font, 50, 130, testMenuSubIndex == 1 ? RED : WHITE, "DISABLE MENU TIMER");
	textprintf(rm.m_backbuf, font, 50, 160, testMenuSubIndex == 2 ? RED : WHITE, "CONTINUOUS PLAY MODE");
	textprintf(rm.m_backbuf, font, 50, 310, testMenuSubIndex == 3 ? RED : WHITE, "FACTORY SETTINGS");
	textprintf(rm.m_backbuf, font, 50, 340, testMenuSubIndex == 4 ? RED : WHITE, "SAVE AND EXIT");

	textprintf(rm.m_backbuf, font, 236, 100, gs.numSongsPerSet == DEFAULT_SONGS_PER_SET ? GREEN : RED, "%d", gs.numSongsPerSet );
	textprintf(rm.m_backbuf, font, 236, 130, GREEN, GET_ON_OFF(gs.isEventMode));
	textprintf(rm.m_backbuf, font, 236, 160, GREEN, GET_ON_OFF(gs.isFreestyleMode));

	textprintf(rm.m_backbuf, font, 50, 400, makecol(196, 255, 255), "PRESS 1P LEFT / RIGHT = select item");
	textprintf(rm.m_backbuf, font, 50, 420, makecol(196, 255, 255), "PRESS 2P LEFT / RIGHT = modify setting");
	textprintf(rm.m_backbuf, font, 50, 440, makecol(196, 255, 255), "PRESS 1P START BUTTON = confirm selection");
}

void renderBookkeeping(int tempSelection)
{
	static char days[7][4] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
	int i = 0;
	DAILY_BOOKKEEP grand_total, week_total;

	// first, calculate the totals (used on every page)
	for ( i = 0; i < 7; i++ )
	{
		DAILY_BOOKKEEP temp = bm.getAllTime(i);
		grand_total.uptime += temp.uptime;
		grand_total.playtime += temp.playtime;
		grand_total.logins += temp.logins;
		grand_total.nonLogins += temp.nonLogins;
		grand_total.coins += temp.coins;
		grand_total.services += temp.services;
		grand_total.spGames += temp.spGames;
		grand_total.dpGames += temp.dpGames;
		grand_total.mpGames += temp.mpGames;
		grand_total.vpGames += temp.vpGames;

		temp = bm.getThisWeek(i);
		week_total.uptime += temp.uptime;
		week_total.playtime += temp.playtime;
		week_total.logins += temp.logins;
		week_total.nonLogins += temp.nonLogins;
		week_total.coins += temp.coins;
		week_total.services += temp.services;
		week_total.spGames += temp.spGames;
		week_total.dpGames += temp.dpGames;
		week_total.mpGames += temp.mpGames;
		week_total.vpGames += temp.vpGames;
	}

	textprintf_centre(rm.m_backbuf, font, 320, 10, WHITE, "VIEW BOOKKEEPING INFORMATION");

	// now render each page
	switch ( testMenuSubIndex )
	{
	case 0:
		textprintf_centre(rm.m_backbuf, font, 320, 35, GREEN, "LIFETIME STATS");

		textprintf(rm.m_backbuf, font, 50, 100, WHITE, "TOTAL ACTIVE TIME: %d", grand_total.uptime);
		textprintf(rm.m_backbuf, font, 50, 120, WHITE, "TOTAL PLAY TIME  : %d", grand_total.playtime);
		textprintf(rm.m_backbuf, font, 50, 160, WHITE, "TOTAL LOGINS  : %d", grand_total.logins);
		textprintf(rm.m_backbuf, font, 50, 180, WHITE, "TOTAL NO LOGIN: %d", grand_total.nonLogins);
		textprintf(rm.m_backbuf, font, 50, 220, WHITE, "TOTAL COINS   : %d", grand_total.coins);
		textprintf(rm.m_backbuf, font, 50, 240, WHITE, "TOTAL SERVICES: %d", grand_total.services);
		textprintf(rm.m_backbuf, font, 50, 280, WHITE, "SINGLES GAMES PLAYED: %d", grand_total.spGames);
		textprintf(rm.m_backbuf, font, 50, 300, WHITE, "DOUBLES GAMES PLAYED: %d", grand_total.dpGames);
		textprintf(rm.m_backbuf, font, 50, 320, WHITE, "VERSUS  GAMES PLAYED: %d", grand_total.vpGames);
		//textprintf(rm.m_backbuf, font, 50, 340, WHITE, "MISSION GAMES PLAYED: %d", grand_total.mpGames);
		break;
	case 1:
		textprintf_centre(rm.m_backbuf, font, 320, 35, GREEN, "COIN DATA OF LAST 7 DAYS");
		textprintf_centre(rm.m_backbuf, font, 320, 235, GREEN, "LIFETIME COIN DATA PER DAY");
		textprintf(rm.m_backbuf, font, 50, 50, WHITE, "    | COINS | SERVC | 1PLAY | DOUBL | 2PLAY | OTHER |");
		textprintf(rm.m_backbuf, font, 50, 250, WHITE, "    | COINS | SERVC | 1PLAY | DOUBL | 2PLAY | OTHER |");
		for ( i = 0; i < 7; i++ )
		{
			DAILY_BOOKKEEP temp = bm.getThisWeek(i);
			textprintf(rm.m_backbuf, font, 50, 70 + 20*i, WHITE, "%s | %5d | %5d | %5d | %5d | %5d | %5d |", 
				days[i], temp.coins, temp.services, temp.spGames, temp.dpGames, temp.vpGames, temp.mpGames);
			temp = bm.getAllTime(i);
			textprintf(rm.m_backbuf, font, 50, 270 + 20*i, WHITE, "%s | %5d | %5d | %5d | %5d | %5d | %5d |", 
				days[i], temp.coins, temp.services, temp.spGames, temp.dpGames, temp.vpGames, temp.mpGames);
		}
		textprintf(rm.m_backbuf, font, 34, 70 + 140, GREEN, "TOTAL | %5d | %5d | %5d | %5d | %5d | %5d |", 
			week_total.coins, week_total.services, week_total.spGames, week_total.dpGames, week_total.vpGames, week_total.mpGames);
		textprintf(rm.m_backbuf, font, 34, 270 + 140, GREEN, "TOTAL | %5d | %5d | %5d | %5d | %5d | %5d |", 
			grand_total.coins, grand_total.services, grand_total.spGames, grand_total.dpGames, grand_total.vpGames, grand_total.mpGames);
		break;
	case 2:
		textprintf_centre(rm.m_backbuf, font, 320, 35, GREEN, "ACTIVITY OF LAST 7 DAYS");
		textprintf_centre(rm.m_backbuf, font, 320, 235, GREEN, "LIFETIME ACTIVITY PER DAY");
		textprintf(rm.m_backbuf, font, 50, 50, WHITE, "    | PWR TIME | PLAYTIME | LOGIN | NOLOG |");
		textprintf(rm.m_backbuf, font, 50, 250, WHITE, "    | PWR TIME | PLAYTIME | LOGIN | NOLOG |");
		for ( i = 0; i < 7; i++ )
		{
			DAILY_BOOKKEEP temp = bm.getThisWeek(i);
			textprintf(rm.m_backbuf, font, 50, 70 + 20*i, WHITE, "%s | %8d | %8d | %5d | %5d |", 
				days[i], temp.uptime, temp.playtime, temp.logins, temp.nonLogins);
			temp = bm.getAllTime(i);
			textprintf(rm.m_backbuf, font, 50, 270 + 20*i, WHITE, "%s | %8d | %8d | %5d | %5d |", 
				days[i], temp.uptime, temp.playtime, temp.logins, temp.nonLogins);
		}
		textprintf(rm.m_backbuf, font, 34, 70 + 140, GREEN, "TOTAL | %8d | %8d | %5d | %5d |", 
			week_total.uptime, week_total.playtime, week_total.logins, week_total.nonLogins);
		textprintf(rm.m_backbuf, font, 34, 270 + 140, GREEN, "TOTAL | %8d | %8d | %5d | %5d |", 
			grand_total.uptime, grand_total.playtime, grand_total.logins, grand_total.nonLogins);
		break;
	case 3:
		textprintf_centre(rm.m_backbuf, font, 320, 75, WHITE, "CLEAR ALL BOOKKEEPING?");
		textprintf(rm.m_backbuf, font, 320, 200, WHITE, "ARE YOU SURE?");
		textprintf(rm.m_backbuf, font, 320, 250, WHITE, "NO / YES");
		if ( tempSelection == 0 )
		{
			textprintf(rm.m_backbuf, font, 320, 250, RED, "NO      ");
		}
		else
		{
			textprintf(rm.m_backbuf, font, 320, 250, RED, "     YES");
		}
		break;
	}

	if ( testMenuSubIndex != 3 )
	{
		textprintf(rm.m_backbuf, font, 50, 450, makecol(196, 255, 255), "PRESS 1P LEFT / RIGHT = PREV / NEXT DATA");
		textprintf(rm.m_backbuf, font, 50, 470, makecol(196, 255, 255), "PRESS 1P START BUTTON = EXIT BOOKKEEPING");
		//textprintf(rm.m_backbuf, font, 50, 470, makecol(196, 255, 255), "PRESS 2P START BUTTON = DATA CLEAR");
	}
	else if ( testMenuSubIndex == 3 )
	{
		textprintf(rm.m_backbuf, font, 50, 430, makecol(196, 255, 255), "PRESS 2P LEFT / RIGHT = CHANGE SELECTION");
		textprintf(rm.m_backbuf, font, 50, 450, makecol(196, 255, 255), "PRESS 1P START BUTTON = CONFIRM SELECTION");
	}
}

void renderSoundOptions()
{
	textprintf_centre(rm.m_backbuf, font, 320, 50, WHITE, "SOUND OPTIONS");

	textprintf(rm.m_backbuf, font, 50, 100, testMenuSubIndex == 0 ? RED : WHITE, "SOUND IN ATTRACT -- NOT IMPLEMENTED");
	textprintf(rm.m_backbuf, font, 50, 130, testMenuSubIndex == 1 ? RED : WHITE, "SCALE CHECK 1");
	textprintf(rm.m_backbuf, font, 50, 160, testMenuSubIndex == 2 ? RED : WHITE, "SCALE CHECK 2");
	textprintf(rm.m_backbuf, font, 50, 310, testMenuSubIndex == 3 ? RED : WHITE, "FACTORY SETTINGS");
	textprintf(rm.m_backbuf, font, 50, 340, testMenuSubIndex == 4 ? RED : WHITE, "SAVE AND EXIT");

	textprintf(rm.m_backbuf, font, 236, 100, GREEN, "NOT YET IMPLEMENTED");

	textprintf(rm.m_backbuf, font, 50, 400, makecol(196, 255, 255), "PRESS 1P LEFT / RIGHT = select item");
	textprintf(rm.m_backbuf, font, 50, 420, makecol(196, 255, 255), "PRESS 2P LEFT / RIGHT = modify setting");
	textprintf(rm.m_backbuf, font, 50, 440, makecol(196, 255, 255), "PRESS 1P START BUTTON = confirm selection");
}

void renderDataOptions()
{
	textprintf_centre(rm.m_backbuf, font, 320, 50, WHITE, "DATA OPTIONS");

	textprintf(rm.m_backbuf, font, 50, 100, testMenuSubIndex == 0 ? RED : WHITE, "ALLOW PLAYER LOGIN");
	textprintf(rm.m_backbuf, font, 236, 100, gs.allowLogins ? GREEN : RED, gs.allowLogins ? "ON" : "OFF" );
	textprintf(rm.m_backbuf, font, 50, 130, testMenuSubIndex == 1 ? RED : WHITE, "RESET PLAYERS HITS CHART");
	textprintf(rm.m_backbuf, font, 50, 160, testMenuSubIndex == 2 ? RED : WHITE, "RESET PREVIOUS LOGIN LIST");
	textprintf(rm.m_backbuf, font, 50, 190, testMenuSubIndex == 3 ? RED : WHITE, "DELETE PLAYER -- NOT IMPLEMENTED");
	textprintf(rm.m_backbuf, font, 50, 220, testMenuSubIndex == 4 ? RED : WHITE, "MODIFY PLAYER PIN -- NOT IMPLEMENTED");
	textprintf(rm.m_backbuf, font, 50, 260, testMenuSubIndex == 5 ? RED : WHITE, "UPDATE SOFTWARE");
	textprintf(rm.m_backbuf, font, 50, 340, testMenuSubIndex == 6 ? RED : WHITE, "EXIT");

	textprintf(rm.m_backbuf, font, 50, 400, makecol(196, 255, 255), "PRESS 1P LEFT / RIGHT = select item");
	textprintf(rm.m_backbuf, font, 50, 420, makecol(196, 255, 255), "PRESS 2P LEFT / RIGHT = modify setting");
	if ( testMenuSubIndex == 1 || testMenuSubIndex == 2 )
	{
		textprintf(rm.m_backbuf, font, 50, 440, makecol(196, 255, 255), "PRESS BOTH START BUTTONS = confirm selection");
	}
	else
	{
		textprintf(rm.m_backbuf, font, 50, 440, makecol(196, 255, 255), "PRESS 1P START BUTTON = confirm selection");
	}
}