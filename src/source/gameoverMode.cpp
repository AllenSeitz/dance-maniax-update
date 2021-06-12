// gameoverMode.cpp implements the transition between the results screen and the attract loop
// source file created by Allen Seitz 6/15/2012

#include "../headers/common.h"
#include "../headers/GameStateManager.h"
#include "../headers/inputManager.h"
#include "../headers/scoreManager.h"
#include "../headers/songwheelMode.h"
#include "../headers/videoManager.h"

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

bool isSpecialGameOver = false;

void renderGameoverLoop();
//
//

void firstGameoverLoop()
{
	timeRemaining = 16000;
	isSpecialGameOver = false;
	gs.loadSong(BGM_GAMEOVER);
	gs.playSong();
	vm.loadScript("DATA/mov/gameover_0.seq");
	if ( sm.player[0].currentSet[gs.numSongsPerSet].songID > 0 ) // got any extra stage at all
	{
		vm.loadScript("DATA/mov/gameover_1.seq");
		em.announcerQuipChance(GUY_SPECIAL_GAMEOVER, 25);
		isSpecialGameOver = true;
	}
	vm.play();
}

void mainGameoverLoop(UTIME dt)
{
	SUBTRACT_TO_ZERO(timeRemaining, dt);

	if ( timeRemaining > 1250 && (im.getKeyState(MENU_START_1P) == JUST_DOWN || im.getKeyState(MENU_START_2P) == JUST_DOWN) )
	{
		timeRemaining = 1250; // player says "hurry up I want to start a new game!"
	}

	if ( timeRemaining <= 0 )
	{
		gs.g_currentGameMode = ATTRACT;
		gs.g_gameModeTransition = 1;
		gs.killSong();
	}

	renderGameoverLoop();
}

void renderGameoverLoop()
{
	rectfill(rm.m_backbuf, 0, 0, 640, 480, makeacol(0, 0, 0, 255));

	vm.renderToSurface(rm.m_backbuf, 0, 0);
	vm.renderToSurface(rm.m_backbuf, 320, 288);
	rectfill(rm.m_backbuf, 0, 192, 640, 288, makeacol(255,255,255,255));

	renderArtistString("GAME OVER", 107, 240+135, 600, 40);
	renderArtistString("DANCE MANIAX", 320+88, 86, 600, 40);
	renderBoldString(isSpecialGameOver ? "YOU ARE A SUPER PLAYER" : "THANK YOU FOR PLAYING!", 187, 227, 640, false, (timeRemaining/200)%3 + 1);

	if ( timeRemaining < 1000 )
	{
		rm.dimScreen(100-(timeRemaining/10));
	}
	if ( timeRemaining > 15700 )
	{
		rm.dimScreen(getValueFromRange(0, 100, (timeRemaining-15700)*100)/300);
	}
}