// xsq_read.cpp reads a .xsq file, which is a memory dump of a chart from MAME (for official simfiles)
// source file created 6/28/2013 by Catastrophe

#include <algorithm>
#include <vector>
#include <stdio.h>

#include "common.h"

// first the chart is read in as a series of these, then sorted, then the beats are turned into milliseconds (and a real chart)
struct XSQ_RECORD
{
	int timing;
	int column;
	int word3;
	int word4;
	int word5;

	XSQ_RECORD()
	{
		reset();
	}
	void reset()
	{
		timing = column = word3 = word4 = word5 = 0;
	}
	bool isFinalNote()
	{
		return column == 0x00FFFF00 && word3 == 0x00FFFF00 && word4 == 0 && word5 == 0; // don't question it
	}
};

int getColumn_XSQ(long ch, char which);
// precondition: ch is the bitfield from the XSQ_RECORD, and which is 0-3 for "how many simultaneous notes"
// postcondition: returns 0-7, or -1 for "no more notes here"

int getColumn_XSQ(long column, char which)
{
	if ( column & 0x00001000 )
	{
		if ( which == 0 )
			return 3;
		else
			which--;
	}
	if ( column & 0x00002000 )
	{
		if ( which == 0 )
			return 2;
		else
			which--;
	}
	if ( column & 0x00004000 )
	{
		if ( which == 0 )
			return 1;
		else
			which--;
	}
	if ( column & 0x00008000 )
	{
		if ( which == 0 )
			return 0;
		else
			which--;
	}

	if ( column & 0x00000100 )
	{
		if ( which == 0 )
			return 7;
		else
			which--;
	}
	if ( column & 0x00000200 )
	{
		if ( which == 0 )
			return 6;
		else
			which--;
	}
	if ( column & 0x00000400 )
	{
		if ( which == 0 )
			return 5;
		else
			which--;
	}
	if ( column & 0x00000800 )
	{
		if ( which == 0 )
			return 4;
		else
			which--;
	}

	return -1;
}

int readXSQ(std::vector<struct ARROW> *chart, std::vector<struct FREEZE> *holds, int songID, int chartType)
{
	char filename[] = "DATA/dwis/101_sm.xsq";
	FILE* fp = NULL;

	// hopefully open the target DWI file
	filename[10] = (songID/100 % 10) + '0';
	filename[11] = (songID/10 % 10) + '0';
	filename[12] = (songID % 10) + '0';
	filename[14] = chartType <= SINGLE_ANOTHER ? 's' : 'd';
	filename[15] = chartType == SINGLE_MILD || chartType == DOUBLE_MILD ? 'm' : 'w';

	if ( fopen_s(&fp, filename, "rb") != 0 )
	{
		char helpString[64] = "song 0000 is missing chart data";
		helpString[5] = (songID / 1000 ) % 10 + '0';
		helpString[6] = (songID / 100 ) % 10 + '0';
		helpString[7] = (songID / 10 ) % 10 + '0';	
		helpString[8] = (songID / 1 ) % 10 + '0';
		globalError(SONG_DATA_INCOMPLETE, helpString);
		return 0;
	}

	// fix any potential bugs regarding data from one chart accidentally merging into the next chart
	chart->clear();
	holds->clear();

	// skip some weird header stuff that I don't understand yet? get right to the good stuff
	char songCode[8] = "";
	long tempoMarkerThingy = 0;
	long displayBPM = 0;
	long gap = 0;
	long endTime = 5000;

	fread_s(&songCode, sizeof(char)*8, sizeof(char), 8, fp);
	fread_s(&tempoMarkerThingy, sizeof(long), sizeof(long), 1, fp);
	fread_s(&displayBPM, sizeof(long), sizeof(long), 1, fp);
	fgetc(fp);
	fread_s(&gap, sizeof(long), sizeof(long), 1, fp);
	for ( int i = 0; i < 155; i++ )
	{
		fgetc(fp);
	}
	fread_s(&endTime, sizeof(long), sizeof(long), 1, fp);
	for ( int i = 0; i < 21; i++ )
	{
		fgetc(fp);
	}

	//gap = 48;

	struct XSQ_RECORD temp;
	int numLoops = 0; // for debugging

	int timePerBeat = 400; // unused anyway, but in the future when I know the bpm, could be used
	while ( 1 )
	{
		numLoops++;
		size_t readlen = fread_s(&temp, sizeof(struct XSQ_RECORD), sizeof(struct XSQ_RECORD), 1, fp);
		if ( readlen == 0 )
		{
			allegro_message("Potential problem with %s", filename);
			break;
		}
		temp.timing += gap;

		if ( temp.isFinalNote() )
		{
			break; // happy ending
		}

		for ( int i = 0; i < 4; i++ )
		{
			int col = getColumn_XSQ(temp.column, i);
			if ( col != -1 ) 
			{
				struct ARROW a;
				a.timing = temp.timing * 60;
				a.color = calculateArrowColor(a.timing, timePerBeat);
				a.type = TAP;
				a.columns[0] = col;
				a.judgement = UNSET;
				chart->push_back(a);
			}
		}
	}

	// always put one of these in the chart
	struct ARROW a;
	a.timing = endTime * 60;
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

