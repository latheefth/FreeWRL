/*


X3D Layering Component

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

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"

#include "../x3d_parser/Bindable.h"
#include "Children.h"
#include "../opengl/OpenGL_Utils.h"


// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/layout.html
/* oct 2015 not sure what I need here
a few easy ones: 
ScreenFontStyle is the same as FontStyle except with pixelSize instead of size
- I suspect implementation code would be primarily in Component_Text.c L.1326
- size (FontStyle) and pointSize (ScreenFontStyle) are both float, so no diff in struct, just NODE_Type
ScreenGroup is like Group or perhaps Transform, except it may need to alter the transform stack 
	with x,y scales so as to treat children's coords as being in screen pixels.
*/
void render_Layout(struct X3D_Node * node){
	if(node && node->_nodeType == NODE_Layout){
	}
}
void rendray_Layout(struct X3D_Node *node){
	//for picking
	float h,r,y;
	struct point_XYZ t_r1,t_r2;
	get_current_ray(&t_r1, &t_r2);

}
