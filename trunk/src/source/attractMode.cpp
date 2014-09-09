// attractMode.cpp implements the attract loop
// source file created by Allen Seitz 7/11/2012

#include "common.h"
#include "GameStateManager.h"
#include "inputManager.h"
#include "lightsManager.h"
#include "scoreManager.h"
#include "songwheelMode.h"
#include "videoManager.h"

extern GameStateManager gs;
extern RenderingManager rm;
extern InputManager im;
extern LightsManager lm;
extern ScoreManager sm;
extern VideoManager vm;
extern EffectsManager em;

extern unsigned long int frameCounter;
extern unsigned long int totalGameTime;
extern UTIME hideCreditsTimer;
extern SongEntry* songs;
extern BITMAP** m_banners;

extern int* songIDs;
extern std::string* songTitles;
extern std::string* songArtists;
extern std::string* movieScripts;

extern UTIME timeRemaining;
extern void renderTimeRemaining(int xc, int yc);

int attractState = 0;     // 0 = movie, 1 = title, 2 = how to play, 3 = hit chart, 4 = recent logins
int attractSubState = 0;  // for convenience, rather than checking elapsed time over and over
UTIME submodeTimer = 0;

int hitlist[20];
int numplays[20];
int newlist[10];

char versionString[128] = "";

BITMAP* m_title = NULL;
BITMAP* m_tile = NULL;
BITMAP* m_logo = NULL;
BITMAP* m_mask = NULL;
BITMAP* m_hitchart = NULL;

const int NUM_ATTRACT_SUBMODES = 5;

void advanceToNextMode()
{
	attractState = (attractState + 1) % NUM_ATTRACT_SUBMODES;
	attractSubState = submodeTimer = 0;
	hideCreditsTimer = 1000;

	// if nothing is new, then skip the newlist step
	if ( attractState == 2 && newlist[0] == -1 )
	{
		attractState = 3;
	}

	switch ( attractState )
	{
	case 0: // movie
		vm.loadScript("DATA/mov/A_TITLE2.seq");
		vm.play();
		lm.loadLampProgram("attract_0.txt");
		break;
	case 1: // title
		vm.stop();
		lm.loadLampProgram("attract_1.txt");
		break;
	case 2: // new songs
		lm.loadLampProgram("attract_2.txt");
		break;
	case 3: // how to
		vm.loadScript("DATA/mov/A_HOWTO.seq");
		vm.play();
		lm.loadLampProgram("attract_3.txt");
		break;
	case 4: // hitchart
		vm.stop();
		lm.loadLampProgram("attract_4.txt");
		break;
	}
}

void calculateTop20()
{
	int i = 0, j = 0;

	for ( i = 0; i < 20; i++ )
	{
		hitlist[i] = -1;
		numplays[i] = -1;
	}

	for ( int i = 0; i < NUM_SONGS; i++ )
	{
		if ( songs[i].version == 0 || songs[i].version == 100 ) // a song that is not in the game or a mission mode special
		{
			continue;
		}

		// check this song against each index of the hitlist
		for ( j = 19; j >= 0; j-- )
		{
			if ( songs[i].numPlays > numplays[j] )
			{
				if ( j != 19 )
				{
					numplays[j+1] = numplays[j]; // move it down the list
					hitlist[j+1] = hitlist[j];
				}
				numplays[j] = songs[i].numPlays;
				hitlist[j] = songs[i].songID;
			}
		}
	}
}

void calculateNewSongs()
{
	int i = 0, j = 0;

	for ( i = 0; i < 10; i++ )
	{
		newlist[i] = -1;
	}

	for ( int i = 0; i < NUM_SONGS; i++ )
	{
		if ( songs[i].isNew && songs[i].version != 0 && songs[i].version != 100 )
		{
			newlist[j] = songs[i].songID;
			j++;

			if ( j == 10 )
			{
				break; // that many new songs shouldn't happen
			}
		}
	}
}

void renderHitlistBackground()
{
	int scroll = (submodeTimer/20) % 96;
	for ( int x = 0; x < 9; x++ )
	for ( int y = 0; y < 7; y++ )
	{
		blit(m_tile, rm.m_backbuf, 0, 0, (-96+scroll) + x*96, (-96+scroll) + y*96, 96, 96);
	}
}

void firstAttractLoop()
{
	if ( m_title == NULL )
	{
		m_title = loadImage("DATA/attract/title.png");
		m_tile = loadImage("DATA/attract/tile.png");
		m_logo = loadImage("DATA/attract/logo.png");
		m_mask = loadImage("DATA/attract/mask.png");
		m_hitchart = loadImage("DATA/attract/hitchart.tga");

		strcpy_s(versionString, 128, __DATE__);
	}

	calculateTop20();
	calculateNewSongs();
	attractSubState = submodeTimer = 0;
	advanceToNextMode();
	gs.killSong(); // when time runs out on the main menu

	gs.leftPlayerPresent = gs.rightPlayerPresent = false;
}

void mainAttractLoop(UTIME dt)
{
	static int blinkTimer = 0;
	static int numBlinks = 0;
	if ( blinkTimer + dt >= 1000 )
	{
		numBlinks++;
	}
	blinkTimer = (blinkTimer + dt) % 1000;
	submodeTimer += dt;

	// skip directly to the songwheel for this special mode
	if ( gs.isFreestyleMode )
	{
		gs.g_currentGameMode = SONGWHEEL;
		gs.g_gameModeTransition = 1;
	}

	// begin a game from any sub-mode
	if ( gs.isFreeplay || gs.numCoins >= gs.numCoinsPerCredit )
	{
		if ( im.getKeyState(MENU_START_1P) == JUST_DOWN || im.getKeyState(MENU_START_2P) == JUST_DOWN )
		{
			if ( im.getKeyState(MENU_START_1P) == JUST_DOWN )
			{
				gs.leftPlayerPresent = true; // intentionally only set one of these here
			}
			else
			{
				gs.rightPlayerPresent = true; // intentionally only set one of these here
			}
			gs.g_currentGameMode = CAUTIONMODE;
			gs.g_gameModeTransition = 1;
			em.playSample(SFX_CREDIT_BEGIN);
			em.announcerQuip(GUY_GAME_BEGIN);
		}
	}

	// skip to the next submode?
	bool holdingLeft = (im.getHoldLength(MENU_LEFT_1P) > 0 && im.getHoldLength(MENU_LEFT_1P) < 1000) || (im.getHoldLength(MENU_LEFT_2P) > 0 && im.getHoldLength(MENU_LEFT_2P) < 1000);
	bool holdingRight = (im.getHoldLength(MENU_RIGHT_1P) > 0 && im.getHoldLength(MENU_RIGHT_1P) < 1000) || (im.getHoldLength(MENU_RIGHT_2P) > 0 && im.getHoldLength(MENU_RIGHT_2P) < 1000);
	if ( holdingLeft && holdingRight && submodeTimer > 2000 )
	{
		advanceToNextMode();
	}

	// render the attract loop
	rectfill(rm.m_backbuf, 0, 0, 640, 480, makeacol(0, 0, 0, 255));
	switch ( attractState )
	{
	case 0: // movie
		vm.renderToSurfaceStretched(rm.m_backbuf, 0, 0, 640, 460);
		if ( submodeTimer > 8000 && submodeTimer <= 11300 )
		{
			masked_blit(m_mask, rm.m_backbuf, 0, 0, 0, 0, 640, 480);
			rm.dimScreen(getValueFromRange(100, 0, (submodeTimer-8000)*100/3200));
		}
		if ( submodeTimer > 11300 )
		{
			advanceToNextMode();
		}
		break;
	case 1: // title
		blit(m_title, rm.m_backbuf, 0, 0, 0, 0, 640, 480);
		renderWhiteString(versionString, 10, 10);
		if ( submodeTimer > 5000 )
		{
			advanceToNextMode();
		}
		break;
	case 2: // new songs
		renderHitlistBackground();

		if ( attractSubState > 9 || newlist[attractSubState] == -1 )
		{
			advanceToNextMode();
		}
		else
		{
			int thisBannerTime = submodeTimer % 5000;
			int thisBannerState = thisBannerTime > 1000 ? 1 : 0;

			int bannerIndex = songID_to_listID(newlist[attractSubState]);
			bool isCourse = songs[bannerIndex].version == 101;
			if ( bannerIndex == -1 )
			{
				//songIndexError(newlist[attractSubState]);
				advanceToNextMode(); // do this instead, or else the machine will be stuck on the error screen (if it ever happens)
			}

			// zoom in
			if ( thisBannerState == 0 )
			{
				int width = getValueFromRange(512, 256, thisBannerTime*100/1000);
				stretch_blit(m_banners[bannerIndex], rm.m_backbuf, 0, 0, 256, 256, (SCREEN_WIDTH/2)-(width/2), (SCREEN_HEIGHT/2)-(width/2), width, width);
			}
			// land and text
			else
			{
				int x = (SCREEN_WIDTH/2)-128;
				int y = (SCREEN_HEIGHT/2)-128;

				// apply brightness flicker
				if ( (thisBannerTime / 250) % 2 == 0 )
				{
					set_luminance_blender(256, 256, 256, 256);
				}
				stretch_blit(m_banners[bannerIndex], rm.m_backbuf, 0, 0, 256, 256, x, y, 256, 256);
				set_alpha_blender(); // the game assumes the graphics are left in this mode

				int color = (submodeTimer/50) % 3 + 1;
				renderBoldString( isCourse ? "NEW NONSTOP COURSE!" : "NEW SONG!", x, y-48, 400, false, color);

				if ( thisBannerTime > 1125 )
				{
					renderBoldString(songTitles[bannerIndex].c_str(), x, y +256+32, 400, false, 0);
				}
				if ( thisBannerTime > 1375 )
				{
					renderArtistString(songArtists[bannerIndex].c_str(), x, y +256+64, 400, 32);
				}
			}

			// check for advance to next banner
			if ( submodeTimer >= (unsigned long)(5000*(attractSubState+1)) )
			{
				attractSubState++;
			}
		}

		break;
	case 3: // how to play
		vm.renderToSurfaceStretched(rm.m_backbuf, 0, 0, 640, 460);
		if ( submodeTimer > 17000 )
		{
			advanceToNextMode();
		}
		break;
	case 4: // hit chart
		renderHitlistBackground();
		masked_blit(m_hitchart, rm.m_backbuf, 0, 0, 34, 32, 573, 34);

		// render banners and stuff for each of the top 20
		for ( int i = 19; i >= 0; i-- )
		{
			int y = -3000 + submodeTimer/6;
			if ( y > 100 )
			{
				y = 100; // don't scroll down too far
			}
			y += 150*i;

			if ( hitlist[i] == -1 )
			{
				continue; // should not happen
			}

			int x = 80;
			if (y < 180 && submodeTimer < 18600 ) // slide in from the right
			{
				x = getValueFromRange(640, 80, (y*100/180));
			}
			if ( submodeTimer >= 18600 ) // the last item needs a special case
			{
				int tempy = (-3000 + submodeTimer/6) + 150*i;
				x = getValueFromRange(640, 80, (tempy*100/180));
				x = MAX(80, x);
			}

			int bannerIndex = songID_to_listID(hitlist[i]);
			if ( bannerIndex == -1 )
			{
				//songIndexError(hitlist[i]);
				advanceToNextMode(); // do this instead, or else the machine will be stuck on the error screen (if it ever happens)
			}
			stretch_blit(m_banners[bannerIndex], rm.m_backbuf, 0, 0, 256, 256, x, y + 11, 128, 128);
			int color = i % 2;
			if ( i == 0 )
			{
				color = (submodeTimer/50) % 3 + 1;
			}
			renderBoldString(songTitles[bannerIndex].c_str(), x + 140, y+11, 400, false, color);
			renderArtistString(songArtists[bannerIndex].c_str(), x + 140, y + 51, 400, 32);
			renderScoreNumber(i+1, x-55, y+11, 2);
#ifdef DMXDEBUG
			renderWhiteNumber(numplays[i], x-55, y+64);
#endif
		}

		if ( submodeTimer > 28000 )
		{
			advanceToNextMode();
		}
		break;
	}

	if ( (gs.isFreeplay || gs.numCoins >= gs.numCoinsPerCredit) && blinkTimer < 500 )
	{
		renderBoldString("PUSH START!", 262, (numBlinks/400)%2 == 1 ? 325 : 275, 400, false);
	}
}