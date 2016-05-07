/*
 * gdisp_lld_U1617s.c
 *
 *  Created on: Apr 2, 2016
 *      Author: daniel
 */

#ifndef DRIVERS_GDISP_U1617S_GDISP_LLD_U1617S_C_
#define DRIVERS_GDISP_U1617S_GDISP_LLD_U1617S_C_



#include "gfx.h"

#if GFX_USE_GDISP

#define GDISP_DRIVER_VMT			GDISPVMT_U1617s
#include <drivers/gdisp/U1617s/gdisp_lld_config.h>
#include "src/gdisp/gdisp_driver.h"

#include <board_U1617s.h>

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#ifndef GDISP_SCREEN_HEIGHT
	#define GDISP_SCREEN_HEIGHT		128
#endif
#ifndef GDISP_SCREEN_WIDTH
	#define GDISP_SCREEN_WIDTH		128
#endif
#ifndef GDISP_INITIAL_CONTRAST
	#define GDISP_INITIAL_CONTRAST	27
#endif
#ifndef GDISP_INITIAL_BACKLIGHT
	#define GDISP_INITIAL_BACKLIGHT	100
#endif

#define GDISP_FLG_NEEDFLUSH			(GDISP_FLG_DRIVER<<0)

#include <drivers/gdisp/U1617s/u1617s.h>

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

// Some common routines and macros
#define RAM(g)							((uint8_t *)g->priv)
#define write_cmd2(g, cmd1, cmd2)		{ write_cmd(g, cmd1); write_cmd(g, cmd2); }
#define write_cmd3(g, cmd1, cmd2, cmd3)	{ write_cmd(g, cmd1); write_cmd(g, cmd2); write_cmd(g, cmd3); }

// Some common routines and macros
#define delay(us)			gfxSleepMicroseconds(us)
#define delay_ms(ms)		gfxSleepMilliseconds(ms)

#define xyaddr(x, y)		( (GDISP_SCREEN_WIDTH >> 2) * y + (x >> 2) )
#define xybits(y)			((x % 4) >> 1)

// Display Buffer(4 Shade mode)
#define DISPLAY_RAM_SIZE	(GDISP_SCREEN_HEIGHT * GDISP_SCREEN_WIDTH / 8) * 2
static uint8_t				s_DisplayRAM[DISPLAY_RAM_SIZE];

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/*
 * As this controller can't update on a pixel boundary we need to maintain the
 * the entire display surface in memory so that we can do the necessary bit
 * operations. Fortunately it is a small display in monochrome.
 * 64 * 128 / 8 = 1024 bytes.
 */

LLDSPEC bool_t gdisp_lld_init(GDisplay *g) {
	// The private area is the display surface.
	//g->priv = gfxAlloc(GDISP_SCREEN_HEIGHT * GDISP_SCREEN_WIDTH / 8);
	g->priv = s_DisplayRAM;

	// Initialise the board interface
	init_board(g);

	// Hardware reset
	setpin_reset(g, TRUE);
	gfxSleepMilliseconds(200);
	setpin_reset(g, FALSE);
	gfxSleepMilliseconds(50);

	acquire_bus(g);

	write_cmd(g, 0xE2);//system reset
	delay(10);
	write_cmd(g, 0x27);
	write_cmd(g, 0x2b);
	write_cmd(g, 0x2f); //set pump control
	write_cmd(g, 0xeb); //set bias=1/11
	write_cmd2(g, 0x81, 0x33); //set contrast,when 0x36:set PM=12,vop=12.8v,4c
	write_cmd(g, 0xa3);
	//write_cmd(g, 0xa9); //set linerate mux,a2
	write_cmd(g, 0xc8);
	write_cmd(g, 0x0b);
	write_cmd(g, 0x89);	// RAM address control(auto,page first)
	write_cmd(g, 0xc4); //MY=1,MX=0:从左到右，再从上到下。
	write_cmd(g, 0xf1); //f1
	write_cmd(g, 0x7f);
	write_cmd(g, 0xd3); //gray shade set
	write_cmd(g, 0xd7); //gray shade set
	write_cmd(g, 0xAf); //set display enable

	delay_ms(10);

    // Finish Init
    post_init_board(g);

 	// Release the bus
	release_bus(g);

	/* Initialise the GDISP structure */
	g->g.Width = GDISP_SCREEN_WIDTH;
	g->g.Height = GDISP_SCREEN_HEIGHT;
	g->g.Orientation = GDISP_ROTATE_0;
	g->g.Powermode = powerOn;
	g->g.Backlight = GDISP_INITIAL_BACKLIGHT;
	g->g.Contrast = GDISP_INITIAL_CONTRAST;
	return TRUE;
}

#if GDISP_HARDWARE_FLUSH
	LLDSPEC void gdisp_lld_flush(GDisplay *g) {
		// Don't flush if we don't need it.
		if (!(g->flags & GDISP_FLG_NEEDFLUSH))
			return;

		acquire_bus(g);

		write_cmd(g, 0x60); //行地址低4 位
		write_cmd(g, 0x70); //行地址高3 位
		write_cmd(g, 0x00); //列地址

		//delay(1);
		taskYIELD();

		write_data(g, RAM(g), DISPLAY_RAM_SIZE);

		release_bus(g);

		g->flags &= ~GDISP_FLG_NEEDFLUSH;
	}
#endif

#if GDISP_HARDWARE_DRAWPIXEL
	LLDSPEC void gdisp_lld_draw_pixel(GDisplay *g) {
		coord_t		x, y;
		uint8_t *	p;
		uint8_t		bits;

		switch(g->g.Orientation) {
		default:
		case GDISP_ROTATE_0:
			x = g->p.x;
			y = g->p.y;
			break;
		case GDISP_ROTATE_90:
			x = g->p.y;
			y = GDISP_SCREEN_HEIGHT-1 - g->p.x;
			break;
		case GDISP_ROTATE_180:
			x = GDISP_SCREEN_WIDTH-1 - g->p.x;
			y = GDISP_SCREEN_HEIGHT-1 - g->p.y;
			break;
		case GDISP_ROTATE_270:
			x = GDISP_SCREEN_HEIGHT-1 - g->p.y;
			y = g->p.x;
			break;
		}

		p = RAM(g) + xyaddr(x, y);
		bits = (x % 4) << 1;
		*p &= ~(0x3 << bits);
		*p |= ((g->p.color & 0x3) << bits);

		g->flags |= GDISP_FLG_NEEDFLUSH;
	}
#endif

#if GDISP_HARDWARE_PIXELREAD
	LLDSPEC color_t gdisp_lld_get_pixel_color(GDisplay *g) {
		coord_t		x, y;

		switch(g->g.Orientation) {
		default:
		case GDISP_ROTATE_0:
			x = g->p.x;
			y = g->p.y;
			break;
		case GDISP_ROTATE_90:
			x = g->p.y;
			y = GDISP_SCREEN_HEIGHT-1 - g->p.x;
			break;
		case GDISP_ROTATE_180:
			x = GDISP_SCREEN_WIDTH-1 - g->p.x;
			y = GDISP_SCREEN_HEIGHT-1 - g->p.y;
			break;
		case GDISP_ROTATE_270:
			x = GDISP_SCREEN_HEIGHT-1 - g->p.y;
			x = g->p.x;
			break;
		}
		return 0; //(RAM(g)[xyaddr(x, y)] & xybit(y)) ? White : Black;
	}
#endif

#if GDISP_NEED_CONTROL && GDISP_HARDWARE_CONTROL
	LLDSPEC void gdisp_lld_control(GDisplay *g) {
		switch(g->p.x) {
		case GDISP_CONTROL_POWER:
			if (g->g.Powermode == (powermode_t)g->p.ptr)
				return;
			switch((powermode_t)g->p.ptr) {
			case powerOff:
			case powerSleep:
			case powerDeepSleep:
				acquire_bus(g);
				write_cmd(g, 0xAE);
				release_bus(g);
				break;
			case powerOn:
				acquire_bus(g);
				write_cmd(g, 0xAF);
				release_bus(g);
				break;
			default:
				return;
			}
			g->g.Powermode = (powermode_t)g->p.ptr;
			return;

		case GDISP_CONTROL_ORIENTATION:
			if (g->g.Orientation == (orientation_t)g->p.ptr)
				return;
			switch((orientation_t)g->p.ptr) {
			/* Rotation is handled by the drawing routines */
			case GDISP_ROTATE_0:
			case GDISP_ROTATE_180:
				g->g.Height = GDISP_SCREEN_HEIGHT;
				g->g.Width = GDISP_SCREEN_WIDTH;
				break;
			case GDISP_ROTATE_90:
			case GDISP_ROTATE_270:
				g->g.Height = GDISP_SCREEN_WIDTH;
				g->g.Width = GDISP_SCREEN_HEIGHT;
				break;
			default:
				return;
			}
			g->g.Orientation = (orientation_t)g->p.ptr;
			return;

		case GDISP_CONTROL_CONTRAST:
            if ((unsigned)g->p.ptr > 100)
            	g->p.ptr = (void *)100;
			acquire_bus(g);
			write_cmd2(g, 0x81, ((unsigned)g->p.ptr) * 193 / 100);
			release_bus(g);
            g->g.Contrast = (unsigned)g->p.ptr;
			return;
		}
	}
#endif // GDISP_NEED_CONTROL

#endif // GFX_USE_GDISP



#endif /* DRIVERS_GDISP_U1617S_GDISP_LLD_U1617S_C_ */
