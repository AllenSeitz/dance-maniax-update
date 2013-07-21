// specialEffects.cpp implements functions to do some special rendering tasks
// file created by Allen Seitz 12-02-09

#include "specialEffects.h"
#include "common.h"
#include "gameplayRendering.h"
#include "gameStateManager.h"
#include "particleSprites.h"

extern RenderingManager rm;
extern GameStateManager gs;

extern int trickBanner1;
extern int trickBanner2;
extern BITMAP* m_banner1;
extern BITMAP* m_banner2;
extern BITMAP* m_statusStars;

void renderLightningBeamHorizontal(int x, int width, int y1, int y2, int seed, int endStyle)
{
	struct AL_POINT dots[5];
	int height = y2 - y1;
	int i = 0;

	ASSERT(height <= 0);
	if ( height <= 0 )
	{
		return;
	}

	// first initialize every point to be a straight horizontal line
	dots[0].x = 0;                                // the 1st dot is automatically at the start
	dots[2].x = seed/32 + 45;                     // the 3rd dot is anywhere 45-72% on the line
	dots[1].x = seed%(dots[2].x-10) + 10;         // the 2nd dot is anywhere 10%-3rd on the line
	dots[3].x = seed%(90-dots[2].x) + dots[2].x;  // the 4th dot is anywhere 3rd-90% on the line
	dots[4].x = 100;                              // the 5th dot is automatically at the end
	for ( i = 0; i < 5; i++ )
	{
		dots[i].x = x + (dots[i].x * width / 100);
		dots[i].y = y1 + height/2;
	}

	// step 2: move dot 2 anywhere
	dots[2].y = seed%(y2-y1) + y1;

	// step 3: move dot 4 as far away from dot 2 as possible, within 16%
	dots[4].y = dots[2].y + (height/4);
	dots[4].y += ((height/2)*(seed/64)/100);
	dots[4].y = dots[4].y > y2 ? dots[4].y - height : dots[4].y;
		
	// step 4a: calculate where dot 4 should be on the line
	dots[3].y = (dots[2].y + dots[4].y)/2;

	// step 4b: move dot 4 10% in either direction, randomly
	if ( seed > 511 )
	{
		dots[3].y -= height/10;
		if ( dots[3].y < y1 )
		{
			dots[3].y += height;
		}
	}
	else
	{
		dots[3].y += height/10;
		if ( dots[3].y > y2 )
		{
			dots[3].y -= height;
		}
	}

	// step 5a: calculate where dot 1 should be on the line
	dots[1].y = (dots[0].y + dots[2].y)/2;

	// step 5b: move dot 1 4-20% in either direction, randomly
	if ( seed > 255 && seed < 768 )
	{
		dots[1].y -= height/(seed%16 + 4);
		if ( dots[1].y < y1 )
		{
			dots[1].y += height;
		}
	}
	else
	{
		dots[1].y += height/(seed%16 + 4);
		if ( dots[1].y > y2 )
		{
			dots[1].y -= height;
		}
	}

	// implement the different end styles
	if ( endStyle == LIGHTNING_END_START )
	{
		dots[4].y = dots[0].y;
	}

	// finally actually render the lines!
	for ( i = 0; i < 4; i++ )
	{
		int neonBlue = 0x8820E0FF;
		int deepBlue = 0x5520F0FF;

		if ( seed % 6 == 0 )
		{
			do_line(rm.m_backbuf, dots[i].x, dots[i].y, dots[i+1].x, dots[i+1].y, neonBlue, drawLightningPixelThick);
		}
		else
		{
			do_line(rm.m_backbuf, dots[i].x, dots[i].y, dots[i+1].x, dots[i+1].y, deepBlue, drawLightningPixel);
		}
	}
}

void renderLightningBeamVertical(int y, int height, int x1, int x2, int seed, int endStyle)
{
	struct AL_POINT dots[4];
	int width = x2 - x1;
	int i = 0;

	ASSERT(width <= 0);
	if ( width <= 0 )
	{
		return;
	}

	// first initialize every point to be a straight vertical line
	dots[0].y = 0;                                // the 1st dot is automatically at the start
	dots[1].y = seed/32 + 45;                     // the 2nd dot is anywhere 45-72% on the line
	dots[2].y = seed%(90-dots[1].y) + dots[1].y;  // the 3rd dot is anywhere 2nd-90% on the line
	dots[3].y = 100;                              // the 4th dot is automatically at the end
	for ( i = 0; i < 4; i++ )
	{
		dots[i].x = x1 + width/2;
		dots[i].y = y + (dots[i].y * height / 100);
	}

	// step 2: move dot 2 anywhere
	dots[1].x = seed%(x2-x1) + x1;

	// step 3: move dot 4 as far away from dot 2 as possible, within 16%
	dots[3].x = dots[1].x + (width/4);
	dots[3].x += ((width/2)*(seed/64)/100);
	dots[3].x = dots[3].x > x2 ? dots[3].x - width : dots[3].x;
		
	// step 4a: calculate where dot 3 should be on the line
	dots[2].x = (dots[1].x + dots[3].x)/2;

	// step 4b: move dot 3 10% in either direction, randomly
	if ( seed > 511 )
	{
		dots[2].x -= width/10;
		if ( dots[2].x < x1 )
		{
			dots[2].x += width;
		}
	}
	else
	{
		dots[2].x += width/10;
		if ( dots[2].x > x2 )
		{
			dots[2].x -= width;
		}
	}

	// implement the different end styles
	if ( endStyle == LIGHTNING_END_START )
	{
		dots[3].x = dots[0].x;
	}

	// finally actually render the lines!
	for ( i = 0; i < 3; i++ )
	{
		drawLightningLine(dots[i].x, dots[i].y, dots[i+1].x, dots[i+1].y, (seed+i)%8);
	}
}

void drawLightningLine(int x1, int y1, int x2, int y2, int brightness)
{
	int innerColor = 0x22FFFFFF;
	int neonBlue   = 0xFF20E0FF;
	//int darkerBlue = 0x442080FF;

	int spread = brightness % 3;

	do_line(rm.m_backbuf, x1+spread, y1+spread, x2+spread, y2+spread, neonBlue, drawLightningPixel);
	do_line(rm.m_backbuf, x1-spread, y1-spread, x2-spread, y2-spread, neonBlue, drawLightningPixel);
	do_line(rm.m_backbuf, x1,   y1,   x2,   y2,   innerColor, drawLightningPixel);	
}

void drawLightningPixelThick(BITMAP *bmp, int x, int y, int color)
{
	for ( int bx = x-1; bx < x+2; bx++ )
	for ( int by = y-1; by < y+2; by++ )
	{
		if ( bx < 0 || bx >= SCREEN_WIDTH || by < 0 || by >= SCREEN_HEIGHT )
		{
			continue;
		}

		// get the pixel at this position
		unsigned long pixel = ((long *)bmp->line[by])[bx];
		int r = (pixel & 0x00FF0000) >> 16;
		int g = (pixel & 0x0000FF00) >> 8;
		int b = (pixel & 0x000000FF);

		// blend it with a neon blue color
		int a = (color & 0xFF000000) >> 24;
		r = WEIGHTED_AVERAGE(((color & 0x00FF0000) >> 16), r, a, 255);
		g = WEIGHTED_AVERAGE(((color & 0x0000FF00) >> 8) , g, a, 255);
		b = WEIGHTED_AVERAGE((color & 0x000000FF)        , b, a, 255);

		long c = (r << 16) + (g << 8) + b;
		((long *)bmp->line[by])[bx] = c;
	}
}

void drawLightningPixel(BITMAP *bmp, int x, int y, int color)
{
	if ( x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT )
	{
		return;
	}

	// get the pixel at this position
	unsigned long pixel = ((long *)bmp->line[y])[x];
	int r = (pixel & 0x00FF0000) >> 16;
	int g = (pixel & 0x0000FF00) >> 8;
	int b = (pixel & 0x000000FF);

	// blend it with the given color
	int a = (color & 0xFF000000) >> 24;
	r = WEIGHTED_AVERAGE(((color & 0x00FF0000) >> 16), r, a, 255);
	g = WEIGHTED_AVERAGE(((color & 0x0000FF00) >> 8) , g, a, 255);
	b = WEIGHTED_AVERAGE((color & 0x000000FF)        , b, a, 255);

	long c = (r << 16) + (g << 8) + b;
	((long *)bmp->line[y])[x] = c;
}

void drawLightningPixelOld(BITMAP *bmp, int x, int y, int brightness)
{
	static int brightnessScale[8] = { 64, 128, 196, 255, 128, 64, 196, 128 };

	if ( x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT )
	{
		return;
	}

	// get the pixel at this position
	unsigned long pixel = ((long *)bmp->line[y])[x];
	int r = (pixel & 0x00FF0000) >> 16;
	int g = (pixel & 0x0000FF00) >> 8;
	int b = (pixel & 0x000000FF);

	// blend it with a neon blue color
	r = WEIGHTED_AVERAGE(32, r, brightnessScale[brightness], 256);
	g = WEIGHTED_AVERAGE(235, g, brightnessScale[brightness], 256);
	b = WEIGHTED_AVERAGE(255, b, brightnessScale[brightness], 256);

	long c = (r << 16) + (g << 8) + b;
	((long *)bmp->line[y])[x] = c;
}

void createFullComboParticles(int player, int type)
{
	struct PARTICLE_INFO a;
	struct PARTICLE_INFO b;

	if ( m_statusStars == NULL )
	{
		m_statusStars = loadImage("DATA/songwheel/status_stars.tga");
	}

	a.texture = b.texture = m_statusStars;
	a.pwidth = b.pwidth = 40;
	a.pheight = b.pheight = 32;

	int numParticlesToMake = 20;
	int bcount = 5;

	switch (type)
	{
	case 0: // good full combo
		a.textureOffsetX = 0;
		a.textureOffsetY = 0; // blue
		b.textureOffsetX = 0;
		b.textureOffsetY = 64; // silver
		break;
	case 1: // great full combo
		a.textureOffsetX = 0;
		a.textureOffsetY = 32; // green
		b.textureOffsetX = 0;
		b.textureOffsetY = 96; // gold
		numParticlesToMake = 30;
		break;
	case 2: // perfect full combo
		a.textureOffsetX = 0;
		a.textureOffsetY = 96; // gold
		b.textureOffsetX = 0;
		b.textureOffsetY = 128; // big gold
		numParticlesToMake = 50;
		bcount = 2;
		break;
	}

	//al_trace("center of lane X: %d\r\n", getCenterOfLanesX(player));
	for ( int i = 0; i < numParticlesToMake; i++ )
	{
		a.x = b.x = getCenterOfLanesX(player) + getValueFromRange(-32, +32, rand()%100) - 20; // -20 is to center the particle
		a.y = b.y = -32 - (i*8);
		a.xvel = b.xvel = getValueFromRange(-50, 50, rand()%100);
		a.yvel = b.yvel = getValueFromRange(350, 650, rand()%100);
		a.timeToLive = b.timeToLive = 5000;

		addParticle( i % bcount == 0 ? b : a);
	}
}