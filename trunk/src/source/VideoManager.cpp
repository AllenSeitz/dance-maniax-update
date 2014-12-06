// videoManager.cpp implements a class which implements the background videos
// source file created by Allen Seitz 6/27/2012

#include <string>
#include "videoManager.h"

// include ffmpeg
//#include <libavcodec/avcodec.h>
//#include <libavformat/avformat.h>

// include apeg
#include <apeg.h>

VideoManager::VideoManager()
{
	haxLowerFramerate = haxNoVideos = false;
	m_noVideo = loadImage("DATA/gameplay/no_video.png");

	if ( fileExists("novideo") )
	{
		haxNoVideos = true;
	}
}

void VideoManager::initialize()
{
	reset();
	//apeg_ignore_audio(true);

	frameData = create_bitmap_ex(32, 320, 192);
	clear_to_color(frameData, 0);
}

void VideoManager::update(UTIME dt)
{
	if ( isStopped || haxNoVideos )
	{
		return;
	}
	currentTime += dt;

	// check for a script step advance
	if ( (script[currentStep+1].timing != -1) && currentTime/10 >= script[currentStep+1].timing/3 ) // because there are 300 ticks per second
	{
		currentStep++;
		loadVideoAtCurrentStep();
	}

	if ( apeg_advance_stream(cmov, true) != APEG_OK)
	{
		al_trace("Problem!\r\n"); // doesn't really matter if it fails
	}

	if( cmov->frame_updated > 0 && cmov->bitmap != NULL )
	{
		stretch_blit(cmov->bitmap, frameData, 0, 0, cmov->w, cmov->h, 0, 0, 320, 192);
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
		globalError(MISSING_VIDEO_SCRIPT, filename);
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
	isStopped = false;

	fclose(fp);
	loadVideoAtCurrentStep();
}

void VideoManager::loadVideoAtCurrentStep()
{
	char filename[256] = "MOV/";
	strcat_s(filename, 256, script[currentStep].filename);
	strcat_s(filename, 256, ".ogg");

	strcpy_s(filename, 256, "MOV/N_OPUTY0.ogg");

	cmov = apeg_open_stream(filename);
}