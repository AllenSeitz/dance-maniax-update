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
extern SongEntry* songs[];
extern BITMAP** m_banners[];

extern int* songIDs;
extern std::string* songTitles;
extern std::string* songArtists;

extern UTIME timeRemaining;
extern void renderTimeRemaining(int xc, int yc);
extern void playTimeLowSFX(UTIME dt);

int currentCourseIndex = 0;
int scrollAnimationTimer = 0;
int bannerAnimationTimer = 0;
char currentlyMovingDirection = 0; // -1 = left, 1 = right, 0 = motionless
int fadeAnimTimer = 0;
int difficulty = 0; // SINGLE_MILD or SINGLE_WILD

UTIME waggleTimer = 0; // wave the blue sensors to change difficulty
const int WAGGLE_TIME = 2750; // how long a waggle input takes (before it is considered not an accidental input)
const int WAGGLE_STEP = 500;

BITMAP* m_background = NULL;
BITMAP* m_courseSelect = NULL;
BITMAP* m_tracks = NULL;
BITMAP* m_courseHelp = NULL;
BITMAP* m_orbs[20];
BITMAP* m_rings;
BITMAP* m_cbanners[8];

// taken directly from the original game
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

const int INTRO_ANIM_LENGTH = 750;
const int BANNER_ANIM_LENGTH = 500;
const int SCROLL_ANIM_LENGTH = 250;

void pickCourse()
{
	gs.g_currentGameMode = GAMEPLAY;
	gs.g_gameModeTransition = 1;
	gs.currentStage = 0;
	gs.killSong();

	for ( int i = 0; i < gs.numSongsPerSet; i++ )
	{
		gs.player[0].stagesPlayed[i] = courseDefinitions[currentCourseIndex][i];
		gs.player[0].stagesLevels[i] = difficulty + (gs.isDoubles ? 10 : 0);
		if ( gs.isVersus )
		{
			gs.player[1].stagesPlayed[i] = courseDefinitions[currentCourseIndex][i];
			gs.player[1].stagesLevels[i] = difficulty;
		}
	}
}

void firstNonstopLoop()
{
	currentCourseIndex = 0;
	scrollAnimationTimer = SCROLL_ANIM_LENGTH;
	bannerAnimationTimer = BANNER_ANIM_LENGTH;
	fadeAnimTimer = INTRO_ANIM_LENGTH;
	currentlyMovingDirection = 1;
	difficulty = SINGLE_MILD;
	waggleTimer = 0;

	if ( m_background == NULL )
	{
		m_background = loadImage("DATA/menus/bg_course.png");
		m_courseSelect = loadImage("DATA/menus/course_select.png");
		m_tracks = loadImage("DATA/menus/track.png");
		m_courseHelp = loadImage("DATA/menus/course_help.tga");
		for ( int i = 0; i < 20; i++ )
		{
			char filename[] = "DATA/menus/orb00.png";
			filename[14] = (i/10)%10 + '0';
			filename[15] = i%10 + '0';
			m_orbs[i] = loadImage(filename);
		}
		m_rings = loadImage("DATA/menus/rings.tga");
		for ( int i = 0; i < 8; i++ )
		{
			char filename[] = "DATA/menus/banner00.png";
			filename[17] = (i/10)%10 + '0';
			filename[18] = i%10 + '0';
			m_cbanners[i] = loadImage(filename);
		}
	}

	timeRemaining = 15999;

	em.playSample(SFX_COURSE_APPEAR);
	gs.loadSong(BGM_DEMO);
	gs.playSong();
}

void mainNonstopLoop(UTIME dt)
{
	// update animation timers
	SUBTRACT_TO_ZERO(fadeAnimTimer, (int)dt);
	SUBTRACT_TO_ZERO(scrollAnimationTimer, (int)dt);
	SUBTRACT_TO_ZERO(waggleTimer, (int)dt);
	if ( scrollAnimationTimer == 0 )
	{
		currentlyMovingDirection = 0;
		SUBTRACT_TO_ZERO(bannerAnimationTimer, (int)dt);
	}
	SUBTRACT_TO_ZERO(timeRemaining, dt);
	playTimeLowSFX(dt);

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
			currentlyMovingDirection = -1;
			scrollAnimationTimer = SCROLL_ANIM_LENGTH;
			bannerAnimationTimer = BANNER_ANIM_LENGTH;
			currentCourseIndex = currentCourseIndex <= 0 ? 6 : currentCourseIndex - 1;
			em.playSample(SFX_COURSE_PREVIEW_LOAD);
		}
		if ( (im.isKeyDown(MENU_RIGHT_1P) || im.isKeyDown(MENU_RIGHT_2P)) && currentlyMovingDirection == 0 )
		{
			currentlyMovingDirection = 1;
			scrollAnimationTimer = SCROLL_ANIM_LENGTH;
			bannerAnimationTimer = BANNER_ANIM_LENGTH;
			currentCourseIndex = currentCourseIndex > 5 ? 0 : currentCourseIndex + 1;
			em.playSample(SFX_COURSE_PREVIEW_LOAD);

			// special way to select hidden "course 8"		
			if ( timeRemaining >= 3000 && timeRemaining <= 4250 && currentCourseIndex == 0 )
			{
				currentCourseIndex = 7;
			}
		}

		// waggle the blues for difficulty
		// P1 waggle controls
		bool hitAnyBlueSensor = im.getKeyState(LL_1P) == JUST_DOWN || im.getKeyState(LR_1P) == JUST_DOWN || im.getKeyState(LL_2P) == JUST_DOWN || im.getKeyState(LR_2P) == JUST_DOWN;
		if ( hitAnyBlueSensor && currentlyMovingDirection == 0 )
		{
			waggleTimer += WAGGLE_STEP;
			if ( waggleTimer > WAGGLE_TIME )
			{
				waggleTimer = 0;
				em.playSample(SFX_DIFFICULTY_MOVE);
				difficulty = difficulty == SINGLE_MILD ? SINGLE_WILD : SINGLE_MILD;
			}
			else
			{
				em.playSample(SFX_NAME_CURSOR_WAGGLE);
			}
		}
	}

	// render the background
	blit(m_background, rm.m_backbuf, 0, 0, 0, 0, 640, 460);
	masked_blit(m_courseSelect, rm.m_backbuf, 0, 0, 250+(fadeAnimTimer/3), 110, 384, 34);
	masked_blit(m_tracks, rm.m_backbuf, 0, 0, 215, 240, 72, 24*gs.numSongsPerSet);
	if ( currentlyMovingDirection == 0 && fadeAnimTimer == 0 )
	{
		for ( int i = 0; i < gs.numSongsPerSet; i++ )
		{
			renderBoldString(songTitles[songID_to_listID(courseDefinitions[currentCourseIndex][i])].c_str(), 293, 236+(i*24), 400, false, difficulty%10 + 1);
		}
	}

	// render the banner
	int percent = bannerAnimationTimer*100 / BANNER_ANIM_LENGTH;
	masked_stretch_blit(m_cbanners[currentCourseIndex], rm.m_backbuf, 0, 0, 160, 24, 215, 166, getValueFromRange(320,1,percent), getValueFromRange(48,1,percent));

	// render the orbs
	percent = scrollAnimationTimer*100 / SCROLL_ANIM_LENGTH;
	if ( scrollAnimationTimer == 0 )
	{
		masked_blit(m_orbs[currentCourseIndex], rm.m_backbuf, 0, 0, 45, 153, 72, 72);
	}
	else
	{
		if ( currentlyMovingDirection == -1 ) // orbs also move left
		{
			int prevCourse = currentCourseIndex == 6 ? 0 : currentCourseIndex + 1;
			masked_blit(m_orbs[currentCourseIndex], rm.m_backbuf, 0, 0, getValueFromRange(45, 640, percent), 153, 72, 72);
			if ( fadeAnimTimer == 0 )
			{
				masked_blit(m_orbs[prevCourse], rm.m_backbuf, 0, 0, getValueFromRange(-72, 45, percent), 153, 72, 72);
			}
		}
		else // orbs move right
		{
			int prevCourse = currentCourseIndex == 0 ? 6 : currentCourseIndex - 1;
			masked_blit(m_orbs[currentCourseIndex], rm.m_backbuf, 0, 0, getValueFromRange(45, -72, percent), 153, 72, 72);
			if ( fadeAnimTimer == 0 )
			{
				masked_blit(m_orbs[prevCourse], rm.m_backbuf, 0, 0, getValueFromRange(640, 45, percent), 153, 72, 72);
			}
		}
	}

	renderTimeRemaining(5, 42);
	draw_trans_sprite(rm.m_backbuf, m_courseHelp, 0, 460-64);

	if ( fadeAnimTimer > 0 )
	{
		rm.dimScreen(getValueFromRange(0, 100, fadeAnimTimer*100/INTRO_ANIM_LENGTH));
	}
	rm.flip();
}