// dmxsq_read.cpp reads a .dmxsq file, which is my custom dmx chart format
// source file created 7/21/2013 by Catastrophe

#include <algorithm>
#include <vector>
#include <stdio.h>

#include "common.h"

bool sortArrowsFunction(struct ARROW a, struct ARROW b)
{
	if ( a.timing == b.timing )
	{
		return a.type < b.type; // puts taps ahead of tempo changes, pretty much
	}
	return a.timing < b.timing;
}

bool sortHoldsFunction(struct FREEZE a, struct FREEZE b)
{
	return a.startTime < b.startTime;
}

int readDMXSQ(std::vector<struct ARROW> *chart, std::vector<struct FREEZE> *holds, int songID, int chartType)
{
	char filename[] = "DATA/dmxsq/101_sm.dmxsq";
	FILE* fp = NULL;

	filename[11] = (songID/100 % 10) + '0';
	filename[12] = (songID/10 % 10) + '0';
	filename[13] = (songID % 10) + '0';
	filename[15] = chartType <= SINGLE_ANOTHER ? 's' : 'd';
	filename[16] = chartType == SINGLE_MILD || chartType == DOUBLE_MILD ? 'm' : 'w';

	// please exist
	if ( fopen_s(&fp, filename, "rb") != 0 )
	{
		globalError(SONG_DATA_INCOMPLETE, filename);
		return 0;
	}

	// fix any potential bugs regarding data from one chart accidentally merging into the next chart
	chart->clear();
	holds->clear();
	int gap = 0;
	float timePerBeat = 400.0; // 150 BPM

	// start reading tags
	char line[256] = "#";

	while ( fgets(line, 256, fp) != NULL )
	{
		char *token, *next;
		if ( line[0] == '#' ) // tag data
		{
			token = strtok_s(line, " ", &next);
		}
		else if ( line[0] == '>' ) // note data
		{
			token = strtok_s(line, ",", &next);
		}
		else
		{
			continue; // neither a tag nor a note
		}

		if ( _strcmpi(token, "#DMXSQ") == 0 )
		{
			TRACE("Loaded an apparent DMXSQ file.");
		}
		else if ( _strcmpi(token, "#VERSION") == 0 )
		{
			TRACE("DMXSQ VERSION: %s", next);
		}
		else if ( _strcmpi(token, "#GAP") == 0 )
		{
			gap = atoi(next);
			al_trace("gap = %d\r\n", gap);
		}
		else if ( _strcmpi(token, ">NOTE") == 0 )
		{
			struct ARROW a;
			a.timing = atoi(next) + gap;
			a.color = calculateArrowColor(a.timing, timePerBeat);
			a.type = TAP;
			token = strtok_s(NULL, " ", &next);
			a.columns[0] = atoi(next);
			a.judgement = UNSET;
			if ( a.columns[0] != -1 )
			{
				chart->push_back(a);
			}
		}
		else if ( _strcmpi(token, ">WNOTE") == 0 )
		{
			struct ARROW a;
			a.timing = atoi(next) + timePerBeat/8 + gap; // such a hack? but it seems the official engine did this?
			a.color = calculateArrowColor(a.timing, timePerBeat);
			a.type = TAP;
			token = strtok_s(NULL, " ", &next);
			a.columns[0] = atoi(next);
			a.judgement = UNSET;
			if ( a.columns[0] != -1 )
			{
				chart->push_back(a);
			}
		}
		else if ( _strcmpi(token, ">SCROLL_RATE") == 0 )
		{
			struct ARROW a;
			a.timing = atoi(next);
			if ( a.timing != 0 )
			{
				a.timing += gap; // the initial tempo isn't affected by the gap
			}
			token = strtok_s(NULL, " ", &next);
			a.color = atoi(next);
			a.type = BPM_CHANGE;
			a.judgement = UNSET;
			chart->push_back(a);

			timePerBeat = BPM_TO_MSEC(a.color); // the only reason I care, is stagger-notes
		}
		else if ( _strcmpi(token, ">END_CHART") == 0 )
		{
			struct ARROW a;
			a.timing = atoi(next) + gap;
			a.type = END_SONG;
			chart->push_back(a);
		}
	}

	// stagger notes may need to be sorted? otherwise only an error would put the chart out of order
	sort(chart->begin(), chart->end(), sortArrowsFunction);

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

int readDMXSQ2P(std::vector<struct ARROW> *chart, std::vector<struct FREEZE> *holds, int songID, int chartType)
{
	int retval = readDMXSQ(chart, holds, songID, chartType);

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

int readDMXSQCenter(std::vector<struct ARROW> *chart, std::vector<struct FREEZE> *holds, int songID, int chartType)
{
	int retval = readDMXSQ(chart, holds, songID, chartType);

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

