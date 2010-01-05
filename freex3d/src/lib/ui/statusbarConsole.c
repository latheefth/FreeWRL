/*
  $Id$

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
#include <list.h>
#include <io_files.h>
#include <resources.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../world_script/CScripts.h"
#include "../vrml_parser/CParseParser.h"
#include "../vrml_parser/CParseLexer.h"
#include "../vrml_parser/CParse.h"

#include <float.h>

#include "../x3d_parser/Bindable.h"
#include "../scenegraph/Collision.h"
#include "../scenegraph/quaternion.h"
#include "../scenegraph/Viewer.h"

#include "../opengl/RasterFont.h"



/*fw fixed size bitmap fonts >>>
first 1: char # 0-127 (ie '!'=33, 'a'=97)
next 6: glBitmap(width,height,xbo,ybo,xadv,yadv,
last 7+: glBitmap(,,,,,,const GLubyte *bitmap);
*/
GLubyte fwLetters7x12[][19] = {
{32,7,12,0,0,7,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
{33,7,12,0,0,7,0,0x0,0x0,0x40,0x40,0x0,0x40,0x40,0x40,0x40,0x40,0x0,0x0},
{34,7,12,0,0,7,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x50,0x50,0x50,0x50,0x0},
{35,7,12,0,0,7,0,0x0,0x0,0x50,0x28,0xfc,0x28,0x28,0x7e,0x24,0x12,0x0,0x0},
{36,7,12,0,0,7,0,0x0,0x20,0x70,0xa8,0x28,0x70,0xa0,0xa8,0x70,0x20,0x0,0x0},
{37,7,12,0,0,7,0,0x0,0x0,0x9c,0x54,0x54,0x2c,0xd0,0xa8,0xa8,0xe6,0x0,0x0},
{38,7,12,0,0,7,0,0x0,0x0,0x64,0x98,0x8c,0x50,0x20,0x58,0x48,0x30,0x0,0x0},
{39,7,12,0,0,7,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x40,0x20,0x10,0x10,0x0},
{40,7,12,0,0,7,0,0x10,0x20,0x20,0x40,0x40,0x40,0x40,0x20,0x20,0x10,0x0,0x0},
{41,7,12,0,0,7,0,0x0,0x40,0x20,0x20,0x10,0x10,0x10,0x20,0x20,0x40,0x0,0x0},
{42,7,12,0,0,7,0,0x0,0x0,0x0,0x0,0x0,0x20,0xa8,0x50,0xa8,0x20,0x0,0x0},
{43,7,12,0,0,7,0,0x0,0x0,0x0,0x8,0x8,0x3e,0x8,0x8,0x0,0x0,0x0,0x0},
{44,7,12,0,0,7,0,0x0,0x8,0x8,0x18,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
{45,7,12,0,0,7,0,0x0,0x0,0x0,0x0,0x0,0xf8,0x0,0x0,0x0,0x0,0x0,0x0},
{46,7,12,0,0,7,0,0x0,0x0,0x18,0x18,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
{47,7,12,0,0,7,0,0x0,0x80,0x80,0x40,0x40,0x20,0x20,0x10,0x10,0x8,0x8,0x0},
{48,7,12,0,0,7,0,0x0,0x0,0x70,0x88,0x88,0xc8,0x98,0x88,0x88,0x70,0x0,0x0},
{49,7,12,0,0,7,0,0x0,0x0,0x38,0x10,0x10,0x10,0x10,0x10,0x30,0x10,0x0,0x0},
{50,7,12,0,0,7,0,0x0,0x0,0xf8,0x80,0x40,0x20,0x10,0x88,0x88,0x70,0x0,0x0},
{51,7,12,0,0,7,0,0x0,0x0,0x70,0x88,0x8,0x8,0x30,0x8,0x8,0xf8,0x0,0x0},
{52,7,12,0,0,7,0,0x0,0x0,0x10,0x10,0x10,0xf8,0x90,0x50,0x50,0x50,0x0,0x0},
{53,7,12,0,0,7,0,0x0,0x0,0xf0,0x8,0x8,0x88,0xf0,0x80,0x80,0xf8,0x0,0x0},
{54,7,12,0,0,7,0,0x0,0x0,0x70,0x88,0x88,0xc8,0xb0,0x80,0x80,0x78,0x0,0x0},
{55,7,12,0,0,7,0,0x0,0x0,0x20,0x20,0x20,0x20,0x10,0x10,0x88,0xf8,0x0,0x0},
{56,7,12,0,0,7,0,0x0,0x0,0x70,0x88,0x88,0x88,0x70,0x88,0x88,0x70,0x0,0x0},
{57,7,12,0,0,7,0,0x0,0x0,0x70,0x88,0x8,0x68,0x98,0x88,0x88,0x70,0x0,0x0},
{58,7,12,0,0,7,0,0x0,0x0,0x80,0x80,0x0,0x0,0x80,0x80,0x0,0x0,0x0,0x0},
{59,7,12,0,0,7,0,0x0,0x20,0x10,0x30,0x0,0x0,0x10,0x10,0x0,0x0,0x0,0x0},
{60,7,12,0,0,7,0,0x0,0x8,0x10,0x20,0x40,0x80,0x40,0x20,0x10,0x8,0x0,0x0},
{61,7,12,0,0,7,0,0x0,0x0,0x0,0x0,0xf8,0x0,0xf8,0x0,0x0,0x0,0x0,0x0},
{62,7,12,0,0,7,0,0x0,0x80,0x40,0x20,0x10,0x8,0x10,0x20,0x40,0x80,0x0,0x0},
{63,7,12,0,0,7,0,0x0,0x20,0x20,0x0,0x20,0x10,0x10,0x8,0x88,0x70,0x0,0x0},
{64,7,12,0,0,7,0,0x0,0x0,0x38,0x40,0x98,0xa4,0xa4,0x98,0x48,0x30,0x0,0x0},
{65,7,12,0,0,7,0,0x0,0x0,0x88,0xf8,0x88,0x88,0x50,0x50,0x20,0x20,0x0,0x0},
{66,7,12,0,0,7,0,0x0,0x0,0xf0,0x88,0x88,0x90,0xf0,0x88,0x88,0xf0,0x0,0x0},
{67,7,12,0,0,7,0,0x0,0x0,0x70,0x88,0x88,0x80,0x80,0x80,0x88,0x70,0x0,0x0},
{68,7,12,0,0,7,0,0x0,0x0,0xf0,0x88,0x88,0x88,0x88,0x88,0x88,0xf0,0x0,0x0},
{69,7,12,0,0,7,0,0x0,0x0,0xf8,0x80,0x80,0x80,0xe0,0x80,0x80,0xf8,0x0,0x0},
{70,7,12,0,0,7,0,0x0,0x0,0x80,0x80,0x80,0x80,0xe0,0x80,0x80,0xf8,0x0,0x0},
{71,7,12,0,0,7,0,0x0,0x0,0x68,0x98,0x88,0x88,0x90,0x80,0x88,0x78,0x0,0x0},
{72,7,12,0,0,7,0,0x0,0x0,0x88,0x88,0x88,0x88,0xf8,0x88,0x88,0x88,0x0,0x0},
{73,7,12,0,0,7,0,0x0,0x0,0x70,0x20,0x20,0x20,0x20,0x20,0x20,0x70,0x0,0x0},
{74,7,12,0,0,7,0,0x0,0x0,0x70,0x88,0x8,0x8,0x8,0x8,0x8,0x1c,0x0,0x0},
{75,7,12,0,0,7,0,0x0,0x0,0x84,0x88,0x90,0xa0,0xc0,0xa0,0x90,0x88,0x0,0x0},
{76,7,12,0,0,7,0,0x0,0x0,0xfc,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x0,0x0},
{77,7,12,0,0,7,0,0x0,0x0,0x88,0xa8,0xa8,0xa8,0xd8,0xd8,0x88,0x88,0x0,0x0},
{78,7,12,0,0,7,0,0x0,0x0,0x88,0x98,0x98,0xa8,0xa8,0xc8,0xc8,0x88,0x0,0x0},
{79,7,12,0,0,7,0,0x0,0x0,0x70,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x0,0x0},
{80,7,12,0,0,7,0,0x0,0x0,0x80,0x80,0x80,0xc0,0xb0,0x88,0x88,0xf0,0x0,0x0},
{81,7,12,0,0,7,0,0xc,0x10,0x70,0xa8,0xa8,0x88,0x88,0x88,0x88,0x70,0x0,0x0},
{82,7,12,0,0,7,0,0x0,0x0,0x84,0x88,0x90,0xa0,0xf0,0x88,0x88,0xf0,0x0,0x0},
{83,7,12,0,0,7,0,0x0,0x0,0x70,0x88,0x8,0x10,0x70,0x80,0x88,0x70,0x0,0x0},
{84,7,12,0,0,7,0,0x0,0x0,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0xf8,0x0,0x0},
{85,7,12,0,0,7,0,0x0,0x0,0x70,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x0,0x0},
{86,7,12,0,0,7,0,0x0,0x0,0x20,0x20,0x50,0x50,0x50,0x88,0x88,0x88,0x0,0x0},
{87,7,12,0,0,7,0,0x0,0x0,0x50,0x50,0xf8,0xa8,0xa8,0xa8,0x88,0x88,0x0,0x0},
{88,7,12,0,0,7,0,0x0,0x0,0x88,0x88,0x50,0x20,0x20,0x50,0x88,0x88,0x0,0x0},
{89,7,12,0,0,7,0,0x0,0x0,0x20,0x20,0x20,0x50,0x50,0x50,0x88,0x88,0x0,0x0},
{90,7,12,0,0,7,0,0x0,0x0,0xf8,0x80,0x40,0x20,0x20,0x10,0x8,0xf8,0x0,0x0},
{91,7,12,0,0,7,0,0x0,0x70,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x70,0x0,0x0},
{92,7,12,0,0,7,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
{93,7,12,0,0,7,0,0x0,0x70,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x70,0x0,0x0},
{94,7,12,0,0,7,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x88,0x50,0x20,0x0,0x0},
{95,7,12,0,0,7,0,0x0,0xfe,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
{96,7,12,0,0,7,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x20,0x40,0x80,0x80,0x0},
{97,7,12,0,0,7,0,0x0,0x0,0x68,0x98,0x98,0x68,0x8,0x70,0x0,0x0,0x0,0x0},
{98,7,12,0,0,7,0,0x0,0x0,0x30,0xc8,0x88,0x88,0xc8,0xb0,0x80,0x80,0x0,0x0},
{99,7,12,0,0,7,0,0x0,0x0,0x70,0x88,0x80,0x80,0x88,0x70,0x0,0x0,0x0,0x0},
{100,7,12,0,0,7,0,0x0,0x0,0x68,0x98,0x88,0x98,0x98,0x68,0x8,0x8,0x0,0x0},
{101,7,12,0,0,7,0,0x0,0x0,0x70,0xc8,0x80,0xf8,0x88,0x70,0x0,0x0,0x0,0x0},
{102,7,12,0,0,7,0,0x0,0x0,0x40,0x40,0x40,0x40,0xf0,0x40,0x48,0x30,0x0,0x0},
{103,7,12,0,0,7,0,0x70,0x8,0x68,0x98,0x88,0x98,0x98,0x60,0x0,0x0,0x0,0x0},
{104,7,12,0,0,7,0,0x0,0x0,0x88,0x88,0x88,0x88,0xd0,0xb0,0x80,0x80,0x0,0x0},
{105,7,12,0,0,7,0,0x0,0x0,0x20,0x20,0x20,0x20,0x20,0x60,0x0,0x20,0x0,0x0},
{106,7,12,0,0,7,0,0x60,0x90,0x90,0x10,0x10,0x10,0x10,0x10,0x0,0x10,0x0,0x0},
{107,7,12,0,0,7,0,0x0,0x0,0x88,0x90,0xa0,0xc0,0xb8,0x80,0x80,0x80,0x0,0x0},
{108,7,12,0,0,7,0,0x0,0x0,0x30,0x20,0x20,0x20,0x20,0x20,0x20,0x60,0x0,0x0},
{109,7,12,0,0,7,0,0x0,0x0,0xa8,0xa8,0xa8,0xa8,0xa8,0xd0,0x0,0x0,0x0,0x0},
{110,7,12,0,0,7,0,0x0,0x0,0x88,0x88,0x88,0x88,0xd0,0xb0,0x0,0x0,0x0,0x0},
{111,7,12,0,0,7,0,0x0,0x0,0x70,0x88,0x88,0x88,0x88,0x70,0x0,0x0,0x0,0x0},
{112,7,12,0,0,7,0,0x80,0x80,0xb0,0xc8,0x88,0xc8,0xa8,0xb0,0x0,0x0,0x0,0x0},
{113,7,12,0,0,7,0,0x8,0x8,0x68,0x98,0x88,0x98,0xa8,0x60,0x0,0x0,0x0,0x0},
{114,7,12,0,0,7,0,0x0,0x0,0x80,0x80,0x80,0xc0,0xa8,0xb0,0x0,0x0,0x0,0x0},
{115,7,12,0,0,7,0,0x0,0x0,0xf0,0x88,0x10,0x60,0x88,0x70,0x0,0x0,0x0,0x0},
{116,7,12,0,0,7,0,0x0,0x0,0x70,0x50,0x40,0x40,0x40,0xf0,0x40,0x0,0x0,0x0},
{117,7,12,0,0,7,0,0x0,0x0,0x68,0x58,0x88,0x88,0x88,0x88,0x0,0x0,0x0,0x0},
{118,7,12,0,0,7,0,0x0,0x0,0x20,0x20,0x50,0x50,0x88,0x88,0x0,0x0,0x0,0x0},
{119,7,12,0,0,7,0,0x0,0x0,0x50,0xa8,0xa8,0x88,0x88,0x88,0x0,0x0,0x0,0x0},
{120,7,12,0,0,7,0,0x0,0x0,0x88,0x48,0x30,0x20,0x50,0x88,0x0,0x0,0x0,0x0},
{121,7,12,0,0,7,0,0x70,0x88,0x28,0x58,0x88,0x88,0x88,0x88,0x0,0x0,0x0,0x0},
{122,7,12,0,0,7,0,0x0,0x0,0xf8,0x40,0x20,0x10,0x18,0xf8,0x0,0x0,0x0,0x0},
{123,7,12,0,0,7,0,0x18,0x20,0x20,0x20,0x20,0xc0,0x20,0x20,0x20,0x18,0x0,0x0},
{124,7,12,0,0,7,0,0x0,0x8,0x8,0x8,0x8,0x8,0x0,0x8,0x8,0x8,0x0,0x0},
{125,7,12,0,0,7,0,0x40,0x20,0x20,0x20,0x20,0x18,0x20,0x20,0x20,0xc0,0x0,0x0},
{126,7,12,0,0,7,0,0x0,0x0,0x0,0x0,0x98,0xb4,0x64,0x0,0x0,0x0,0x0,0x0},
{127,7,12,0,0,7,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
{255,0,0,0,0,0,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
};

GLubyte fwLetters7x14[][21] = {
{32,7,14,0,0,7,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
{33,7,14,0,0,7,0,0x0,0x0,0x0,0x20,0x20,0x0,0x20,0x20,0x20,0x20,0x20,0x0,0x0,0x0},
{34,7,14,0,0,7,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x50,0x50,0x50,0x0,0x0},
{35,7,14,0,0,7,0,0x0,0x0,0x0,0x50,0x50,0xf8,0x50,0x50,0xf8,0x50,0x0,0x0,0x0,0x0},
{36,7,14,0,0,7,0,0x0,0x0,0x20,0x70,0xa8,0x28,0x30,0x60,0xa0,0xa8,0x70,0x20,0x0,0x0},
{37,7,14,0,0,7,0,0x0,0x0,0x0,0x98,0x54,0x54,0x2c,0x20,0x10,0xc8,0xa8,0xa0,0x60,0x0},
{38,7,14,0,0,7,0,0x0,0x0,0x0,0x74,0x88,0x8c,0x90,0x60,0x20,0x50,0x48,0x30,0x0,0x0},
{39,7,14,0,0,7,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x20,0x10,0x10,0x0,0x0},
{40,7,14,0,0,7,0,0x0,0x0,0x0,0x20,0x40,0x40,0x80,0x80,0x80,0x80,0x80,0x40,0x40,0x20},
{41,7,14,0,0,7,0,0x0,0x0,0x0,0x20,0x10,0x10,0x8,0x8,0x8,0x8,0x10,0x10,0x20,0x0},
{42,7,14,0,0,7,0,0x0,0x0,0x0,0x0,0x0,0x20,0xa8,0x70,0x70,0xa8,0x20,0x0,0x0,0x0},
{43,7,14,0,0,7,0,0x0,0x0,0x0,0x0,0x20,0x20,0xf8,0x20,0x20,0x0,0x0,0x0,0x0,0x0},
{44,7,14,0,0,7,0,0x0,0x20,0x10,0x10,0x30,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
{45,7,14,0,0,7,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x78,0x0,0x0,0x0,0x0,0x0,0x0},
{46,7,14,0,0,7,0,0x0,0x0,0x0,0x30,0x30,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
{47,7,14,0,0,7,0,0x0,0x0,0x0,0x80,0x80,0x40,0x40,0x20,0x20,0x10,0x10,0x8,0x0,0x0},
{48,7,14,0,0,7,0,0x0,0x0,0x0,0x70,0x88,0x88,0xc8,0xb8,0x88,0x88,0x70,0x0,0x0,0x0},
{49,7,14,0,0,7,0,0x0,0x0,0x0,0x38,0x10,0x10,0x10,0x10,0x10,0x10,0x30,0x10,0x0,0x0},
{50,7,14,0,0,7,0,0x0,0x0,0x0,0xf8,0x80,0x40,0x20,0x10,0x8,0x88,0x88,0x70,0x0,0x0},
{51,7,14,0,0,7,0,0x0,0x0,0x0,0x70,0x88,0x8,0x8,0x30,0x8,0x88,0x70,0x0,0x0,0x0},
{52,7,14,0,0,7,0,0x0,0x0,0x0,0x10,0x10,0xf8,0x90,0x50,0x30,0x10,0x10,0x0,0x0,0x0},
{53,7,14,0,0,7,0,0x0,0x0,0x0,0x70,0x88,0x8,0x8,0x8,0xf0,0x80,0x80,0xf8,0x0,0x0},
{54,7,14,0,0,7,0,0x0,0x0,0x0,0x70,0x88,0x88,0x88,0xf0,0x80,0x80,0x90,0x70,0x0,0x0},
{55,7,14,0,0,7,0,0x0,0x0,0x0,0x20,0x20,0x20,0x20,0x20,0x10,0x8,0x8,0xf8,0x0,0x0},
{56,7,14,0,0,7,0,0x0,0x0,0x0,0x70,0x88,0x88,0x88,0x88,0x70,0x88,0x88,0x70,0x0,0x0},
{57,7,14,0,0,7,0,0x0,0x0,0x0,0x70,0x8,0x8,0x8,0x78,0x88,0x88,0x88,0x70,0x0,0x0},
{58,7,14,0,0,7,0,0x0,0x0,0x0,0x0,0x20,0x20,0x0,0x20,0x20,0x0,0x0,0x0,0x0,0x0},
{59,7,14,0,0,7,0,0x0,0x0,0x40,0x20,0x10,0x30,0x0,0x10,0x10,0x0,0x0,0x0,0x0,0x0},
{60,7,14,0,0,7,0,0x0,0x0,0x0,0x8,0x10,0x20,0x40,0x80,0x40,0x20,0x10,0x8,0x0,0x0},
{61,7,14,0,0,7,0,0x0,0x0,0x0,0x0,0x0,0x0,0x78,0x0,0x78,0x0,0x0,0x0,0x0,0x0},
{62,7,14,0,0,7,0,0x0,0x0,0x0,0x40,0x20,0x10,0x8,0x4,0x8,0x10,0x20,0x40,0x0,0x0},
{63,7,14,0,0,7,0,0x0,0x0,0x20,0x20,0x0,0x20,0x20,0x10,0x88,0x88,0x70,0x0,0x0,0x0},
{64,7,14,0,0,7,0,0x0,0x0,0x0,0x20,0x50,0x90,0xa8,0xa8,0xb8,0x88,0x48,0x30,0x0,0x0},
{65,7,14,0,0,7,0,0x0,0x0,0x0,0x88,0x88,0xf8,0x88,0x50,0x50,0x20,0x20,0x20,0x0,0x0},
{66,7,14,0,0,7,0,0x0,0x0,0x0,0xf0,0x88,0x88,0x88,0xf0,0x90,0x88,0x88,0xf0,0x0,0x0},
{67,7,14,0,0,7,0,0x0,0x0,0x0,0x70,0x88,0x80,0x80,0x80,0x80,0x88,0x88,0x70,0x0,0x0},
{68,7,14,0,0,7,0,0x0,0x0,0x0,0xf0,0x88,0x88,0x88,0x88,0x88,0x88,0xf0,0x0,0x0,0x0},
{69,7,14,0,0,7,0,0x0,0x0,0x0,0xf8,0x80,0x80,0x80,0xe0,0x80,0x80,0x80,0xf8,0x0,0x0},
{70,7,14,0,0,7,0,0x0,0x0,0x0,0x80,0x80,0x80,0x80,0xe0,0x80,0x80,0x80,0xf8,0x0,0x0},
{71,7,14,0,0,7,0,0x0,0x0,0x0,0x68,0x98,0x88,0x88,0x98,0x80,0x88,0x48,0x70,0x0,0x0},
{72,7,14,0,0,7,0,0x0,0x0,0x0,0x88,0x88,0x88,0x88,0xf8,0x88,0x88,0x88,0x0,0x0,0x0},
{73,7,14,0,0,7,0,0x0,0x0,0x0,0x70,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x70,0x0,0x0},
{74,7,14,0,0,7,0,0x0,0x0,0x0,0x60,0x90,0x10,0x10,0x10,0x10,0x10,0x10,0x30,0x0,0x0},
{75,7,14,0,0,7,0,0x0,0x0,0x0,0x8c,0x90,0xa0,0xc0,0xa0,0x90,0x88,0x88,0x0,0x0,0x0},
{76,7,14,0,0,7,0,0x0,0x0,0x0,0xf8,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x0,0x0,0x0},
{77,7,14,0,0,7,0,0x0,0x0,0x0,0x88,0x88,0x88,0xa8,0xa8,0xd8,0x88,0x88,0x0,0x0,0x0},
{78,7,14,0,0,7,0,0x0,0x0,0x0,0x88,0x98,0xb8,0xa8,0xc8,0xc8,0x88,0x88,0x0,0x0,0x0},
{79,7,14,0,0,7,0,0x0,0x0,0x0,0x70,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x0,0x0,0x0},
{80,7,14,0,0,7,0,0x0,0x0,0x0,0x80,0x80,0x80,0x80,0x80,0xf0,0x88,0x88,0xf0,0x0,0x0},
{81,7,14,0,0,7,0,0x0,0x18,0x20,0x70,0xc8,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x0,0x0},
{82,7,14,0,0,7,0,0x0,0x0,0x0,0x88,0x90,0x90,0xa0,0xf0,0x88,0x88,0xf0,0x0,0x0,0x0},
{83,7,14,0,0,7,0,0x0,0x0,0x0,0x70,0x88,0x8,0x8,0x10,0x60,0x80,0x88,0x70,0x0,0x0},
{84,7,14,0,0,7,0,0x0,0x0,0x0,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0xf8,0x0,0x0,0x0},
{85,7,14,0,0,7,0,0x0,0x0,0x0,0x70,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x0,0x0,0x0},
{86,7,14,0,0,7,0,0x0,0x0,0x0,0x20,0x20,0x20,0x50,0x50,0x88,0x88,0x88,0x0,0x0,0x0},
{87,7,14,0,0,7,0,0x0,0x0,0x0,0x88,0x88,0xd8,0xa8,0xa8,0x88,0x88,0x88,0x0,0x0,0x0},
{88,7,14,0,0,7,0,0x0,0x0,0x0,0x88,0x88,0x50,0x50,0x20,0x50,0x50,0x88,0x88,0x0,0x0},
{89,7,14,0,0,7,0,0x0,0x0,0x0,0x20,0x20,0x20,0x20,0x50,0x50,0x50,0x88,0x88,0x0,0x0},
{90,7,14,0,0,7,0,0x0,0x0,0x0,0xf8,0x80,0x40,0x40,0x20,0x10,0x10,0x8,0xf8,0x0,0x0},
{91,7,14,0,0,7,0,0x0,0x0,0x30,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x30,0x0,0x0},
{92,7,14,0,0,7,0,0x0,0x0,0x0,0x8,0x8,0x10,0x10,0x20,0x20,0x40,0x40,0x80,0x80,0x0},
{93,7,14,0,0,7,0,0x0,0x0,0x30,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x30,0x0,0x0},
{94,7,14,0,0,7,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x88,0x50,0x20,0x0,0x0},
{95,7,14,0,0,7,0,0x0,0xfc,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
{96,7,14,0,0,7,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x10,0x20,0x20,0x0,0x0},
{97,7,14,0,0,7,0,0x0,0x0,0x0,0x68,0x98,0x88,0x78,0x8,0x88,0x70,0x0,0x0,0x0,0x0},
{98,7,14,0,0,7,0,0x0,0x0,0x0,0xb0,0xc8,0x88,0x88,0x88,0xc8,0xb0,0x80,0x80,0x0,0x0},
{99,7,14,0,0,7,0,0x0,0x0,0x0,0x70,0x88,0x80,0x80,0x80,0x88,0x70,0x0,0x0,0x0,0x0},
{100,7,14,0,0,7,0,0x0,0x0,0x0,0x68,0x98,0x88,0x88,0x98,0x68,0x8,0x8,0x8,0x0,0x0},
{101,7,14,0,0,7,0,0x0,0x0,0x0,0x70,0x88,0x80,0xb8,0xc8,0x88,0x70,0x0,0x0,0x0,0x0},
{102,7,14,0,0,7,0,0x0,0x0,0x0,0x20,0x20,0x20,0x20,0x20,0x70,0x20,0x24,0x18,0x0,0x0},
{103,7,14,0,0,7,0,0x78,0x88,0x8,0x68,0x98,0x88,0x88,0x88,0x98,0x68,0x0,0x0,0x0,0x0},
{104,7,14,0,0,7,0,0x0,0x0,0x0,0x88,0x88,0x88,0x88,0x88,0xc8,0xb0,0x80,0x80,0x0,0x0},
{105,7,14,0,0,7,0,0x0,0x0,0x0,0x10,0x10,0x10,0x10,0x10,0x10,0x30,0x0,0x10,0x0,0x0},
{106,7,14,0,0,7,0,0x60,0x90,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x30,0x0,0x10,0x0,0x0},
{107,7,14,0,0,7,0,0x0,0x0,0x0,0x84,0x88,0x90,0xe0,0xa0,0x90,0x90,0x80,0x80,0x0,0x0},
{108,7,14,0,0,7,0,0x0,0x0,0x0,0x30,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x60,0x0,0x0},
{109,7,14,0,0,7,0,0x0,0x0,0x0,0xa8,0xa8,0xa8,0xa8,0xa8,0xe8,0x90,0x0,0x0,0x0,0x0},
{110,7,14,0,0,7,0,0x0,0x0,0x0,0x88,0x88,0x88,0x88,0x88,0xc8,0xb0,0x0,0x0,0x0,0x0},
{111,7,14,0,0,7,0,0x0,0x0,0x0,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x0,0x0,0x0,0x0},
{112,7,14,0,0,7,0,0x80,0x80,0x80,0xb0,0xc8,0xc8,0x88,0x88,0xc8,0xb0,0x0,0x0,0x0,0x0},
{113,7,14,0,0,7,0,0xc,0x8,0x8,0x68,0x98,0x88,0x88,0x88,0x78,0x0,0x0,0x0,0x0,0x0},
{114,7,14,0,0,7,0,0x0,0x0,0x0,0x80,0x80,0x80,0x80,0x80,0xc8,0xb0,0x0,0x0,0x0,0x0},
{115,7,14,0,0,7,0,0x0,0x0,0x0,0x70,0x88,0x8,0x70,0x80,0x80,0x78,0x0,0x0,0x0,0x0},
{116,7,14,0,0,7,0,0x0,0x0,0x0,0x10,0x28,0x20,0x20,0x20,0x70,0x20,0x20,0x0,0x0,0x0},
{117,7,14,0,0,7,0,0x0,0x0,0x0,0x6c,0x98,0x88,0x88,0x88,0x88,0x0,0x0,0x0,0x0,0x0},
{118,7,14,0,0,7,0,0x0,0x0,0x0,0x20,0x20,0x50,0x50,0x50,0x88,0x88,0x0,0x0,0x0,0x0},
{119,7,14,0,0,7,0,0x0,0x0,0x0,0x50,0x50,0xa8,0xa8,0xa8,0x88,0x88,0x0,0x0,0x0,0x0},
{120,7,14,0,0,7,0,0x0,0x0,0x0,0x88,0x88,0x50,0x20,0x50,0x88,0x88,0x0,0x0,0x0,0x0},
{121,7,14,0,0,7,0,0x30,0xc8,0x8,0x68,0x98,0x88,0x88,0x88,0x88,0x88,0x0,0x0,0x0,0x0},
{122,7,14,0,0,7,0,0x0,0x0,0x0,0xf8,0x80,0x40,0x20,0x10,0x8,0xf8,0x0,0x0,0x0,0x0},
{123,7,14,0,0,7,0,0x0,0x0,0x10,0x20,0x20,0x20,0x20,0xc0,0x20,0x20,0x20,0x20,0x10,0x0},
{124,7,14,0,0,7,0,0x0,0x0,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x0,0x0},
{125,7,14,0,0,7,0,0x0,0x0,0x80,0x40,0x40,0x40,0x40,0x20,0x40,0x40,0x40,0x40,0x80,0x0},
{126,7,14,0,0,7,0,0x0,0x0,0x0,0x0,0x0,0x0,0x10,0xa8,0xa8,0x40,0x0,0x0,0x0,0x0},
{255,0,0,0,0,0,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
};

GLubyte fwLetters8x15[][22] = {
{32,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
{33,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x20,0x20,0x0,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x0},
{35,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x24,0x24,0x24,0xfe,0x24,0x24,0x24,0xfe,0x24,0x0,0x0},
{36,8,15,0,0,8,0,0x0,0x0,0x0,0x10,0x38,0x54,0x94,0x14,0x18,0x10,0x70,0x90,0x94,0x78,0x10},
{37,8,15,0,0,8,0,0x0,0x0,0x0,0x80,0x44,0x4a,0x2a,0x34,0x10,0x10,0x48,0xa8,0xa4,0x44,0x0},
{38,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x74,0x88,0x94,0xa0,0x40,0x40,0xa0,0x90,0x50,0x20,0x0},
{39,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x40,0x20,0x20,0x30,0x30,0x0,0x0},
{40,8,15,0,0,8,0,0x0,0x0,0x0,0x8,0x10,0x20,0x20,0x40,0x40,0x40,0x40,0x20,0x20,0x10,0x8},
{41,8,15,0,0,8,0,0x0,0x0,0x40,0x20,0x10,0x10,0x8,0x8,0x8,0x8,0x8,0x10,0x10,0x20,0x40},
{42,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x0,0x0,0x10,0x54,0x38,0x38,0x54,0x10,0x0,0x0,0x0},
{43,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x10,0x10,0x10,0xfe,0x10,0x10,0x10,0x10,0x0,0x0,0x0},
{44,8,15,0,0,8,0,0x0,0x0,0x20,0x10,0x18,0x18,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
{45,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xf8,0x0,0x0,0x0,0x0,0x0,0x0},
{46,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x30,0x30,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
{47,8,15,0,0,8,0,0x0,0x0,0x40,0x40,0x20,0x20,0x10,0x10,0x8,0x8,0x4,0x4,0x2,0x2,0x0},
{48,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x78,0x84,0x84,0xc4,0xa4,0x9c,0x84,0x84,0x84,0x78,0x0},
{49,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x38,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x30,0x10,0x0},
{50,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0xfe,0x80,0x40,0x20,0x10,0x8,0x6,0x82,0x82,0x7c,0x0},
{51,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x78,0x84,0x4,0x4,0x4,0x18,0x4,0x4,0x84,0x78,0x0},
{52,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x8,0x8,0x8,0x8,0xfc,0x88,0x48,0x28,0x18,0x8,0x0},
{53,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x78,0x84,0x4,0x4,0x84,0xf8,0x80,0x80,0x80,0xfc,0x0},
{54,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x7c,0x84,0x82,0xc2,0xa4,0x98,0x80,0x84,0x44,0x38,0x0},
{55,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x20,0x20,0x10,0x10,0x10,0x10,0x8,0x4,0x4,0xfc,0x0},
{56,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x78,0x84,0x84,0x84,0x84,0x84,0x78,0x84,0x84,0x78,0x0},
{57,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x78,0x84,0x4,0x34,0x4c,0x84,0x84,0x84,0x44,0x38,0x0},
{58,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x30,0x30,0x0,0x0,0x0,0x30,0x30,0x0,0x0,0x0,0x0},
{59,8,15,0,0,8,0,0x0,0x40,0x20,0x10,0x30,0x30,0x0,0x0,0x30,0x30,0x0,0x0,0x0,0x0,0x0},
{60,8,15,0,0,8,0,0x0,0x0,0x0,0x4,0x8,0x10,0x20,0x40,0x80,0x40,0x20,0x10,0x8,0x4,0x0},
{61,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xf8,0x0,0x0,0xf8,0x0,0x0,0x0,0x0},
{62,8,15,0,0,8,0,0x0,0x0,0x80,0x40,0x20,0x10,0x8,0x4,0x4,0x8,0x10,0x20,0x40,0x80,0x0},
{63,8,15,0,0,8,0,0x0,0x0,0x0,0x10,0x10,0x0,0x0,0x10,0x18,0x4,0x2,0x82,0x44,0x38,0x0},
{64,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x38,0x44,0x80,0x98,0xa4,0xa4,0x9c,0x84,0x48,0x30,0x0},
{65,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x84,0x84,0xfc,0x84,0x48,0x48,0x48,0x30,0x30,0x0,0x0},
{66,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0xf8,0x84,0x84,0x84,0x84,0xf8,0x84,0x84,0x84,0xf8,0x0},
{67,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x78,0x84,0x80,0x80,0x80,0x80,0x80,0x80,0x84,0x78,0x0},
{68,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0xf0,0x88,0x84,0x84,0x84,0x84,0x84,0x88,0xf0,0x0,0x0},
{69,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0xfc,0x80,0x80,0x80,0x80,0xf0,0x80,0x80,0xfc,0x0,0x0},
{70,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x80,0x80,0x80,0x80,0x80,0xf0,0x80,0x80,0x80,0xfe,0x0},
{71,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x7a,0x86,0x82,0x82,0x82,0x8c,0x80,0x80,0x44,0x38,0x0},
{72,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x84,0x84,0x84,0x84,0xfc,0x84,0x84,0x84,0x84,0x84,0x0},
{73,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x38,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x38,0x0},
{74,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x70,0x88,0x88,0x8,0x8,0x8,0x8,0x8,0x8,0x18,0x0},
{75,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x86,0x88,0x90,0xa0,0xc0,0xa0,0x90,0x88,0x84,0x80,0x0},
{76,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0xfc,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x0},
{77,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x82,0x82,0x92,0x92,0xaa,0xaa,0xc6,0xc6,0x82,0x82,0x0},
{78,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x84,0x8c,0x8c,0x94,0x94,0xa4,0xa4,0xc4,0xc4,0x84,0x0},
{79,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x78,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x78,0x0},
{80,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x80,0x80,0x80,0x80,0xb8,0xc4,0x84,0x84,0x84,0xf8,0x0},
{81,8,15,0,0,8,0,0x0,0x4,0x18,0x20,0x7c,0xa2,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x7c,0x0},
{82,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x82,0x84,0x8c,0x88,0xfc,0x82,0x82,0x82,0x82,0xfc,0x0},
{83,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x78,0x84,0x4,0x4,0x18,0x60,0x80,0x80,0x84,0x7c,0x0},
{84,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0xfe,0x0},
{85,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x7c,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x0},
{86,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x10,0x10,0x28,0x44,0x44,0x44,0x44,0x82,0x82,0x82,0x0},
{87,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x44,0x44,0xaa,0xaa,0x92,0x92,0x92,0x82,0x82,0x0,0x0},
{88,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x84,0x84,0x48,0x48,0x30,0x30,0x4c,0x44,0x84,0x84,0x0},
{89,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x10,0x10,0x10,0x10,0x10,0x28,0x28,0x44,0x82,0x82,0x0},
{90,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0xfe,0x80,0x40,0x40,0x20,0x10,0x8,0x4,0x4,0xfe,0x0},
{91,8,15,0,0,8,0,0x0,0x0,0x0,0xe0,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0xe0,0x0},
{92,8,15,0,0,8,0,0x0,0x0,0x4,0x4,0x8,0x8,0x10,0x10,0x20,0x20,0x40,0x40,0x80,0x80,0x0},
{93,8,15,0,0,8,0,0x0,0x0,0x0,0x38,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x38,0x0},
{94,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x44,0x28,0x10,0x0},
{95,8,15,0,0,8,0,0x0,0xfe,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
{96,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x20,0x40,0xc0,0xc0,0x0},
{97,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x74,0x88,0x98,0x68,0x8,0x88,0x70,0x0,0x0,0x0,0x0},
{98,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0xb8,0xc4,0x84,0xc4,0xc4,0xb8,0x80,0x80,0x80,0x0,0x0},
{99,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x78,0x84,0x80,0x80,0x80,0x84,0x78,0x0,0x0,0x0,0x0},
{100,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x74,0x8c,0x8c,0x84,0x8c,0x74,0x4,0x4,0x4,0x0,0x0},
{101,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x78,0x84,0x80,0xbc,0xc4,0x84,0x78,0x0,0x0,0x0,0x0},
{102,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x20,0x20,0x20,0x20,0x78,0x20,0x20,0x24,0x3c,0x0,0x0},
{103,8,15,0,0,8,0,0x18,0x64,0x4,0x4,0x34,0x4c,0x84,0x84,0x84,0x8c,0x74,0x0,0x0,0x0,0x0},
{104,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x84,0x84,0x84,0x84,0x84,0xc4,0xb8,0x80,0x80,0x80,0x0},
{105,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x10,0x10,0x10,0x10,0x10,0x10,0x30,0x0,0x10,0x0,0x0},
{106,8,15,0,0,8,0,0x40,0xa0,0x90,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x30,0x0,0x10,0x0},
{107,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x84,0x98,0xb0,0xc0,0xa0,0x90,0x88,0x80,0x80,0x0,0x0},
{108,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x18,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x30,0x0,0x0},
{109,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x54,0x54,0x54,0x54,0x54,0x54,0xa8,0x0,0x0,0x0,0x0},
{110,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x84,0x84,0x84,0x84,0x84,0xc8,0xb8,0x0,0x0,0x0,0x0},
{111,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x78,0x84,0x84,0x84,0x84,0x84,0x78,0x0,0x0,0x0,0x0},
{112,8,15,0,0,8,0,0x80,0x80,0x80,0x80,0xb8,0xa4,0xc4,0x84,0x84,0xc4,0xa4,0x18,0x0,0x0,0x0},
{113,8,15,0,0,8,0,0x2,0x4,0x4,0x4,0x74,0x8c,0x8c,0x84,0x84,0x8c,0x74,0x0,0x0,0x0,0x0},
{114,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x80,0x80,0x80,0x80,0xc0,0xa4,0xb8,0x0,0x0,0x0,0x0},
{115,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0xf8,0x84,0x4,0x38,0x40,0x84,0x78,0x0,0x0,0x0,0x0},
{116,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x30,0x28,0x20,0x20,0x20,0x20,0x78,0x20,0x20,0x0,0x0},
{117,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x74,0x4c,0x84,0x84,0x84,0x84,0x84,0x0,0x0,0x0,0x0},
{118,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x30,0x30,0x48,0x48,0x84,0x84,0x84,0x0,0x0,0x0,0x0},
{119,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x24,0x5a,0x92,0x92,0x82,0x82,0x82,0x0,0x0,0x0,0x0},
{120,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x84,0x84,0x48,0x30,0x48,0x84,0x84,0x0,0x0,0x0,0x0},
{121,8,15,0,0,8,0,0x38,0x44,0x84,0x4,0x74,0x8c,0x84,0x84,0x84,0x84,0x0,0x0,0x0,0x0,0x0},
{122,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0xfc,0x80,0x40,0x20,0x10,0x8,0xfc,0x0,0x0,0x0,0x0},
{123,8,15,0,0,8,0,0x0,0x0,0x30,0x40,0x40,0x40,0x40,0x40,0xc0,0x40,0x40,0x40,0x40,0x30,0x0},
{124,8,15,0,0,8,0,0x0,0x0,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x0},
{125,8,15,0,0,8,0,0x0,0x0,0x0,0x60,0x10,0x10,0x10,0x10,0x18,0x10,0x10,0x10,0x10,0x60,0x0},
{126,8,15,0,0,8,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x98,0xb4,0x64,0x0,0x0,0x0,0x0,0x0},
{255,0,0,0,0,0,0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
};
/* <<< bitmap fonts */

typedef struct {int x; int y;} XY;
GLuint fwFontOffset[3];
XY fwFontSize[3];

void fwMakeRasterFonts()
{
	int ichar;
	GLuint i, j;
	GLuint fwFontOffset7x12;
	GLuint fwFontOffset7x14;
	GLuint fwFontOffset8x15;

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   fwFontOffset7x12 = glGenLists (128);
   for(i=0;i<128;i++)
   {
		ichar = fwLetters7x12[i][0];
		if( ichar == 255 )break;
	   glNewList(fwFontOffset7x12 + fwLetters7x12[i][0],GL_COMPILE);
	   glBitmap((GLsizei)fwLetters7x12[i][1],(GLsizei)fwLetters7x12[i][2],
		   (GLfloat)fwLetters7x12[i][3],(GLfloat)fwLetters7x12[i][4],
		   (GLfloat)fwLetters7x12[i][5], (GLfloat)fwLetters7x12[i][6],
		   &fwLetters7x12[i][7]);
	   glEndList();
   }
   fwFontOffset7x14 = glGenLists (128);
   for(i=0;i<128;i++)
   {
		ichar = fwLetters7x14[i][0];
		if( ichar == 255 )break;
	   glNewList(fwFontOffset7x14 + fwLetters7x14[i][0],GL_COMPILE);
	   glBitmap((GLsizei)fwLetters7x14[i][1],(GLsizei)fwLetters7x14[i][2],
		   (GLfloat)fwLetters7x14[i][3],(GLfloat)fwLetters7x14[i][4],
		   (GLfloat)fwLetters7x14[i][5], (GLfloat)fwLetters7x14[i][6],
		   &fwLetters7x14[i][7]);
	   glEndList();
   }
   fwFontOffset8x15 = glGenLists (128);
   for(i=0;i<128;i++)
   {
		ichar = fwLetters8x15[i][0];
		if( ichar == 255 )break;
	   glNewList(fwFontOffset8x15 + fwLetters8x15[i][0],GL_COMPILE);
	   glBitmap((GLsizei)fwLetters8x15[i][1],(GLsizei)fwLetters8x15[i][2],
		   (GLfloat)fwLetters8x15[i][3],(GLfloat)fwLetters8x15[i][4],
		   (GLfloat)fwLetters8x15[i][5], (GLfloat)fwLetters8x15[i][6],
		   &fwLetters8x15[i][7]);
	   glEndList();
   }
   fwFontOffset[0] = fwFontOffset7x12;
   fwFontOffset[1] = fwFontOffset7x14;
   fwFontOffset[2] = fwFontOffset8x15;
   fwFontSize[0].x = 7;
   fwFontSize[0].y = 12;
   fwFontSize[1].x = 7;
   fwFontSize[1].y = 14;
   fwFontSize[2].x = 8;
   fwFontSize[2].y = 15;
}

static int fontInitialized = 0;
XY bmWH = {10,15}; /* simple bitmap font from redbook above, width and height in pixels */
XY mouse2screen(int x, int y)
{
	XY xy;
	xy.x = x;
	xy.y = screenHeight -y;
	return xy;
}
XY screen2text(int x, int y)
{
	XY rc;
	rc.x = x/bmWH.x; //10; 
	rc.y = (int)((screenHeight -y)/bmWH.y); //15.0 ); 
	return rc;
}
XY text2screen( int col, int row)
{
	XY xy;
	xy.x = col*bmWH.x; //10; 
	xy.y = screenHeight - (row+1)*bmWH.y; //15;
	return xy;
}

void initFont(void)
{
	/*initialize raster bitmap font above */
   glShadeModel (GL_FLAT);
   fwMakeRasterFonts();
   fontInitialized = 1;
}
int bmfontsize = 2; /* 0,1 or 2 */
void printString(char *s)
{
   glPushAttrib (GL_LIST_BIT);
   glListBase(fwFontOffset[bmfontsize]);
   bmWH.x = fwFontSize[bmfontsize].x;
   bmWH.y = fwFontSize[bmfontsize].y;
   glCallLists(strlen(s), GL_UNSIGNED_BYTE, (GLubyte *) s);
   glPopAttrib ();
}

static int sb_hasString = FALSE;
static struct Uni_String *myline;
extern int clipPlane;
static int loopcount = 0;
static int hadString = 0;

#define MAX_BUFFER_SIZE 4096
static char buffer[MAX_BUFFER_SIZE] = "\0";
void render_init(void);

#define STATUS_LEN 2000

/* make sure that on a re-load that we re-init */
void kill_status (void) {
	/* hopefully, by this time, rendering has been stopped */
}
void handleButtonOver(){}
void handleOptionPress(){}
void handleButtonPress(){}

void setMenuButton_collision(int val){}
void setMenuButton_texSize(int size){}
void setMenuButton_headlight(int val){}
void setMenuButton_navModes(int type){}
char messagebar[200];
void setMessageBar()
{
	sprintf(&messagebar[0],"%10f",myFps);
	sprintf(&messagebar[15],"%s",myMenuStatus);
}
void setMenuStatus(char *stat) {
    strncpy (myMenuStatus, stat, MAXSTAT);
    setMessageBar();
}

void setMenuFps (float fps) {
    myFps = fps;
    setMessageBar();
}

int handleStatusbarHud(int mev, int* clipplane)
{ return 0; }



/* trigger a update */
void update_status(char* msg)
{
	if (!msg) {
		buffer[0] = '\0';
	} else {
		strncpy(buffer, msg, MAX_BUFFER_SIZE);
	}
}


/**
 *   drawStatusBar: update the status text on top of the 3D view
 *                  using a 2D projection and raster characters.
 */
#include <list.h>
static int showConText = 0;
s_list_t *conlist;
int concount;
void hudSetConsoleMessage(char *buffer)
{
	s_list_t* item;
	/*calling program keeps ownership of buffer and deletes or recycles buffer*/
	char *buffer2;
	int len = strlen(buffer)+1;
	buffer2 = malloc(len);
	strncpy(buffer2,buffer,len);
	item = ml_new(buffer2);
	if(!conlist)
		conlist = item;
	else
		ml_append(conlist,item); /*append to bottom*/
	concount++;

	if( concount > 50 ) // > MAXMESSAGES number of scrolling lines
	{
		s_list_t* temp;
		free((char*)conlist->elem);
		conlist = ml_delete_self(conlist, conlist); /*delete from top*/
		concount--;
	}
}

void printConsoleText()
{
	/* ConsoleMessage() comes out as a multi-line history rendered over the scene */
	int jstart;
	int j = 0;
	XY xybottom;
	jstart = j;
	{
		s_list_t *__l;
		s_list_t *next;
		s_list_t *_list = conlist;
		/* lets keep the scrolling text from touching the bottom of the screen */
		xybottom = screen2text(0,0); 
		jstart = max(0,concount-(xybottom.y - 3)); /* keep it 3 lines off the bottom */
		for(__l=_list;__l!=NULL;) 
		{
			next = ml_next(__l); /* we need to get next from __l before action deletes element */ 
			if(j >= jstart) /* no need to print off-screen text */
			{
				XY xy = text2screen(0,j-jstart);
				FW_GL_WINDOWPOS2I(xy.x,xy.y); 
				printString(__l->elem); 
			}
			j++;
			__l = next; 
		}
	}
}
void drawStatusBar() 
{
	char *p; 
	float c[4];
	int ic[4];
	Console_writeToHud = 1;
	//Console_writeToCRT = 1;
	//Console_writeToFile = 0;
	//if(showButtons)
	//{
	//	renderButtons();
	//	return;
	//}
	if (0){ //!sb_hasString ){ //&& !showConText &&!butStatus[8] &&!butStatus[9] && !butStatus[10]) {
		if(hadString)
		{
			/* clear the status bar because there's nothing to show */
			FW_GL_SCISSOR(0,0,screenWidth,clipPlane);
			FW_GL_ENABLE(GL_SCISSOR_TEST);
			FW_GL_CLEAR(GL_COLOR_BUFFER_BIT);
			FW_GL_DISABLE(GL_SCISSOR_TEST);
			hadString = 0;
		}
		return;
	}
	/* to improve frame rates we don't need to update the status bar every loop,
	because the mainloop scene rendering should be using a scissor test to avoid FW_GL_CLEAR()ing 
	the statusbar area. 
	*/
	hadString = 1;
	sb_hasString = 1;
	clipPlane = 16;
	loopcount++;
	if(loopcount < 15 && !hadString) return;
	loopcount = 0;

	/* OK time to update the status bar */
	if(!fontInitialized) initFont();
	/* unconditionally clear the statusbar area */
	FW_GL_SCISSOR(0,0,screenWidth,clipPlane);
	FW_GL_ENABLE(GL_SCISSOR_TEST);
	FW_GL_CLEAR(GL_COLOR_BUFFER_BIT);
	FW_GL_DISABLE(GL_SCISSOR_TEST);

	// you must call drawStatusBar() from render() just before swapbuffers 
	FW_GL_DEPTHMASK(FALSE);
	FW_GL_DISABLE(GL_DEPTH_TEST);
	glColor3f(1.0f,1.0f,1.0f);
	//glWindowPos seems to set the bitmap color correctly in windows
	FW_GL_WINDOWPOS2I(5,0); 
	if(sb_hasString)
	{
		p = buffer;
		/* print status bar text - things like PLANESENSOR */
		printString(p); 
		hadString = 1;
	}
	FW_GL_WINDOWPOS2I(300,0);
	printString(messagebar);

	printConsoleText();

	FW_GL_DEPTHMASK(TRUE);
	FW_GL_ENABLE(GL_DEPTH_TEST);
	glFlush();

}
