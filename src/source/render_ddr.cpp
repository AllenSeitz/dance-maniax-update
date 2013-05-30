// render_ddr.cpp holds refactored code for "DDR mode"
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
extern BITMAP* m_arrowSource[6][4][9];
extern BITMAP* m_stepZoneSource[6][2];
extern BITMAP* m_judgements[8][3];       // the second dimension is only used for M/P/G
extern BITMAP* m_combo[5];
extern BITMAP* m_comboNums;
extern BITMAP* m_flashes[4][2];          // 4 frames x 6 colors, played on top of the step zone for marvellouses and OKs
extern BITMAP* m_flares[6][4][3];        // 6 arrows x 4 animations frames x 3 colors, for when a note is hit

#define DDR_STEP_ZONE_Y 64
#define DDR_STEP_ZONE_REV_Y 380
#define DDR_COMBO_Y 250

//////////////////////////////////////////////////////////////////////////////
// Functions
//////////////////////////////////////////////////////////////////////////////
int getStepZonePos_DDR(char player)
{
	// pick a starting x coordinate
	if ( gs.isDoubles )
	{
		return 64;
	}
	else if ( gs.isVersus )
	{
		return player == 0 ? 32 : 320 + 32;
	}
	return 192; // one player is always centered
}

int getColumnOffsetX_DDR(int column)
{
	int x = 0;	

	// center the columns in different positions for each game mode
	if ( gs.isDoubles )
	{
		x = 64 + column*64; // center the 8 arrows
	}
	else if ( gs.isVersus || gs.player[0].centerLeft || gs.player[0].centerRight )
	{
		if ( column < 4 || gs.player[0].centerLeft )
		{
			x = 32 + column*64; // 1P
		}
		else
		{
			x = 320 + 32 + (column-4)*64; // 2P
		}
	}
	else
	{
		x = 192 + column*64; // singles center play
	}

	// uh-oh! was a shock arrow hit?
	if ( gs.player[0].shockAnimTimer > 0 && (gs.isDoubles || column < 4) )
	{
		int percent = gs.player[0].shockAnimTimer * 100 / SHOCK_ANIM_TIME;
		
		// immediately reverse all columns, then slowly unreverse them
		int shockx = 192 - column*64 + 192;
		if ( gs.isDoubles )
		{
			shockx = 64 - column*64 + 448;
		}
		if ( gs.isVersus )
		{
			shockx = 32 - column*64 + 192;
		}

		return WEIGHTED_AVERAGE(shockx, x, percent, 100);
	}
	if ( gs.player[1].shockAnimTimer > 0 && column >= 4 )
	{
		int percent = gs.player[1].shockAnimTimer * 100 / SHOCK_ANIM_TIME;
		
		// immediately reverse all columns, then slowly unreverse them
		int shockx = 320 + 32 - (column-4)*64 + 192;
		return WEIGHTED_AVERAGE(shockx, x, percent, 100);
	}


	return x;
}

void renderStepZone_DDR(int player)
{
	static int columnSource[10] = { 0, 1, 2, 3, 4, 5, 0, 2, 3, 5 };
	int x = getStepZonePos_DDR(player);
	int blink = gs.player[player].stepZoneBlinkTimer > 0 ? 1 : 0;	// pick the state of the step zone

	for ( int i = 0; i < 10; i++ )
	{
		int arrow = columnSource[i];
		if ( (i == 1 || i == 4) && !gs.isSolo )
		{
			continue;
		}
		if ( (i == 0 || i == 2 || i == 3 || i == 5) && !gs.isDoubles && !gs.isSolo )
		{
			continue;
		}
		if ( i >= 6 && gs.isSolo && !gs.isDoubles )
		{
			continue;
		}

		// render the actual step zone
		if ( !gs.player[player].darkModifier && gs.player[player].shockAnimTimer == 0 )
		{
			renderStepZonePiece_DDR(m_stepZoneSource[arrow][blink], x, DDR_STEP_ZONE_Y, GET_RESIZE(gs.player[player].stepZoneResizeTimers[i]));
		}

		x += 64;
	}
}

void renderFlaresAndFlashes_DDR(int player)
{
	static int columnSource[10] = { 0, 1, 2, 3, 4, 5, 0, 2, 3, 5 };
	int x = getStepZonePos_DDR(player);

	for ( int i = 0; i < 10; i++ )
	{
		int arrow = columnSource[i];
		int frame = 3 - (gs.player[player].laneFlareTimers[i] / (HIT_FLASH_DISPLAY_TIME / 4)) % 4;
		if ( (i == 1 || i == 4) && !gs.isSolo )
		{
			continue;
		}
		if ( (i == 0 || i == 2 || i == 3 || i == 5) && !gs.isDoubles && !gs.isSolo )
		{
			continue;
		}
		if ( i >= 6 && gs.isSolo && !gs.isDoubles )
		{
			continue;
		}

		if ( gs.player[player].laneFlareTimers[i] > 0 )
		{
			// also draw a bright white flash on the spot for a marvelous
			if ( gs.player[player].laneFlareColors[i] == 0 || gs.player[player].laneFlareColors[i] == 3 )
			{
				draw_trans_sprite(rm.m_backbuf, m_flashes[frame][0], x-5, DDR_STEP_ZONE_Y-5);
			}

			// render a yellow flash for O.K.!
			if ( gs.player[player].laneFlareColors[i] == 4 )
			{
				draw_trans_sprite(rm.m_backbuf, m_flashes[frame][1], x-5, DDR_STEP_ZONE_Y-5);
			}

			// TODO: while holding a note ( == 2 )
		}		

		// render the fading arrow outline last
		if ( gs.player[player].laneFlareTimers[i] > 0 && gs.player[player].laneFlareColors[i] < 3 )
		{
			draw_trans_sprite(rm.m_backbuf, m_flares[arrow][frame][gs.player[player].laneFlareColors[i]], x, DDR_STEP_ZONE_Y);
		}
		x += 64;
	}
}

void renderStepZonePiece_DDR(BITMAP* src, int x, int y, int scale)
{
	// copy to a temporary space, scale there, then do a transparent blit
	clear_to_color(rm.m_temp64, 0x00000000); // argb format for the color
	int s = (64 * scale) / 100;
	int d = (64 - s) / 2;
	stretch_blit(src, rm.m_temp64, 0, 0, 64, 64, d, d, s, s);
	draw_trans_sprite(rm.m_backbuf, rm.m_temp64, x, y);
}

void renderDDRChart(int player)
{
	int n = gs.player[player].currentNote;	
	int x = getStepZonePos_DDR(player);
	int pps = gs.player[player].scrollRate * gs.player[player].speedMod / 10; // pixels per second

	// where to start rendering the chart?
	// TODO: implement sudden
	//unsigned long startTime = gs.player[player].currentChart[gs.player[player].currentNote].timing;

	// where to stop rendering the chart?
	// TODO: implement hidden
	//unsigned long endTime = currentChart[currentNote].timing + (CHART_HEIGHT * 1000 / pps);

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
		renderDDRNote(player, gs.player[player].currentChart[n], x, y + DDR_STEP_ZONE_Y);

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
	y = DDR_STEP_ZONE_Y;
	n = gs.player[player].currentNote - 1;
	while ( y > -64 && n >= 0 )
	{
		if ( gs.player[player].currentChart[n].color != 5 && gs.player[player].currentChart[n].type != SCROLL_STOP )
		{
			y = DDR_STEP_ZONE_Y - ((gs.player[player].timeElapsed - gs.player[player].currentChart[n].timing) * pps / 1000);
			renderDDRNote(player, gs.player[player].currentChart[n], x, y);
		}
		--n;
	}

	// render the hold notes
	int holdIndex = gs.player[player].currentFreeze;
	pausedTime = gs.player[player].stopLength > 0 ? gs.player[player].timeElapsed + gs.player[player].stopLength - gs.player[player].stopTime : 0;
	while ( holdIndex < (int)gs.player[player].freezeArrows.size() )
	{
		int retval = renderDDRHoldNote(player, gs.player[player].freezeArrows[holdIndex], time, pausedTime);
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

void renderDDRNote(int player, struct ARROW n, int x, int y)
{
	int numColumns = getNumColumns();

	// implement reverse - the stepzone will take care of itself, this is just the notes
	int originalY = y;
	if ( gs.player[player].reverseModifier != 0 )
	{
		y = DDR_STEP_ZONE_REV_Y - y + 64; // the +64 is to account for the height of the note
	}

	switch ( n.type )
	{
	case TAP:
		renderDDRArrow(player, n.columns[0], n.color, n.judgement, getColumnOffsetX_DDR(n.columns[0]), gs.player[player].isColumnReversed(n.columns[0]) ? y : originalY);
		break;
	case JUMP:
		renderDDRArrow(player, n.columns[0], n.color, n.judgement, getColumnOffsetX_DDR(n.columns[0]), gs.player[player].isColumnReversed(n.columns[0]) ? y : originalY);
		renderDDRArrow(player, n.columns[1], n.color, n.judgement, getColumnOffsetX_DDR(n.columns[1]), gs.player[player].isColumnReversed(n.columns[1]) ? y : originalY);
		break;
	//case HOLD_START:
	//case HOLD_END:
		// intentionally do nothing
		break;
	case SHOCK:
		if ( gs.isSolo )
		{
			for ( int i = 0; i < numColumns; i++ )
			{
				renderDDRArrow(player, i, 7, n.judgement, x, y);
			}
		}
		else
		{
			if ( gs.isDoubles )
			{
				renderDDRArrow(player, 0, 7, n.judgement, x, y);
				renderDDRArrow(player, 2, 7, n.judgement, x, y);
				renderDDRArrow(player, 3, 7, n.judgement, x, y);
				renderDDRArrow(player, 5, 7, n.judgement, x, y);
			}
			renderDDRArrow(player, 6, 7, n.judgement, x, y);
			renderDDRArrow(player, 7, 7, n.judgement, x, y);
			renderDDRArrow(player, 8, 7, n.judgement, x, y);
			renderDDRArrow(player, 9, 7, n.judgement, x, y);
		}

		for ( int i = 0; i < 6; i++ )
		{
			int width = numColumns * 64;
			//int seed = ((timeElapsed/32)%1024); // TODO: revert this change
			int seed = ((totalGameTime/32)%1024);			
			renderLightningBeamHorizontal(x, width, y, y+64, (seed+170*i)%1024, LIGHTNING_END_START);
		}
		break;
	case NEW_SECTION:
		break;
	case END_SONG:
		line(rm.m_backbuf, x, y+0, x+64*numColumns, y+0, END_SECTION_COLOR1);
		line(rm.m_backbuf, x, y+1, x+64*numColumns, y+1, END_SECTION_COLOR2);
		line(rm.m_backbuf, x, y+2, x+64*numColumns, y+2, END_SECTION_COLOR1);
		line(rm.m_backbuf, x, y+3, x+64*numColumns, y+3, COLOR_WHITE);
		break;
	case BPM_CHANGE:
		line(rm.m_backbuf, x, y+0, x+64*numColumns, y+0, BPM_CHANGE_COLOR1);
		line(rm.m_backbuf, x, y+1, x+64*numColumns, y+1, BPM_CHANGE_COLOR2);
		line(rm.m_backbuf, x, y+2, x+64*numColumns, y+2, BPM_CHANGE_COLOR1);
		line(rm.m_backbuf, x, y+3, x+64*numColumns, y+3, COLOR_WHITE);
		renderWhiteNumber(n.color, x, y-10);
		break;
	case SCROLL_STOP:
		line(rm.m_backbuf, x, y+0, x+64*numColumns, y+0, SCROLL_STOP_COLOR1);
		line(rm.m_backbuf, x, y+1, x+64*numColumns, y+1, SCROLL_STOP_COLOR2);
		line(rm.m_backbuf, x, y+2, x+64*numColumns, y+2, SCROLL_STOP_COLOR1);
		line(rm.m_backbuf, x, y+3, x+64*numColumns, y+3, COLOR_WHITE);
		renderWhiteNumber(n.color, x, y-10);
		break;
	}
}

void renderDDRArrow(int player, int column, int color, int judgement, int x, int y)
{
	static int columnSource[10] = { 0, 2, 3, 5, 0, 2, 3, 5, 1, 4 };
	BITMAP* source = m_arrowSource[0][0][0];

	if ( (judgement >= MARVELLOUS && judgement <= GREAT) || judgement == IGNORED )
	{
		return; // these arrows disappear once they reach the top or are hit
	}

	// freeze arrows (color 5, or melted = 8) are a little different than other arrows
	int frame = 0;
	if ( color == 5 || color == 8 )
	{
		frame = 0;
		source = m_arrowSource[columnSource[column]][frame][color];
	}
	// shock arrows (color 7) are mostly like other arrows
	else if ( color == 7 )
	{
		frame = (gs.player[player].stepZoneBeatTimer / 100) % 4;
		source = m_arrowSource[columnSource[column]][frame][7];
	}
	else
	{
		// calculate the length of a quarter beat, then set frame to a number 0-3
		frame = (gs.player[player].stepZoneBeatTimer / 100) % 4;
		source = m_arrowSource[columnSource[column]][frame][(color + gs.player[player].colorCycle)%4];
	}
	masked_blit(source, rm.m_backbuf, 0, 0, x, y, 64, 64);
}

char renderDDRHoldNote(int player, struct FREEZE f, UTIME time, long pausedTime)
{
	int pps = gs.player[player].scrollRate * gs.player[player].speedMod / 10; // pixels per second

	// calculate how much time is spent inside tempo stops between the startTime and the endTime
	int pauseTotal = pausedTime;
	unsigned int n = gs.player[player].currentNote;
	while ( n < gs.player[player].currentChart.size() )
	{
		if ( gs.player[player].currentChart[n].timing >= f.endTime1 )
		{
			break;
		}
		if ( gs.player[player].currentChart[n].type == SCROLL_STOP )
		{
			if ( gs.player[player].currentChart[n].timing < f.startTime )
			{
				pausedTime += gs.player[player].currentChart[n].color;
			}
			pauseTotal += gs.player[player].currentChart[n].color;
		}
		n++;
	}

	// find the top and bottom of this hold note
	int topy = DDR_STEP_ZONE_Y + ((f.startTime - time - pausedTime) * pps / 1000);
	int bot1 = DDR_STEP_ZONE_Y + ((f.endTime1 - time - pauseTotal) * pps / 1000);
	int bot2 = DDR_STEP_ZONE_Y + ((f.endTime2 - time - pauseTotal) * pps / 1000);
	if ( f.startTime < time )
	{
		topy = DDR_STEP_ZONE_Y - ((time - f.startTime) * pps / 1000);
	}
	if ( f.endTime1 < time )
	{
		bot1 = DDR_STEP_ZONE_Y - ((time - f.endTime1) * pps / 1000);
	}
	if ( f.endTime2 < time )
	{
		bot2 = DDR_STEP_ZONE_Y - ((time - f.endTime2) * pps / 1000);
	}

	// check for "don't render me" conditions
	if ( bot1 < -64 && (f.endTime2 == -1 || bot2 < -64) )
	{
		return HOLD_PASSED_BY;
	}
	if ( topy > SCREEN_HEIGHT )
	{
		return HOLD_TOO_EARLY;
	}
	if ( f.judgement == OK )
	{
		return HOLD_RENDERED;
	}

	// render the first hold note
	int x = getColumnOffsetX_DDR(f.columns[0]);
	int color = HOLD_UNTOUCHED;
	if (f.isHeld == 1 && f.meltTime1 == 0)
	{
		color = HOLD_BRIGHT;
	}
	if ( f.judgement == NG || (f.isHeld == 1 && f.meltTime1 > 0) )
	{
		color = HOLD_MELT;
	}
	int column = f.columns[0];
	topy = f.isHeld == 1 ? DDR_STEP_ZONE_Y : topy;
	renderDDRHoldNoteBody(column, x, topy, bot1, color);

	// render the second hold note
	if ( f.columns[1] != -1 )
	{
		x = getColumnOffsetX_DDR(f.columns[1]);
		color = HOLD_UNTOUCHED;
		if (f.isHeld == 1 && f.meltTime2 == 0)
		{
			color = HOLD_BRIGHT;
		}
		if ( f.judgement == NG || (f.isHeld == 1 && f.meltTime2 > 0) )
		{
			color = HOLD_MELT;
		}
		column = f.columns[1];
		renderDDRHoldNoteBody(column, x, topy, bot2, color);
	}

	return HOLD_RENDERED;
}

// this function
void renderDDRHoldNoteBody(int column, int x, int topy, int bot, int color )
{
	static int columnSource[10] = { 0, 2, 3, 5, 0, 2, 3, 5, 1, 4 };

	int frame = 0;
	int source = 6;
	int rectcolor = makeacol(arrowColors[5][0], arrowColors[5][1], arrowColors[5][2], 255);
	if ( color == HOLD_BRIGHT )
	{
		frame = 3;
		rectcolor = 0xFFDDFFDD;
	}
	if ( color == HOLD_MELT )
	{
		source = 8;
		rectcolor = 0xFF505050;
	}

	// down arrows render differently than holds in other columns
	if ( column == 2 || column == 7 )
	{
		rectfill(rm.m_backbuf, x+1, topy+32, x+62, bot+26, rectcolor);
		int y = bot + 16;

		while ( y > topy + 16 )
		{
			masked_blit(m_arrowSource[columnSource[column]][frame][source], rm.m_backbuf, 0, 16, x, y, 64, 48);
			y -= 16;
		}

		// the top of the hold is where the adjustment is made, if the length isn't % 16
		int fractionalArrowHeight = (y - topy) % 16;
		masked_blit(m_arrowSource[columnSource[column]][frame][source], rm.m_backbuf, 0, 0, x, y, 64, fractionalArrowHeight);

		renderDDRArrow(0, column, source == 8 ? 8 : 5, 0, getColumnOffsetX_DDR(column), topy);
	}
	else
	{
		// solo arrows are technically a little narrower than other arrows
		if ( column == 1 || column == 4 )
		{
			rectfill(rm.m_backbuf, x+9, topy+16, x+55, bot+0, rectcolor);
		}
		else
		{
			rectfill(rm.m_backbuf, x+1, topy+32, x+62, bot+31, rectcolor);
		}
		int y = topy;

		// the top of the hold is where the adjustment is made, if the length isn't % 16
		int fractionalArrowHeight = (bot - y) % 16;
		masked_blit(m_arrowSource[columnSource[column]][frame][source], rm.m_backbuf, 0, 0, x, y, 64, fractionalArrowHeight);
		y += fractionalArrowHeight;

		while ( y < bot )
		{
			masked_blit(m_arrowSource[columnSource[column]][frame][source], rm.m_backbuf, 0, 0, x, y, 64, 48);
			y += 16;
		}
		masked_blit(m_arrowSource[columnSource[column]][frame][source], rm.m_backbuf, 0, 0, x, bot, 64, 64);
		renderDDRArrow(0, column, source == 8 ? 8 : 5, 0, getColumnOffsetX_DDR(column), topy);
	}
}

void renderDDRJudgement(int judgement, int time, int x, int y)
{
	BITMAP* source = m_judgements[judgement-1][0];

	if ( judgement >= 1 && judgement <= 8 )
	{
		int percent = time * 100 / JUDGEMENT_DISPLAY_TIME; // 0-100, how much time has passed?

		// happy judgements bounce slightly
		if ( judgement < GOOD && judgement >= MARVELLOUS )
		{
			if ( percent < 5 )
			{
				source = m_judgements[judgement-1][1];
			}
			else if ( percent < 10 )
			{
				source = m_judgements[judgement-1][2];
			}
		}
		// bads shake from side to side (goods are stationary)
		/*
		else if ( judgement == BAD )
		{
			percent = percent % 25;
			if ( percent < 13 )
			{
				x -= percent / 2;
			}
			else
			{
				x += (percent-12) / 2;
			}
		}
		*/
		// misses sink slightly
		else if ( judgement == MISS ) 
		{
			y = y + percent/10 - 4;
		}
	
		draw_trans_sprite(rm.m_backbuf, source, x-source->w/2, y-source->h/2);
	}
}

void renderDDRCombo(int combo, int time, int centerX, int color)
{
	int numDigits = 1, frame = 0;

	if ( combo < 4 )
	{
		return;
	}
	if ( combo > 9999 )
	{
		combo = 9999;
	}

	// this is for the counting-up scaling animation
	frame = time >= 100 ? 0 : (time / 25) + 1;

	// how many digits to render?
	if ( combo >= 1000 )
	{
		numDigits = 4;
	}
	else if ( combo >= 100 )
	{
		numDigits = 3;
	}
	else if ( combo >= 10 )
	{
		numDigits = 2;
	}

	// center the split of the word "combo" and the number
	int x = centerX;
	draw_trans_sprite(rm.m_backbuf, m_combo[color], x, DDR_COMBO_Y-21+35);

	for ( int i = 0, tens = 1; i < numDigits; i++, tens *= 10 )
	{
		int num = (combo/tens) % 10;
		renderDDRComboNum(num, frame, color, x - (i+1)*32, DDR_COMBO_Y+35);
	}
}

void renderDDRComboNum(int num, int size, int color, int x, int y)
{
	static int heights[] = { 43, 40, 48, 56, 64 };

	// copy to a temporary space, maybe scale there, then do a transparent blit
	clear_to_color(rm.m_temp64, 0x00000000); // argb format for the color
	if ( size == 0 )
	{
		blit(m_comboNums, rm.m_temp64, num*32, color*43, 0, 0, 32, 43);
	}
	else
	{
		stretch_blit(m_comboNums, rm.m_temp64, num*32, color*43, 32, 43, 0, 0, 32, heights[size]);
	}
	draw_trans_sprite(rm.m_backbuf, rm.m_temp64, x, y-heights[size]);
}