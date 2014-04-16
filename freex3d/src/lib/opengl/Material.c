/*


Only do material settings that "matter" and bounds check all values.

*/


/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/



#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>
#include "Material.h"

#include <libFreeWRL.h>

/* #include "LinearAlgebra.h" */

int verify_rotate(GLfloat *params) {
	/* angle very close to zero? */
	if (fabs(params[3]) < 0.001) return FALSE;
	return TRUE;
}

int verify_translate(GLfloat *params) {
	/* no translation? */

	if ((fabs(params[0]) < 0.001) &&
		(fabs(params[1]) < 0.001) &&
		(fabs(params[2]) < 0.001))  return FALSE;
	return TRUE;
}


int verify_scale(GLfloat *params) {
	/* no translation? */

	if ((fabs(params[0]-1.0) < 0.001) &&
		(fabs(params[1]-1.0) < 0.001) &&
		(fabs(params[2]-1.0) < 0.001))  return FALSE;

	return TRUE;
}
