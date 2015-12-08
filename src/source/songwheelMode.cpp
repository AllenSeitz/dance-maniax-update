// songwheelMode.cpp implements the old "2nd Mix" style songwheel
// source file created by Allen Seitz 7/18/2009

#include "common.h"
#include "GameStateManager.h"
#include "inputManager.h"
#include "lightsManager.h"
#include "ScoreManager.h"
#include "songwheelMode.h"

#include "gameplayRendering.h" // for renderGrade()

extern GameStateManager gs;
extern RenderingManager rm;
extern InputManager im;
extern LightsManager lm;
extern ScoreManager sm;
extern EffectsManager em;

extern SongEntry* songs;
SongEntry* songlist = NULL;

extern std::string* songTitles;
extern std::string* songArtists;

extern unsigned long int frameCounter;
extern unsigned long int totalGameTime;
extern volatile UTIME last_utime;
extern bool allsongsDebug;


//////////////////////////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////////////////////////
#define WHEEL_STEP_TIME 180

#define NUM_BANNERS 12
int shadowQuads[NUM_BANNERS][8] = 
{
	{ 138,58,  195,45,  171,68,  112,81  },	// top
	{ 80,107,  120,91,  97,141,  57,152  },	
	{ 133,96,  216,91,  207,148, 120,150 },
	{ 45,174,  61,167,  55,227,  39,228  },
	{ 72,173,  137,167, 135,237, 69,237  },
	{ 163,178, 259,178, 259,250, 163,250 }, // current
	{ 46,250,  84,252,  100,314, 62,305  },
	{ 111,269, 198,271, 206,333, 124,330 },
	{ 233,280, 323,277, 316,334, 231,340 },
	{ 85,332,  140,344, 166,382, 115,369 },
	{ 180,354, 267,354, 265,387, 187,388 },
	{ 307,352, 361,339, 342,369, 287,386 },
};

// these can be calculated by firstSongwheelLoop()
int bannerQuads[NUM_BANNERS][8] = 
{
	{ 129,51,  186,38,  162,61,  103,74  }, // top
	{ 70,99,   110,83,  87,133,  47,144  }, // 1
	{ 126,87,  209,82,  200,139, 113,141 }, // 2
	{ 35,174,  51,167,  45,227,  29,228  }, // 3
	{ 65,167,  130,161, 128,231, 62,231  }, // 4
	{ 160,171, 256,171, 256,243, 160,243 }, // current
	{ 35,243,  73,245,  89,307,  51,298  }, // 6
	{ 101,261, 188,263, 196,325, 114,322 }, // 7
	{ 226,272, 316,269, 309,326, 224,332 }, // 8
	{ 79,338,  134,350, 160,388, 109,375 }, // 9
	{ 181,361, 268,361, 266,394, 188,395 }, // 10
	{ 313,358, 367,345, 348,375, 293,392 }  // 11
};

// when rotating right, where does each banner go?
int targetQuadsCCW[NUM_BANNERS][8] = 
{
	{ 126,87,  209,82,  200,139, 113,141 }, // 2
	{ 65,167,  130,161, 128,231, 62,231  }, // 4
	{ 160,171, 256,171, 256,243, 160,243 }, // current
	{ 35,243,  73,245,  89,307,  51,298  }, // 6
	{ 101,261, 188,263, 196,325, 114,322 }, // 7
	{ 226,272, 316,269, 309,326, 224,332 }, // 8
	{ 79,338,  134,350, 160,388, 109,375 }, // 9
	{ 181,361, 268,361, 266,394, 188,395 }, // 10
	{ 313,358, 367,345, 348,375, 293,392 }, // 11

	// the bottom three fall off
	{ 109,375, 109,375, 160,388, 160,388 }, // below 9
	{ 188,395, 188,395, 266,394, 266,394 }, // below 10
	{ 293,392, 293,392, 348,375, 348,375 }  // below 11
};

int targetQuadsCW[NUM_BANNERS][8] = 
{
	{ 129,51,  129,51,  186,38,  186,38  }, // spins up and away (old top)
	{ 70,99,   70,99,   110,83,  110,83  }, // spins up and away (old 1)
	{ 129,51,  186,38,  162,61,  103,74  }, // top
	{ 35,174,  35,174,  51,167,  51,167  }, // spins up and away (old 3)
	{ 70,99,   110,83,  87,133,  47,144  }, // 1
	{ 126,87,  209,82,  200,139, 113,141 }, // 2
	{ 35,174,  51,167,  45,227,  29,228  }, // 3
	{ 65,167,  130,161, 128,231, 62,231  }, // 4
	{ 160,171, 256,171, 256,243, 160,243 }, // current
	{ 35,243,  73,245,  89,307,  51,298  }, // 6
	{ 101,261, 188,263, 196,325, 114,322 }, // 7
	{ 226,272, 316,269, 309,326, 224,332 }, // 8
};

#define INTRO_ANIM_LENGTH 2000
#define INTRO_ANIM_STEP_TIME 500
#define TITLE_ANIM_LENGTH 1000

//////////////////////////////////////////////////////////////////////////////
// Variables
//////////////////////////////////////////////////////////////////////////////
bool isSphereMoving = false;
bool isMovementClockwise = false;
int  rotateAnimTime = 0;
int  currentQuads[NUM_BANNERS][8];  // where the banners currently are
int  targetQuads[NUM_BANNERS][8];   // where each banner wants to be
int  songwheelIndex = 0;
int  maxSongwheelIndex = 0;
UTIME timeRemaining = 60000;
int  introAnimTimer = 0;
int  introAnimSteps = 5;
UTIME nextStageAnimTimer = 0;
#define NEXT_STAGE_ANIM_TIME 2000

int  titleAnimTimer = 0;
int  displayBPM = 150;              // oscillates between min and max
int  displayBPMTimer = 0;
char displayBPMState = 0;

UTIME previewTimeStarted = 0;
UTIME previewTimeRemaining = 0;
int previewSongID = 0;

bool isInSubmenu = false;
char currentSubmenu = 0;            // only used for one player
char separateSubmenu[2] = {0,0};    // only used for versus mode
bool submenuDone[2] = {0,0};		// only used for versus mode
bool skippingSubmenu = false;
char maniaxSelect[2] = {0,0};		// used to access the hidden Maniax difficulty

bool isRandomSelect = false;        // set to true when random select begins


//////////////////////////////////////////////////////////////////////////////
// Assets
//////////////////////////////////////////////////////////////////////////////
BITMAP* m_songBG = NULL;
BITMAP* m_titleArea;
BITMAP* m_stars[2][2];
BITMAP* m_lockedIcon;
extern BITMAP* m_time;
extern BITMAP* m_timeDigits[2];
BITMAP* m_menu[3];
BITMAP* m_musicSelect;
//BITMAP* m_miniStatus;
BITMAP* m_statusStars;
BITMAP* m_new;
BITMAP* m_versions;

extern BITMAP** m_banners;
V3D* pp[4];            // allocate once, use repeatedly for renderBanner()


//////////////////////////////////////////////////////////////////////////////
// Functions
//////////////////////////////////////////////////////////////////////////////
void playTimeLowSFX(UTIME dt);
//
//

void updatePreview(UTIME dt);
//
//

void loadPreview(int songid);
// precondition: songid is [100..999]
// postcondition: changes currentPreview, should only be called from updatePreview()

void renderNextStageAnim();
//
//

void renderNextStageAnimBanner(int textureID, int percent);
//
//

bool isSongAvailable(int index)
{
	bool earnedIt = true;

	if ( songs[index].unlockFlag != UNLOCK_METHOD_NONE )
	{
		earnedIt = false;

		if ( gs.isDoubles ) // if any of the 3 charts are unlocked, then it goes on the songwheel
		{
			for ( int i = 3; i < 6; i++ )
			{
				earnedIt = earnedIt || sm.player[0].allTime[index][i].unlockStatus > 0;
			}
		}
		else
		{
			for ( int i = SINGLE_MILD; i <= SINGLE_ANOTHER; i++ ) // if either player has any of the three charts, then it goes on the songwheel
			{
				earnedIt = earnedIt || sm.player[0].allTime[index][i].unlockStatus > 0 || sm.player[1].allTime[index][i].unlockStatus > 0;
			}
		}
	}

	if ( (gs.isDoubles && songs[index].mildDouble == 0 && songs[index].wildDouble == 0) || (!gs.isDoubles && songs[index].mildSingle == 0 && songs[index].wildSingle == 0) )
	{
		return false;
	}

	return songs[index].version != 0 && songs[index].version < 100 && (earnedIt || allsongsDebug);
}

bool isManiaxChartAvailableHere()
{
	if ( gs.isDoubles )
	{
		return songlist[songwheelIndex].maniaxDouble != 0;
	}
	return songlist[songwheelIndex].maniaxSingle != 0;
}

void killPreviewClip()
{
	gs.killSongIfPreview();
	//stop_sample(currentPreview);
}

void firstSongwheelLoop()
{
	// load assets
	if ( m_songBG == NULL )
	{
		m_songBG = loadImage("DATA/songwheel/songwheel_background.tga");
		m_titleArea = loadImage("DATA/songwheel/title_area.tga");

		m_stars[0][0] = loadImage("DATA/songwheel/green_star.tga");
		m_stars[0][1] = loadImage("DATA/songwheel/blank_green_star.tga");
		m_stars[1][0] = loadImage("DATA/songwheel/red_star.tga");
		m_stars[1][1] = loadImage("DATA/songwheel/blank_red_star.tga");
		m_lockedIcon = loadImage("DATA/songwheel/locked_icon.tga");

		m_menu[0] = loadImage("DATA/songwheel/menu_mild.tga");
		m_menu[1] = loadImage("DATA/songwheel/menu_wild.tga");
		m_menu[2] = loadImage("DATA/songwheel/menu_cancel.tga");
		m_musicSelect = loadImage("DATA/songwheel/music_select.tga");
		//m_miniStatus = loadImage("DATA/songwheel/mini_status.tga");
		m_statusStars = loadImage("DATA/songwheel/status_stars.tga");
		m_new = loadImage("DATA/songwheel/new.tga");
		m_versions = loadImage("DATA/songwheel/versions.tga");
	}

	// reset state
	isSphereMoving = false;
	rotateAnimTime = 0;
	timeRemaining = 60999;
	titleAnimTimer = 0;
	isInSubmenu = false;
	separateSubmenu[0] = separateSubmenu[1] = currentSubmenu = 0;
	maniaxSelect[0] = maniaxSelect[1] = 0;
	introAnimTimer = INTRO_ANIM_LENGTH;
	introAnimSteps = 3;
	isRandomSelect = false;
	nextStageAnimTimer = 0;

	// figure out how many songs are visible on the songwheel
	maxSongwheelIndex = 0;
	for ( int i = 0; i < NUM_SONGS; i++ )
	{
		if ( isSongAvailable(i) )
		{
			maxSongwheelIndex++;
		}
	}

	// put only the songs which should be on t1he songwheel, on the songwheel
	if ( songlist != NULL )
	{
		delete songlist;
	}
	songlist = new SongEntry[maxSongwheelIndex];
	int nextIndex = 0;
	int futureStartIndex = maxSongwheelIndex; // would start on "Broken my Heart" (or whatever is first) if it doesn't find "Wuv U" (301)
	int favoriteMusicHack = 0; // used later TODO: dont do this
	for ( int i = 0; i < NUM_SONGS; i++ )
	{
		if ( isSongAvailable(i) )
		{
			songlist[nextIndex] = songs[i];

			// certain charts may be locked
			if ( songs[i].unlockFlag != UNLOCK_METHOD_NONE && !allsongsDebug )
			{
				if ( sm.player[0].allTime[i][3].unlockStatus == 0 )
				{
					songlist[nextIndex].mildDouble = 0;
				}
				if ( sm.player[0].allTime[i][4].unlockStatus == 0 )
				{
					songlist[nextIndex].wildDouble = 0;
				}
				if ( sm.player[0].allTime[i][SINGLE_MILD].unlockStatus == 0 && sm.player[1].allTime[i][SINGLE_MILD].unlockStatus == 0 )
				{
					songlist[nextIndex].mildSingle = 0;
				}
				if ( sm.player[0].allTime[i][SINGLE_WILD].unlockStatus == 0 && sm.player[1].allTime[i][SINGLE_WILD].unlockStatus == 0 )
				{
					songlist[nextIndex].wildSingle = 0;
				}
			}

			if ( songlist[nextIndex].version == 5 && futureStartIndex == maxSongwheelIndex )
			{
				futureStartIndex = nextIndex; // start at the first custom song
			}

			// TODO: dont do this
			if ( songlist[nextIndex].songID == 310 )
			{
				favoriteMusicHack = nextIndex;
			}

			nextIndex++;
		}
	}
	ASSERT(nextIndex == maxSongwheelIndex);

	// HACK: swap "Wuv U" and "Caramelldansen (speedycake remix)"
	// TODO: dont do this
	if ( favoriteMusicHack > 0 )
	{
		SongEntry wuvu = songlist[futureStartIndex];
		songlist[futureStartIndex] = songlist[favoriteMusicHack];
		songlist[favoriteMusicHack] = wuvu;
	}

	// this is for the intro anim
	songwheelIndex = futureStartIndex-3;
	isMovementClockwise = true;
	prepareForSongwheelRotation(); // must do this or call postSongwheelRotation()

	for (int c = 0; c < 4; c++)
	{
		pp[c] = new V3D();
	}

	/* dynamically calculate the banner quads based on their upper-left corner (easier for me)
	for ( int i = 0; i < NUM_BANNERS; i++ )
	{
		int xdiff = bannerQuads[i][0] - shadowQuads[i][0];
		int ydiff = bannerQuads[i][1] - shadowQuads[i][1];

		for ( int j = 2; j < 8; j+=2 )
		{
			bannerQuads[i][j] = shadowQuads[i][j] + xdiff;
			bannerQuads[i][j+1] = shadowQuads[i][j+1] + ydiff;
		}
		al_trace("{ %d,%d %d,%d %d,%d %d,%d },\n", bannerQuads[i][0], bannerQuads[i][1], bannerQuads[i][2], bannerQuads[i][3], bannerQuads[i][4], bannerQuads[i][5], bannerQuads[i][6], bannerQuads[i][7]);
	}
	//*/

	//currentPreview = NULL;
	previewTimeStarted = last_utime;
	previewTimeRemaining = 0;
	previewSongID = 0;

	gs.player[0].resetStages();
	gs.player[1].resetStages();
	gs.currentStage = 0;
	gs.killSong();
	em.playSample(SFX_SONGWHEEL_APPEAR);

	if ( gs.isFreestyleMode )
	{
		gs.player[0].resetAll();
		gs.currentStage = 0;
		gs.isVersus = false;
		gs.isDoubles = false;
	}

	lm.loadLampProgram("songwheel.txt");
	im.setCooldownTime(0);
}

void mainSongwheelLoop(UTIME dt)
{
	if ( introAnimTimer > 0 )
	{
		introAnimTimer -= dt;
		if ( isSphereMoving )
		{
			updateSongwheelRotation(dt);

			if ( !isSphereMoving )
			{
				introAnimSteps--;
				if ( introAnimSteps > 0 )
				{
					prepareForSongwheelRotation();
				}
			}
		}
		renderIntroAnim( (INTRO_ANIM_LENGTH-introAnimTimer) * 100 / INTRO_ANIM_LENGTH );
		return;
	}

	SUBTRACT_TO_ZERO(nextStageAnimTimer, dt);

	if ( gs.isFreestyleMode && im.getKeyState(MENU_START_2P) == JUST_DOWN )
	{
		gs.isDoubles = !gs.isDoubles; // another special case for this mode
	}
	else if ( isSphereMoving )
	{
		updateSongwheelRotation(dt);
		titleAnimTimer = 0;

		// holding the button down means continous motion - not a stop (and flicker) on each banner
		if ( !isSphereMoving && !isRandomSelect )
		{
			if ( (im.isKeyDown(MENU_LEFT_1P) || im.isKeyDown(MENU_LEFT_2P)) && !(im.isKeyDown(MENU_RIGHT_1P) || im.isKeyDown(MENU_RIGHT_2P)) )
			{
				isMovementClockwise = false;
				prepareForSongwheelRotation();
				em.playSample(SFX_SONGWHEEL_MOVE);
			}
			else if ( (im.isKeyDown(MENU_RIGHT_1P) || im.isKeyDown(MENU_RIGHT_2P)) && !(im.isKeyDown(MENU_LEFT_1P) || im.isKeyDown(MENU_LEFT_2P)) )
			{
				isMovementClockwise = true;
				prepareForSongwheelRotation();
				em.playSample(SFX_SONGWHEEL_MOVE);
			}
		}
	}
	else if ( isInSubmenu )
	{
		isRandomSelect = false;

		// arrow keys work differently per player
		if ( gs.isVersus )
		{
			if ( im.getKeyState(MENU_LEFT_1P) == JUST_DOWN && !submenuDone[0] )
			{
				separateSubmenu[0] = separateSubmenu[0] == 0 ? 0 : separateSubmenu[0] - 1;
				em.playSample(SFX_DIFFICULTY_MOVE);
				maniaxSelect[0] = 0;
			}
			else if ( im.getKeyState(MENU_RIGHT_1P) == JUST_DOWN && !submenuDone[0] )
			{
				separateSubmenu[0] = separateSubmenu[0] >= 2 ? separateSubmenu[0] : separateSubmenu[0] + 1;
				em.playSample(SFX_DIFFICULTY_MOVE);
				maniaxSelect[0] = separateSubmenu[0] == 2 ? maniaxSelect[0]+1 : 0;

				if ( maniaxSelect[0] >= 11 )
				{
					if ( isManiaxChartAvailableHere() )
					{
						separateSubmenu[0] = 1;
						submenuDone[0] = true;
						gs.player[0].stagesPlayed[gs.currentStage] = gs.player[1].stagesPlayed[gs.currentStage] = songlist[songwheelIndex].songID;
						gs.player[0].stagesLevels[gs.currentStage] = SINGLE_ANOTHER;
						if ( submenuDone[0] && submenuDone[1] )
						{
							skippingSubmenu = true;
						}
						em.playSample(SFX_MENU_PICK);
						em.playSample(GUY_MANIAX);
					}
					else
					{
						maniaxSelect[0] = 0;
						em.playSample(SFX_MENU_STUCK);
					}
				}
			}

			if ( im.getKeyState(MENU_LEFT_2P) == JUST_DOWN && !submenuDone[1] )
			{
				separateSubmenu[1] = separateSubmenu[1] == 0 ? 0 : separateSubmenu[1] - 1;
				em.playSample(SFX_DIFFICULTY_MOVE);
				maniaxSelect[1] = 0;
			}
			else if ( im.getKeyState(MENU_RIGHT_2P) == JUST_DOWN && !submenuDone[1] )
			{
				separateSubmenu[1] = separateSubmenu[1] >= 2 ? separateSubmenu[1] : separateSubmenu[1] + 1;
				em.playSample(SFX_DIFFICULTY_MOVE);
				maniaxSelect[1] = separateSubmenu[1] == 2 ? maniaxSelect[1]+1 : 0;

				if ( maniaxSelect[1] >= 11 )
				{
					if ( isManiaxChartAvailableHere() )
					{
						separateSubmenu[1] = 1;
						submenuDone[1] = true;
						gs.player[0].stagesPlayed[gs.currentStage] = gs.player[1].stagesPlayed[gs.currentStage] = songlist[songwheelIndex].songID;
						gs.player[1].stagesLevels[gs.currentStage] = SINGLE_ANOTHER;
						if ( submenuDone[0] && submenuDone[1] )
						{
							skippingSubmenu = true;
						}
						em.announcerQuip(GUY_MANIAX);
						em.playSample(SFX_MENU_PICK);
					}
					else
					{
						maniaxSelect[1] = 0;
						em.playSample(SFX_MENU_STUCK);
					}
				}
			}
		}
		else
		{
			if ( im.getKeyState(MENU_LEFT_1P) == JUST_DOWN || im.getKeyState(MENU_LEFT_2P) == JUST_DOWN )
			{
				currentSubmenu = currentSubmenu == 0 ? 0 : currentSubmenu - 1;
				em.playSample(SFX_DIFFICULTY_MOVE);
				maniaxSelect[0] = 0;
			}
			else if ( im.getKeyState(MENU_RIGHT_1P) == JUST_DOWN || im.getKeyState(MENU_RIGHT_2P) == JUST_DOWN )
			{
				currentSubmenu = currentSubmenu >= 2 ? currentSubmenu : currentSubmenu + 1;
				em.playSample(SFX_DIFFICULTY_MOVE);
				maniaxSelect[0] = currentSubmenu == 2 ? maniaxSelect[0]+1 : 0;

				if ( maniaxSelect[0] >= 11 )
				{
					if ( isManiaxChartAvailableHere() )
					{
						currentSubmenu = 1;
						submenuDone[0] = true;
						skippingSubmenu = true;
						gs.player[0].stagesPlayed[gs.currentStage] = songlist[songwheelIndex].songID;
						gs.player[0].stagesLevels[gs.currentStage] = gs.isDoubles ? DOUBLE_ANOTHER : SINGLE_ANOTHER;
						maniaxSelect[0] = maniaxSelect[1] = 0;
						em.announcerQuip(GUY_MANIAX);
						em.playSample(SFX_MENU_PICK);
					}
					else
					{
						maniaxSelect[0] = 0;
						em.playSample(SFX_MENU_STUCK);
					}
				}
			}
		}

		// the start buttons pretty much work the same regardless of versus mode
		if ( im.getKeyState(MENU_START_1P) == JUST_DOWN || im.getKeyState(MENU_START_2P) == JUST_DOWN || skippingSubmenu )
		{
			if ( !skippingSubmenu )
			{
				if ( gs.isVersus )
				{
					if ( im.getKeyState(MENU_START_1P) == JUST_DOWN && !submenuDone[0] )
					{
						if ( separateSubmenu[0] < 2 )
						{
							submenuDone[0] = true;
							gs.player[0].stagesPlayed[gs.currentStage] = songlist[songwheelIndex].songID;
							gs.player[0].stagesLevels[gs.currentStage] = separateSubmenu[0] == 3 ? 2 : separateSubmenu[0];
							em.playSample(submenuDone[1] ? SFX_NONE : SFX_START_BUTTON);
						}
						else
						{
							isInSubmenu = false; // partner cancelled
							em.playSample(SFX_MENU_SORT);
							separateSubmenu[0] = separateSubmenu[0] == 3 ? 1 : separateSubmenu[0];
							separateSubmenu[1] = separateSubmenu[1] == 3 ? 1 : separateSubmenu[1];
						}
					}
					if ( im.getKeyState(MENU_START_2P) == JUST_DOWN && !submenuDone[1] )
					{
						if ( separateSubmenu[1] < 2 )
						{
							submenuDone[1] = true;
							gs.player[1].stagesPlayed[gs.currentStage] = songlist[songwheelIndex].songID;
							gs.player[1].stagesLevels[gs.currentStage] = separateSubmenu[1] == 3 ? 2 : separateSubmenu[1];
							em.playSample(submenuDone[0] ? SFX_NONE : SFX_START_BUTTON);
						}
						else
						{
							isInSubmenu = false; // partner cancelled
							em.playSample(SFX_MENU_SORT);
							separateSubmenu[0] = separateSubmenu[0] == 3 ? 1 : separateSubmenu[0];
							separateSubmenu[1] = separateSubmenu[1] == 3 ? 1 : separateSubmenu[1];
						}
					}
				}
				else
				{
					if ( currentSubmenu < 2 )
					{
						gs.player[0].stagesPlayed[gs.currentStage] = gs.player[1].stagesPlayed[gs.currentStage] = songlist[songwheelIndex].songID;
						gs.player[0].stagesLevels[gs.currentStage] = gs.player[1].stagesLevels[gs.currentStage] = currentSubmenu;
						if ( gs.isDoubles )
						{
							gs.player[0].stagesLevels[gs.currentStage] += DOUBLE_MILD;
						}
					}
					else
					{
						isInSubmenu = false;
						em.playSample(SFX_MENU_SORT);
					}
				}
			}

			// done with the submenu? confirm a song?
			if ( (isInSubmenu && submenuDone[0] && submenuDone[1]) || skippingSubmenu )
			{
				isInSubmenu = skippingSubmenu = false;
				gs.currentStage++;
				int realIndex = songID_to_listID(songlist[songwheelIndex].songID);
				songs[realIndex].numPlays++;

				// fix maniax select for versus play
				separateSubmenu[0] = separateSubmenu[0] == 3 ? 1 : separateSubmenu[0];
				separateSubmenu[1] = separateSubmenu[1] == 3 ? 1 : separateSubmenu[1];
				maniaxSelect[0] = maniaxSelect[1] = 0;

				if ( timeRemaining < 10000 )
				{
					timeRemaining = 10000;
				}

				if ( gs.currentStage >= gs.numSongsPerSet || gs.isFreestyleMode )
				{
					gs.currentStage = 0;
					gs.g_currentGameMode = GAMEPLAY;
					gs.g_gameModeTransition = 1;
					//stop_sample(currentPreview);
					killPreviewClip();
					em.playSample(SFX_FORCEFUL_SELECTION);
				}
				else
				{
					em.playSample(SFX_SONGWHEEL_PICK);
					nextStageAnimTimer = NEXT_STAGE_ANIM_TIME;
				}
			}
		}
	}
	else
	{
		if ( (im.isKeyDown(MENU_LEFT_1P) || im.isKeyDown(MENU_LEFT_2P)) && !isRandomSelect && !(im.isKeyDown(MENU_RIGHT_1P) || im.isKeyDown(MENU_RIGHT_2P)) )
		{
			isMovementClockwise = false;
			prepareForSongwheelRotation();
			em.playSample(SFX_SONGWHEEL_MOVE);
		}
		else if ( (im.isKeyDown(MENU_RIGHT_1P) || im.isKeyDown(MENU_RIGHT_2P)) && !isRandomSelect && !(im.isKeyDown(MENU_LEFT_1P) || im.isKeyDown(MENU_LEFT_2P)) )
		{
			isMovementClockwise = true;
			prepareForSongwheelRotation();
			em.playSample(SFX_SONGWHEEL_MOVE);
		}
		else if ( im.getKeyState(MENU_START_1P) == JUST_DOWN || im.getKeyState(MENU_START_2P) == JUST_DOWN )
		{
			isInSubmenu = true;
			submenuDone[0] = submenuDone[1] = !gs.isVersus;
			em.playSample(SFX_SONGWHEEL_PICK);
			isRandomSelect = false;

			// check for songs with a locked chart and skip the difficulty selection submenu
			if ( gs.isDoubles && songlist[songwheelIndex].wildDouble == 0 )
			{
				isInSubmenu = skippingSubmenu = true;
				gs.player[0].stagesPlayed[gs.currentStage] = gs.player[1].stagesPlayed[gs.currentStage] = songlist[songwheelIndex].songID;
				gs.player[0].stagesLevels[gs.currentStage] = gs.player[1].stagesLevels[gs.currentStage] = DOUBLE_MILD;
			}
			if ( gs.isDoubles && songlist[songwheelIndex].mildDouble == 0 )
			{
				isInSubmenu = skippingSubmenu = true;
				gs.player[0].stagesPlayed[gs.currentStage] = gs.player[1].stagesPlayed[gs.currentStage] = songlist[songwheelIndex].songID;
				gs.player[0].stagesLevels[gs.currentStage] = gs.player[1].stagesLevels[gs.currentStage] = DOUBLE_WILD;
			}
			if ( !gs.isDoubles && songlist[songwheelIndex].wildSingle == 0 )
			{
				isInSubmenu = skippingSubmenu = true;
				gs.player[0].stagesPlayed[gs.currentStage] = gs.player[1].stagesPlayed[gs.currentStage] = songlist[songwheelIndex].songID;
				gs.player[0].stagesLevels[gs.currentStage] = gs.player[1].stagesLevels[gs.currentStage] = SINGLE_MILD;
			}
			if ( !gs.isDoubles && songlist[songwheelIndex].mildSingle == 0 )
			{
				isInSubmenu = skippingSubmenu = true;
				gs.player[0].stagesPlayed[gs.currentStage] = gs.player[1].stagesPlayed[gs.currentStage] = songlist[songwheelIndex].songID;
				gs.player[0].stagesLevels[gs.currentStage] = gs.player[1].stagesLevels[gs.currentStage] = SINGLE_WILD;
			}
		}

		// check for random select
		if ( !isRandomSelect && (im.getHoldLength(MENU_LEFT_1P) > 3000 || im.getHoldLength(MENU_LEFT_2P) > 3000) && (im.getHoldLength(MENU_RIGHT_1P) > 3000 || im.getHoldLength(MENU_RIGHT_2P) > 3000) )
		{
			isRandomSelect = true;
		}
	}

	// update the display BPM
	displayBPMTimer += dt;
	switch (displayBPMState)
	{
	case 1:
		displayBPM = getValueFromRange(songlist[songwheelIndex].minBPM, songlist[songwheelIndex].maxBPM, displayBPMTimer >= 1000 ? 100 : displayBPMTimer/10);
		if ( displayBPMTimer >= 2000 )
		{
			displayBPMTimer -= 2000;
			displayBPMState = -1; // going down to the min bpm
		}
		break;
	case -1:
		displayBPM = getValueFromRange(songlist[songwheelIndex].maxBPM, songlist[songwheelIndex].minBPM, displayBPMTimer >= 1000 ? 100 : displayBPMTimer/10);
		if ( displayBPMTimer >= 2000 )
		{
			displayBPMTimer -= 2000;
			displayBPMState = 1; // going down to the min bpm
		}
		break;
	default:
		if ( displayBPMTimer >= 1000 )
		{
			displayBPMTimer -= 1000;
			displayBPMState = 1; // going up to the maximum
		}
		break;
	}

	// update random select
	if ( isRandomSelect )
	{
		songwheelIndex = songwheelIndex + 1 >= maxSongwheelIndex ? 0 : songwheelIndex + 1;
	}

	// song preview related
	if ( !isSphereMoving )
	{
		updatePreview(dt);
	}

	// update the time remaining
	if ( !isSphereMoving )
	{
		if ( !gs.isEventMode && !gs.isFreestyleMode )
		{
			SUBTRACT_TO_ZERO(timeRemaining, dt);
		}
		if ( titleAnimTimer < TITLE_ANIM_LENGTH )
		{
			titleAnimTimer = MIN(titleAnimTimer + dt, TITLE_ANIM_LENGTH);
		}
		playTimeLowSFX(dt);
	}
	if ( timeRemaining <= 0 ) // ran out of time - pick randomly
	{
		while ( gs.currentStage < gs.numSongsPerSet )
		{
			int randIndex = rand()%maxSongwheelIndex;
			gs.player[0].stagesPlayed[gs.currentStage] = gs.player[1].stagesPlayed[gs.currentStage] = songlist[randIndex].songID;
			if ( gs.isDoubles )
			{
				gs.player[0].stagesLevels[gs.currentStage] = gs.currentStage == 0 ? DOUBLE_MILD : DOUBLE_WILD;
				if ( songlist[randIndex].mildDouble == 0 )
				{
					gs.player[0].stagesLevels[gs.currentStage] = DOUBLE_WILD;
				}
				if ( songlist[randIndex].wildDouble == 0 )
				{
					gs.player[0].stagesLevels[gs.currentStage] = DOUBLE_MILD;
				}
			}
			else
			{
				gs.player[0].stagesLevels[gs.currentStage] = gs.player[1].stagesLevels[gs.currentStage] = gs.currentStage == 0 ? SINGLE_MILD : SINGLE_WILD;
				if ( songlist[randIndex].mildSingle == 0 )
				{
					gs.player[0].stagesLevels[gs.currentStage] = SINGLE_WILD;
					gs.player[1].stagesLevels[gs.currentStage] = SINGLE_WILD;
				}
				if ( songlist[randIndex].wildSingle == 0 )
				{
					gs.player[0].stagesLevels[gs.currentStage] = SINGLE_MILD;
					gs.player[1].stagesLevels[gs.currentStage] = SINGLE_MILD;
				}
			}
			gs.currentStage++;
		}
		gs.currentStage = 0;
		gs.g_currentGameMode = GAMEPLAY;
		gs.g_gameModeTransition = 1;
		killPreviewClip();
		//stop_sample(currentPreview);
	}

	// light the menu buttons for whoever is logged in (although either works)
	bool use1P = gs.isDoubles || gs.isVersus || gs.leftPlayerPresent;
	bool use2P = gs.isDoubles || gs.isVersus || gs.rightPlayerPresent;
	lm.setLamp(lampStart, use1P ? 100 : 0);
	lm.setLamp(lampLeft, use1P ? 100 : 0);
	lm.setLamp(lampRight, use1P ? 100 : 0);
	lm.setLamp(lampStart+1, use2P ? 100 : 0);
	lm.setLamp(lampLeft+1, use2P ? 100 : 0);
	lm.setLamp(lampRight+1, use2P ? 100 : 0);

	if ( gs.g_currentGameMode == SONGWHEEL ) // fixes a bug with calling this function after transitioning away
	{
		renderSongwheelLoop();
	}
}

void renderSongwheelLoop()
{
	blit(m_songBG, rm.m_backbuf, 0, 0, 0, 0, 640, 480);
	draw_trans_sprite(rm.m_backbuf, m_musicSelect, 260, 135); 

	renderPickedSongs(100);

	// render the shadows under the banners
	drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
	for ( int i = 0; i < NUM_BANNERS; i++ )
	{
		polygon(rm.m_backbuf, 4, shadowQuads[i], makeacol(0, 0, 0, 95));
	}
	set_alpha_blender(); // the game assumes the graphics are left in this mode
	
	// which banner goes where on the wheel?
	static char bannerOffsetIndex[NUM_BANNERS] = { -2, -6, -1, -9, -5, 0, -8, -4, 1, -7, -3, 2};

	for ( int i = 0; i < NUM_BANNERS; i++ )
	{
		if ( i != 5 || isSphereMoving )
		{
			int bannerIndex = songwheelIndex + bannerOffsetIndex[i];
			if ( bannerIndex < 0 )
			{
				bannerIndex += maxSongwheelIndex;
			}
			if ( bannerIndex > maxSongwheelIndex-1 )
			{
				bannerIndex -= maxSongwheelIndex;
			}
			bannerIndex = songID_to_listID(songlist[bannerIndex].songID);
			renderBanner(bannerIndex, i, songs[bannerIndex].isNew);
		}
	}

	// render the current banner last
	if ( !isSphereMoving )
	{
		renderTitleArea(titleAnimTimer * 100 / TITLE_ANIM_LENGTH);
	}

	renderTimeRemaining();
	renderLoginStats(0, 0);
	renderLoginStats(320, 1);

	if ( isInSubmenu )
	{
		rectfill(rm.m_backbuf, 250, 300, 550, 370, makeacol(146, 16, 255, 128));
		if ( !gs.isVersus )
		{
			if ( currentSubmenu == 3 )
			{
				masked_blit(m_menu[1], rm.m_backbuf, 0, 0, 256, 305, 288, 60); // maniax, does not happen (this line gets skipped)
			}
			else
			{
				masked_blit(m_menu[currentSubmenu], rm.m_backbuf, 0, 0, 256, 305, 288, 60);
			}
		}
		else
		{
			int p1y = separateSubmenu[0]*20 + 305;
			int p2y = separateSubmenu[1]*20 + 305;
			if ( separateSubmenu[0] == 3 )
			{
				renderWhiteString("MANIAX", 236, p1y);
				//masked_blit(m_menu[separateSubmenu[0]], rm.m_backbuf, 0, 0, 256, 305, 288, 60);
			}
			else
			{
				renderWhiteString("1P", 236, p1y);
				masked_blit(m_menu[separateSubmenu[0]], rm.m_backbuf, 0, 0, 256, 305, 288, 60);
			}

			if ( separateSubmenu[1] == 3 )
			{
				renderWhiteString("MANIAX", 256+288, p2y);
				//masked_blit(m_menu[separateSubmenu[1]], rm.m_backbuf, 0, separateSubmenu[1]*20, 256, p2y, 288, 20);
			}
			else
			{
				renderWhiteString("2P", 256+288, p2y);
				masked_blit(m_menu[separateSubmenu[1]], rm.m_backbuf, 0, separateSubmenu[1]*20, 256, p2y, 288, 20);
			}
		}
	}

	// render the "pick next stage" animation
	if ( nextStageAnimTimer > 0 )
	{
		renderNextStageAnim();
	}
}

void renderIntroAnim(int percent)
{
	blit(m_songBG, rm.m_backbuf, 0, 0, 0, 0, 640, 480);
	draw_trans_sprite(rm.m_backbuf, m_musicSelect, getValueFromRange(SCREEN_WIDTH, 260, percent), 135); 

	renderTimeRemaining();
	renderPickedSongs(percent);

	// render the shadows under the banners
	drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
	for ( int i = 0; i < NUM_BANNERS; i++ )
	{
		polygon(rm.m_backbuf, 4, shadowQuads[i], makeacol(0, 0, 0, 95));
	}
	set_alpha_blender(); // the game assumes the graphics are left in this mode
	
	// which banner goes where on the wheel?
	static char bannerOffsetIndex[NUM_BANNERS] = { -2, -6, -1, -9, -5, 0, -8, -4, 1, -7, -3, 2};

	for ( int i = 0; i < NUM_BANNERS; i++ )
	{
		int bannerIndex = songwheelIndex + bannerOffsetIndex[i];
		if ( bannerIndex < 0 )
		{
			bannerIndex += maxSongwheelIndex;
		}
		if ( bannerIndex > maxSongwheelIndex-1 )
		{
			bannerIndex -= maxSongwheelIndex;
		}
		bannerIndex = songID_to_listID(songlist[bannerIndex].songID);
		renderBanner(bannerIndex, i, songs[bannerIndex].isNew);
	}
}

void renderNextStageAnim()
{
	int percent = nextStageAnimTimer * 100 / NEXT_STAGE_ANIM_TIME;
	int black = makeacol(0, 0, 0, 255);
	char stageString[32] = "";
	char ordinals[6][8] = { "", "2nd ", "3rd ", "4th ", "Final " };

	strcpy_s(stageString, "Choose " );
	strcat_s( stageString, ordinals[gs.currentStage == gs.numSongsPerSet-1 ? 4 : gs.currentStage] );
	strcat_s(stageString, "Stage");

	// black line: in
	if ( percent > 90 )
	{
		rectfill(rm.m_backbuf, SCREEN_WIDTH, 300, getValueFromRange(0, SCREEN_WIDTH, (percent-90)*100/10), 340, black);
	}
	// text: in
	else if ( percent > 60 )
	{
		rectfill(rm.m_backbuf, 0, 300, SCREEN_WIDTH, 340, black);
		renderBoldString(stageString, getValueFromRange(SCREEN_WIDTH/2, SCREEN_WIDTH, (percent-60)*100/30), 306, 999, false, 0);
	}
	// pause
	else if ( percent > 40 )
	{
		rectfill(rm.m_backbuf, 0, 300, SCREEN_WIDTH, 340, black);
		renderBoldString(stageString, SCREEN_WIDTH/2, 306, 999, false, 0);
	}
	// text: out
	else if ( percent > 10 )
	{
		rectfill(rm.m_backbuf, 0, 300, SCREEN_WIDTH, 340, black);
		renderBoldString(stageString, getValueFromRange(-100, SCREEN_WIDTH/2, (percent-10)*100/30), 306, 999, false, 0);
	}
	// black line: out
	else 
	{
		rectfill(rm.m_backbuf, 0, 300, getValueFromRange(0, SCREEN_WIDTH, (percent)*100/10), 340, black);
	}

	int firstHalfProgress = getValueFromRange(0, 100, (percent-50)*100/50);
	renderNextStageAnimBanner(songID_to_listID(gs.player[0].stagesPlayed[ gs.currentStage-1 ]), percent > 50 ? firstHalfProgress : 0);
}

void prepareForSongwheelRotation()
{
	// copy either [targetQuadsCW] or [targetQuadsCCW] to targetQuads
	if ( !isMovementClockwise )
	{
		for ( int i = 0; i < NUM_BANNERS; i++ )
		for ( int j = 0; j < 8; j++ )
		{
			targetQuads[i][j] = targetQuadsCW[i][j];
		}
		songwheelIndex = songwheelIndex <= 0 ? maxSongwheelIndex - 1 : songwheelIndex - 1;
	}
	else
	{
		for ( int i = 0; i < NUM_BANNERS; i++ )
		for ( int j = 0; j < 8; j++ )
		{
			targetQuads[i][j] = targetQuadsCCW[i][j];
		}
		songwheelIndex = songwheelIndex + 1 >= maxSongwheelIndex ? 0 : songwheelIndex + 1;
	}

	isSphereMoving = true;
	rotateAnimTime = introAnimTimer > 0 ? INTRO_ANIM_STEP_TIME : WHEEL_STEP_TIME;
	updateSongwheelRotation(0);
	killPreviewClip();
	//stop_sample(currentPreview);
}

void postSongwheelRotation()
{
	// set the current positions of all the banners
	for ( int i = 0; i < NUM_BANNERS; i++ )
	{
		for ( int j = 0; j < 8; j++ )
		{
			currentQuads[i][j] = bannerQuads[i][j];
		}
	}
	isSphereMoving = false;

	displayBPM = songlist[songwheelIndex].minBPM;
	displayBPMTimer = 0;
	displayBPMState = 0;
}

int getCoordinateOfPickedSong(int which)
{
	int startingX = SCREEN_W - 5 - (gs.numSongsPerSet * 71);
	return startingX + which*71;
}

void renderPickedSongs(int percent)
{
	int startingX = getCoordinateOfPickedSong(0);
	int x = startingX;

	// do something totally different in convention / freestyle mode
	if ( gs.isFreestyleMode )
	{
		renderBoldString("FREESTYLE MODE - 1 STAGE", 340, 30, 300, false, 3);
		if ( gs.isDoubles )
		{
			renderBoldString("DOUBLES PLAY", 340, 65, 300, false, 2);
		}
		else
		{
			renderBoldString("SINGLES PLAY", 340, 65, 300, false, 1);
		}
		renderArtistString("Press 2P Start to switch", 340, 90, 300, 100);
		return;
	}

	for ( int i = 0; i < gs.numSongsPerSet; i++ )
	{
		if ( gs.player[0].stagesPlayed[i] == -1 || i >= gs.currentStage )
		{
			// render an empty slot
			rectfill(rm.m_backbuf, x, 5, x+64, 69, makeacol(0, 0, 0, getValueFromRange(0, 64, percent)));
			if ( percent > 80 )
			{
				renderWhiteNumber(i+1, x + 10, 15);
			}
		}
		else
		{
			// render a real banner
			stretch_blit(m_banners[songID_to_listID(gs.player[0].stagesPlayed[i])], rm.m_backbuf, 0, 0, 256, 256, x, 5, 64, 64);
		}

		x += 71;
	}
}

void updateSongwheelRotation(UTIME dt)
{
	rotateAnimTime -= dt;
	if ( rotateAnimTime <= 0 )
	{
		postSongwheelRotation();
		return;
	}

	int percent = rotateAnimTime * 100 / (introAnimTimer > 0 ? INTRO_ANIM_STEP_TIME : WHEEL_STEP_TIME);

	// 'tween' each quad to where it wants to be
	for ( int i = 0; i < NUM_BANNERS; i++ )
	{
		for ( int j = 0; j < 8; j++ )
		{
			currentQuads[i][j] = getValueFromRange(bannerQuads[i][j], targetQuads[i][j], percent);
		}
	}
}

// this is a super-private helper function which should only be called from renderBanner() and renderNextStageAnimBanner()
void doRenderBanner(int textureID, bool isNew)
{
	pp[3]->u = itofix(0); // use hi-res 256x256 square banners
	pp[3]->v = itofix(255);

	pp[0]->u = itofix(0);
	pp[0]->v = itofix(0);

	pp[1]->u = itofix(255);
	pp[1]->v = itofix(0);

	pp[2]->u = itofix(255);
	pp[2]->v = itofix(255);


	//polygon3d(rm.m_backbuf, POLYTYPE_FLAT, m_banners[textureID], 4, pp);
	if ( m_banners[textureID] != NULL )
	{
		quad3d(rm.m_backbuf, POLYTYPE_ATEX, m_banners[textureID], pp[0], pp[1], pp[2], pp[3]);
	}
	if ( isNew )
	{
		draw_trans_sprite(rm.m_backbuf, m_new, fixtoi(pp[0]->x), fixtoi(pp[0]->y) - 10);
	}
}

void renderBanner(int textureID, int coordsIndex, bool isNew)
{
	for ( int i = 0; i < 4; i++ )
	{
		pp[i]->x = itofix(currentQuads[coordsIndex][i*2]);
		pp[i]->y = itofix(currentQuads[coordsIndex][i*2 +1]);
		pp[i]->z = 0;
		pp[i]->c = 0;//makeacol(255,255,255,255);
		pp[i]->u = pp[i]->v = 0;
	}

	doRenderBanner(textureID, isNew);
}

void renderNextStageAnimBanner(int textureID, int percent)
{
	int tx = getCoordinateOfPickedSong(gs.currentStage - 1);
	int startPoint[8] = { 160,171, 256,171, 256,243, 160,243 };  // the starting point of the banner
	int targetPoint[8] = { tx,5, tx+64,5, tx+64,5+64, tx,5+64 }; // the destination in the upper-right

	// interpolate!
	int currentQuad[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	for ( int c = 0; c < 8; c++ )
	{
		currentQuad[c] = getValueFromRange(targetPoint[c], startPoint[c], percent);
	}

	for ( int i = 0; i < 4; i++ )
	{
		pp[i]->x = itofix(currentQuad[i*2]);
		pp[i]->y = itofix(currentQuad[i*2 +1]);
		pp[i]->z = 0;
		pp[i]->c = 0;//makeacol(255,255,255,255);
		pp[i]->u = pp[i]->v = 0;
	}

	if ( textureID == -1 )
	{
		al_trace("Trying to render a banner with index -1 in renderNextStageAnimBanner()");
	}
	doRenderBanner(textureID, false);
}

void renderTitleArea(int percent)
{
	unsigned char bpm[] = "000";
	int globalIndex = songID_to_listID(songlist[songwheelIndex].songID);

	if ( percent < 20 )
	{
		draw_trans_sprite(rm.m_backbuf, m_titleArea, getValueFromRange(SCREEN_WIDTH, 0, percent*5), 164);
	}
	else
	{
		draw_trans_sprite(rm.m_backbuf, m_titleArea, 0, 164);
	}

	renderBanner(globalIndex, 5, songlist[songwheelIndex].isNew);

	if ( percent >= 35 )
	{
		renderBoldString(songTitles[globalIndex].c_str(), 293, 170, 365, false);
		renderDifficultyStars(398, 243, 0, gs.isDoubles ? songlist[songwheelIndex].mildDouble : songlist[songwheelIndex].mildSingle);	
		renderDifficultyStars(433, 225, 1, gs.isDoubles ? songlist[songwheelIndex].wildDouble : songlist[songwheelIndex].wildSingle);
	}
	if ( percent >= 65 )
	{
		renderArtistString(songArtists[globalIndex].c_str(), 293, 196, 365, 16);

		bpm[0] = (displayBPM / 100 % 10) + '0';
		bpm[1] = (displayBPM / 10 % 10) + '0';
		bpm[2] = (displayBPM / 1 % 10) + '0';
		renderBoldString(bpm, 583, 238, 100, false);
	}
	if ( percent >= 85 )
	{
		masked_blit(m_versions, rm.m_backbuf, 0, songlist[songwheelIndex].version*60, 33, 178, 96, 60);

		// render the semi-secret maniax indicator (a red star near the version icon)
		if ( gs.isDoubles ? songlist[songwheelIndex].maniaxDouble : songlist[songwheelIndex].maniaxSingle > 0 )
		{
			draw_trans_sprite(rm.m_backbuf, m_stars[1][0], 10, 178);
		}
	}
}

void renderDifficultyStars(int x, int y, char type, char level)
{
	int xdiff = level <= 5 ? 23 : 13; // for 5 or less stars, do not stack them
	int starLimit = level <= 5 ? 5 : 9;

	if ( type == 0 )
	{
		xdiff *= -1; // mild goes left, wild goes right
	}
	if ( level == 0 )
	{
		starLimit = 0; // for locked charts
		draw_trans_sprite(rm.m_backbuf, m_lockedIcon, x, y);
	}

	for ( int i = 1; i <= starLimit; i++ )
	{
		draw_trans_sprite(rm.m_backbuf, m_stars[type][level <= i-1 ? 1 : 0], x, y);
		x += xdiff;
	}
}

void renderTimeRemaining(int xcoord, int ycoord)
{
	if ( timeRemaining > 99000 || timeRemaining < 0 )
	{
		return;
	}

	int tensDigit = timeRemaining/10000 % 10;
	int onesDigit = timeRemaining/1000 % 10;
	char color = 0, yoff = 0;
	
	if ( timeRemaining < 6000 )
	{
		color = (timeRemaining/50) % 2;
	}
	if ( timeRemaining % 1000 < 100 )
	{
		yoff = (timeRemaining % 1000)/20;
	}

	if ( color == 0 )
	{
		draw_trans_sprite(rm.m_backbuf, m_time, 5, 15); // blink when time is low
	}

	if ( onesDigit == 0 )
	{
		masked_blit(m_timeDigits[color], rm.m_backbuf, 0, tensDigit*32, xcoord, ycoord+yoff, 32, 32);
	}
	else
	{
		masked_blit(m_timeDigits[color], rm.m_backbuf, 0, tensDigit*32, xcoord, ycoord, 32, 32);
	}
	masked_blit(m_timeDigits[color], rm.m_backbuf, 0, onesDigit*32, xcoord+32, ycoord+yoff, 32, 32);
}

void playTimeLowSFX(UTIME dt)
{
	static UTIME beepAt[8] = { 5000, 4000, 3000, 2500, 2000, 1500, 1000, 500 };

	for ( int i = 0; i < 8; i++ )
	{
		if ( timeRemaining < beepAt[i] && (timeRemaining + dt) >= beepAt[i] )
		{
			em.playSample(SFX_TIMER_LOW);
		}
	}
}

// only works for a single player, and really just for debugging
void renderExtendedStats()
{
	renderWhiteString("  PERFECT:", 445, 290);
	renderWhiteString("    GREAT:", 445, 305);
	renderWhiteString("     GOOD:", 445, 320);
	renderWhiteString("     MISS:", 445, 335);
	renderWhiteString("MAX COMBO:", 445, 350);
	int chartIndex = 0;
	renderWhiteNumber(sm.player[0].allTime[songwheelIndex][chartIndex].perfects, 445+110, 290);
	renderWhiteNumber(sm.player[0].allTime[songwheelIndex][chartIndex].greats,   445+110, 305);
	renderWhiteNumber(sm.player[0].allTime[songwheelIndex][chartIndex].goods,    445+110, 320);
	renderWhiteNumber(sm.player[0].allTime[songwheelIndex][chartIndex].misses,   445+110, 335);
	renderWhiteNumber(sm.player[0].allTime[songwheelIndex][chartIndex].maxCombo, 445+110, 350);
}

void renderLoginStats(int x, int side)
{
	int mildIndex = gs.isDoubles ? getChartIndexFromType(DOUBLE_MILD) : getChartIndexFromType(SINGLE_MILD);
	int wildIndex = gs.isDoubles ? getChartIndexFromType(DOUBLE_WILD) : getChartIndexFromType(SINGLE_WILD);
	char buffer[10] = "";

	if ( !sm.player[side].isLoggedIn )
	{
		return;
	}

	int offset = 5;
	for ( int i = 0; i < 8; i++ )
	{
		renderNameLetter(sm.player[side].displayName[i], x+offset, 410, side+1);
		offset += 32;
	}

	int allTimeIndex = songID_to_listID(songlist[songwheelIndex].songID);
	int whichFrame = displayBPMTimer > 1000 ? (displayBPMTimer-1000)/125 : 0;
	int mildColor = 6, wildColor = 6; // intentionally transparent

	if ( sm.player[side].allTime[allTimeIndex][mildIndex].status >= STATUS_FULL_GOOD_COMBO )
	{
		mildColor = sm.player[side].allTime[allTimeIndex][mildIndex].status - 3;
		if ( mildColor == 2 )
		{
			mildColor = displayBPMState == -1 ? 3 : 4; // pick a gold star, and use the alternate 'grow' animation every other pass
		}
	}
	if ( sm.player[side].allTime[allTimeIndex][wildIndex].status >= STATUS_FULL_GOOD_COMBO )
	{
		wildColor = sm.player[side].allTime[allTimeIndex][wildIndex].status - 3;
		if ( wildColor == 2 )
		{
			wildColor = displayBPMState == -1 ? 3 : 4; // pick a gold star, and use the alternate 'grow' animation every other pass
		}
	}

	// render MILD
	_itoa_s(sm.player[side].allTime[allTimeIndex][mildIndex].getScore(), buffer, 9, 10);
	addLeadingZeros(buffer, 7);
	renderBoldString((unsigned char *)buffer, x+5, 440, 320, false, 1);
	//masked_blit(m_miniStatus, rm.m_backbuf, 0, sm.player[side].allTime[allTimeIndex][mildIndex].status * 32, x+120, 433, 40, 32);
	if ( sm.player[side].allTime[allTimeIndex][mildIndex].status >= STATUS_CLEARED )
	{
		renderGrade(sm.player[side].allTime[allTimeIndex][mildIndex].grade, x+105, 433-8);
	}
	masked_blit(m_statusStars, rm.m_backbuf, whichFrame * 40, mildColor * 32, x+105+20, 433, 40, 32);

	// render WILD
	_itoa_s(sm.player[side].allTime[allTimeIndex][wildIndex].getScore(), buffer, 9, 10);
	addLeadingZeros(buffer, 7);
	renderBoldString((unsigned char *)buffer, x+165, 440, 320, false, 2);
	//masked_blit(m_miniStatus, rm.m_backbuf, 0, sm.player[side].allTime[allTimeIndex][wildIndex].status * 32, x+280, 433, 40, 32);
	if ( sm.player[side].allTime[allTimeIndex][wildIndex].status >= STATUS_CLEARED )
	{
		renderGrade(sm.player[side].allTime[allTimeIndex][wildIndex].grade, x+265, 433-8);
	}
	masked_blit(m_statusStars, rm.m_backbuf, whichFrame * 40, wildColor * 32, x+265+20, 433, 40, 32);
}

void updatePreview(UTIME dt)
{
	SUBTRACT_TO_ZERO(previewTimeRemaining, dt);

	if ( previewSongID != songlist[songwheelIndex].songID )
	{
		if ( isRandomSelect && last_utime - previewTimeStarted < 2000 )
		{
			return;
		}

		previewSongID = songlist[songwheelIndex].songID;
		previewTimeStarted = last_utime;
		loadPreview(previewSongID);
		if ( isRandomSelect )
		{
			gs.playSongPreview(pickRandomInt(3,12,15,20)*100);
			//play_sample(currentPreview, 255, 127, pickRandomInt(3,12,15,20)*100, 1); 
		}
		else
		{
			gs.playSongPreview(0);
			//play_sample(currentPreview, 255, 127, 1000, 1); 
		}
		//previewTimeRemaining = getSampleLength(currentPreview);
		previewTimeRemaining = gs.currentSongLength;
	}
}

void loadPreview(int songid)
{
	previewSongID = songid;
	gs.loadSong(songid, true);
}

/*
// switched to mp3s because 300 MB of wav files was inappropriate, but made dynamic pitched previews easier
void loadPreview(int songid)
{
	char filename[] = "DATA/sfx/pre_000.wav";
	filename[13] = (songid/100)%10 + '0';
	filename[14] = (songid/10)%10 + '0';
	filename[15] = (songid/1)%10 + '0';

	if ( !fileExists(filename) )
	{
		filename[13] = '1'; // fix this problem
		filename[14] = '0';
		filename[15] = '1';
	}

	destroy_sample(currentPreview);
	currentPreview = load_sample(filename);
}
//*/