/*******************************************************************
 Copyright (C) 1998 Tuomas J. Lukka 2003 John Stewart, Ayla Khan CRC Canada
 Portions Copyright (C) 1998 John Breen
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*
 * $Id$
 *
 */

#include "OpenGL_Utils.h"

static int render_frame = 5;	/* do we render, or do we sleep? */
static int now_mapped = 1;		/* are we on screen, or minimized? */


int
get_now_mapped()
{
	return now_mapped;
}


void
set_now_mapped(int val)
{
	now_mapped = val;
}


/* should we render? */
void
set_render_frame()
{
	/* render a couple of frames to let events propagate */
	render_frame = 5;
}


int
get_render_frame()
{
	return (render_frame && now_mapped);
}


void 
dec_render_frame()
{
	if (render_frame > 0) {
		render_frame--;
	}
}
