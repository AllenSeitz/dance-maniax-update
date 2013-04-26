// recordingMode.cpp is a temporary hack designed to quickly allow me to outline charts
// source file created by Allen Seitz 1/5/10

#include "common.h"

#include "gameStateManager.h"
#include "inputManager.h"

extern GameStateManager gs;
extern InputManager im;
extern RenderingManager rm;

int previousKey = 6;
int superPreviousKey = 9;

std::vector<UTIME> timings;
std::vector<int> placements;
std::vector<int> types; // has to be tap, jump, shock, hold start, or NOTE

// hardcoded for simplicity - used to quantize the input
int bpms[10] =     {4901, 19600, 36000,   4901,  19600,  22500,  26000,   4901,  15000 };
int bpmTimes[10] = {   0, 16280, 84000, 106100, 112000, 131800, 167000, 226400, 999999 };
int bpmIndex = 0;
UTIME lastBPMChange = 0;
// 84400 = tempo stop for 475 ms

UTIME calculateQuantizedTime(UTIME ct)
{
	int bpm = bpms[bpmIndex];
	int halfbeat = (6000000 / bpm) / 2; // how many milliseconds are in 1/8 beat

	// calculate the timing of the last half-beat before the current note
	UTIME prevBeat = ((ct - lastBPMChange) / halfbeat) * halfbeat + lastBPMChange;
	UTIME nextBeat = prevBeat + halfbeat;

	// which is closer to the current time? prevBeat or nextBeat?
	if ( ct - prevBeat < nextBeat - ct )
	{
		return prevBeat;
	}
	return nextBeat;
}

int beginTime;

void firstRecordingLoop()
{
	timings.clear();
	placements.clear();
	types.clear();

	gs.player[0].resetAll();
	//gs.loadSong(101);
	bpmIndex = 0;
}

void mainRecordingLoop(UTIME dt)
{
	gs.player[0].timeElapsed += dt;
	im.updateKeyStates(dt);

	// render a little something
	clear_to_color(rm.m_backbuf, 0);
	renderWhiteNumber(gs.player[0].timeElapsed, 100, 100);
	renderWhiteString("NOTES: ", 400, 100);
	renderWhiteNumber((int)timings.size(), 480, 100);
	renderWhiteString("BPM: ", 400, 150);
	renderWhiteNumber(bpms[bpmIndex]/100, 450, 150);
	renderWhiteString("1/8th = ", 400, 170);
	renderWhiteNumber((6000000 / bpms[bpmIndex]) / 2, 490, 170);
	rm.flip();

	// update the current BPM
	while ( bpmTimes[bpmIndex+1] < (int)gs.player[0].timeElapsed && bpmTimes[bpmIndex+1] != 999999 )
	{
		bpmIndex++;

		timings.push_back(bpmTimes[bpmIndex]);
		placements.push_back(bpms[bpmIndex]/100);
		types.push_back(BPM_CHANGE);
	}

	// check input
	for ( int i = 6; i < 10; i++ )
	{
		if ( im.getKeyState(i) == JUST_DOWN )
		{
			int lastIndex = (int)timings.size() - 1;

			// wait! was the previous note a tap, and was it within the jump window?
			if ( lastIndex != -1 && types[lastIndex] == TAP && gs.player[0].timeElapsed - timings[lastIndex] < 50 )
			{
				// change that previous tap into a jump!
				types[lastIndex] = JUMP;
			}
			else
			{
				// decide between SAME, NEXT, RETURN, and XOVER/ACROSS
				int pattern = NEXT;
				if ( i == previousKey )
				{
					pattern = SAME;
				}
				else if ( i == superPreviousKey )
				{
					pattern = RETURN;
				}
				else if ( (i==6 && previousKey==9) || (i==7 && previousKey==8) || (i==8 && previousKey==7) || (i==9 && previousKey==6) )
				{
					pattern = rand()%2 == 0 ? ACROSS : XOVER; // can't tell, but it makes a difference - tweak it manually
				}

				// create a tap
				timings.push_back(calculateQuantizedTime(gs.player[0].timeElapsed));
				placements.push_back(pattern);
				types.push_back(TAP);
			}

			// remember where our feet are
			superPreviousKey = previousKey;
			previousKey = i;
		}
	}

	while (keypressed() == TRUE)
	{
		int k = readkey() >> 8;

		if ( k == KEY_END )
		{
			// create a shock
			timings.push_back(calculateQuantizedTime(gs.player[0].timeElapsed));
			placements.push_back(-1);
			types.push_back(SHOCK);
		}
		if ( k == KEY_SPACE )
		{
			// create a ??? event (I use these to note where I want to put tempo changes, etc)
			timings.push_back(calculateQuantizedTime(gs.player[0].timeElapsed));
			placements.push_back(-1);
			types.push_back(-1);
		}
		if ( k == KEY_ENTER )
		{
			// print everything in a format that I can copy/paste into a hardcoded array -_(>_<)_-
			for ( unsigned int i = 0; i < timings.size(); i++ )
			{
				if ( i != 0 && timings[i] == timings[i-1] )
				{
					continue;
				}

				al_trace("    ");
				if ( timings[i] >= 100000 )
				{
					al_trace(" %d, ", timings[i]);
				}
				else if ( timings[i] >= 10000 )
				{
					al_trace("  %d, ", timings[i]);
				}
				else
				{
					al_trace("   %d, ", timings[i]);
				}

				if ( placements[i] == SAME )
				{
					al_trace("SAME,   ");
				}
				else if ( placements[i] == OVER )
				{
					al_trace("OVER,   ");
				}
				else if ( placements[i] == RETURN )
				{
					al_trace("RETURN, ");
				}
				else if ( placements[i] == XOVER )
				{
					al_trace("XOVER,  ");
				}
				else if ( placements[i] == NEXT )
				{
					al_trace("NEXT,   ");
				}
				else if ( types[i] != BPM_CHANGE )
				{
					al_trace("-1,     ");
				}

				if ( types[i] == TAP )
				{
					al_trace("TAP, ");
				}
				else if ( types[i] == JUMP )
				{
					al_trace("JUMP, ");
				}
				else if ( types[i] == SHOCK )
				{
					al_trace("SHOCK, ");
				}
				else if ( types[i] == BPM_CHANGE )
				{
					al_trace("%d,  BPM_CHANGE,", placements[i]);
				}
				else
				{
					al_trace("WWWWWWWWWW, ");
				}

				al_trace("\n");
			 }
		}
	}
}