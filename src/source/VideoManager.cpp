// videoManager.cpp implements a class which implements the background videos
// source file created by Allen Seitz 6/27/2012

#include <string>
#include "../headers/videoManager.h"

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
	apeg_ignore_audio(true);

	frameData = create_bitmap_ex(32, 320, 192);
	clear_to_color(frameData, 0);
	videoBuffer = NULL;
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
	// check for the movie ending and loop the whole thing after 5 seconds
	if ( script[currentStep+1].timing == -1 && currentStep > 1 && (currentTime/10 + 1500 >= script[currentStep].timing/3) )
	{
		currentStep = 1;
		currentTime = 0;
		al_trace("Restarting video script.\r\n");
		loadVideoAtCurrentStep();
	}

	if ( cmov != NULL )
	{
		if ( apeg_advance_stream(cmov, true) != APEG_OK)
		{
			al_trace("Video problem! Breakpoint!\r\n"); // doesn't really matter if it fails
		}
		if( cmov->frame_updated > 0 && cmov->bitmap != NULL )
		{
			//stretch_blit(cmov->bitmap, frameData, 0, 0, cmov->w, cmov->h, 0, 0, 320, 192);
			blit(cmov->bitmap, frameData, 0, 0, 0, 0, 320, 192);
		}
	}
}

void VideoManager::renderToSurface(BITMAP* surface, int x, int y)
{
	//renderWhiteNumber(currentTime, 100, 84);
	if ( frameData == NULL )
	{
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

	// in the original game step 1 was always one of the four J_ movies
	// these were stored on the onboard flash for speed
	// they would play for the first 1-5 seconds while the CD-ROM streamed the mp3
	// the play length was listed as 0, since the videos on disc would resume as soon as the CD-ROM was ready
	// I could omit the J_ step entirely, since it is no longer needed to mask load times anywmore
	// but those videos are part of the game, so I show them for 1500ms, then switch to the real script
	if ( currentStep > 1 && script[2].timing == 0 )
	{
		script[1].timing = 0; // should be overwriting 1500
		script[2].timing = 1500; // should be overwriting 0
	}

	currentStep = 1;
	isStopped = false;

	fclose(fp);
	loadVideoAtCurrentStep();
}

void VideoManager::loadVideoAtCurrentStep()
{
	char filename[256] = "DATA/video/";
	if ( script[currentStep].filename[0] == '*' )
	{
		strcat_s(filename, 256, script[currentStep-1].filename); // check for videos named '*'. I think the original game used this to denote "play that video again"
	}
	else
	{
		strcat_s(filename, 256, script[currentStep].filename);
	}
	strcat_s(filename, 256, ".ogg");

	// default case: show a placeholder texture
	unloadVideo();
	clear_to_color(frameData, makecol(255,255,255));
	textprintf_centre(frameData, font, 160, 90, makecol(0,0,0), "%s", script[currentStep].filename);

	loadVideo(filename);
	if ( cmov == NULL )
	{
		al_trace("Movie %s did not open for whatever reason.\r\n", filename);
		return;
	}

	if ( apeg_advance_stream(cmov, true) != APEG_OK)
	{
		al_trace("Video problem! Breakpoint!\r\n"); // doesn't really matter if it fails
	}
	blit(cmov->bitmap, frameData, 0, 0, 0, 0, 320, 192);
}

void VideoManager::loadVideo(char* filename)
{
	FILE* fp = NULL;
	if ( fopen_s(&fp, filename, "rb") != 0 )
	{
		al_trace("Movie %s is missing.\r\n", filename);
		return; // do NOT summon error mode for this because the application might in debugging/lite mode
	}

	// get the length of the video
	fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	// load the entire video into an unreasonably large memory buffer!
	videoBuffer = malloc(fsize);
	fread(videoBuffer, fsize, 1, fp);
	fclose(fp);

	// sanity check the buffer contents or else APEG may hang
	if ( ((char*)videoBuffer)[0] == 'O' && ((char*)videoBuffer)[1] == 'g' && ((char*)videoBuffer)[2] == 'g' && ((char*)videoBuffer)[3] == 'S' )
	{
		cmov = apeg_open_memory_stream(videoBuffer, fsize);
	}
	else
	{
		al_trace("Video stream did not seem to contain Ogg data.\r\n");
		unloadVideo();
	}
}

void VideoManager::unloadVideo()
{
	if ( cmov != NULL )
	{
		apeg_reset_stream(cmov);
		apeg_close_stream(cmov);
	}
	if ( videoBuffer != NULL )
	{
		free(videoBuffer);
	}
	videoBuffer = NULL;
	cmov = NULL;
}