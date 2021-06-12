// ppp_read.cpp reads a .BIN file, which is extracted from Para Para Paradise
// source file created 6/2/2016 by Catastrophe
// NOTE: source file was created in a hurry as a proof of concept

#include <algorithm>
#include <vector>
#include <stdio.h>

#include "../headers/common.h"

// after the 32 byte header, the chart is a sequence of these 12 byte structs
struct PPP_RECORD
{
	unsigned short int timing; // delta from the note event
	unsigned char type; // 0 = arrow, 4 = BPM set, C = measure bar, 6 = end song, F = end bga?, FF = end of file
	unsigned char zero_pad1;
	unsigned char zero_pad2;
	unsigned char column; // 0-4, column of arrow
	unsigned short int hold_length;
	unsigned short int unused1;
	unsigned short int unused2;

	PPP_RECORD()
	{
		reset();
	}
	void reset()
	{
		timing = type = column = hold_length = 0;
	}
	bool isFinalNote()
	{
		return type == 6 || type == 0xFF;
	}
};

// rofl hardcoded to return true with a constant filename
bool doesExistPPP(int songID)
{
	char filename[] = "DATA/ppp/01_1.BIN";
	FILE* fp = NULL;

	if ( fopen_s(&fp, filename, "rb") != 0 )
	{
		return false;
	}

	fclose(fp);
	return true;
}

int readPPP(std::vector<struct ARROW> *chart, std::vector<struct FREEZE> *holds, int songID, int chartType)
{
	char filename[] = "DATA/ppp/01_C.BIN";
	FILE* fp = NULL;

	if ( fopen_s(&fp, filename, "rb") != 0 )
	{
		char helpString[64] = "PPP TEST CHART is missing chart data (.BIN)"; // TODO: legit error message
		globalError(SONG_DATA_INCOMPLETE, helpString);
		return 0;
	}

	chart->clear();
	holds->clear();

	// skip the header, which contains some sort of count and the BPM
	char skip = 0;
	unsigned char displayBPM = 150;
	for ( int i = 0; i < 32; i++ )
	{
		fread_s(&skip, sizeof(char), sizeof(char), 1, fp);
		if ( i == 9 )
		{
			displayBPM = skip;
		}
	}

	struct PPP_RECORD temp;
	int timePerBeat = BPM_TO_MSEC(displayBPM); // used to calculate the arrow color, even though DMX is flat
	long currentTime = 0;

	while ( 1 )
	{
		struct ARROW a;
		size_t readlen = fread_s(&temp, sizeof(struct PPP_RECORD), sizeof(struct PPP_RECORD), 1, fp);

		// byte swap the shorts because they're big endian
		temp.timing = (temp.timing>>8) | (temp.timing<<8);
		temp.hold_length = (temp.hold_length>>8) | (temp.hold_length<<8);
		temp.unused1 = (temp.unused1>>8) | (temp.unused1<<8);
		temp.unused2 = (temp.unused1>>8) | (temp.unused2<<8);
		currentTime += temp.timing;

		if ( readlen == 0 )
		{
			allegro_message("Potential problem with %s", filename);
			break;
		}

		// make an arrow note
		a.timing = currentTime;
		if ( temp.type == 0 ) 
		{
			a.color = calculateArrowColor(a.timing, timePerBeat);
			a.type = TAP;
			a.columns[0] = temp.column;
			a.judgement = UNSET;
			chart->push_back(a);

			// also make a hold?
			if ( temp.hold_length != 0 )
			{
				struct FREEZE f;
				f.startTime = currentTime;
				f.columns[0] = temp.column;
				f.endTime1 = currentTime + temp.hold_length;
				holds->push_back(f);
			}
		}
		// make a BPM note
		else if ( temp.type == 4 )
		{
			a.type = BPM_CHANGE;
			a.color = temp.column;
			chart->push_back(a);
			timePerBeat = BPM_TO_MSEC(temp.column); // update this for other arrows
		}
		// make a END_SONG note
		else if ( temp.type == 6 )
		{
			a.type = END_SONG;
			chart->push_back(a);
		}

		if ( temp.isFinalNote() )
		{
			break; // happy ending
		}
	}

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