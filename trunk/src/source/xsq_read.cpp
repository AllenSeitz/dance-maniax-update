// xsq_read.cpp reads a .xsq file, which is my memory dump of a chart from MAME (for chart data from the original series)
// source file created 6/28/2013 by Catastrophe
// updated 9/21/2013 to try to fix the gap

#include <algorithm>
#include <vector>
#include <stdio.h>

#include "common.h"

// the chart in memory is a sequence of these. I'm not sure what each field is for
struct XSQ_RECORD
{
	int timing;
	int word2; // linked list node pointer? timing in frames? distance to next note?
	int column;
	int word4; // always 0x0000FFFF
	int word5; // always zero

	XSQ_RECORD()
	{
		reset();
	}
	void reset()
	{
		timing = column = word2 = word4 = word5 = 0;
	}
	bool isFinalNote()
	{
		return column == 0xFFFF0000 && word4 == 0xFFFF0000 && word5 == 0; // don't question it
	}
};

int getColumn_XSQ(long ch, char which);
// precondition: ch is the bitfield from the XSQ_RECORD, and which is 0-3 for "how many simultaneous notes"
// postcondition: returns 0-7, or -1 for "no more notes here"

int getColumn_XSQ(long column, char which)
{
	if ( (column & 0xFFFE0000) == 0xFFFE0000 ) // 0xFE might begin a chart, and 0xFF might end one
	{
		return -1;
	}
	if ( column & 0x00100000 )
	{
		if ( which == 0 )
			return 3;
		else
			which--;
	}
	if ( column & 0x00200000 )
	{
		if ( which == 0 )
			return 2;
		else
			which--;
	}
	if ( column & 0x00400000 )
	{
		if ( which == 0 )
			return 1;
		else
			which--;
	}
	if ( column & 0x00800000 )
	{
		if ( which == 0 )
			return 0;
		else
			which--;
	}

	if ( column & 0x00010000 )
	{
		if ( which == 0 )
			return 7;
		else
			which--;
	}
	if ( column & 0x00020000 )
	{
		if ( which == 0 )
			return 6;
		else
			which--;
	}
	if ( column & 0x00040000 )
	{
		if ( which == 0 )
			return 5;
		else
			which--;
	}
	if ( column & 0x00080000 )
	{
		if ( which == 0 )
			return 4;
		else
			which--;
	}

	return -1;
}

// This function is really stupid, and I'm sorry.
// There are three songs in official DMX with significant tempo changes:
// -- 110, Heaven is a '57, 190 ->  95 -> 190
// -- 126, boss nonstop #1, 138 -> 130 -> 200
// -- 230, boss nonstop #2, 180 -> 190
// There are also two songs which have large number of tempo changes:
// -- 227, I Will Follow Him, (8) tempo changes after the last note
// -- 231, ABSOLUTE, (6) tempo changes mostly for the last note
// Finally these songs have 'invisible' tempo changes:
// -- 104, Ain't it Good
// -- 202, Virtual Mind
// -- 213, Stay (organic house version)
// -- 226, Matsuri (J-Summer Mix)
// The problem I'm having is that I can detect when a tempo change happens in the chart data, but the
// byte that I'm provided makes no sense. It is a different value for different charts of the same song,
// although it at least appears reliably at the same expected time. Given that its value only increases
// for songs with multiple tempo changes I think it might be an index into another table. However, I
// can't find that other table. But I do know which value I'm expecting to see, so I provide that instead.
int hackGuessBPM(int songID, int currentTime)
{
	switch (songID)
	{
		// trivial cases
	case 104: return 134; // aint it good
	case 202: return 128; // virtual mind
	case 213: return 125; // stay1
	case 226: return 180; // matsuri

		// gimmick slowdown endings - skipped
	case 227: return 148; // i will follow him
	case 231: return 140; // absolute

		// songs with meaningful scroll rate changes
	case 110:
		if ( currentTime == 24258 )
			return 95;
		else
			return 190;
	case 126:
		if ( currentTime == 11224 )
			return 130;//138;
		else if ( currentTime == 11375 )
			return 130;
		else if ( currentTime == 23694 ) // don't know exactly what's up with this one
			return 130;
		else if ( currentTime == 23834 )
			return 200;
		else
			return 200;
	case 230: // boss 2 and all of its edits have exactly one tempo change, fortunately
	case 245:
	case 246:
	case 247:
	case 248:
	case 249:
	case 250:
		return 190;
	}
	return 150;
}

int readXSQ(std::vector<struct ARROW> *chart, std::vector<struct FREEZE> *holds, int songID, int chartType)
{
	char filename[] = "DATA/xsq/101_sm.xsq";
	FILE* fp = NULL;

	// hopefully open the target XSQ file
	filename[ 9] = (songID/100 % 10) + '0';
	filename[10] = (songID/10 % 10) + '0';
	filename[11] = (songID % 10) + '0';
	filename[13] = chartType <= SINGLE_ANOTHER ? 's' : 'd';
	filename[14] = chartType == SINGLE_MILD || chartType == DOUBLE_MILD ? 'm' : 'w';

	if ( fopen_s(&fp, filename, "rb") != 0 )
	{
		char helpString[64] = "song 0000 is missing chart data (xsq)";
		helpString[5] = (songID / 1000 ) % 10 + '0';
		helpString[6] = (songID / 100 ) % 10 + '0';
		helpString[7] = (songID / 10 ) % 10 + '0';	
		helpString[8] = (songID / 1 ) % 10 + '0';
		globalError(SONG_DATA_INCOMPLETE, helpString);
		return 0;
	}

	chart->clear();
	holds->clear();

	// skip some weird header stuff that I don't understand yet? get right to the good stuff
	char songCode[8] = "";
	char skip = 0;
	long tempoMarkerThingy = 0;
	long displayBPM = 0;
	long gap = 0; // always!

	fread_s(&songCode, sizeof(char)*8, sizeof(char), 8, fp);
	fread_s(&tempoMarkerThingy, sizeof(long), sizeof(long), 1, fp);
	fread_s(&displayBPM, sizeof(long), sizeof(long), 1, fp);
	//fread_s(&skip, sizeof(char), sizeof(char), 1, fp);
	//fread_s(&gap, sizeof(long), sizeof(long), 1, fp);
	for ( int i = 0; i < 160; i++ )
	{
		fread_s(&skip, sizeof(char), sizeof(char), 1, fp);
	}

	// start with this
	struct ARROW first;
	first.timing = 0;
	first.type = BPM_CHANGE;
	first.color = displayBPM;
	chart->push_back(first);

	struct XSQ_RECORD temp;
	int numLoops = 0; // for an odd special case
	int timePerBeat = BPM_TO_MSEC(displayBPM); // used to calculate the arrow color, even though DMX is flat

	while ( 1 )
	{
		numLoops++;
		size_t readlen = fread_s(&temp, sizeof(struct XSQ_RECORD), sizeof(struct XSQ_RECORD), 1, fp);
		if ( readlen == 0 )
		{
			allegro_message("Potential problem with %s", filename);
			break;
		}
		temp.timing += gap; // zero anyways

		// I don't understand this, but it is needed for some reason (remember, these are memory dumps)
		if ( numLoops == 2 )
		{
			if ( strlen(songCode) <= 4 )
			{
				temp.word2 = temp.column;
				temp.column = temp.word4;
				temp.word4 = temp.word5;
				fread_s(&temp.word5, sizeof(long), sizeof(long), 1, fp); // should still be zero anyways, like always			
			}
			else
			{
				// this extra byte in between timing and word2 is extra weird, but then again the song code here ('boss2', 'stay2', 'cbos0') is one byte longer?
				temp.word2 = 0;                // unused
				temp.column = temp.word4 >> 8; // it never matters to worry about the 'carry'
				temp.word4 = 0xFFFF0000;       // unused
				temp.word5 = 0;                // unused
				fread_s(&skip, sizeof(char), sizeof(char), 1, fp);       // songs with an extra letter move everything else one byte back ("boss2", "stay2", "cbos1")
				fread_s(&temp.word5, sizeof(long), sizeof(long), 1, fp); // should still be zero anyways, like always			
			}
		}

		//al_trace("%d, %d, %d, %d, %d\r\n", temp.timing, temp.word2, temp.column, temp.word4, temp.word5);

		int experimental = (temp.column & 0x000000FF);
		if ( experimental != 0 )
		{
			if ( (temp.column & 0xFFFE0000) == 0xFFFE0000 ) // 0xFE might begin a chart, and 0xFF might end one
			{
				//al_trace("Start chart: %u\n", experimental);
			}
			else
			{
				//al_trace("Found something: %x\n", temp.column); //experimental);
				//al_trace("%d, %d, %x\n", songID, temp.timing, temp.column);

				// make a tempo change event
				struct ARROW a;
				uint64_t ms = temp.timing; // because this next operation just barely overflows 32 bits, temporarily
				ms = ms * 333333;
				ms = ms / 100000; // charts are at 18fps, so multiplly by 3.333333
				a.timing = ms;

				a.color = hackGuessBPM(songID, temp.timing);
				a.type = BPM_CHANGE;
			
				chart->push_back(a);
			}
		}

		for ( int i = 0; i < 4; i++ )
		{
			int col = getColumn_XSQ(temp.column, i);
			if ( col != -1 ) 
			{
				struct ARROW a;

				//a.timing = temp.timing * 60;
				//a.timing = temp.timing * 350 / 100; // seems to be off by +1 frame on each successive note
				// NOTE: word2 is exactly 15.234121810393 times larger than 'timing' (word1)
				//a.timing = temp.timing * 338688 / 100000;

				uint64_t ms = temp.timing; // because this next operation just barely overflows 32 bits, temporarily
				ms = ms * 333333;
				ms = ms / 100000; // charts are at 18fps, so multiplly by 3.333333
				a.timing = ms;

				a.color = calculateArrowColor(a.timing, timePerBeat);
				a.type = TAP;
				a.columns[0] = col;
				a.judgement = UNSET;
				chart->push_back(a);
			}
		}

		if ( temp.isFinalNote() )
		{
			break; // happy ending
		}
	}

	// always put one of these in the chart
	struct ARROW a;
	a.timing = chart->at(chart->size() - 1).timing + 3000;
	a.type = END_SONG;
	chart->push_back(a);

	fclose(fp);

	// count the maximum score that this chart is worth. it is needed while the chart is being played
	int maxScore = 0;
	for ( size_t i = 0; i < chart->size(); i++ )
	{
		if ( chart->at(i).type == TAP || chart->at(i).type == JUMP )
		{
			maxScore += 2;
		}
	}
	maxScore += holds->size()*2;
	return maxScore;
}

int readXSQ2P(std::vector<struct ARROW> *chart, std::vector<struct FREEZE> *holds, int songID, int chartType)
{
	int retval = readXSQ(chart, holds, songID, chartType);

	// transform the chart
	for ( std::vector<struct ARROW>::iterator c = chart->begin(); c != chart->end(); c++ )
	{
		for ( int i = 0; i < 4; i++ )
		{
			if ( c->columns[i] >= 0 && c->columns[i] < 4 )
			{
				c->columns[i] += 4;
			}
		}
	}

	// just for completeness - but official charts have no holds
	for ( std::vector<struct FREEZE>::iterator f = holds->begin(); f != holds->end(); f++ )
	{
		for ( int i = 0; i < 2; i++ )
		{
			if ( f->columns[i] >= 0 && f->columns[i] < 4 )
			{
				f->columns[i] += 4;
			}
		}
	}

	return retval;
}

int readXSQCenter(std::vector<struct ARROW> *chart, std::vector<struct FREEZE> *holds, int songID, int chartType)
{
	int retval = readXSQ(chart, holds, songID, chartType);

	// transform the chart
	for ( std::vector<struct ARROW>::iterator c = chart->begin(); c != chart->end(); c++ )
	{
		for ( int i = 0; i < 4; i++ )
		{
			switch ( c->columns[i] )
			{
			case 0: c->columns[i] = 3; break;
			case 1: c->columns[i] = 2; break;
			case 2: c->columns[i] = 5; break;
			case 3: c->columns[i] = 4; break;
			default:
				break;
			}
		}
	}

	// just for completeness - but official charts have no holds
	for ( std::vector<struct FREEZE>::iterator f = holds->begin(); f != holds->end(); f++ )
	{
		for ( int i = 0; i < 2; i++ )
		{
			switch ( f->columns[i] )
			{
			case 0: f->columns[i] = 3; break;
			case 1: f->columns[i] = 2; break;
			case 2: f->columns[i] = 5; break;
			case 3: f->columns[i] = 4; break;
			default:
				break;
			}
		}
	}

	return retval;
}

