// resultsMode.cpp implements the main results loop for Dance Maniax
// source file created by Allen Seitz 2/29/2012

#include "common.h"

#include "gameStateManager.h"
#include "inputManager.h"
#include "lightsManager.h"
#include "scoreManager.h"
#include "songwheelMode.h"

extern RenderingManager rm;
extern GameStateManager gs;
extern ScoreManager     sm;
extern LightsManager	lm;
extern InputManager     im;
extern EffectsManager   em;
extern unsigned long int frameCounter;
extern unsigned long int totalGameTime;

extern UTIME timeRemaining;
extern void playTimeLowSFX(UTIME dt);

//////////////////////////////////////////////////////////////////////////////
// Graphics
//////////////////////////////////////////////////////////////////////////////
BITMAP* m_resultsBG = NULL;
BITMAP* m_resultTop = NULL;
BITMAP* m_resultBottom = NULL;
BITMAP* m_clearStatus = NULL;
BITMAP* m_resultSub = NULL;
BITMAP* m_pcStar = NULL;
extern BITMAP** m_banners;


//////////////////////////////////////////////////////////////////////////////
// Variables
//////////////////////////////////////////////////////////////////////////////
int resultFadeTimer = 0;
int secondAnimTimer = 0;
bool doneIntroAnim = false;
int currentPlayer = 0; // will be 1 if showing results for 2P
int scrollX = 64;
int originalX = 64;
int targetScrollX = -1;
UTIME scrollTweenTime = 0;
int stageLimit = 3;

#define INTRO_ANIM_LENGTH 1000


//////////////////////////////////////////////////////////////////////////////
// Functions
//////////////////////////////////////////////////////////////////////////////
void renderResult(int which, int x);
// precondition: which is a stage number
// postcondition: renders graphics to the backbuf

void firstResultsLoop()
{
	// load assets
	if ( m_resultsBG == NULL )
	{
		m_resultsBG = loadImage("DATA/results/results.tga");
		m_resultTop = loadImage("DATA/results/result_high.tga");
		m_resultBottom = loadImage("DATA/results/result_low.tga");
		m_clearStatus = loadImage("DATA/results/clear_status.tga");
		m_resultSub = loadImage("DATA/results/result_sub.tga");
		m_pcStar = loadImage("DATA/results/pc_star.tga");
	}

	resultFadeTimer = secondAnimTimer = 0;
	doneIntroAnim = false;
	currentPlayer = 0;
	scrollX = originalX = 740; // offscreen, gotta scroll in
	targetScrollX = 64;
	scrollTweenTime = 0;
	blit(rm.m_backbuf, rm.m_backbuf1, 0, 0, 0, 0, 640, 480); // prepare for the animation
	blit(rm.m_backbuf, rm.m_backbuf2, 0, 0, 0, 0, 640, 480);

	// calculate how many stages were actually played
	for ( int i = 0; i < 7; i++ )
	{
		if ( sm.player[currentPlayer].currentSet[i].songID < 100 )
		{
			break;
		}
		stageLimit = i-1;
	}

	em.playSample(SFX_RESULTS_APPEAR);
	timeRemaining = 20000;
	lm.loadLampProgram("results.txt");

	im.setCooldownTime(0);
}

void mainResultsLoop(UTIME dt)
{
	resultFadeTimer += dt;
	secondAnimTimer = (secondAnimTimer + dt) % 750;
	if ( !gs.isEventMode && !gs.isFreestyleMode )
	{
		SUBTRACT_TO_ZERO(timeRemaining, dt);
	}
	playTimeLowSFX(dt);

	// intro animation?
	if ( resultFadeTimer >= INTRO_ANIM_LENGTH && !doneIntroAnim )
	{
		resultFadeTimer = 0;
		doneIntroAnim = true;
		if ( gs.currentSong != BGM_RESULT )
		{
			gs.loadSong(BGM_RESULT);
			gs.playSong();
		}

		// the announcer has a few words for you
		int averageScore = 0;
		int numStages = 0;
		for ( int i = 0; i < 7; i++ )
		{
			if ( sm.player[currentPlayer].currentSet[i].songID >= 100 )
			{
				averageScore += sm.player[currentPlayer].currentSet[i].getScore();
				numStages++;
			}
		}
		if ( averageScore > 0 && numStages > 0 )
		{
			averageScore /= numStages;
		}

		if ( averageScore >= 900000 )
		{
			em.announcerQuip(GUY_RESULT_S);
		}
		else if ( averageScore >= 800000 )
		{
			em.announcerQuip(GUY_RESULT_B);
		}
		else if ( averageScore >= 700000 )
		{
			em.announcerQuip(GUY_RESULT_C);
		}
		else if ( averageScore > 400000 )
		{
			em.announcerQuip(GUY_RESULT_D);
		}
		else
		{
			em.announcerQuip(GUY_RESULT_E);
		}
	}
	if ( !doneIntroAnim && resultFadeTimer < (INTRO_ANIM_LENGTH/2) )
	{
		rm.renderWipeAnim(getValueFromRange(0, 14, resultFadeTimer*100/(INTRO_ANIM_LENGTH/2)));
		return;
	}

	blit(m_resultsBG, rm.m_backbuf, 0, 0, 0, 0, 640, 480);
	rectfill(rm.m_backbuf, 0, 460, 640, 480, 0);

	// render the two words 'result' which fade in and out
	if ( resultFadeTimer > 2000 )
	{
		resultFadeTimer -= 2000;
	}
	int topAlpha = getValueFromRange(0, 255, resultFadeTimer * 100 / 1000);
	if ( resultFadeTimer > 1000 )
	{
		topAlpha = getValueFromRange(255, 0, (resultFadeTimer-1000) * 100 / 1000);
	}
	int bottomAlpha = 255 - topAlpha;

	set_trans_blender(0,0,0,topAlpha);
	draw_trans_sprite(rm.m_backbuf, m_resultTop, 216, 2);
	set_trans_blender(0,0,0,bottomAlpha);
	draw_trans_sprite(rm.m_backbuf, m_resultBottom, 0, 402);	
	set_alpha_blender(); // the game assumes the graphics are left in this mode

	renderNameString(sm.player[currentPlayer].displayName, 107, 40, 0);	

	// render the song results
	scrollX = getValueFromRange(targetScrollX, originalX, scrollTweenTime*100/400 ); // quickly slide the results in from the right
	SUBTRACT_TO_ZERO(scrollTweenTime, dt);
	for ( int i = 0; i < 7; i++ )
	{
		renderResult(i, scrollX + (192*i));
	}

	// render the rest of the intro animation
	if ( !doneIntroAnim && resultFadeTimer >= (INTRO_ANIM_LENGTH/2) )
	{
		int fadeTime = resultFadeTimer-(INTRO_ANIM_LENGTH/2);
		rm.dimScreen(getValueFromRange(100, 0, fadeTime*100/(INTRO_ANIM_LENGTH/2)));
	}

	// all done rendering
	if ( doneIntroAnim )
	{
		renderTimeRemaining(5, 36);
	}

	// check for input
	if ( (im.getKeyState(MENU_LEFT_1P) == HELD_DOWN || im.getKeyState(MENU_LEFT_2P) == HELD_DOWN) && scrollTweenTime == 0 && scrollX < 64 )
	{
		scrollTweenTime = 400;
		targetScrollX += 192;
		originalX = scrollX;
		em.playSample(SFX_SONGWHEEL_MOVE);
	}
	if ( (im.getKeyState(MENU_RIGHT_1P) == HELD_DOWN || im.getKeyState(MENU_RIGHT_2P) == HELD_DOWN) && scrollTweenTime == 0 && scrollX > (stageLimit-2)*(-192) )
	{
		scrollTweenTime = 400;
		targetScrollX -= 192;
		originalX = scrollX;		
		em.playSample(SFX_SONGWHEEL_MOVE);
	}
	if ( im.getKeyState(MENU_START_1P) == JUST_DOWN || im.getKeyState(MENU_START_2P) == JUST_DOWN || timeRemaining <= 0 )
	{
		if ( gs.isVersus && currentPlayer == 0 )
		{
			firstResultsLoop(); // reboot the mode, lol
			currentPlayer = 1;
		}
		else
		{
			if ( sm.player[0].isLoggedIn || (gs.isVersus && sm.player[1].isLoggedIn) )
			{
				gs.g_currentGameMode = GAMEOVER;//VOTEMODE;
			}
			else
			{
				gs.g_currentGameMode = GAMEOVER;
			}

			if ( gs.isFreestyleMode )
			{
				gs.player[0].resetAll();
				gs.g_currentGameMode = SONGWHEEL;
			}

			gs.g_gameModeTransition = 1;
		}
	}
}

void renderResult(int which, int x)
{
	static int colors[3] = { makeacol(41, 239, 115, 255), makeacol(247, 41, 173, 255), makeacol(76, 0, 190, 255) };

	if ( sm.player[currentPlayer].currentSet[which].songID < 100 )
	{
		//renderWhiteString("STAGE", x + 20, 100);
		//renderWhiteNumber(which, x, 100);
		return; // it's fine to call this function on every stage. It just won't do anything for the non-stages.
	}

	// song title
	//renderBoldString(songTitles[songID_to_listID(sm.player[currentPlayer].currentSet[which].songID)], x-32, 72, 192, false);
	//renderWhiteString(songTitles[songID_to_listID(sm.player[currentPlayer].currentSet[which].songID)], x-32, 72);

	// status
	int frame = getValueFromRange(0, 10, secondAnimTimer * 100 / 750);
	int statusy = 73; // 357
	switch( sm.player[currentPlayer].currentSet[which].status )
	{
	case STATUS_FULL_PERFECT_COMBO:
	case STATUS_FULL_GREAT_COMBO:
	case STATUS_FULL_GOOD_COMBO:
		masked_blit(m_clearStatus, rm.m_backbuf, 0, frame*32, x-32, statusy, 192, 32);
		break;
	case STATUS_FAILED:
		masked_blit(m_clearStatus, rm.m_backbuf, 0, 352, x-32, statusy, 192, 32);
		break;
	case STATUS_CLEARED:
		masked_blit(m_clearStatus, rm.m_backbuf, 0, 384, x-32, statusy, 192, 32);
		break;
	default:
		break;
	}

	// draw a box around the banner to indicate difficulty
	stretch_blit(m_banners[songID_to_listID(sm.player[currentPlayer].currentSet[which].songID)], rm.m_backbuf, 0, 0, 256, 256, x, 100, 128, 128);
	int level = sm.player[currentPlayer].currentSet[which].chartID % 10;
	rect(rm.m_backbuf, x, 100, x+128, 228, colors[level]);
	rect(rm.m_backbuf, x+1, 101, x+127, 227, colors[level]);

	// counts
	masked_blit(m_resultSub, rm.m_backbuf, 0, 0, x-32, 230, 192, 32);
	renderScoreNumber(sm.player[currentPlayer].currentSet[which].perfects, x-32+108, 230-4, 3);
	if ( sm.player[currentPlayer].currentSet[which].getScore() == 1000000 )
	{
		int star = getValueFromRange(0, 11, secondAnimTimer * 100 / 750);
		masked_blit(m_pcStar, rm.m_backbuf, (star/4)*64, (star%4)*64, x+32, 280, 64, 64);		
	}
	else
	{
		masked_blit(m_resultSub, rm.m_backbuf, 0, 32, x-32, 260, 192, 32);
		renderScoreNumber(sm.player[currentPlayer].currentSet[which].greats, x-32+108, 230+26, 3);

		if ( sm.player[currentPlayer].currentSet[which].goods > 0 || sm.player[currentPlayer].currentSet[which].misses > 0 )
		{
			masked_blit(m_resultSub, rm.m_backbuf, 0, 64, x-32, 290, 192, 32);
			renderScoreNumber(sm.player[currentPlayer].currentSet[which].goods, x-32+108, 230+56, 3);
		}
		if ( sm.player[currentPlayer].currentSet[which].misses > 0 )
		{
			masked_blit(m_resultSub, rm.m_backbuf, 0, 96, x-32, 320, 192, 32);
			renderScoreNumber(sm.player[currentPlayer].currentSet[which].misses, x-32+108, 230+90, 3);
		}
	}

	// score
	renderScoreNumber(sm.player[currentPlayer].currentSet[which].getScore(), x-32+6, 357, 7);
}
