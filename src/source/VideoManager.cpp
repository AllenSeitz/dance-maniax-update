// videoManager.cpp implements a class which implements the background videos
// source file created by Allen Seitz 6/27/2012

#include <string>
#include "videoManager.h"

VideoManager::VideoManager()
{
	haxLowerFramerate = haxNoVideos = false;
	m_noVideo = loadImage("DATA/gameplay/no_video.png");

	if ( fileExists("videofix") )
	{
		haxLowerFramerate = true;
	}
	if ( fileExists("novideo") )
	{
		haxNoVideos = true;
	}
	reset();
}

void VideoManager::update(UTIME dt, bool skipLoad)
{
	bool reloadFrame = false;
	if ( isStopped )
	{
		return;
	}
	currentTime += dt;
	msSinceFrame += dt;

	// check for a frame advance
	if ( msSinceFrame >= msPerFrame && numFrames > 0 )
	{
		currentFrame = playDirection == 1 ? currentFrame+1 : currentFrame-1;
		currentFrame = (currentFrame + numFrames) % numFrames; // implement looping fowards and backwards

		msSinceFrame -= msPerFrame;
		reloadFrame = true;		
	}

	// check for a script step advance
	if ( (script[currentStep+1].timing != -1) && currentTime/10 >= script[currentStep+1].timing/3 ) // because there are 300 ticks per second
	{
		currentStep++;

		msSinceFrame = 0;
		msPerFrame = 33;
		currentFrame = 0;
		playDirection = 1; // foward
		reloadFrame = true;
		numFrames = calculateNumFrames();

		if ( numFrames <= 80 && haxLowerFramerate )
		{
			msPerFrame = 100; // slow it down a bunch. sync be damned
		}
		if ( numFrames <= 80 && haxNoVideos )
		{
			msPerFrame = 99999999; // STOP
		}
	}

	if ( reloadFrame && !skipLoad )
	{
		loadFrame();
	}
}

void VideoManager::renderToSurface(BITMAP* surface, int x, int y)
{
	//renderWhiteNumber(currentTime, 100, 84);
	if ( frameData == NULL )
	{
		renderWhiteString("stop", 100, 52);
		return;
	}
	blit(frameData, surface, 0, 0, x, y, 320, 192);

	//renderWhiteNumber(script[currentStep].beta, 100, 52);
	//renderWhiteNumber(numFrames, 120, 52);
	//renderWhiteNumber(script[currentStep].delta, 200, 52);
	//renderWhiteNumber(script[currentStep].timing/300, 100, 64);
	//renderWhiteString(script[currentStep].filename, 200, 64);
	//renderWhiteNumber(currentTime, 100, 84);
}

void VideoManager::renderToSurfaceStretched(BITMAP* surface, int x, int y, int width, int height)
{
	if ( frameData == NULL )
	{
		return;
	}
	stretch_blit(frameData, surface, 0, 0, 320, 192, x, y, width, height);
}

void VideoManager::loadScript(const char* filename)
{
	reset();
	FILE* fp = NULL;

	if ( fopen_s(&fp, filename, "rb") != 0 )
	{
		return;
	}

	// load script steps until the file is exhausted
	while ( fread(&script[currentStep], sizeof(struct MOVIE_SEQ_STEP), 1, fp) > 0 )
	{
		currentStep++;
	}
	script[currentStep].clear(); // so that I'll know later when the script has ended
	if ( currentStep > 1 && script[2].timing == 0 )
	{
		currentStep = 0; // skip the loading crap (start at 5000 then drop to a lower time) that is present in the original files
	}
	else
	{
		currentStep = 0; // start at the beginning like normal
	}

	fclose(fp);
}

void VideoManager::loadFrame()
{
	// are hax required?
	if ( numFrames <= 80 && haxNoVideos )
	{
		frameData = m_noVideo;
		return;
	}

	// yes this is expensive to do every frame!
	char filename[256] = "MOV/";
	if ( script[currentStep].filename[1] == 0 ) // a simple test for null filenames and "*" filenames
	{
		destroy_bitmap(frameData);
		frameData = NULL;
		return;
	}
	strcat_s(filename, 256, script[currentStep].filename);
	strcat_s(filename, 256, "/000");
	filename[strlen(filename)-3] = (currentFrame/100 % 10) + '0';
	filename[strlen(filename)-2] = (currentFrame/10 % 10) + '0';
	filename[strlen(filename)-1] = (currentFrame/1 % 10) + '0';
	strcat_s(filename, 256, ".png");

	destroy_bitmap(frameData);
	frameData = load_bitmap(filename, NULL);

	if ( frameData == NULL )
	{
		frameData = create_bitmap(320, 192);
		clear_to_color(frameData, makecol(255,255,255));
		textprintf_centre(frameData, font, 160, 90, makecol(0,0,0), "%s", script[currentStep].filename);
	}
}

int VideoManager::calculateNumFrames()
{
	// first check for special videos - with "E_" as the prefix
	if ( _stricmp(script[currentStep].filename, "E_DMX1") == 0 )
	{
		return 255;
	}
	else if ( _stricmp(script[currentStep].filename, "E_TITLE0") == 0 )
	{
		return 450;
	}
	else if ( _stricmp(script[currentStep].filename, "E_HOWTO0") == 0 )
	{
		return 512;
	}
	else if ( _stricmp(script[currentStep].filename, "E_DMX1") == 0 )
	{
		return 255;
	}
	else if ( script[currentStep].filename[0] == 'E' && script[currentStep].filename[2] != 'I' )
	{
		return 302; // the demo loop videos of the girls playing one of four songs
	}
	else if ( script[currentStep].filename[0] == 'J' )
	{
		return 150; // the four videos extracted from the flash memory
	}

	// the video is a generic video, so it will always be 75 or 80 frames
	char filename[256] = "DATA/mov/";
	strcat_s(filename, 256, script[currentStep].filename);
	strcat_s(filename, 256, "/079.png");
	if ( fileExists(filename) )
	{
		return 80;
	}
	return 75;
}