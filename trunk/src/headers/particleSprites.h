// particleSprites.h implements a basic particle system
// file created by Allen Seitz July 7th 2013

#ifndef _PARTICLESPRITES_H_
#define _PARTICLESPRITES_H_

#pragma warning(disable : 4312)
#include <allegro.h>			// allegro library
#pragma warning(default : 4312)

struct PARTICLE_INFO
{
	BITMAP* texture;
	int textureOffsetX;
	int textureOffsetY;
	int pwidth;
	int pheight;

	float x;
	float y;
	float xvel;
	float yvel;

	int timeToLive;

	PARTICLE_INFO::PARTICLE_INFO()
	{
		texture = NULL;
		textureOffsetX = textureOffsetY = 0;
		pwidth = pheight = 1;
		x = y = 0;
		xvel = yvel = 0;
		timeToLive = 0;
	}
};

void addParticle(struct PARTICLE_INFO &newp);
// precondition: don't do anything dumb with the parameter
// postcondition: this particle will be added to the GLOBAL list. Now please call update() whenever.

void updateParticles(int dt);
// precondition: milliseconds this frame
// postcondition: the particles will be moved or killed

void renderParticles(BITMAP* target);
// precondition: target is a back buffer
// postcondition: particles (if any) have been rendered

void killAllParticles();
// postcondition: resets the particle engine back ot its original state

#endif // end include guard