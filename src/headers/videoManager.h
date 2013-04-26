// videoManager.h declares a class which implements the background videos
// source file created by Allen Seitz 6/27/2012

#ifndef _VIDEOMANAGER_H_
#define _VIDEOMANAGER_H_

#include "common.h"

// this structure is borrowed from DMX2ja directly and has not been fully reverse engineered
struct MOVIE_SEQ_STEP
{
public:
	short int alpha;    // unsure, always 0
	short int beta;     // flags: always 0, 1, 4, or 7
	char gamma;         // unsure, always 0
	char delta;         // flags: always 0 or 1
	short int epsilon;  //
	short int zeta;     // unsure, always 0
	short int eta;      // unsure, always 0
	short int theta;    // unsure, always 0
	short int omega;    // how to play the video: 1-5
	short int timing;   // timing is in ticks, 300 ticks per second
	char filename[14];  // which file to load

	MOVIE_SEQ_STEP::MOVIE_SEQ_STEP()
	{
		clear();
	}

	void MOVIE_SEQ_STEP::clear()
	{
		alpha = beta = gamma = delta = epsilon = zeta = omega = 0;
		timing = -1;
		memset(filename, 0, 14);
	}
};

class VideoManager
{
public:
	VideoManager::VideoManager();

	void update(UTIME dt, bool skipLoad);
	// precondition: dt is in milliseconds, the video script should already be loaded
	// postcondition: advances the movie script
	// NOTE: skipLoad will prevent loading the next frame from disk, if applicable

	void reset()
	{
		currentTime = currentStep = currentFrame = 0;
		frameData = NULL;
		msPerFrame = msSinceFrame = playDirection = numFrames = 0;
		isStopped = true;
	}

	void play()
	{
		isStopped = false;
	}

	void stop()
	{
		isStopped = true;
	}

	// used by the attract loop for extra long movies with hardcoded lengths
	void alternateSetNumFrames(int frames)
	{
		numFrames = frames;
	}

	void renderToSurface(BITMAP* surface, int x, int y);
	void renderToSurfaceStretched(BITMAP* surface, int x, int y, int width, int height);
	// precondition: surface is 320x192 for the unstreteched version
	// postcondition: surface has been modified

	void loadScript(const char* filename);
	// precondition: file exists on disk, or else movie playback will stop
	// postcondition: this movie will replace any currently playing movie, also calls reset()

private:
	struct MOVIE_SEQ_STEP script[100]; // nothing will ever be larger, probably
	int currentTime;   // counts milliseconds as they pass
	int currentStep;   // which script step we're on
	int currentFrame;  // for the current movie, which frame we're on
	BITMAP* frameData; // the pixel contents of the current frame
	int msPerFrame;    // basically the framerate for the current movie
	int msSinceFrame;  // time since the last frame change
	int playDirection; // which direction to step for the current movie
	int numFrames;     // the number of frames in the currently loaded video
	bool isStopped;    // pause movie playback

	bool haxLowerFramerate; // option for less stressful video loading
	bool haxNoVideos;       // option for computers which just cannot handle it
	BITMAP* m_noVideo;		// used with haxNoVideos

	void loadFrame();
	// precondition: should only be called from within update()
	// postcondition: frameData has been updated from disk or set to NULL

	int calculateNumFrames();
	// precondition: the current movie exists
	// postcondition: returns 75 or 80
};

#endif // end include guard