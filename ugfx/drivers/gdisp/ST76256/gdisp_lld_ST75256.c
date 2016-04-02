/*
 * gdisp_lld_ST75256.c
 *
 *  Created on: Mar 30, 2016
 *      Author: daniel
 */

#include "gfx.h"

#if GFX_USE_GDISP

#define GDISP_DRIVER_VMT			GDISPVMT_ST75256
#include <drivers/gdisp/ST75256/gdisp_lld_config.h>
#include "src/gdisp/gdisp_driver.h"

#include <board_ST75256.h>

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#ifndef GDISP_SCREEN_HEIGHT
	#define GDISP_SCREEN_HEIGHT		64
#endif
#ifndef GDISP_SCREEN_WIDTH
	#define GDISP_SCREEN_WIDTH		128
#endif
#ifndef GDISP_INITIAL_CONTRAST
	#define GDISP_INITIAL_CONTRAST	51
#endif
#ifndef GDISP_INITIAL_BACKLIGHT
	#define GDISP_INITIAL_BACKLIGHT	100
#endif

#define GDISP_FLG_NEEDFLUSH			(GDISP_FLG_DRIVER<<0)

#include <drivers/gdisp/ST7565R/st7565r.h>

#endif // GFX_USE_GDISP




