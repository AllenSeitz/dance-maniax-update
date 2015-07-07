// render_dmx.cpp holds refactored code for "DMX mode"
// source file created by Allen Seitz 12/26/2011

#include "gameStateManager.h"
#include "inputManager.h"
#include "gameplayRendering.h"
#include "specialEffects.h"

extern RenderingManager rm;
extern GameStateManager gs;
extern InputManager im; // only for rendering columns 'held' down
extern unsigned long int frameCounter;
extern unsigned long int totalGameTime;

//////////////////////////////////////////////////////////////////////////////
// constants
//////////////////////////////////////////////////////////////////////////////
#define DMX_STEP_ZONE_Y 70
#define DMX_STEP_ZONE_REV_Y 380
#define DMX_COMBO_Y 250

//////////////////////////////////////////////////////////////////////////////
// BITMAPs
//////////////////////////////////////////////////////////////////////////////
extern BITMAP* m_dmxHeader;
extern BITMAP* m_dmxFooter;
extern BITMAP* m_stepZoneSourceDMX[2];
extern BITMAP* m_notesDMX[3];
extern BITMAP* m_dmxCombos[8];
extern BITMAP* m_dmxFevers[8];
extern BITMAP* m_comboDMX;
extern BITMAP* m_leftLifeGaugeCover;
extern BITMAP* m_rightLifeGaugeCover;
extern BITMAP* m_leftSideHalo[3][2];
extern BITMAP* m_rightSideHalo[3][2];
extern BITMAP* m_judgementsDMX;
extern BITMAP* m_dmxFireworks[2][6][16];
extern BITMAP* m_dmxTransLane;

//////////////////////////////////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////////////////////////////////
int getColumnOffsetX_DMX(int column)
{
	if ( gs.player[0].centerLeft )
	{
		return 96 + 32*column;
	}
	else if ( gs.player[0].centerRight )
	{
		return 414 + 32*(column-4); // versus play columns 0-7
	}
	else if ( gs.isVersus )
	{
		if ( column < 4 )
		{
			return 96 + 32*column;
		}
		else
		{
			return 414 + 32*(column-4); // versus play columns 0-7
		}
	}
	else if ( gs.isDoubles )
	{
		return 192 + 32*column + (column/4*2)-2; // that last term in the forumla creates the thick 'gap' in the middle
	}
	else // center play
	{
		int swapcol = column;
		switch (column)
		{
		case 2: swapcol = 3; break;
		case 3: swapcol = 2; break;
		case 4: swapcol = 5; break;
		case 5: swapcol = 4; break;
		}
		return 192 + 32*swapcol; // render player1 in the center
	}

	//return 256 + 32*column; // render player1 in the center
}

// only used by renderBatteryLives()
int getLeftmostColumnX_DMX(int player)
{
	if ( gs.isDoubles )
	{
		return getColumnOffsetX_DMX(0);
	}
	else if ( gs.isVersus )
	{
		return getColumnOffsetX_DMX(player*4);
	}
	else if ( gs.player[0].isCenter() )
	{
		return getColumnOffsetX_DMX(3);
	}
	else if ( gs.player[0].centerRight )
	{
		return getColumnOffsetX_DMX(4);
	}
	return getColumnOffsetX_DMX(0); // singles play, left side
}

int getColorOfColumn(int column)
{
	if ( column == 0 || column == 3 || column == 4 || column == 7 )
	{
		return 1;
	}
	return 0;
}

void renderDMXChart(int player)
{
	int n = gs.player[player].currentNote;	

	int pps = gs.player[player].scrollRate * gs.player[player].speedMod / 10; // pixels per second

	// where to start rendering the chart?
	// TODO: implement sudden
	//unsigned long startTime = gs.currentChart[gs.currentNote].timing;

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

	// render the hold notes
	int holdIndex = gs.player[player].currentFreeze;
	int holdPausedTime = gs.player[player].stopLength > 0 ? gs.player[player].timeElapsed + gs.player[player].stopLength - gs.player[player].stopTime : 0;

	//int countHoldsProcessed = 0;
	while ( holdIndex < (int)gs.player[player].freezeArrows.size() )
	{
		//countHoldsProcessed++;
		int retval = renderDMXHoldNote(player, gs.player[player].freezeArrows[holdIndex], time, holdPausedTime);
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
	//al_trace("Num holds processed: %d\r\n", countHoldsProcessed);

	int y = (int(gs.player[player].currentChart[n].timing) - int(time)) * pps / 1000;
	while ( y - (pausedTime * pps / 1000) < SCREEN_HEIGHT )
	{
		y = (int(gs.player[player].currentChart[n].timing) - int(time) - pausedTime) * pps / 1000;
		renderDMXNote(player, gs.player[player].currentChart[n], y + DMX_STEP_ZONE_Y);

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

	// render the late notes - TODO: fix the optimization here to work for reverse
	y = DMX_STEP_ZONE_Y;
	n = gs.player[player].currentNote - 1;
	while ( y > -64 && n >= 0 )
	{
		if ( gs.player[player].currentChart[n].color != 5 && gs.player[player].currentChart[n].type != SCROLL_STOP )
		{
			y = DMX_STEP_ZONE_Y - ((gs.player[player].timeElapsed - gs.player[player].currentChart[n].timing) * pps / 1000);
			renderDMXNote(player, gs.player[player].currentChart[n], y);
		}
		--n;
	}
}

void renderDMXNote(int player, struct ARROW n, int y)
{
	int x = getColumnOffsetX_DMX(n.columns[0]);
	int leftX = getColumnOffsetX_DMX(0);
	int rightX = getColumnOffsetX_DMX(3) + 34-1;
	if ( gs.isDoubles || (gs.isVersus && player == 1) || gs.player[0].centerRight )
	{
		rightX = getColumnOffsetX_DMX(7) + 34-1;
		if ( (gs.isVersus && player == 1) || gs.player[0].centerRight )
		{
			leftX = getColumnOffsetX_DMX(4);
		}
	}
	if ( gs.isSingles() && gs.player[0].isCenter() )
	{
		leftX = getColumnOffsetX_DMX(1);
		rightX = getColumnOffsetX_DMX(6) +34-1;
	}

	// implement reverse - the stepzone will take care of itself, this is just the notes
	int originalY = y;
	if ( gs.player[player].reverseModifier != 0 )
	{
		y = DMX_STEP_ZONE_REV_Y - y;
	}

	switch ( n.type )
	{
	case TAP:
		renderDMXArrow(player, n.columns[0], 0, n.judgement, x-2, gs.player[player].isColumnReversed(n.columns[0]) ? y+40 : originalY);
		//renderWhiteNumber(n.timing, getColumnOffsetX_DMX(0)-64, gs.player[player].isColumnReversed(n.columns[0]) ? y+40 : originalY);
		break;
	case JUMP:
		for ( int i = 0; i < 4; i++ )
		{
			if ( n.columns[i] != -1 )
			{
				x = getColumnOffsetX_DMX(n.columns[i]);
				renderDMXArrow(player, n.columns[i], 0, n.judgement, x-2, gs.player[player].isColumnReversed(n.columns[i]) ? y+40 : originalY);
			}
			//renderWhiteNumber(n.timing, getColumnOffsetX_DMX(0)-64, gs.player[player].isColumnReversed(n.columns[0]) ? y+40 : originalY);
		}
		break;
	//case HOLD_START:
	//case HOLD_END:
		//break; // intentionally do nothing
	case SHOCK:
		break; // intentionally do nothing
	case NEW_SECTION:
		break;
	case END_SONG:
		//renderEndSongMarker(leftX, rightX, (gs.player[player].reverseModifier != 0 ? y+74 : y));
		break;
	case BPM_CHANGE:
		if ( ABS(n.color - gs.player[player].scrollRate) >= 5 && n.timing > 0 )
		{
			renderBPMMarker(leftX, rightX, (gs.player[player].reverseModifier != 0 ? y+74 : y), n.color);
		}
		break;
	case SCROLL_STOP:
		renderTempoStopMarker(leftX, rightX, (gs.player[player].reverseModifier != 0 ? y+74 : y), n.color);
		break;
	}
}

void renderDMXArrow(int player, int column, int color, int judgement, int x, int y)
{
	if ( (judgement >= MARVELLOUS && judgement <= GREAT) || judgement == IGNORED )
	{
		return; // these arrows disappear once they reach the top or are hit
	}
	if ( judgement == GOOD && (gs.player[player].danceManiaxMode || gs.player[player].drummaniaMode) )
	{
		return; // in these modes, goods also count as hit
	}

	// calculate the length of a quarter beat, then set frame to a number 0-7
	int frame = getValueFromRange(0, 7, (gs.player[player].stepZoneBeatTimer*100 / gs.player[player].stepZoneTimePerBeat));
	if ( gs.player[player].stepZoneBlinkTimer > 0 )
	{
		frame += 8;
	}
	int colColor = getColorOfColumn(column);
	if ( color == 2 )
	{
		colColor = 2; // make it gold, otherwise ignore this parameter
	}

	masked_blit(m_notesDMX[colColor], rm.m_backbuf, 0, frame*40, x, y, 40, 40);
	set_alpha_blender(); // the game assumes the graphics are left in this mode
}

char renderDMXHoldNote(int player, struct FREEZE f, UTIME time, long pausedTime)
{
	int pps = gs.player[player].scrollRate * gs.player[player].speedMod / 10; // pixels per second

	// calculate how much time is spent inside tempo stops between the startTime and the endTime
	long pauseTotal = pausedTime;
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

	// calculate how many pixels away from the stepzone the hold note is
	int headPosition = (int(f.startTime) - int(time) - pausedTime) * pps / 1000;
	int tailPosition1 = (int(f.endTime1) - int(time) - pauseTotal) * pps / 1000;
	int tailPosition2 = (int(f.endTime2) - int(time) - pauseTotal) * pps / 1000;
	if ( f.isHeld == 1 )
	{
		headPosition = 0; // it's being held
	}

	// check for "don't render me" conditions
	if ( tailPosition1 < -40 && (f.endTime2 == -1 || tailPosition2 < -40) )
	{
		return HOLD_PASSED_BY;
	}
	if ( headPosition > SCREEN_HEIGHT )
	{
		return HOLD_TOO_EARLY;
	}
	if ( f.judgement == OK )
	{
		return HOLD_RENDERED;
	}

	// render the hold note - TODO: why is this loop using meltTime1 and tailPosition1 on both loops? (note - holds are always separate anyways)
	for ( int d = 0; d < 2; d++ )
	{
		if ( f.columns[d] != -1 )
		{
			// calculate the column and color of the hold note
			int column = f.columns[0];
			int x = getColumnOffsetX_DMX(column);
			int color = HOLD_UNTOUCHED;
			if (f.isHeld == 1 && f.meltTime1 == 0)
			{
				color = HOLD_DMX_BRIGHT;
			}
			if ( f.judgement == NG || (f.isHeld == 1 && f.meltTime1 > 0) )
			{
				color = HOLD_DMX_MELT;
			}

			// calculate the exact y position, taking into account reverse
			int topy = DMX_STEP_ZONE_Y + headPosition + 17;
			int bottomy = DMX_STEP_ZONE_Y + tailPosition1 + 17;
			if ( gs.player[player].isColumnReversed(column) )
			{
				topy = DMX_STEP_ZONE_REV_Y - headPosition - 17;
				bottomy = DMX_STEP_ZONE_REV_Y - tailPosition1 - 17;
			}

			//renderWhiteNumber(f.startTime, getColumnOffsetX_DMX(3)+43, topy);
			renderDMXHoldNoteBody(column, x, topy, bottomy, color, player);
		}
	}

	return HOLD_RENDERED;
}

void renderDMXHoldNoteBody(int column, int x, int topy, int bot, int color, int player)
{
	static int lineColors[8][34] = {
		{ 0,0,0,0,0,0,  0,0, 1,1, 2,2, 3,3,3,3,3,3,3,3,3,3, 2,2, 1,1, 0,0, 0,0,0,0,0,0 }, // visually, the charge note
		{ 0,0,0,0,0,0,  0,0, 1,1, 2,2, 3,3,3,3,3,3,3,3,3,3, 2,2, 1,1, 0,0, 0,0,0,0,0,0 }, 

		{ 0,0,0,0,0,0,  0,0, 0,1, 1,2, 2,3,3,3,3,3,3,3,3,2, 2,1, 1,0, 0,0, 0,0,0,0,0,0 }, 
		{ 0,0,0,0,0,0,  0,0, 0,1, 1,2, 2,3,3,3,3,3,3,3,3,2, 2,1, 1,0, 0,0, 0,0,0,0,0,0 }, 

		{ 0,0,0,0,0,0,  0,0, 0,2, 2,3, 3,3,3,2,2,2,2,3,3,3, 3,2, 2,0, 0,0, 0,0,0,0,0,0 }, 
		{ 0,0,0,0,0,0,  0,0, 0,2, 2,3, 3,3,3,2,2,2,2,3,3,3, 3,2, 2,0, 0,0, 0,0,0,0,0,0 }, 

		{ 0,0,0,0,0,0,  0,0, 0,1, 1,2, 2,3,3,3,3,3,3,3,3,2, 2,1, 1,0, 0,0, 0,0,0,0,0,0 }, 
		{ 0,0,0,0,0,0,  0,0, 0,1, 1,2, 2,3,3,3,3,3,3,3,3,2, 2,1, 1,0, 0,0, 0,0,0,0,0,0 }, 
	};

	if ( color == HOLD_UNTOUCHED )
	{
		color = column == 1 || column == 2 || column == 5 || column == 6 ? HOLD_DMX_RED : HOLD_DMX_BLUE;
	}

	for ( int chargex = 6; chargex < 34-6; chargex++ )
	{
		int frame = 0;
		if ( color == HOLD_DMX_BRIGHT )
		{
			frame = getValueFromRange(0, 7, (gs.player[player].stepZoneBeatTimer*100 / gs.player[player].stepZoneTimePerBeat));
		}
		int col = makeacol( dmxHoldColors[color][lineColors[frame][chargex]][0], dmxHoldColors[color][lineColors[frame][chargex]][1], dmxHoldColors[color][lineColors[frame][chargex]][2], 255 );

		int endcapMod = 0;
		if ( lineColors[frame][chargex] == 0 )
		{
			endcapMod = 3; // this color is a little shorter, to create a subtle 'end cap' on the hold note
		}

		line(rm.m_backbuf, x+chargex, topy, x+chargex, bot-endcapMod, col);

		// for debugging
		//line(rm.m_backbuf, 0, topy, 640, topy, makeacol(255,255,255,255));
		//for ( int i = 0; i < 10; i++ )
		//{
		//	line(rm.m_backbuf, 0, bot+i, 640, bot+i, makeacol(255,128,128,255));
		//}
	}
}

void renderUnderFrameDMX()
{
	if ( !gs.isVersus )
	{
		int blink = gs.player[0].stepZoneBlinkTimer > 0 ? 0 : 1;
		int lv = blink ? gs.player[0].stagesLevels[gs.currentStage] % DOUBLE_MILD : 3;
		masked_blit(m_leftSideHalo[lv][0], rm.m_backbuf, 0, 0, 0, 12, 320, 64); // for singles and doubles, join the two halves of the halo together
		masked_blit(m_rightSideHalo[lv][0], rm.m_backbuf, 0, 0, 320, 12, 320, 64);

		masked_blit(m_leftSideHalo[lv][1], rm.m_backbuf, 0, 0, 0, 402, 320, 48); // the bottom halo
		masked_blit(m_rightSideHalo[lv][1], rm.m_backbuf, 0, 0, 320, 402, 320, 48);
	}
	else
	{
		int blink = gs.player[0].stepZoneBlinkTimer > 0 ? 0 : 1;
		int lvl = blink ? gs.player[0].stagesLevels[gs.currentStage] % DOUBLE_MILD : 3;
		blink = gs.player[1].stepZoneBlinkTimer > 0 ? 0 : 1; // probably useless
		int lvr = blink ? gs.player[1].stagesLevels[gs.currentStage] % DOUBLE_MILD : 3;
		masked_blit(m_leftSideHalo[lvl][0], rm.m_backbuf, 0, 0, 0, 12, 320, 64); // 1P is the left half, 2P colors the right half
		masked_blit(m_rightSideHalo[lvr][0], rm.m_backbuf, 0, 0, 320, 12, 320, 64);

		masked_blit(m_leftSideHalo[lvl][1], rm.m_backbuf, 0, 0, 0, 402, 320, 48);
		masked_blit(m_rightSideHalo[lvr][1], rm.m_backbuf, 0, 0, 320, 402, 320, 48);
	}
}

void renderOverFrameDMX()
{
	if ( !gs.isVersus )
	{
		masked_blit(m_rightLifeGaugeCover, rm.m_backbuf, 0, 0, 404, 14, 176, 32);
	}
	masked_blit(m_dmxHeader, rm.m_backbuf, 0, 0, 0, 0, 640, 64);
	masked_blit(m_dmxFooter, rm.m_backbuf, 0, 0, 0, 416, 640, 64);
}							 

void renderStepZoneDMX(int player)
{
	int startColumn = player == 0 ? 0 : 4;
	if ( gs.isSingles() )
	{
		if ( gs.player[0].centerLeft )
		{
			startColumn = 0;
		}
		else if ( gs.player[0].centerRight )
		{
			startColumn = 4;
		}
		else
		{
			startColumn = 2; // center play
		}
	}
	int endColumn = gs.isDoubles ? 8 : startColumn+4;

	for ( int i = startColumn; i < endColumn; i++ )
	{
		int x = getColumnOffsetX_DMX(i);
		int blink = gs.player[player].stepZoneBlinkTimer > 0 ? 0 : 1;	// pick the state of the step zone
		int hitcolor = 0x7F000000;
		int outlineColor = 0xFFB4B4B4;
		const int fadeOutLength = 100;
		if ( blink == 1 )
		{
			static int colors[3] = { makeacol(41, 239, 115, 255), makeacol(239, 101, 190, 255), makeacol(76, 0, 190, 255) };
			int stageColor = gs.player[player].stagesLevels[gs.currentStage] % 10;
			outlineColor = colors[stageColor];
		}
 
		if ( gs.player[player].laneFlareColors[i] == 2 )
		{
			blink = 2; // currently holding a hold note in this column
		}

		int color = getColorOfColumn(i);
		if ( im.isKeyDown(i) )
		{
			hitcolor = color == 0 ? 0xEECC0000 : 0xEE0000CC;
		}
		else if ( im.getReleaseLength(i) <= fadeOutLength )
		{
			int a = getValueFromRange(0xFF, 0x7F, im.getReleaseLength(i) * 100/ fadeOutLength);
			if ( i ==4 )
			{
				al_trace("%d\r\n", a);
			}
			hitcolor = color == 0 ? makeacol32(0xFF, 0, 0, a) : makeacol32(0, 0, 0xFF, a);
		}
		renderStepLaneDMX(x, hitcolor, outlineColor);
		masked_blit(m_stepZoneSourceDMX[color], rm.m_backbuf, blink*34, 0, x, (gs.player[player].isColumnReversed(i) ? DMX_STEP_ZONE_REV_Y-34 : DMX_STEP_ZONE_Y), 34, 38);
	}
}

void renderStepLaneDMX(int x, int bgColor, int outlineColor)
{
	line(rm.m_backbuf, x, 40, x, 434, outlineColor);
	line(rm.m_backbuf, x+1, 40, x+1, 434, outlineColor);
	line(rm.m_backbuf, x+32, 40, x+32, 434, outlineColor);
	line(rm.m_backbuf, x+33, 40, x+33, 434, outlineColor);

	if ( rm.useAlphaLanes )
	{
		// if this worked without killing the fps, that would be nice
		//*
		drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
		rectfill(rm.m_backbuf, x+2, 26, x+31, 444, bgColor);
		drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);
		set_alpha_blender(); // the game assumes the graphics are left in this mode
		//*/

		// this was actually pretty good (the asset was a solid black rectangle)
		/*
		set_trans_blender(0,0,0,128);
		draw_trans_sprite(rm.m_backbuf, m_dmxTransLane, x+2, 26);
		set_alpha_blender(); // the game assumes the graphics are left in this mode
		//*/

		// try this instead?
		/*
		int quad[8] = 	{ x+2,26,  x+2,444,  x+31,444,  x+31,26  };
		drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
		polygon(rm.m_backbuf, 4, quad, bgColor);
		set_alpha_blender(); // the game assumes the graphics are left in this mode
		//*/
	}
	else
	{
		rectfill(rm.m_backbuf, x+2, 26, x+31, 444, bgColor);
	}
}

void renderDMXCombo(int combo, int time, int centerX, int color)
{
	int numDigits = 1, frame = 0, flash = 0;

	if ( combo < 5 ) // DMX starts at 5
	{
		return;
	}
	if ( combo > 9999 )
	{
		combo = 9999;
	}

	// this is for the white flash animation
	flash = time >= 100 ? 5 : time/25;
	//color = (combo/100) % 5;
	frame = flash < 5 ? 4 + flash : color;

	// how many digits to render? Note: the official game renders combos starting at 004
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

	// center the word "combo" above the number
	if ( time >= JUDGEMENT_DISPLAY_TIME )
	{
		masked_blit(m_comboDMX, rm.m_backbuf, 0, 0, centerX-48, DMX_COMBO_Y-26, 96, 24);
	}

	// combo "FEVER" displays every 2 seconds is when combo >= 1000
	if ( combo >= 1000 && (totalGameTime / 2000) % 2 == 0 )
	{
		masked_blit(m_dmxFevers[frame], rm.m_backbuf, 0, 0, centerX-92, DMX_COMBO_Y, 184, 54);
	}
	else
	{
		int x = centerX + (64*numDigits)/2 - 64;
		for ( int i = 0, tens = 1; i < numDigits; i++, tens *= 10 )
		{
			int num = (combo/tens) % 10;
			ASSERT(num < 10);
			int sourcex = (num / 2) * 64;
			int sourcey = (num % 2) * 64;
			ASSERT(sourcex < 320 && sourcey < 128);
			masked_blit(m_dmxCombos[frame], rm.m_backbuf, sourcex, sourcey, x - (i*64), DMX_COMBO_Y, 64, 64);
		}
	}
}

void renderDMXJudgement(int judgement, int time, int x, int y)
{
	int sy = 0;

	switch(judgement)
	{
	case MARVELLOUS:
	case PERFECT:
		sy = 0; break;
	case GREAT:
		sy = 32; break;
	case GOOD:
		sy = 64; break;
	//case BAD:
	case MISS:
		sy = 96; break;
	default:
		sy = 128; break;
	}

	if ( judgement >= 1 && judgement <= 8 )
	{
		int percent = time * 100 / JUDGEMENT_DISPLAY_TIME; // 0-100, how much time has passed?

		// happy judgements bounce slightly
		if ( judgement < GOOD && judgement >= MARVELLOUS )
		{
			if ( percent < 50 )
			{
				y = y - percent/10 - 4;
			}
			else
			{
				percent -= 50;
				y = y - 9 + percent/10;
			}
		}
		// misses sink slightly
		else if ( judgement == MISS ) 
		{
			y = y + percent/10 - 4;
		}
	
		masked_blit(m_judgementsDMX, rm.m_backbuf, 0, sy, x-64, y, 128, 32);
	}
}

void renderDMXFlaresAndFlashes(int player)
{
	int numColumns = gs.isDoubles || gs.isVersus ? 8 : 8;

	for ( int i = 0; i < numColumns; i++ )
	{
		int frame = 15 - ((gs.player[player].laneFlareTimers[i] / (HIT_FLASH_DISPLAY_TIME / 16)) % 16);
		int x = getColumnOffsetX_DMX(i);

		if ( gs.player[player].laneFlareTimers[i] > 0 )
		{
			int bigSmall = gs.player[player].displayCombo >= 100 ? 1 : 0;
			int color = (gs.player[player].displayCombo/100) % 5;

			// for a 'marvelous' render a flashing firework
			if ( gs.player[player].laneFlareColors[i] == 0 || gs.player[player].laneFlareColors[i] == 3 )
			{
				//color = ((gs.player[player].laneFlareTimers[i] / 20) % 3) + 2;
			}

			// render an offcolor firework explosion for an O.K.!
			if ( gs.player[player].laneFlareColors[i] == 4 )
			{
				bigSmall = 1;
				color = (color+1) % 5; // frame % 5;
			}

			// while holding a hold note, do something entirely different than a firework
			if ( gs.player[player].laneFlareColors[i] != 2 )
			{
				set_add_blender(0,0,0,256);
				draw_trans_sprite(rm.m_backbuf, m_dmxFireworks[bigSmall][color][frame], x-46, (gs.player[player].isColumnReversed(i) ? DMX_STEP_ZONE_REV_Y-34 : DMX_STEP_ZONE_Y)-46);
				set_alpha_blender(); // the game assumes the graphics are left in this mode
			}
		}
	}
}

void renderBatteryLivesDMX(int player)
{
	int xpos = getLeftmostColumnX_DMX(player) - 20;
	int ypos = DMX_STEP_ZONE_Y + 10;
	int direction = 1;
	if ( gs.player[player].isColumnReversed(0) )
	{
		ypos = DMX_STEP_ZONE_REV_Y - 28;
		direction = -1;
	}

	for ( int i = 0; i < gs.player[player].lifebarLives; i++ )
	{
		int frame = getValueFromRange(0, 7, (gs.player[player].stepZoneBeatTimer*100 / gs.player[player].stepZoneTimePerBeat));
		if ( frame < 0 || frame > 7 )
		{
			frame = 7;
		}
		masked_stretch_blit(m_notesDMX[2], rm.m_backbuf, 0, frame*40, 40, 40, xpos, ypos, 20, 20);
		ypos += 20*direction;
	}
}