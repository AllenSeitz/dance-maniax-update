// render_dm.cpp holds refactored code for "DM mode"
// source file created by Allen Seitz 12/26/2011

#include "gameStateManager.h"
#include "gameplayRendering.h"
#include "specialEffects.h"

extern RenderingManager rm;
extern GameStateManager gs;
extern unsigned long int frameCounter;
extern unsigned long int totalGameTime;

//////////////////////////////////////////////////////////////////////////////
// BITMAPs
//////////////////////////////////////////////////////////////////////////////
extern BITMAP* m_columnJudgements[8][3];
extern BITMAP* m_comboNumsDM;
extern BITMAP* m_comboWordDM;

#define DM_STEP_ZONE_Y 70

//////////////////////////////////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////////////////////////////////
int getColumnOffsetX_DM(int column)
{
	int columnWidths[] = {50, 50, 64, 50, 50, 50, 50, 64, 50, 50}; // unlike DDR, they're not all the same width

	// different game modes use different offsets for the columns
	int x = 0, type = 0, i = 0;

	if ( gs.isSolo && !gs.isDoubles )
	{
		type = 1;

	}
	else if ( !gs.isSolo && gs.isDoubles )
	{
		type = 0;
	}
	else if ( gs.isSolo && gs.isDoubles )
	{
		type = 2;
	}
	else // singles play, assume 2P
	{
		type = 3;
	}

	// increment x by 50 or 64, depending on the width of the column
	for ( i = 0; i < columnX[type][column]; i++ )
	{
		x += columnWidths[i];
	}

	// uh-oh! was a shock arrow hit?
	if ( gs.player[0].shockAnimTimer > 0 )
	{
		int percent = gs.player[0].shockAnimTimer * 100 / SHOCK_ANIM_TIME;
		
		// immediately reverse all columns, then slowly unreverse them
		static int shockedColumns[10] = { 5, 4, 3, 2, 1, 0, 9, 8, 7, 6 };
		int shockx = 0;
		for ( i = 0; i < columnX[type][shockedColumns[column]]; i++ )
		{
			shockx += columnWidths[i];
		}

		return WEIGHTED_AVERAGE(shockx, x, percent, 100);
	}

	return x;
}

void renderDMChart(int player)
{
	int n = gs.player[player].currentNote;	
	int x = getStepZonePos_DDR(player);
	int pps = gs.player[player].scrollRate * gs.player[player].speedMod / 10; // pixels per second

	// implement tempo stops - calculate the moment that this chart should render
	UTIME time = gs.player[player].timeElapsed;
	int pausedTime = 0;
	if ( gs.player[player].stopLength > 0 )
	{
		time = gs.player[player].stopTime;
		pausedTime += gs.player[player].timeElapsed + gs.player[player].stopLength - gs.player[player].stopTime;
	}

	int y = (gs.player[player].currentChart[n].timing - time) * pps / 1000;
	while ( y - (pausedTime * pps / 1000) < SCREEN_HEIGHT )
	{
		y = ((gs.player[player].currentChart[n].timing - time - pausedTime) * pps / 1000);	
		renderDMNote(player, gs.player[player].currentChart[n], x, y + DM_STEP_ZONE_Y);

		// implement the correct calculation of the distance between notes due to tempo stops
		if ( gs.player[player].currentChart[n].type == SCROLL_STOP )
		{
			pausedTime += gs.player[player].currentChart[n].color;
		}

		// next item in the list
		n++;
		if ( n == (int)gs.player[player].currentChart.size() )
		{
			break;
		}
	}

	// render the late notes
	y = DM_STEP_ZONE_Y;
	n = gs.player[player].currentNote - 1;
	while ( y > -64 && n >= 0 )
	{
		if ( gs.player[player].currentChart[n].color != 5 && gs.player[player].currentChart[n].type != SCROLL_STOP )
		{
			y = DM_STEP_ZONE_Y - ((gs.player[player].timeElapsed - gs.player[player].currentChart[n].timing) * pps / 1000);
			renderDMNote(player, gs.player[player].currentChart[n], x, y);
		}
		--n;
	}

	// render the hold notes
	int holdIndex = gs.player[player].currentFreeze;
	pausedTime = gs.player[player].stopLength > 0 ? gs.player[player].timeElapsed + gs.player[player].stopLength - gs.player[player].stopTime : 0;
	while ( holdIndex < (int)gs.player[player].freezeArrows.size() )
	{
		int retval = renderDMHoldNote(gs.player[player].freezeArrows[holdIndex], time, pausedTime);
		if ( retval == HOLD_PASSED_BY )
		{
			//break;
		}
		else if ( retval == HOLD_TOO_EARLY )
		{
			break;
		}
		holdIndex++;
	}
}

void renderDMNote(int player, struct ARROW n, int x, int y)
{
	int stepZoneWidth = 214 + (gs.isDoubles ? 214 : 0) + (gs.isSolo ? 100 : 0);

	switch ( n.type )
	{
	case JUMP:
		renderDMChip(player, n.columns[0], n.color, n.judgement, x, y);
		renderDMChip(player, n.columns[1], n.color, n.judgement, x, y);
		break;
	case TAP:
		renderDMChip(player, n.columns[0], n.color, n.judgement, x, y);
		break;
	case SHOCK:
		for ( int i = 0; i < 4; i++ )
		{
			int seed = ((totalGameTime/32)%1024);			
			renderLightningBeamHorizontal(x, stepZoneWidth, y, y+14, (seed+170*i)%1024, LIGHTNING_END_START);
		}
		renderDMChip(player, 0, 7, UNSET, x, y);
		renderDMChip(player, 0, 7, UNSET, stepZoneWidth-50, y);
		break;
	case NEW_SECTION:
	case END_SONG:
		break;
	case BPM_CHANGE:
		line(rm.m_backbuf, x, y+0, stepZoneWidth, y+0, BPM_CHANGE_COLOR1);
		line(rm.m_backbuf, x, y+1, stepZoneWidth, y+1, BPM_CHANGE_COLOR2);
		line(rm.m_backbuf, x, y+2, stepZoneWidth, y+2, BPM_CHANGE_COLOR1);
		line(rm.m_backbuf, x, y+3, stepZoneWidth, y+3, COLOR_WHITE);
		renderWhiteNumber(n.color, x, y-10);
		break;
	case SCROLL_STOP:
		line(rm.m_backbuf, x, y+0, stepZoneWidth, y+0, SCROLL_STOP_COLOR1);
		line(rm.m_backbuf, x, y+1, stepZoneWidth, y+1, SCROLL_STOP_COLOR2);
		line(rm.m_backbuf, x, y+2, stepZoneWidth, y+2, SCROLL_STOP_COLOR1);
		line(rm.m_backbuf, x, y+3, stepZoneWidth, y+3, COLOR_WHITE);
		renderWhiteNumber(n.color, x, y-10);
		break;
	}
}

void renderDMChip(int player, int column, int color, int judgement, int x, int y)
{
	int drum = column >=6 ? column-6 : column;
	//int frame = ((gs.stepZoneBeatTimer / 100) + gs.colorCycle) % 4;
	x += getColumnOffsetX_DM(column);
	UNUSED(y);
	UNUSED(player);

	if ( (judgement >= MARVELLOUS && judgement <= GREAT) || judgement == IGNORED )
	{
		return; // these arrows disappear once they reach the top or are hit
	}

	// shock arrow! in Drummania!
	if ( color == 7 )
	{
		//blit(m_notesDM[6], rm.m_backbuf, 0, 8*0, x, y, 50, 8);
	}
	// render a bass pedal
	else if ( drum == 2 )
	{
		//blit(m_notesDM[drum], rm.m_backbuf, 51, 8*frame, x, y, 64, 8);
	}
	// render anything else
	else
	{
		//blit(m_notesDM[drum], rm.m_backbuf, 0, 8*frame, x, y, 50, 8);
	}
}

char renderDMHoldNote(struct FREEZE f, UTIME time, long pausedTime)
{
	UNUSED(f);
	UNUSED(time);
	UNUSED(pausedTime);
	return HOLD_TOO_EARLY;
}

void renderDMCombo(int combo, int player)
{
	int numDigits = 2;

	if ( combo < 10 )
	{
		return;
	}
	if ( combo > 9999 )
	{
		combo = 9999;
	}

	// how many digits to render?
	if ( combo >= 1000 )
	{
		numDigits = 4;
	}
	else if ( combo >= 100 )
	{
		numDigits = 3;
	}

	// center the split of the word "combo" and the number
	int x = getStepZonePos_DDR(player) + getNumColumns()/2 * 64;
	masked_blit(m_comboWordDM, rm.m_backbuf, 0, 0, x, JUDGEMENT_Y + 40 + 30, 72, 30);

	for ( int i = 0, tens = 1; i < numDigits; i++, tens *= 10 )
	{
		int num = (combo/tens) % 10;
		int y = JUDGEMENT_Y + 40 - (gs.player[player].drummaniaCombo[i]/25);
		renderDMComboNum(num, x - (i+1)*36, y);
	}
}

void renderDMComboNum(int num, int x, int y)
{
	masked_blit(m_comboNumsDM, rm.m_backbuf, (num/5)*36, (num%5)*60, x, y, 36, 60);
}

void renderColumnJudgements(int player)
{
	for ( int i = 0; i < 10; i++ )
	{
		if ( gs.player[player].columnJudgement[i] >= MARVELLOUS && gs.player[player].columnJudgement[i] <= NG && gs.player[player].columnJudgeTime[i] < JUDGEMENT_DISPLAY_TIME )
		{
			BITMAP* source = m_columnJudgements[gs.player[player].columnJudgement[i]-1][0];
			int percent = gs.player[player].columnJudgeTime[i] * 100 / JUDGEMENT_DISPLAY_TIME; // 0-100, how much time has passed?

			// all DM judgements pop on the screen (like marv/perfect/great)
			if ( percent < 5 )
			{
				source = m_columnJudgements[gs.player[player].columnJudgement[i]-1][1];
			}
			else if ( percent < 10 )
			{
				source = m_columnJudgements[gs.player[player].columnJudgement[i]-1][2];
			}

			// Drummania uses staggered columns, but don't stagger O.K.! or N.G.
			int y = JUDGEMENT_Y - 48;
			if ( gs.player[player].isColumnReversed(i) )
			{
				y = REVERSE_JUDGEMENT_Y;
			}
			if ( i % 2 == 0 && gs.player[player].columnJudgement[i] < OK )
			{
				y += 16;
			}

			int jx = getColumnOffsetX_DDR(i)-source->w/2;
			int colWidth = getColumnOffsetX_DDR(1) - getColumnOffsetX_DDR(0);
			if ( gs.player[player].danceManiaxMode )
			{
				jx = getColumnOffsetX_DMX(i)-source->w/2;
				colWidth = getColumnOffsetX_DMX(1) - getColumnOffsetX_DMX(0) + 4;
			}
			if ( gs.player[player].drummaniaMode )
			{
				jx = getColumnOffsetX_DM(i)-source->w/2;
				colWidth = 40; // not true, because of the base pedal, but produces the correct result
			}
			draw_trans_sprite(rm.m_backbuf, source, jx + colWidth/2, y-source->h/2);
		}
	}
}