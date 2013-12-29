// gameplayRendering.cpp contains functions for rendering the game loop
// source file created by Allen Seitz 12-20-09
// 12-22-09: major refactoring to isolate DDR specific functions
// 12-26-11: yet again, even more refactoring for each game mode (as well as adding DMX)

#include "gameStateManager.h"
#include "gameplayRendering.h"
#include "particleSprites.h"
#include "scoreManager.h"
#include "specialEffects.h"
#include "videoManager.h"

extern RenderingManager rm;
extern GameStateManager gs;
extern ScoreManager     sm;
extern VideoManager     vm;
extern unsigned long int frameCounter;
extern unsigned long int totalGameTime;
extern int retireTimer;
extern int fullComboAnimStep; // 0 = not started, 1 = started
extern int fullComboAnimTimer;
extern bool fullComboP1;
extern bool fullComboP2;

// song transition
extern bool isMidTransition;
extern UTIME songTransitionTime;
extern BITMAP** m_banners;
extern const UTIME BANNER_ANIM_LENGTH;
const UTIME BANNER_ANIM_STEP = 500;


//////////////////////////////////////////////////////////////////////////////
// Graphics
//////////////////////////////////////////////////////////////////////////////
BITMAP* m_stepZoneSource[6][2];
BITMAP* m_arrowSource[6][4][9];   // 6 arrows x 4 animation frames x 9 colors (4 vivid, 1 grey, 2 freeze, 1 shock, 1 melt)
BITMAP* m_judgements[8][3];       // the second dimension is only used for M/P/G
BITMAP* m_columnJudgements[8][3];
BITMAP* m_combo[5];
BITMAP* m_comboNums;
BITMAP* m_flashes[4][2];          // 4 frames x 6 colors, played on top of the step zone for marvellouses and OKs
BITMAP* m_flares[6][4][3];        // 6 arrows x 4 animations frames x 3 colors, for when a note is hit
BITMAP* m_stageDisplay;
BITMAP* m_grades;
BITMAP* m_banner1 = NULL;
BITMAP* m_banner2 = NULL;
BITMAP* m_fullComboAnim[11];
BITMAP* m_fcRays[4];
BITMAP* m_speedIcons = NULL;

//BITMAP* m_notesBM[3];             // 2 colors + shock color, animations are built in
//BITMAP* m_notesDM[6];             // 5 colors + shock color, animations are built in
BITMAP* m_comboNumsDM;
BITMAP* m_comboWordDM;

BITMAP* m_dmxHeader;
BITMAP* m_dmxFooter;
BITMAP* m_leftLifeGaugeCover;
BITMAP* m_rightLifeGaugeCover;
BITMAP* m_leftSideHalo[4][2];
BITMAP* m_rightSideHalo[4][2];

BITMAP* m_stepZoneSourceDMX[2];
BITMAP* m_notesDMX[3];

BITMAP* m_dmxCombos[8];
BITMAP* m_dmxFevers[8];
BITMAP* m_comboDMX;
BITMAP* m_judgementsDMX;
BITMAP* m_dmxFireworks[2][6][16];
BITMAP* m_dmxTransLane;

// the timer is global - it is also used on other menus
BITMAP* m_time;
BITMAP* m_timeDigits[2];


//////////////////////////////////////////////////////////////////////////////
// Functions
//////////////////////////////////////////////////////////////////////////////
int getNumColumns()
{
	int numColumns = 4;
	if ( gs.isSolo )
	{
		numColumns += 2;
	}
	if ( gs.isDoubles || gs.isVersus )
	{
		numColumns += 4;
	}
	return numColumns;
}

int getCenterOfLanesX(int player)
{
	int columnWidth = 32; // dmx

	if ( player == 1 )
	{
		return getColumnOffsetX_DMX(4)+1 + 2*columnWidth;
	}

	if ( gs.isDoubles )
	{
		return getColumnOffsetX_DMX(0)+1 + 4*columnWidth;
	}
	else if ( gs.player[0].centerRight )
	{
		return getColumnOffsetX_DMX(4)+1 + 2*columnWidth;
	}
	else if ( gs.isVersus || gs.player[0].centerLeft )
	{
		return getColumnOffsetX_DMX(0)+1 + 2*columnWidth;
	}

	return getColumnOffsetX_DMX(0)+65 + 2*columnWidth;
}

int getColumnOffsetX_ANY(int column)
{
	if ( gs.player[0].danceManiaxMode )
	{
		return getColumnOffsetX_DMX(column);
	}
	else if ( gs.player[0].drummaniaMode )
	{
		return getColumnOffsetX_DM(column);
	}
	return getColumnOffsetX_DDR(column);
}

void loadGameplayGraphics()
{
	int arrow = 0, frame = 0, color = 0;
	BITMAP* temp = NULL;

	// load these once and use them throughout the program
	m_time = loadImage("DATA/songwheel/time.tga");
	m_timeDigits[0] = loadImage("DATA/songwheel/timer_blue.tga");
	m_timeDigits[1] = loadImage("DATA/songwheel/timer_red.tga");
	m_stageDisplay = loadImage("DATA/gameplay/stage_nums.tga");
	m_grades = loadImage("DATA/gameplay/grades.tga");

	// load graphics once and leave them loaded for the whole program (meh)
	m_stepZoneSource[0][0] = loadImage("DATA/gameplay/SPR_DanceZoneLR_02.tga");
	m_stepZoneSource[1][0] = loadImage("DATA/gameplay/SPR_DanceZoneSolo_02.tga");
	m_stepZoneSource[2][0] = loadImage("DATA/gameplay/SPR_DanceZoneUD_02.tga");
	m_stepZoneSource[3][0] = loadImage("DATA/gameplay/SPR_DanceZoneUD_02.tga");
	m_stepZoneSource[4][0] = loadImage("DATA/gameplay/SPR_DanceZoneSolo_02.tga");
	m_stepZoneSource[5][0] = loadImage("DATA/gameplay/SPR_DanceZoneLR_02.tga");
	m_stepZoneSource[0][1] = loadImage("DATA/gameplay/SPR_DanceZoneLR_01.tga");
	m_stepZoneSource[1][1] = loadImage("DATA/gameplay/SPR_DanceZoneSolo_01.tga");
	m_stepZoneSource[2][1] = loadImage("DATA/gameplay/SPR_DanceZoneUD_01.tga");
	m_stepZoneSource[3][1] = loadImage("DATA/gameplay/SPR_DanceZoneUD_01.tga");
	m_stepZoneSource[4][1] = loadImage("DATA/gameplay/SPR_DanceZoneSolo_01.tga");
	m_stepZoneSource[5][1] = loadImage("DATA/gameplay/SPR_DanceZoneLR_01.tga");
	draw_sprite_v_flip(m_stepZoneSource[3][0], m_stepZoneSource[2][0], 0, 0);
	draw_sprite_h_flip(m_stepZoneSource[4][0], m_stepZoneSource[1][0], 0, 0);
	draw_sprite_h_flip(m_stepZoneSource[5][0], m_stepZoneSource[0][0], 0, 0);
	draw_sprite_v_flip(m_stepZoneSource[3][1], m_stepZoneSource[2][1], 0, 0);
	draw_sprite_h_flip(m_stepZoneSource[4][1], m_stepZoneSource[1][1], 0, 0);
	draw_sprite_h_flip(m_stepZoneSource[5][1], m_stepZoneSource[0][1], 0, 0);

	//////////////////////////////////////////////////////////////////////////
	// allocate memory, pre-flip, and pre-color each arrow
	//////////////////////////////////////////////////////////////////////////
	for ( arrow = 0; arrow < 6; arrow++ )
	for ( frame = 0; frame < 4; frame++ )
	for ( color = 0; color < 9; color++ )
	{
		m_arrowSource[arrow][frame][color] = create_bitmap_ex(32, 64, 64);
		if ( arrow >= 3 ) // because draw_sprite_h_flip omits the mask color
		{
			clear_to_color(m_arrowSource[arrow][frame][color], 0xFF00FF);
		}
	}
	
	temp = loadImage("DATA/gameplay/SPR_ArrowLR.tga");
	for ( frame = 0; frame < 4; frame++ )
	for ( color = 0; color < 9; color++ )
	{
		blit(temp, m_arrowSource[0][frame][color], 0, frame*64, 0, 0, 64, 64);
		draw_sprite_h_flip(m_arrowSource[5][frame][color], m_arrowSource[0][frame][color], 0, 0);
	}
	destroy_bitmap(temp);

	temp = loadImage("DATA/gameplay/SPR_ArrowSolo.tga");
	for ( frame = 0; frame < 4; frame++ )
	for ( color = 0; color < 9; color++ )
	{
		blit(temp, m_arrowSource[1][frame][color], 0, frame*64, 0, 0, 64, 64);
		draw_sprite_h_flip(m_arrowSource[4][frame][color], m_arrowSource[1][frame][color], 0, 0);
	}
	destroy_bitmap(temp);

	temp = loadImage("DATA/gameplay/SPR_ArrowUD.tga");
	for ( frame = 0; frame < 4; frame++ )
	for ( color = 0; color < 9; color++ )
	{
		blit(temp, m_arrowSource[2][frame][color], frame*64, 0, 0, 0, 64, 64);
		draw_sprite_v_flip(m_arrowSource[3][frame][color], m_arrowSource[2][frame][color], 0, 0);
	}
	destroy_bitmap(temp);

	// arrow 8 is the shock arrow, and looks different from the rest
	for ( arrow = 0; arrow < 6; arrow++ )
	for ( frame = 0; frame < 4; frame++ )
	{
		replaceColor(m_arrowSource[arrow][frame][7], 0x4E4E4E, makecol(255, 255, 255));
		replaceColor(m_arrowSource[arrow][frame][7], 0xFFFFFF, makecol(255, 255, 255));
	}

	//* pre-color the first four arrows, leave the 4th intentionally grey, make the 5th green for holds, and make the 7th blue for shocks
	for ( color = 0; color < 8; color++ )
	{
		if ( color == 4 )
		{
			continue;
		}

		for ( arrow = 0; arrow < 6; arrow++ )
		for ( frame = 0; frame < 4; frame++ )
		{
			for ( int y = 0; y < 64; y++ )
			for ( int x = 0; x < 64; x++ )
			{
				unsigned long c = ((long *)m_arrowSource[arrow][frame][color]->line[y])[x];  
				int a = (c & 0xFF000000) >> 24;
				int r = (c & 0x00FF0000) >> 16;
				int g = (c & 0x0000FF00) >> 8;
				int b = (c & 0x000000FF);
				if ( (r>0 || g>0 || b>0) && c != 0xFFFFFF && c != 0x4E4E4E && c != 0xFF00FF ) // skip three specific colors in the asset
				{
					int blendedR = WEIGHTED_AVERAGE((arrowColors[color][0]), (arrowColors[color+1][0]), y, 64);
					int blendedG = WEIGHTED_AVERAGE((arrowColors[color][1]), (arrowColors[color+1][1]), y, 64);
					int blendedB = WEIGHTED_AVERAGE((arrowColors[color][2]), (arrowColors[color+1][2]), y, 64);
					r = WEIGHTED_AVERAGE(blendedR, ARROW_BRIGHTNESS, r, 256);
					g = WEIGHTED_AVERAGE(blendedG, ARROW_BRIGHTNESS, g, 256);
					b = WEIGHTED_AVERAGE(blendedB, ARROW_BRIGHTNESS, b, 256);
					//r = r * (78+(y/2)) / 100; // implement a weak shading
					//g = g * (78+(y/2)) / 100;
					//b = b * (78+(y/2)) / 100;
					c = (a << 24) + (r << 16) + (g << 8) + b;
					((long *)m_arrowSource[arrow][frame][color]->line[y])[x] = c;
				}
			}
		}
	}

	// arrow 7 is the same as arrow 6, but the black outline is replaced with the freeze arrow color
	for ( arrow = 0; arrow < 6; arrow++ )
	for ( frame = 0; frame < 4; frame++ )
	{
		blit(m_arrowSource[arrow][frame][5], m_arrowSource[arrow][frame][6], 0, 0, 0, 0, 64, 64);
		replaceColor(m_arrowSource[arrow][frame][6], 0, makecol(arrowColors[5][0], arrowColors[5][1], arrowColors[5][2]));
	}

	// arrow 8 is the shock arrow, and looks different from the rest (add dark lines and lightning)
	for ( arrow = 0; arrow < 6; arrow++ )
	for ( frame = 0; frame < 4; frame++ )
	{
		// first replace the outline color
		replaceColor(m_arrowSource[arrow][frame][7], 0, SHOCK_OUTLINE_COLOR);

		int Ys[9] = {11, 12, 13, 31, 32, 33, 51, 52, 53};

		// second draw a few dark lines through the arrow to give it a texture
		for ( int yi = 0; yi < 9; yi++ )
		for ( int x = 0; x < 64; x++ )
		{
			unsigned long c = ((long *)m_arrowSource[arrow][frame][7]->line[Ys[yi]])[x];  
			int r = (c & 0x00FF0000) >> 16;
			int g = (c & 0x0000FF00) >> 8;
			int b = (c & 0x000000FF);
			if ( c != SHOCK_OUTLINE_COLOR && c != 0xFF00FF ) // skip two specific colors in the asset
			{
				r = (r * 4) / ((yi%3)+5);
				g = (g * 4) / ((yi%3)+5);
				b = (b * 4) / ((yi%3)+5);
				c = (r << 16) + (g << 8) + b;
				((long *)m_arrowSource[arrow][frame][7]->line[Ys[yi]])[x] = c;
			}
		}
	}

	// arrow 9 is the melted freeze arrow, and is basically just black
	for ( arrow = 0; arrow < 6; arrow++ )
	for ( frame = 0; frame < 4; frame++ )
	{
		// darken the entire image
		for ( int y = 0; y < 64; y++ )
		for ( int x = 0; x < 64; x++ )
		{
			unsigned long c = ((long *)m_arrowSource[arrow][frame][8]->line[y])[x];
			if ( c != 0xFF00FF )
			{
				int r = ((c & 0x00FF0000) >> 16) / 2;
				int g = ((c & 0x0000FF00) >> 8) / 2;
				int b = (c & 0x000000FF) / 2;
				c = (r << 16) + (g << 8) + b;
				((long *)m_arrowSource[arrow][frame][8]->line[y])[x] = c;
			}
		}
	}

	//*/
	//////////////////////////////////////////////////////////////////////////
	// end overly-complicated arrow graphics initialization
	//////////////////////////////////////////////////////////////////////////

	// load and tint each Drummania note
	/*
	for ( color = 0; color < 6; color++ )
	{
		m_notesDM[color] = loadImage("DATA/gameplay/drummania/SPR_DM.tga");

		for ( int y = 0; y < 32; y++ )
		for ( int x = 0; x < 114; x++ )
		{
			unsigned long c = ((long *)m_notesDM[color]->line[y])[x];  
			int r = (c & 0x00FF0000) >> 16;
			int g = (c & 0x0000FF00) >> 8;
			int b = (c & 0x000000FF);
			r = WEIGHTED_AVERAGE(drummaniaColors[color][0], ARROW_BRIGHTNESS, r, 256);
			g = WEIGHTED_AVERAGE(drummaniaColors[color][1], ARROW_BRIGHTNESS, g, 256);
			b = WEIGHTED_AVERAGE(drummaniaColors[color][2], ARROW_BRIGHTNESS, b, 256);
			c = (r << 16) + (g << 8) + b;
			((long *)m_notesDM[color]->line[y])[x] = c;
		}
	}
	//*/

	// load and tint each beatmania note
	/*
	for ( color = 0; color < 3; color++ )
	{
		m_notesBM[color] = loadImage("DATA/gameplay/beatmania/SPR_BM.tga");

		for ( int y = 0; y < 56; y++ )
		for ( int x = 0; x < 90; x++ )
		{
			unsigned long c = ((long *)m_notesBM[color]->line[y])[x];  
			int r = (c & 0x00FF0000) >> 16;
			int g = (c & 0x0000FF00) >> 8;
			int b = (c & 0x000000FF);
			r = WEIGHTED_AVERAGE(beatmaniaColors[color][0], ARROW_BRIGHTNESS, r, 256);
			g = WEIGHTED_AVERAGE(beatmaniaColors[color][1], ARROW_BRIGHTNESS, g, 256);
			b = WEIGHTED_AVERAGE(beatmaniaColors[color][2], ARROW_BRIGHTNESS, b, 256);
			c = (r << 16) + (g << 8) + b;
			((long *)m_notesBM[color]->line[y])[x] = c;
		}
	}
	//*/

	// load the judgements
	int i = 0;
	for ( i = 0; i < 8; i++ )
	{
		char filename[] = "DATA/gameplay/judge_0.tga";
		filename[20] = i + '0';
		m_judgements[i][0] = loadImage(filename);

		char dmfilename[] = "DATA/gameplay/drummania/judge_0.tga";
		dmfilename[30] = i + '0';
		m_columnJudgements[i][0] = loadImage(dmfilename);
	}
	for ( i = 1; i < 3; i++ ) // marvellous, perfect, and great need animation frames
	{
		int j = 0; 
		for ( j = 0; j < 8; j++ )
		{
			m_judgements[j][i] = create_bitmap(256+i*5, 32+i*5);
			stretch_blit(m_judgements[j][0], m_judgements[j][i], 0, 0, 256, 32, 0, 0, 256+i*5, 32+i*5);

			m_columnJudgements[j][i] = create_bitmap(256+i*5, 32+i*5);
			stretch_blit(m_columnJudgements[j][0], m_columnJudgements[j][i], 0, 0, 256, 32, 0, 0, 256+i*5, 32+i*5);			
		}
	}

	// load the 'lane flashes' (marvellous/OK notebombs, really)
	for ( color = 0; color < 2; color++ )
	{
		char filename[] = "DATA/gameplay/flash_0.tga";
		filename[20] = color + '0';
		temp = loadImage(filename);

		for ( frame = 0; frame < 4; frame++ )
		{
			m_flashes[frame][color] = create_bitmap(75, 75);
			blit(temp, m_flashes[frame][color], 0, frame*75, 0, 0, 75, 75);
		}
		destroy_bitmap(temp);
	}

	// generate the 'lane flares' (the fade for when any note is hit)
	for ( arrow = 0; arrow < 6; arrow++ )
	for ( frame = 0; frame < 4; frame++ )
	for ( color = 0; color < 3; color++ )
	{
		m_flares[arrow][frame][color] = create_bitmap(64, 64);
		blit(m_stepZoneSource[arrow][0], m_flares[arrow][frame][color], 0, 0, 0, 0, 64, 64);
		replaceColor(m_flares[arrow][frame][color], 0xFF797979, ((frame*24+160)<<24) + (flareColors[color][0]<<16) + (flareColors[color][1]<<8) + (flareColors[color][2]) );
		replaceColor(m_flares[arrow][frame][color], 0x40000000, ((frame*24+160)<<24) + (flareColors[color][0]<<16) + (flareColors[color][1]<<8) + (flareColors[color][2]) );
		replaceColor(m_flares[arrow][frame][color], 0xFFF6F6F6, ((frame*24+160)<<24) + (flareColors[color][0]<<16) + (flareColors[color][1]<<8) + (flareColors[color][2]) );
	}

	// load combo graphics
	temp = loadImage("DATA/gameplay/combo.tga");
	for ( color = 0; color < 5; color++ )
	{
		m_combo[color] = create_bitmap(128, 24);
		blit(temp, m_combo[color], 0, color*24, 0, 0, 128, 24);
	}
	destroy_bitmap(temp);
	m_comboNums = loadImage("DATA/gameplay/combo_nums.tga");

	// load the special drummania graphics
	m_comboNumsDM = loadImage("DATA/gameplay/drummania/combo_nums.tga");
	m_comboWordDM = loadImage("DATA/gameplay/drummania/combo.tga");

	// load the special dmx graphics
	m_dmxHeader = loadImage("DATA/gameplay/dmx/header.tga");
	m_dmxFooter = loadImage("DATA/gameplay/dmx/footer.tga");
	m_stepZoneSourceDMX[0] = loadImage("DATA/gameplay/dmx/default_hitzone_00.tga");
	m_stepZoneSourceDMX[1] = loadImage("DATA/gameplay/dmx/default_hitzone_01.tga");
	m_notesDMX[0] = loadImage("DATA/gameplay/dmx/default_donut_00.tga");
	m_notesDMX[1] = loadImage("DATA/gameplay/dmx/default_donut_01.tga");
	m_notesDMX[2] = loadImage("DATA/gameplay/dmx/default_donut_02.tga");
	m_comboDMX = loadImage("DATA/gameplay/dmx/combo_word.BMP");
	for ( color = 0; color < 8; color++ )
	{
		char cFilename[] = "DATA/gameplay/dmx/combo_0000.BMP";
		char fFilename[] = "DATA/gameplay/dmx/fever_0000.BMP";
		cFilename[27] = fFilename[27] = color + '0';
		m_dmxCombos[color] = loadImage(cFilename);
		m_dmxFevers[color] = loadImage(fFilename);
	}
	m_leftLifeGaugeCover = loadImage("DATA/gameplay/dmx/left_blank.tga");
	m_rightLifeGaugeCover = loadImage("DATA/gameplay/dmx/right_blank.tga");
	for ( color = 0; color < 4; color++ )
	{
		char lFilename[] = "DATA/gameplay/dmx/1P_halo_0.tga";
		char rFilename[] = "DATA/gameplay/dmx/2P_halo_1.tga";
		lFilename[26] = rFilename[26] = color + '0';
		m_leftSideHalo[color][0] = loadImage(lFilename);
		m_rightSideHalo[color][0] = loadImage(rFilename);
		lFilename[21] = rFilename[21] = 'l';
		lFilename[22] = rFilename[22] = 'o';
		lFilename[23] = rFilename[23] = 'w';
		lFilename[24] = rFilename[24] = 'h';
		m_leftSideHalo[color][1] = loadImage(lFilename);
		m_rightSideHalo[color][1] = loadImage(rFilename);
	}
	m_dmxTransLane = loadImage("DATA/gameplay/dmx/trans_lane.tga");

	m_judgementsDMX = loadImage("DATA/gameplay/dmx/judgements.tga");
	for ( color = 0; color < 6; color++ )
	{
		char sFilename[] = "DATA/gameplay/dmx/smallbomb_0000.tga";
		char lFilename[] = "DATA/gameplay/dmx/largebomb_0000.tga";
		sFilename[31] = lFilename[31] = color + '0';
		BITMAP* sbmp = loadImage(sFilename);
		BITMAP* lbmp = loadImage(lFilename);

		// separate each frame - this is needed for a special rendering mode later
		for ( int frame = 0; frame < 16; frame++ )
		{
			m_dmxFireworks[0][color][frame] = create_bitmap_ex(32, 128, 128);
			m_dmxFireworks[1][color][frame] = create_bitmap_ex(32, 128, 128);
			int sx = (frame / 4) * 128;
			int sy = (frame % 4) * 128;
			blit(sbmp, m_dmxFireworks[0][color][frame], sx, sy, 0, 0, 128, 128);
			blit(lbmp, m_dmxFireworks[1][color][frame], sx, sy, 0, 0, 128, 128);
		}
		destroy_bitmap(sbmp);
		destroy_bitmap(lbmp);
	}

	// load full combo graphics
	temp = loadImage("DATA/gameplay/full_combo_anim.tga");
	for ( frame = 0; frame < 11; frame++ )
	{
		m_fullComboAnim[frame] = create_bitmap_ex(32, 266, 32);
		blit(temp, m_fullComboAnim[frame], 0, frame*32, 0, 0, 266, 32);
	}
	destroy_bitmap(temp);
	m_fcRays[0] = loadImage("DATA/gameplay/fc_rays_00.tga");
	m_fcRays[1] = loadImage("DATA/gameplay/fc_rays_01.tga");
	m_fcRays[2] = loadImage("DATA/gameplay/fc_rays_02.tga");
	m_fcRays[3] = loadImage("DATA/gameplay/fc_rays_03.tga");

	// load more graphics
	m_speedIcons = loadImage("DATA/gameplay/speed_icons.tga");
}

void renderStageDisplay()
{
	int y = 140;
	if ( gs.currentStage == -1 )
	{
		y = 140; // demo play = blank
	}
	else if ( gs.currentStage < gs.numSongsPerSet-1 )
	{
		y = 20*gs.currentStage;
	}
	else if ( gs.currentStage == gs.numSongsPerSet-1 )
	{
		y = 80; // "FINAL STAGE"
	}
	else
	{
		y = (gs.player[0].timeElapsed % 200) < 100 ? 120 : 100; // "EXTRA STAGE" blinks
	}
	masked_blit(m_stageDisplay, rm.m_backbuf, 0, y, 241, 10, 160, 20);

	// render the between-stage transition
	if ( isMidTransition )
	{
		int bannerx = 192; // centered
		if ( songTransitionTime < BANNER_ANIM_STEP )
		{
			bannerx = getValueFromRange(-256, 192, songTransitionTime*100 / BANNER_ANIM_STEP);
		}
		else if ( songTransitionTime > BANNER_ANIM_STEP*2 )
		{
			bannerx = getValueFromRange(192, 640, (songTransitionTime-(BANNER_ANIM_STEP*2))*100 / BANNER_ANIM_STEP);
		}

		if ( songTransitionTime <= 0 )
		{
			isMidTransition = false;
		}

		//al_trace("bannerx = %d\r\n", bannerx);
		int bannerIndex = songID_to_listID(gs.player[0].stagesPlayed[gs.currentStage]);
		blit(m_banners[bannerIndex], rm.m_backbuf, 0, 0, bannerx, 112, 256, 256);
	}
}

void renderGrade(int grade, int x, int y)
{
	int sy = 48 * grade; // F,E,D,C,...,A,AA (S),AAA (SS)

	int color = 0; // yellow/green/blue/red
	if ( grade <= GRADE_F )
	{
		color = 3;
	}
	else if ( grade <= GRADE_D )
	{
		color = 2;
	}
	else if ( grade <= GRADE_B )
	{
		color = 1;
	}
	int sx = 48 * color;

	masked_blit(m_grades, rm.m_backbuf, sx, sy, x, y, 48, 48);
}

void renderSpeedMod(int player, int scrollRate, int speedMod)
{
	//renderWhiteNumber(speedMod/10, x, y);
	//renderWhiteLetter('.', x+10, y);
	//renderWhiteNumber(speedMod%10, x+20, y);
	//renderWhiteLetter('x', x+40, y);
	//renderWhiteNumber(scrollRate*speedMod/10, x+70, y);
	//renderWhiteLetter('=', x+100, y);
	//renderWhiteNumber(MSEC_TO_BPM(scrollRate), x+120, y);

	int x = getColumnOffsetX_ANY(0) - 64;
	int y = 360;
	if ( player == 0 && !gs.isDoubles && !gs.isVersus && gs.player[0].isCenter() )
	{
		x = getColumnOffsetX_ANY(3) - 64;
	}
	if ( player == 1 || (player == 0 && gs.player[0].centerRight) )
	{
		x = getColumnOffsetX_ANY(7) +32 + 32; // should only happen in DMX mode
	}
	if ( gs.player[player].reverseModifier != 0 )
	{
		y = 84;
	}

	int index = (speedMod / 5) - 1;
	UNUSED(scrollRate); // maybe someday render BPM somewhere on screen if it is appropriate?
	masked_blit(m_speedIcons, rm.m_backbuf, 0, index*24, x, y, 32, 24);
}

void renderLifebar(int player)
{
	static int blueColors[15] = { 0x6B84FF, 0x8494FF, 0x8494FF, 0xBDCEFF, 0xFFFFFF, 0xC6CEFF, 0xA5B5FF, 0x8494FF, 0x8494FF,
								  0x6B84FF, 0x8494FF, 0x8494FF, 0x6B84FF, 0x8494FF, 0x8494FF };
	static int goldColors[15] = { 0xFFA851, 0xFFC171, 0xFFC171, 0xFFCDAF, 0xFFFFFF, 0xFFE0BC, 0xFFC895, 0xFFC171, 0xFFC171,
								  0xFFA851, 0xFFC171, 0xFFC171, 0xFFA851, 0xFFC171, 0xFFC171 };
	int bgc1 = 0x525E60;
	int bgc2 = 0x525EB0; //0x62699B;

	int startx    = player == 0 ? 234 : 404;
	int direction = player == 0 ? -1 : +1;    // right to left or left to right?
	int numPixels = getValueFromRange(0, 170, gs.player[player].lifebarPercent/10);
	int ci = getValueFromRange( 0, 14, (gs.player[player].stepZoneBeatTimer*100 / gs.player[player].stepZoneTimePerBeat) );

	solid_mode();
	for ( int i = 0; i < 170; i++ )
	{
		if ( i < numPixels )
		{
			line(rm.m_backbuf, startx + (direction*i), 0, startx + (direction*i), 46, numPixels >= 119 ? goldColors[ci] : blueColors[ci]);
			ci = (ci + 1) % 15;
		}
		else // use a background color instead of the "color index" ci
		{
			int color = ci < 8 ? getValueFromRange(bgc2, bgc1, (ci-8)*100/8) : getValueFromRange(bgc1, bgc2, ci*100/7); // pulse the blue color back and forth [0..7] then [8..14] then [0..7]
			if ( gs.player[player].useBattery )
			{
				int red = ci < 8 ? getValueFromRange(0, 255, (ci-8)*100/8) : getValueFromRange(255, 0, ci*100/7);
				color = makecol(red, 0, 0);
			}
			line(rm.m_backbuf, startx + (direction*i), 0, startx + (direction*i), 46, color);
		}
	}
	set_alpha_blender(); // the game assumes the graphics are left in this mode

	if ( gs.player[player].useBattery )
	{
		renderBatteryLivesDMX(player);
	}
}

void renderFullComboAnim(int x, int time, int step)
{
	int frame = (time / 50) % 4;
	int y = 86;
	//y = (gs.player[player].isColumnReversed(i) ? DMX_STEP_ZONE_REV_Y-34 : DMX_STEP_ZONE_Y)-46;
	time %= 1000;

	x -= 64; // because we pass in the CENTER of the player's play area, and this animation is 128 pixels wide

	if ( step == 1 )
	{
		set_add_blender(0,0,0,256);
		draw_trans_sprite(rm.m_backbuf, m_fcRays[frame], x, y-300+time);
		//blit(m_fcRays[frame], rm.m_backbuf, 0, 0, x+65, y, 128, 216);
		set_alpha_blender(); // the game assumes the graphics are left in this mode
	}
	else if ( step == 2 ) // "FULL COMBO" fading in
	{
		frame = MIN(time/60, 10);
		draw_trans_sprite(rm.m_backbuf, m_fullComboAnim[frame], x-69, 320);
	}
	else if ( step == 3 ) // stationary text
	{
		draw_trans_sprite(rm.m_backbuf, m_fullComboAnim[10], x-69, 320);
	}
	else if ( step > 3 ) // "FULL COMBO" fading out
	{
		frame = 10 - time/60;
		if ( frame >= 0 )
		{
			draw_trans_sprite(rm.m_backbuf, m_fullComboAnim[frame], x-69, 320);
		}
	}
}

void renderEndSongMarker(int x1, int x2, int y)
{
	line(rm.m_backbuf, x1, y+0, x2, y+0, END_SECTION_COLOR1);
	line(rm.m_backbuf, x1, y+1, x2, y+1, END_SECTION_COLOR2);
	line(rm.m_backbuf, x1, y+2, x2, y+2, END_SECTION_COLOR1);
	line(rm.m_backbuf, x1, y+3, x2, y+3, COLOR_WHITE);
}

void renderBPMMarker(int x1, int x2, int y, int bpm)
{
	line(rm.m_backbuf, x1, y+0, x2, y+0, BPM_CHANGE_COLOR1);
	line(rm.m_backbuf, x1, y+1, x2, y+1, BPM_CHANGE_COLOR2);
	line(rm.m_backbuf, x1, y+2, x2, y+2, BPM_CHANGE_COLOR1);
	line(rm.m_backbuf, x1, y+3, x2, y+3, COLOR_WHITE);
	renderWhiteNumber(bpm, x2+3, y-10);
}

void renderTempoStopMarker(int x1, int x2, int y, int len)
{
	line(rm.m_backbuf, x1, y+0, x2, y+0, SCROLL_STOP_COLOR1);
	line(rm.m_backbuf, x1, y+1, x2, y+1, SCROLL_STOP_COLOR2);
	line(rm.m_backbuf, x1, y+2, x2, y+2, SCROLL_STOP_COLOR1);
	line(rm.m_backbuf, x1, y+3, x2, y+3, COLOR_WHITE);
	renderWhiteNumber(len, x2+3, y+15);
}

void renderGameplay()
{
	//blit(m_bg, rm.m_backbuf, 0, 0, 0, 0, 640, 480);
	clear_to_color(rm.m_backbuf, 0);
	vm.renderToSurfaceStretched(rm.m_backbuf, 0, 46, 640, 384);

	if ( gs.player[0].danceManiaxMode )
	{
		renderUnderFrameDMX();
	}

	// do this whole thing once for each player
	for ( int p = 0; p < (gs.isVersus ? 2 : 1); p++ )
	{
	// this makes rendering the combo and judgements easier later
	int centered_x = getColumnOffsetX_DMX(p==0 ? 0 : 4) + 64;
	if ( gs.player[p].danceManiaxMode == false )
	{
		centered_x = getColumnOffsetX_DDR(p==0 ? 0 : 4) + 128;
	}
	if ( gs.isDoubles || (gs.isSingles() && gs.player[0].isCenter()) )
	{
		centered_x = 320;
	}
	if ( gs.isSingles() )
	{
		if ( gs.player[0].centerLeft )
		{
			centered_x = getColumnOffsetX_DMX(0) + 64;
		}
		else if ( gs.player[0].centerRight )
		{
			centered_x = getColumnOffsetX_DMX(4) + 64;
		}
	}

	// usually the combo goes behind the chart
	if ( !gs.player[p].drummaniaMode && !gs.player[p].danceManiaxMode )
	{
		renderDDRCombo(gs.player[p].displayCombo, gs.player[p].judgementTime, centered_x, gs.player[p].comboColor);
	}

	// step zone
	if ( gs.player[p].danceManiaxMode )
	{
		renderStepZoneDMX(p);
		renderDMXCombo(gs.player[p].displayCombo, gs.player[p].judgementTime, centered_x, gs.player[p].comboColor);
	}
	else
	{
		renderStepZone_DDR(p);
	}

	// timing judgements - under the notes
	if ( gs.player[p].judgementTime < JUDGEMENT_DISPLAY_TIME )
	{
		if ( gs.player[p].danceManiaxMode )
		{
			renderDMXJudgement(gs.player[p].lastJudgement, gs.player[p].judgementTime, centered_x, JUDGEMENT_Y);
		}
	}

	// chart and arrows/rings/chips/whatever
	if ( gs.player[p].currentChart.size() > 0 )
	{
		if ( gs.player[p].danceManiaxMode )
		{
			renderDMXFlaresAndFlashes(p);
			renderDMXChart(p);
		}
		else
		{
			renderDDRChart(p);
			renderFlaresAndFlashes_DDR(p);
		}
	}

	renderLifebar(p);

	// timing judgements - on top of the notes
	if ( gs.player[p].judgementTime < JUDGEMENT_DISPLAY_TIME )
	{
		if ( !gs.player[p].danceManiaxMode )
		{
			renderDDRJudgement(gs.player[p].lastJudgement, gs.player[p].judgementTime, centered_x, JUDGEMENT_Y);
		}
	}
	renderColumnJudgements(p);

	// drummania's combo goes on top of the chart
	if ( gs.player[p].drummaniaMode )
	{
		renderDMCombo(gs.player[p].displayCombo, p);
	}

	renderSpeedMod(p, gs.player[p].scrollRate, gs.player[p].speedMod);

	} // done with 1P 2P loop

	renderParticles(rm.m_backbuf);

	// render things on the frame
	if ( gs.player[0].danceManiaxMode )
	{
		renderOverFrameDMX();
	}
	renderStageDisplay();
	for ( int i = 0; i < (gs.isVersus ? 2 : 1); i++ )
	{
		int grade = sm.player[i].currentSet[gs.currentStage].calculateGrade();
		renderGrade(grade, i == 0 ? 6 : 586, 0);
		renderScoreNumber(sm.player[i].currentSet[gs.currentStage].getScore(), i == 0 ? 32 : 426, 444, 7);
	}

	/* current chart time display
	renderWhiteNumber(gs.player[0].timeElapsed, 10, 70);
	//*/

	// retire timer
	if ( retireTimer > 14000 )
	{
		renderBoldString("GIVE UP? RETIRE?", 223, 200, 640, false, (retireTimer/50)%4);
		char digit = (retireTimer-14000)/1000;
		renderNameLetter('5'-digit, 304, 240, (retireTimer/75)%3);
	}

	if ( fullComboP1 )
	{
		int centerx = getCenterOfLanesX(0);
		renderFullComboAnim(centerx, fullComboAnimTimer, fullComboAnimStep);
	}
	if ( fullComboP2 )
	{
		int centerx = getCenterOfLanesX(1);
		renderFullComboAnim(centerx, fullComboAnimTimer, fullComboAnimStep);
	}
}

void renderDebugNote(struct ARROW n, int x, int y)
{
	char tempstring[256] = "";
	if ( n.columns[0] == -1 )
	{
		sprintf_s(tempstring, 256, "%ld = %d", n.timing, n.type); 
	}
	else if ( n.columns[1] == -1 )
	{
		sprintf_s(tempstring, 256, "%ld = %d [%d]", n.timing, n.type, n.columns[0]); 
	}
	else
	{
		sprintf_s(tempstring, 256, "%ld = %d [%d %d]", n.timing, n.type, n.columns[0], n.columns[1]); 
	}
	renderWhiteString(tempstring, x, y);
}