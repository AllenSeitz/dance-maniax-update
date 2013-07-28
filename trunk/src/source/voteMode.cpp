// voteMode.cpp implements the post-game weekly vote screen
// source file created by Allen Seitz 7/21/2013

#include "common.h"
#include "GameStateManager.h"
#include "inputManager.h"
#include "scoreManager.h"
#include "songwheelMode.h"
#include "videoManager.h"

extern GameStateManager gs;
extern RenderingManager rm;
extern InputManager im;
extern ScoreManager sm;
extern VideoManager vm;
extern EffectsManager em;

extern unsigned long int frameCounter;
extern unsigned long int totalGameTime;

extern UTIME timeRemaining;
extern void renderTimeRemaining(int xc, int yc);

extern BITMAP* m_tile;
BITMAP* m_weeklyVote;

void renderVoteLoop();
//
//

void firstVoteLoop()
{
	timeRemaining = 10000;
	gs.loadSong(BGM_DEMO);
	gs.playSong();

	if ( m_tile == NULL )
	{
		m_tile = loadImage("DATA/attract/tile.png");
	}
	if ( m_weeklyVote == NULL )
	{
		m_weeklyVote = loadImage("DATA/vote/weeklyVote.tga");
	}

	// why not?
	em.announcerQuipChance(GUY_SONGWHEEL, 25);
}

void mainVoteLoop(UTIME dt)
{
	SUBTRACT_TO_ZERO(timeRemaining, dt);

//	if ( timeRemaining > 1250 && (im.getKeyState(MENU_START_1P) == JUST_DOWN || im.getKeyState(MENU_START_2P) == JUST_DOWN) )
//	{
//		timeRemaining = 1250; // player says "hurry up I want to start a new game!"
//	}

	if ( timeRemaining <= 0 )
	{
		gs.g_currentGameMode = GAMEOVER;
		gs.g_gameModeTransition = 1;
		gs.killSong();
	}

	renderVoteLoop();
}

void renderVoteLoop()
{
	int scroll = (timeRemaining) % 96;
	for ( int x = 0; x < 9; x++ )
	for ( int y = 0; y < 7; y++ )
	{
		blit(m_tile, rm.m_backbuf, 0, 0, (-96+scroll) + x*96, (-96+scroll) + y*96, 96, 96);
	}
	masked_blit(m_weeklyVote, rm.m_backbuf, 0, 0, 115, 32, 410, 34);

	if ( timeRemaining < 1000 )
	{
		rm.dimScreen(100-(timeRemaining/10));
	}
	if ( timeRemaining > 9700 )
	{
		rm.dimScreen(getValueFromRange(0, 100, (timeRemaining-15700)*100)/300);
	}

	rm.flip();
}