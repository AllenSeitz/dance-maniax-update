// nonstopMode.cpp implements the old, traditional course selection menu
// source file created by Allen Seitz 8/11/2012

#include "common.h"
#include "GameStateManager.h"
#include "inputManager.h"
#include "scoreManager.h"
#include "videoManager.h"

extern GameStateManager gs;
extern RenderingManager rm;
extern InputManager im;
extern ScoreManager sm;
extern VideoManager vm;
extern EffectsManager em;

extern unsigned long int frameCounter;
extern unsigned long int totalGameTime;

extern BITMAP** m_banners;
extern SongEntry* songs;
extern int NUM_SONGS;
extern std::string* songTitles;
extern std::string* songArtists;

extern int NUM_COURSES;
SongEntry* nonstops = NULL;
int numVisibleCourses = 0;

extern UTIME timeRemaining;
extern void renderTimeRemaining(int xc, int yc);
extern void playTimeLowSFX(UTIME dt);

int currentCourseIndex = 0;
int scrollAnimationTimer = 0;
int bannerAnimationTimer = 0;
char currentlyMovingDirection = 0; // -1 = left, 1 = right, 0 = motionless
char previousScrollDirection = 0;
int fadeAnimTimer = 0;

extern int  displayBPM;              // oscillates between min and max
extern int  displayBPMTimer;
extern char displayBPMState;

BITMAP* m_background = NULL;
BITMAP* m_courseSelect = NULL;
//BITMAP* m_tracks = NULL;
BITMAP* m_courseHelp = NULL;
BITMAP* m_titleAreaNonstop = NULL;
BITMAP* m_nonstopStars = NULL;
BITMAP* m_orbs[20];
//BITMAP* m_rings;
extern BITMAP* m_miniStatus;

BITMAP* m_currentText;

// taken directly from the original game
/*
const int courseDefinitions[8][5] =
{
	{ 121, 124, 123, 103, 111 }, // world groove.....................................indi.....mobo.....bail.....gorg.....afro.....
	{ 102, 108, 117, 114, 111 }, // from DDR...............ÿ.........................puty.....keep.....butt.....dyna.....afro.....
	{ 101, 119, 110, 122, 109 }, // dance party............ÿ.........................brok.....itsa.....heav.....horn.....disc.....
	{ 105, 106, 104, 112, 107 }, // club mix...............ÿ.........................alln.....body.....aint.....mean.....drlo.....
	{ 124, 110, 114, 111, 125 }, // over 170.........................................mobo.....heav.....dyna.....afro.....punk.....
	{ 109, 122, 114, 111, 125 }, // challenge..............ÿ.........................disc.....horn.....dyna.....afro.....punk.....
	{ 115, 120, 113, 118, 116 }, // major songs......................................loco.....toge.....inmy.....lets.....mysh.....
	{ 101, 109, 103, 110, 111 }, // .......................ÿ.........................brok.....disc.....gorg.....heav.....afro.....
};
*/

const int INTRO_ANIM_LENGTH = 750;
const int BANNER_ANIM_LENGTH = 500;
const int SCROLL_ANIM_LENGTH = 250;

void renderNonstopTitleArea(int percent);
//
//

void renderHighScores(int x, int player);
//
//

void loadCurrentText();
//
//

void pickCourse()
{
	gs.g_currentGameMode = GAMEPLAY;
	gs.g_gameModeTransition = 1;
	gs.currentStage = 0;
	gs.killSong();

	for ( int side = 0; side < 2; side++ )
	{
		gs.player[side].stagesPlayed[0] = nonstops[currentCourseIndex].songID;
		gs.player[side].stagesLevels[0] = gs.isDoubles ? DOUBLE_WILD : SINGLE_WILD;
		gs.player[side].stagesPlayed[1] = 0; // a blank songid will end the set early
	}
}

void firstNonstopLoop()
{
	currentCourseIndex = 0;
	numVisibleCourses = 0;
	displayBPM = 150;
	displayBPMState = 0;
	displayBPMTimer = 0;

	scrollAnimationTimer = SCROLL_ANIM_LENGTH;
	bannerAnimationTimer = BANNER_ANIM_LENGTH;
	fadeAnimTimer = INTRO_ANIM_LENGTH;
	currentlyMovingDirection = previousScrollDirection = 1;

	if ( m_background == NULL )
	{
		m_background = loadImage("DATA/menus/bg_course.png");
		m_courseSelect = loadImage("DATA/menus/course_select.png");
		//m_tracks = loadImage("DATA/menus/track.png");
		m_courseHelp = loadImage("DATA/menus/course_help.tga");
		m_titleAreaNonstop = loadImage("DATA/menus/nonstop_title_area.png");
		m_nonstopStars = loadImage("DATA/menus/nonstop_stars.tga");
		for ( int i = 0; i < 20; i++ )
		{
			char filename[] = "DATA/menus/orb00.png";
			filename[14] = (i/10)%10 + '0';
			filename[15] = i%10 + '0';
			m_orbs[i] = loadImage(filename);
		}
		//m_rings = loadImage("DATA/menus/rings.tga");
	}
	if ( m_miniStatus == NULL )
	{
		m_miniStatus = loadImage("DATA/songwheel/mini_status.tga");
	}

	// show only songs which are marked as being nonstops
	if ( nonstops != NULL )
	{
		delete nonstops;
	}
	int nextIndex = 0;
	nonstops = new SongEntry[NUM_COURSES];
	for ( int i = 0; i < NUM_SONGS; i++ )
	{
		if ( songs[i].version == 101 )
		{
			if ( songs[i].unlockFlag != UNLOCK_METHOD_NONE )
			{
				continue; // course is locked
			}

			// okay! its in! add it to the list of courses
			nonstops[nextIndex] = songs[i];
			nextIndex++;
			numVisibleCourses++;
		}
	}

	if ( numVisibleCourses <= 0 )
	{
		globalError(BAD_NONSTOP_DATA, "No nonstop courses available?");
	}

	// begin mode
	timeRemaining = 30999;
	loadCurrentText();

	em.playSample(SFX_COURSE_APPEAR);
	gs.loadSong(BGM_DEMO);
	gs.playSong();
}

void mainNonstopLoop(UTIME dt)
{
	// update animation timers
	SUBTRACT_TO_ZERO(fadeAnimTimer, (int)dt);
	SUBTRACT_TO_ZERO(scrollAnimationTimer, (int)dt);
	if ( scrollAnimationTimer == 0 )
	{
		currentlyMovingDirection = 0;
		SUBTRACT_TO_ZERO(bannerAnimationTimer, (int)dt);
	}
	SUBTRACT_TO_ZERO(timeRemaining, dt);
	playTimeLowSFX(dt);

	// update the display BPM
	displayBPMTimer += dt;
	switch (displayBPMState)
	{
	case 1:
		displayBPM = getValueFromRange(nonstops[currentCourseIndex].minBPM, nonstops[currentCourseIndex].maxBPM, displayBPMTimer >= 1000 ? 100 : displayBPMTimer/10);
		if ( displayBPMTimer >= 2000 )
		{
			displayBPMTimer -= 2000;
			displayBPMState = -1; // going down to the min bpm
		}
		break;
	case -1:
		displayBPM = getValueFromRange(nonstops[currentCourseIndex].maxBPM, nonstops[currentCourseIndex].minBPM, displayBPMTimer >= 1000 ? 100 : displayBPMTimer/10);
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

	// input
	if ( fadeAnimTimer == 0 )
	{
		// pick this course
		if ( ((im.getKeyState(MENU_START_1P) == JUST_DOWN || im.getKeyState(MENU_START_2P) == JUST_DOWN) && currentlyMovingDirection == 0) || timeRemaining == 0 )
		{
			pickCourse();
			em.playSample(SFX_FORCEFUL_SELECTION);
			em.announcerQuip(GUY_NONSTOP_BEGIN);
		}

		// scroll left or right?
		if ( (im.isKeyDown(MENU_LEFT_1P) || im.isKeyDown(MENU_LEFT_2P)) && currentlyMovingDirection == 0 )
		{
			currentlyMovingDirection = previousScrollDirection = -1;
			scrollAnimationTimer = SCROLL_ANIM_LENGTH;
			bannerAnimationTimer = BANNER_ANIM_LENGTH;
			currentCourseIndex = currentCourseIndex <= 0 ? numVisibleCourses-1 : currentCourseIndex - 1;
			em.playSample(SFX_COURSE_PREVIEW_LOAD);
			loadCurrentText();
		}
		if ( (im.isKeyDown(MENU_RIGHT_1P) || im.isKeyDown(MENU_RIGHT_2P)) && currentlyMovingDirection == 0 )
		{
			currentlyMovingDirection = previousScrollDirection = 1;
			scrollAnimationTimer = SCROLL_ANIM_LENGTH;
			bannerAnimationTimer = BANNER_ANIM_LENGTH;
			currentCourseIndex = currentCourseIndex >= numVisibleCourses-1 ? 0 : currentCourseIndex + 1;
			em.playSample(SFX_COURSE_PREVIEW_LOAD);
			loadCurrentText();
		}
	}

	// render the background
	blit(m_background, rm.m_backbuf, 0, 0, 0, 0, 640, 460);
	masked_blit(m_courseSelect, rm.m_backbuf, 0, 0, 250+(fadeAnimTimer/3), 110, 384, 34);
	//masked_blit(m_tracks, rm.m_backbuf, 0, 0, 215, 240, 72, 24*gs.numSongsPerSet);

	// render the banner and the text
	int percent = 100 - (bannerAnimationTimer*100 / BANNER_ANIM_LENGTH);
	renderNonstopTitleArea(percent);
	renderHighScores(0, 0);
	renderHighScores(320, 1);

	// render the fancy course text area
	percent = 100 - (bannerAnimationTimer*100 / BANNER_ANIM_LENGTH);
	if ( m_currentText != NULL )
	{
		blit(m_currentText, rm.m_backbuf, 0, 0, 240, 234, 400, getValueFromRange(1, 150, percent));
	}

	// render the COURSE 1/6 display
	char numCoursesString[256] = "";
	sprintf_s(numCoursesString, "COURSE: %d / %d", currentCourseIndex + 1, NUM_COURSES);
	renderBoldString(numCoursesString, 400, 10, 500, false);

	renderTimeRemaining(5, 42);
	//draw_trans_sprite(rm.m_backbuf, m_courseHelp, 0, 460-64);

	if ( fadeAnimTimer > 0 )
	{
		rm.dimScreen(getValueFromRange(0, 100, fadeAnimTimer*100/INTRO_ANIM_LENGTH));
	}
	rm.flip();
}

void renderNonstopTitleArea(int percent)
{
	unsigned char bpm[] = "000";
	int globalIndex = songID_to_listID( nonstops[currentCourseIndex].songID );

	// purple background
	if ( percent < 20 )
	{
		// render a quickly flying orb
		int orbx = 0;
		if ( previousScrollDirection == -1 )
		{
			orbx = getValueFromRange(-72, 640, scrollAnimationTimer*100/SCROLL_ANIM_LENGTH);
			draw_trans_sprite(rm.m_backbuf, m_titleAreaNonstop, getValueFromRange(SCREEN_WIDTH, 0, percent*5), 147);
		}
		if ( previousScrollDirection == 1 )
		{
			orbx = getValueFromRange(640, -72, scrollAnimationTimer*100/SCROLL_ANIM_LENGTH);
			draw_trans_sprite(rm.m_backbuf, m_titleAreaNonstop, getValueFromRange(-SCREEN_WIDTH, 0, percent*5), 147);
		}
		masked_blit(m_orbs[currentCourseIndex%20], rm.m_backbuf, 0, 0, orbx, 153, 72, 72);
		//masked_blit(m_orbs[currentCourseIndex], rm.m_backbuf, 0, 0, getValueFromRange(45, 640, percent), 153, 72, 72);
	}
	else
	{
		draw_trans_sprite(rm.m_backbuf, m_titleAreaNonstop, 0, 147);
	}

	// banner
	int squareSize = getValueFromRange(1,196,percent); // grow from 1 to 196
	masked_stretch_blit(m_banners[globalIndex], rm.m_backbuf, 0, 0, 256, 256, 24 + (196-squareSize)/2, 96 + (196-squareSize)/2, squareSize, squareSize);

	// text and stuff
	if ( percent >= 35 )
	{
		renderBoldString(songTitles[globalIndex].c_str(), 293, 170-17, 365, false);
		int numStars = gs.isDoubles ? nonstops[currentCourseIndex].wildDouble : nonstops[currentCourseIndex].wildSingle;
		for ( int st = 0; st < 9; st++ )
		{
			masked_blit(m_nonstopStars, rm.m_backbuf, st <= numStars ? 0 : 24, 0, 276 + 24*st, 203, 24, 24);
		}
	}
	if ( percent >= 65 )
	{
		renderArtistString(songArtists[globalIndex].c_str(), 293, 196-17, 365, 16);

		bpm[0] = (displayBPM / 100 % 10) + '0';
		bpm[1] = (displayBPM / 10 % 10) + '0';
		bpm[2] = (displayBPM / 1 % 10) + '0';
		renderBoldString(bpm, 527, 203, 100, false);
	}
}

void renderHighScores(int x, int side)
{
	int chartIndex = gs.isDoubles ? getChartIndexFromType(DOUBLE_WILD) : getChartIndexFromType(SINGLE_WILD);
	char buffer[10] = "";

	if ( !sm.player[side].isLoggedIn )
	{
		return;
	}

	int offset = 20;
	for ( int i = 0; i < 8; i++ )
	{
		renderNameLetter(sm.player[side].displayName[i], x+offset, 400, side+1);
		offset += 32;
	}

	int allTimeIndex = songID_to_listID(nonstops[currentCourseIndex].songID);
	_itoa_s(sm.player[side].allTime[allTimeIndex][chartIndex].getScore(), buffer, 9, 10);
	addLeadingZeros(buffer, 7);
	renderBoldString((unsigned char *)buffer, x+20, 430, 330, false, 2);
	masked_blit(m_miniStatus, rm.m_backbuf, 0, sm.player[side].allTime[allTimeIndex][chartIndex].status * 32, x+120, 423, 40, 32);
}

void loadCurrentText()
{
	char bfilename[] = "DATA/banners/ns_0000.tga";
	int c = nonstops[currentCourseIndex].songID;
	bfilename[16] = (c/1000)%10 + '0';
	bfilename[17] = (c/100)%10 + '0';
	bfilename[18] = (c/10)%10 + '0';
	bfilename[19] = c%10 + '0';
	m_currentText = load_bitmap(bfilename, NULL); // fine if it is null
}
