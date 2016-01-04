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
#include "../scenegraph/RenderFuncs.h"

// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/layout.html
/* oct 2015 not sure what I need here
a few easy ones: 
ScreenFontStyle is the same as FontStyle except with pixelSize instead of size
- I suspect implementation code would be primarily in Component_Text.c L.1326
- size (FontStyle) and pointSize (ScreenFontStyle) are both float, so no diff in struct, just NODE_Type
ScreenGroup is like Group or perhaps Transform, except it may need to alter the transform stack 
	with x,y scales so as to treat children's coords as being in screen pixels.
Layout			- (just info node used in other nodes)
LayoutGroup		- (like Group, sets viewport, layout) prep, fin, ChildC [compileC]
LayoutLayer		- (like Group, sets viewport, layout, Ortho) prep, fin, ChildC [compileC]
ScreenFontStyle	- (same as FontStyle node - just info node for Text)
ScreenGroup		- (like Group except modifies scale) prep, fin, ChildC [compileC]
(note regular Group is funny: it sorts children by depth often, for transparency blending purposes
	- but I assume we won't do any sorting by depth from our group-like nodes, therefore we don't need Compile)
*/

void prep_LayoutGroup(struct X3D_Node *node){
}
void child_LayoutGroup(struct X3D_Node *node){
}
void fin_LayoutGroup(struct X3D_Node *node){
}
void prep_LayoutLayer(struct X3D_Node *node){
}
void child_LayoutLayer(struct X3D_Node *node){
}
void fin_LayoutLayer(struct X3D_Node *node){
}
void prep_ScreenGroup(struct X3D_Node *node){
}
void child_ScreenGroup(struct X3D_Node *node){
}
void fin_ScreenGroup(struct X3D_Node *node){
}