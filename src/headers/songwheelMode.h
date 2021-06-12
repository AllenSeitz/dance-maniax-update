// songwheelMode.h declares functions for the songwheel
// source file created by Allen Seitz 9-17-2011

#ifndef _SONGWHEELMODE_H_
#define _SONGWHEELMODE_H_

#include "../headers/common.h"

//////////////////////////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Primary Functions - these call the mode-specific functions
//////////////////////////////////////////////////////////////////////////////
void firstSongwheelLoop();
//
//

void mainSongwheelLoop(UTIME dt);
//
//

void renderSongwheelLoop();
// precondition: called only once per frame after the logic is complete
// postcondition: rm.m_backbuf contains an image of the current game state

void renderIntroAnim(int percent);
// precondition: called only once per frame when the mode is first initialized
// postcondition: rm.m_backbuf contains a fresh image

//////////////////////////////////////////////////////////////////////////////
// Mode-Specific Functions
//////////////////////////////////////////////////////////////////////////////
void prepareForSongwheelRotation();
//
//

void postSongwheelRotation();
//
//

void updateSongwheelRotation(UTIME dt);
//
//

void renderBanner(int textureID, int coordsIndex, bool isNew);
//
//

void renderPickedSongs(int percent);
// precondition: relies on the GameStateManager stagesPlayed[] array
// postcondition: song banners have been rendered to the top right corner

void renderTitleArea(int percent);
//
//

void renderDifficultyStars(int x, int y, char type, char level);
// precondition: type = mild (0) or wild (1), level = 1-10
// postcondition: renders stars to the backbuf

void renderTimeRemaining(int xcoord = 5, int ycoord = 42);
//
//

void renderLoginStats(int x, int side);
//
//

#endif // end include guard