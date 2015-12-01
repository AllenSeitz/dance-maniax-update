// prototypeSelect.cpp implements a mode to choose between prototype simfiles
// original source file, songwheelMode.cpp, created by Allen Seitz 7/18/2009
// this source file started as a copy/paste of that file 1/27/2015 (and greatly diverged)

#include "common.h"
#include "GameStateManager.h"
#include "inputManager.h"

extern GameStateManager gs;
extern RenderingManager rm;
extern InputManager im;
extern EffectsManager em;

SongEntry* protoSongs = NULL;
std::string* songTitles;
int NUM_PROTO_SONGS = 0;
#define MAX_PROTO_SONGS 100

extern unsigned long int frameCounter;
extern unsigned long int totalGameTime;
extern volatile UTIME last_utime;


//////////////////////////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////////////////////////
#define WHEEL_STEP_TIME 180

#define INTRO_ANIM_LENGTH 2000
#define INTRO_ANIM_STEP_TIME 500
#define TITLE_ANIM_LENGTH 1000

//////////////////////////////////////////////////////////////////////////////
// Variables
//////////////////////////////////////////////////////////////////////////////
bool isSphereMoving = false;
bool isMovementClockwise = false;
int  rotateAnimTime = 0;
int  songwheelIndex = 0;
int  maxSongwheelIndex = 0;
UTIME timeRemaining = 60000;
int  introAnimTimer = 0;
int  introAnimSteps = 5;
UTIME nextStageAnimTimer = 0;
#define NEXT_STAGE_ANIM_TIME 2000

int  titleAnimTimer = 0;
int  displayBPM = 150;
int  displayBPMTimer = 0;
char displayBPMState = 0;

bool isInSubmenu = false;
char currentSubmenu = 0;            // only used for one player
char separateSubmenu[2] = {0,0};    // only used for versus mode
bool submenuDone[2] = {0,0};		// only used for versus mode

//////////////////////////////////////////////////////////////////////////////
// Assets
//////////////////////////////////////////////////////////////////////////////
extern BITMAP* m_songBG = NULL;
extern BITMAP* m_titleArea;
extern BITMAP* m_stars[2][2];
extern BITMAP* m_lockedIcon;
extern BITMAP* m_time;
extern BITMAP* m_timeDigits[2];
extern BITMAP* m_menu[3];
extern BITMAP* m_musicSelect;
//BITMAP* m_miniStatus;
extern BITMAP* m_statusStars;
extern BITMAP* m_new;
extern BITMAP* m_versions;

//BITMAP** m_protoBanners;


//////////////////////////////////////////////////////////////////////////////
// Functions
//////////////////////////////////////////////////////////////////////////////
void playTimeLowSFX(UTIME dt);
//
//

void updatePreview(UTIME dt);
//
//

void renderNextStageAnim();
//
//

void firstPrototypeSelectLoop()
{
	// reset state
	isSphereMoving = false;
	rotateAnimTime = 0;
	timeRemaining = 60999;
	titleAnimTimer = 0;
	isInSubmenu = false;
	separateSubmenu[0] = separateSubmenu[1] = currentSubmenu = 0;
	introAnimTimer = INTRO_ANIM_LENGTH;
	introAnimSteps = 3;
	nextStageAnimTimer = 0;

	// load as many prototype songs as possible
	int i = 0;
	if ( protoSongs != NULL )
	{
		delete protoSongs;
	}
	protoSongs = new SongEntry[MAX_PROTO_SONGS];
	for ( i = 10000; i < 10000 + MAX_PROTO_SONGS; i++ )
	{
		// check for song existing, load songTitles, protoSongs, set NUM_PROTO_SONGS = i
	}
	maxSongwheelIndex = 0;
	songwheelIndex = 0;

	gs.player[0].resetStages();
	gs.player[1].resetStages();
	gs.currentStage = 0;
	gs.killSong();
	em.playSample(SFX_SONGWHEEL_APPEAR);
	gs.loadSong(BGM_DEMO);
	gs.playSong();

	im.setCooldownTime(0);
}

void mainSongwheelLoop(UTIME dt)
{
	SUBTRACT_TO_ZERO(nextStageAnimTimer, dt);

	if ( gs.isFreestyleMode && im.getKeyState(MENU_START_2P) == JUST_DOWN )
	{
		gs.isDoubles = !gs.isDoubles; // another special case for this mode
	}
	else if ( isSphereMoving )
	{
		titleAnimTimer = 0;
	}
	else if ( isInSubmenu )
	{
		// arrow keys work differently per player
		if ( gs.isVersus )
		{
			if ( im.getKeyState(MENU_LEFT_1P) == JUST_DOWN && !submenuDone[0] )
			{
				separateSubmenu[0] = separateSubmenu[0] == 0 ? 0 : separateSubmenu[0] - 1;
				em.playSample(SFX_DIFFICULTY_MOVE);
			}
			else if ( im.getKeyState(MENU_RIGHT_1P) == JUST_DOWN && !submenuDone[0] )
			{
				separateSubmenu[0] = separateSubmenu[0] >= 2 ? separateSubmenu[0] : separateSubmenu[0] + 1;
				em.playSample(SFX_DIFFICULTY_MOVE);
			}

			if ( im.getKeyState(MENU_LEFT_2P) == JUST_DOWN && !submenuDone[1] )
			{
				separateSubmenu[1] = separateSubmenu[1] == 0 ? 0 : separateSubmenu[1] - 1;
				em.playSample(SFX_DIFFICULTY_MOVE);
			}
			else if ( im.getKeyState(MENU_RIGHT_2P) == JUST_DOWN && !submenuDone[1] )
			{
				separateSubmenu[1] = separateSubmenu[1] >= 2 ? separateSubmenu[1] : separateSubmenu[1] + 1;
				em.playSample(SFX_DIFFICULTY_MOVE);
			}
		}
		else
		{
			if ( im.getKeyState(MENU_LEFT_1P) == JUST_DOWN || im.getKeyState(MENU_LEFT_2P) == JUST_DOWN )
			{
				currentSubmenu = currentSubmenu == 0 ? 0 : currentSubmenu - 1;
				em.playSample(SFX_DIFFICULTY_MOVE);
			}
			else if ( im.getKeyState(MENU_RIGHT_1P) == JUST_DOWN || im.getKeyState(MENU_RIGHT_2P) == JUST_DOWN )
			{
				currentSubmenu = currentSubmenu >= 2 ? currentSubmenu : currentSubmenu + 1;
				em.playSample(SFX_DIFFICULTY_MOVE);
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

				if ( timeRemaining < 10000 )
				{
					timeRemaining = 10000;
				}

				if ( gs.currentStage >= gs.numSongsPerSet || gs.isFreestyleMode )
				{
					gs.currentStage = 0;
					gs.g_currentGameMode = GAMEPLAY;
					gs.g_gameModeTransition = 1;
					stop_sample(currentPreview);
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
		if ( (im.isKeyDown(MENU_LEFT_1P) || im.isKeyDown(MENU_LEFT_2P)) /*&& !isRandomSelect*/ && !(im.isKeyDown(MENU_RIGHT_1P) || im.isKeyDown(MENU_RIGHT_2P)) )
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
		stop_sample(currentPreview);
	}

	if ( gs.g_currentGameMode == SONGWHEEL )
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
	stop_sample(currentPreview);
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

