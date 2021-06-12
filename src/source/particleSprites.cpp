// particleSprites.cpp implements a basic particle system
// file created by Allen Seitz July 7th 2013

#include <vector>

#include "../headers/particleSprites.h"

std::vector<struct PARTICLE_INFO> mainParticles;

void addParticle(struct PARTICLE_INFO &newp)
{
	mainParticles.push_back(newp);
}

void updateParticles(int dt)
{
	std::vector<struct PARTICLE_INFO> livingParticles;

	//al_trace("Num particles: %d\r\n", mainParticles.size());
	for ( size_t i = 0; i < mainParticles.size(); i++ )
	{
		mainParticles[i].timeToLive -= dt;

		if ( mainParticles[i].timeToLive > 0 )
		{
			mainParticles[i].x += mainParticles[i].xvel * dt/1000;
			mainParticles[i].y += mainParticles[i].yvel * dt/1000;
			livingParticles.push_back(mainParticles[i]);
		}
	}

	killAllParticles();
	mainParticles = livingParticles;
}

void renderParticles(BITMAP* target)
{
	for ( size_t i = 0; i < mainParticles.size(); i++ )
	{
		masked_blit(mainParticles[i].texture, target, mainParticles[i].textureOffsetX, mainParticles[i].textureOffsetY, 
			(int)mainParticles[i].x, (int)mainParticles[i].y, mainParticles[i].pwidth, mainParticles[i].pheight);
	}
}

void killAllParticles()
{
	mainParticles.clear();
}
