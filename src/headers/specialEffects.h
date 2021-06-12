// specialEffects.h implements functions to do some special rendering tasks
// file created by Allen Seitz 12-02-09

#ifndef _SPECIALEFFECTS_H_
#define _SPECIALEFFECTS_H_

#include "../headers/common.h"

void renderLightningBeamHorizontal(int x, int width, int y1, int y2, int seed, int endStyle);
void renderLightningBeamVertical(int y, int height, int x1, int x2, int seed, int endStyle);
// precondition: seed is [0..1023], x1 < x2, and y1 < y2, endstyle is an enum
// postcondition: a lightning bolt that will fill the area is rendered to the backbuf

#define LIGHTNING_END_START 0
#define LIGHTNING_END_ANY   1
#define LIGHTNING END_FORK  2

void drawLightningLine(int x1, int y1, int x2, int y2, int brightness);
// precondition: called only as a helper function by another 'lightning' function
// postcondition: does the work of rendering pixels to the backbuf

void drawLightningPixel(BITMAP *bmp, int x, int y, int color);
void drawLightningPixelThick(BITMAP *bmp, int x, int y, int color);
void drawLightningPixelOld(BITMAP *bmp, int x, int y, int brightness);
// precondition: called only as a helper function by drawLightningLine
// postcondition: actually does the work of rendering pixels to the backbuf

void renderBannerAnim();
// precondition: the next banner is already determined
// postcondition: renders the next step of the banner animation

void createFullComboParticles(int player, int type);
// precondition: type = 0 for good combo, 1 = great combo, 2 = perfect combo
// postcondition: creates the particles just offscreen at (x, 0)

#endif // end include guard