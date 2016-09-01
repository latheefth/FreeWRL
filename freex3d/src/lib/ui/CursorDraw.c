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
#include <display.h>

#include "../vrml_parser/Structs.h"
#include "main/headers.h"
#include "vrml_parser/Structs.h"
#include "scenegraph/Viewer.h"
#include "scenegraph/Component_Shape.h"
#include "opengl/OpenGL_Utils.h"
#include "opengl/Textures.h"
#include "opengl/LoadTextures.h"
#include "main/MainLoop.h"
#include "scenegraph/RenderFuncs.h"
#include "statusbar.h"


/* I made a 32x32 image in Gimp, and exported to C Struct format */
static const struct {
  int  	 width;
  int  	 height;
  int  	 bytes_per_pixel; /* 3:RGB, 4:RGBA */ 
  GLubyte 	 pixel_data[32 * 32 * 4 + 1];
} circleCursor = {
  32, 32, 4,
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\20\0\0\0""0\0\0\0P\0\0\0o"
  "\0\0\0\177\0\0\0\177\0\0\0o\0\0\0P\0\0\0""0\0\0\0\20\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\40\0\0\0p***\302WWW\350qqq\372\177\177\177\377\177\177\177\377"
  "qqq\372WWW\350***\302\0\0\0p\0\0\0\40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0""0000\251ddd\363\213\213"
  "\213\377\216\216\216\377\206\206\206\377\200\200\200\377\200\200\200\377"
  "\206\206\206\377\216\216\216\377\213\213\213\377ddd\363000\251\0\0\0""0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0""0,"
  ",,\266yyy\373\247\247\247\377ttt\366<<<\313\33\33\33\227\0\0\0\200\0\0\0"
  "\200\33\33\33\227<<<\313ttt\366\247\247\247\377yyy\373,,,\266\0\0\0""0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\40""000\251yyy\373\213"
  "\213\213\377GGG\347\33\33\33\226\0\0\0@\0\0\0\20\0\0\0\0\0\0\0\0\0\0\0\20"
  "\0\0\0@\33\33\33\226GGG\347\213\213\213\377yyy\373000\251\0\0\0\40\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\20\0\0\0pddd\363\247\247\247\377GGG\347"
  "\0\0\0\177\0\0\0\40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\40\0\0\0\177GGG\347\247\247\247\377ddd\363\0\0\0p\0\0\0\20\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0""0***\302\213\213\213\377ttt\366\33\33\33\226\0\0\0\40\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\40\33"
  "\33\33\226ttt\366\213\213\213\377***\302\0\0\0""0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0PWWW\350\216\216\216\377<<<\313\0\0\0@\0\0\0\0\0\0\0\0\0\0\0\20\0\0"
  "\0@\0\0\0o\0\0\0o\0\0\0@\0\0\0\20\0\0\0\0\0\0\0\0\0\0\0@<<<\313\216\216\216"
  "\377WWW\350\0\0\0P\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0oqqq\372\206\206\206\377"
  "\33\33\33\227\0\0\0\20\0\0\0\0\0\0\0\0\0\0\0@,,,\271ccc\365ccc\365,,,\271"
  "\0\0\0@\0\0\0\0\0\0\0\0\0\0\0\20\33\33\33\227\206\206\206\377qqq\372\0\0"
  "\0o\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\177\177\177\177\377\200\200\200\377\0\0"
  "\0\200\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0occc\365\337\337\337\377\337\337\337"
  "\377ccc\365\0\0\0o\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\200\200\200\200\377\177"
  "\177\177\377\0\0\0\177\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\177\177\177\177\377"
  "\200\200\200\377\0\0\0\200\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0occc\365\337\337"
  "\337\377\337\337\337\377ccc\365\0\0\0o\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\200"
  "\200\200\200\377\177\177\177\377\0\0\0\177\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "oqqq\372\206\206\206\377\33\33\33\227\0\0\0\20\0\0\0\0\0\0\0\0\0\0\0@,,,"
  "\271ccc\365ccc\365,,,\271\0\0\0@\0\0\0\0\0\0\0\0\0\0\0\20\33\33\33\227\206"
  "\206\206\377qqq\372\0\0\0o\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0PWWW\350\216\216"
  "\216\377<<<\313\0\0\0@\0\0\0\0\0\0\0\0\0\0\0\20\0\0\0@\0\0\0o\0\0\0o\0\0"
  "\0@\0\0\0\20\0\0\0\0\0\0\0\0\0\0\0@<<<\313\216\216\216\377WWW\350\0\0\0P"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0""0***\302\213\213\213\377ttt\366\33\33\33"
  "\226\0\0\0\40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\40\33\33\33\226ttt\366\213\213\213\377***\302\0\0\0""0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\20\0\0\0pddd\363\247\247\247\377GGG\347\0\0\0\177"
  "\0\0\0\40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\40\0\0\0"
  "\177GGG\347\247\247\247\377ddd\363\0\0\0p\0\0\0\20\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\40""000\251yyy\373\213\213\213\377GGG\347\33\33\33\226\0"
  "\0\0@\0\0\0\20\0\0\0\0\0\0\0\0\0\0\0\20\0\0\0@\33\33\33\226GGG\347\213\213"
  "\213\377yyy\373000\251\0\0\0\40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0""0,,,\266yyy\373\247\247\247\377ttt\366<<<\313\33\33\33\227"
  "\0\0\0\200\0\0\0\200\33\33\33\227<<<\313ttt\366\247\247\247\377yyy\373,,"
  ",\266\0\0\0""0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0""0000\251ddd\363\213\213\213\377\216\216\216\377\206\206\206"
  "\377\200\200\200\377\200\200\200\377\206\206\206\377\216\216\216\377\213"
  "\213\213\377ddd\363000\251\0\0\0""0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\40\0\0\0p***\302WWW\350"
  "qqq\372\177\177\177\377\177\177\177\377qqq\372WWW\350***\302\0\0\0p\0\0\0"
  "\40\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\20\0\0\0""0\0\0\0P\0\0\0o\0\0\0\177\0\0"
  "\0\177\0\0\0o\0\0\0P\0\0\0""0\0\0\0\20\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0",
};
//
//1-2 4
//|  /|
//0 3 5
GLfloat cursorVert[] = {
	-.05f, -.05f, 0.0f,
	-.05f,  .05f, 0.0f,
	 .05f,  .05f, 0.0f,
	-.05f, -.05f, 0.0f,
	 .05f,  .05f, 0.0f,
	 .05f, -.05f, 0.0f};
GLfloat cursorTex[] = {
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 1.0f,
	1.0f, 0.0f};

typedef struct pCursorDraw{
	GLuint textureID;
	int done;
}* ppCursorDraw;
void *CursorDraw_constructor(){
	void *v = MALLOCV(sizeof(struct pCursorDraw));
	memset(v,0,sizeof(struct pCursorDraw));
	return v;
}
void CursorDraw_init(struct tCursorDraw *t){
	//public
	//private
	t->prv = CursorDraw_constructor();
	{
		ppCursorDraw p = (ppCursorDraw)t->prv;
		p->done = 0;
		p->textureID = 0;
	}
}

typedef struct {int x; int y;} XY;
XY mouse2screen2(int x, int y)
{
	XY xy;
	xy.x = x;
	//xy.y = gglobal()->display.screenHeight -y;
	xy.y = y;
	return xy;
}
typedef struct {GLfloat x; GLfloat y;} FXY;
FXY screen2normalized( GLfloat x, GLfloat y )
{
	FXY xy;
	xy.x = (x / gglobal()->display.screenWidth)*2.0f -1.0f;
	xy.y = (y / gglobal()->display.screenHeight)*2.0f -1.0f;
	return xy;
}
static GLfloat cursIdentity[] = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};
struct cline {
	int n;  //0 means no more lines
	GLfloat p[6]; //max 3 xy points, fill unused with 0f
};
static struct cline cur_fiducials [] = {
	{3,{-.02f,.0f, 0.0f,-.02f, .02f,.0f}}, // v offset downward a bit to get on the screen at the top
	{0,{0.0f,0.0f,0.0f,0.0f,0.0f,0.0f}},
};
static struct cline cur_down [] = {
	{3,{-.02f,.02f, .0f,.0f, .02f,.02f}}, // v
	{0,{.0f,.0f,.0f,.0f,.0f,.0f}},
};
static struct cline cur_up [] = {
	{3,{-.02f,-.02f, .0f,.0f, .02f,-.02f}}, // ^
	{0,{.0f,.0f,.0f,.0f,.0f,.0f}},
};
static struct cline cur_hover [] = {
	{2,{-.02f,.0f, .02f,.0f, .0f,.0f}}, // +
	{2,{.0f,-.02f, .0f,.02f, .0f,.0f}},
	{0,{.0f,.0f,.0f,.0f,.0f,.0f}},
};
static struct cline cur_over [] = {
	{2,{.0f,.0f, .0f,.005f, .0f,.0f}}, // !
	{2,{.0f,.008f, .0f,.02f, .0f,.0f}},
	{0,{.0f,.0f,.0f,.0f,.0f,.0f}},
};
/* - in CursorDraw.h
enum cursor_type {
	CURSOR_UP = 0,
	CURSOR_DOWN,
	CURSOR_HOVER,
	CURSOR_OVER,
	CURSOR_FIDUCIALS
};
*/
static struct cline *cursor_array [] = {
	cur_up,
	cur_down,
	cur_hover,
	cur_over,
	cur_fiducials,
	NULL,
};
/* attempt to draw fiducials with lines - draws wrong place */
s_shader_capabilities_t *getMyShader(unsigned int rq_cap0);
void fiducialDrawB(int cursortype, int x, int y)
{
	XY xy;
	FXY fxy;
	int i,k;
	GLfloat p[3][2];
	float aspect;
	GLint  positionLoc;
	struct cline *cur, *line;
	s_shader_capabilities_t *scap;
	ttglobal tg = gglobal();

	//as of May 2016 the mouse/touch events come in the pick() stack relative to the whole window
	// -not shifted relative to the current vport in the vport stack.
	// if that changes, then the following few lines would also need to change
	xy = mouse2screen2(x,y);
	FW_GL_VIEWPORT(0, 0, tg->display.screenWidth, tg->display.screenHeight);
	fxy = screen2normalized((GLfloat)xy.x,(GLfloat)xy.y);
	aspect = (float)tg->display.screenHeight/(float)tg->display.screenWidth;


	FW_GL_DEPTHMASK(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	scap = getMyShader(NO_APPEARANCE_SHADER);
	enableGlobalShader(scap);
	glUniformMatrix4fv(scap->ModelViewMatrix, 1, GL_FALSE, cursIdentity); 
	glUniformMatrix4fv(scap->ProjectionMatrix, 1, GL_FALSE, cursIdentity);


	//FW_GL_VERTEX_POINTER(2, GL_FLOAT, 0, (GLfloat *)p);
	//sendArraysToGPU(GL_LINE_STRIP, 0, 3);
	positionLoc =  scap->Vertices; //glGetAttribLocation ( shader, "fw_Vertex" );

	cur = cursor_array[cursortype];
	k = 0;
	line = &cur[k];
	while(line->n){
		for(i=0;i<line->n;i++){
			p[i][0] = line->p[i*2]*aspect + fxy.x;
			p[i][1] = line->p[i*2 + 1] + fxy.y;
		}
		glVertexAttribPointer (positionLoc, 2, GL_FLOAT, 
							   GL_FALSE, 0, p );
		glDrawArrays(GL_LINE_STRIP,0,line->n);
		k++;
		line = &cur[k];
	}
	
	FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, 0);
	FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, 0);


	glEnable(GL_DEPTH_TEST);
	FW_GL_DEPTHMASK(GL_TRUE);
	restoreGlobalShader();
}
void fiducialDraw(int ID, int x, int y, float angleDeg)
{
	//pre- may 8, 2016
	XY xy;
	FXY fxy;
	int i;
	GLfloat p[3][2];
	GLint  positionLoc;
	s_shader_capabilities_t *scap;
	ttglobal tg = gglobal();

	xy = mouse2screen2(x,y);
	FW_GL_VIEWPORT(0, 0, tg->display.screenWidth, tg->display.screenHeight);
	fxy = screen2normalized((GLfloat)xy.x,(GLfloat)xy.y);
	//I was hoping for a little v at the top

	p[0][0] = -.01f;
	p[0][1] =  .01f;
	p[1][0] =  .00f;
	p[1][1] =  .00f;
	p[2][0] =  .01f;
	p[2][1] =  .01f;
	if(angleDeg != 0.0f){
		GLfloat cosine, sine, angleRad, xx,yy;
		angleRad = angleDeg * (float)PI / 180.0f;
		cosine = cosf(angleRad);
		sine = sinf(angleRad);
		for(i=0;i<3;i++){
			xx = cosine*p[i][0] + sine*p[i][1];
			yy = -sine*p[i][0] + cosine*p[i][1];
			p[i][0]=xx;
			p[i][1]=yy;
		}
	}
	if(ID == 1){
		for(i=0;i<3;i++)
			p[i][1] -= .01f;
	}
	for(i=0;i<3;i++){
		p[i][0] += fxy.x;
		p[i][1] += fxy.y;
	}

	FW_GL_DEPTHMASK(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	scap = getMyShader(NO_APPEARANCE_SHADER);
	enableGlobalShader(scap);
	glUniformMatrix4fv(scap->ModelViewMatrix, 1, GL_FALSE, cursIdentity); 
	glUniformMatrix4fv(scap->ProjectionMatrix, 1, GL_FALSE, cursIdentity);


	//FW_GL_VERTEX_POINTER(2, GL_FLOAT, 0, (GLfloat *)p);
	//sendArraysToGPU(GL_LINE_STRIP, 0, 3);
	positionLoc =  scap->Vertices; //glGetAttribLocation ( shader, "fw_Vertex" );
	glVertexAttribPointer (positionLoc, 2, GL_FLOAT, 
						   GL_FALSE, 0, p );
	glDrawArrays(GL_LINE_STRIP,0,3);

	FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, 0);
	FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, 0);


	glEnable(GL_DEPTH_TEST);
	FW_GL_DEPTHMASK(GL_TRUE);
	restoreGlobalShader();
}
/* the slave cursor method emulates a multitouch, but doesn't suppress 
   the regular mouse cursor, so don't draw ID=0 
   currently no use of angle
   as of March 14, 2012 I'm using this only in stereovision mode, to draw
   viewport alignment fiducials
   */
void statusbarHud_DrawCursor(GLint textureID,int x,int y);

unsigned int getCircleCursorTextureID(){
	//not bad texture for use in testing elsewhere
	ppCursorDraw p;
	ttglobal tg = gglobal();
	p = (ppCursorDraw)tg->CursorDraw.prv;
	if(!p->done)
	{
		glGenTextures(1, &p->textureID);
		glBindTexture(GL_TEXTURE_2D, p->textureID);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, circleCursor.width, circleCursor.height, 0, GL_RGBA , GL_UNSIGNED_BYTE, circleCursor.pixel_data);
		p->done = 1; 
	}
	return p->textureID;
}
void cursorDraw(int ID, int x, int y, float angle) 
{
	XY xy;
	FXY fxy;
	int i,j;
	//GLint shader;
	GLint  positionLoc, texCoordLoc, textureLoc;
    //GLint textureCount;
    GLint textureMatrix0;
	ppCursorDraw p;
	GLfloat cursorVert2[18];
	//GLushort ind[] = {0,1,2,3,4,5};
	//GLint pos, tex;
	s_shader_capabilities_t *scap;
	ttglobal tg = gglobal();
	p = (ppCursorDraw)tg->CursorDraw.prv;

	//if( ID == 0 )return;

	if(!p->done)
	{
		glGenTextures(1, &p->textureID);
		glBindTexture(GL_TEXTURE_2D, p->textureID);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, circleCursor.width, circleCursor.height, 0, GL_RGBA , GL_UNSIGNED_BYTE, circleCursor.pixel_data);
		p->done = 1; 
	}
#ifdef STATUSBAR_HUD
	//Nov 2015: I find this works with emulate_multitouch and multi_window
	statusbarHud_DrawCursor(p->textureID,x,y);
	return;
#endif
#ifndef NEWWAY_COPIED_FROM_STATUSBARHUD_CURSORDRAW
	//Nov 2015: I find this does NOT work 100% with emulate_multitouch and multi_window - it sometimes makes regular scene geometry invisible
	FW_GL_DEPTHMASK(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	//if(p->programObject == 0) initProgramObject();
	//glUseProgram ( p->programObject );
	scap = getMyShader(ONE_TEX_APPEARANCE_SHADER);
	enableGlobalShader(scap);
	//shader = getAppearanceProperties()->currentShaderProperties->myShaderProgram;

	xy = mouse2screen2(x,y);
	FW_GL_VIEWPORT(0, 0, tg->display.screenWidth, tg->display.screenHeight);
	fxy = screen2normalized((GLfloat)xy.x,(GLfloat)xy.y);
	//fxy.y -= 1.0;
	//fxy.x -= 1.0;
	for(i=0;i<6;i++){
		for(j=0;j<3;j++)
			cursorVert2[i*3 + j] = cursorVert[i*3 +j];
		cursorVert2[i*3 +0] += fxy.x;
		cursorVert2[i*3 +1] += fxy.y;
	}
	positionLoc =  scap->Vertices; //glGetAttribLocation ( shader, "fw_Vertex" );
	glVertexAttribPointer (positionLoc, 3, GL_FLOAT, 
						   GL_FALSE, 0, cursorVert2 );
	// Load the texture coordinate
	//texCoordLoc =  glGetAttribLocation ( shader, "fw_MultiTexCoord0"); //"fw_TexCoords" );
	texCoordLoc = scap->TexCoords[0];
	glVertexAttribPointer ( texCoordLoc, 2, GL_FLOAT,
						   GL_FALSE, 0, cursorTex );  //fails - p->texCoordLoc is 429xxxxx - garbage
	//glUniform4f(p->color4fLoc,0.7f,0.7f,0.9f,1.0f);
	glEnableVertexAttribArray (positionLoc );
	glEnableVertexAttribArray ( texCoordLoc);

	//// Bind the base map - see above
	glActiveTexture ( GL_TEXTURE0 );
	glBindTexture ( GL_TEXTURE_2D, p->textureID );

	// Set the base map sampler to texture unit to 0
	//textureLoc =  glGetAttribLocation ( shader, "fw_Texture_unit0"); //"fw_Texture0" );
	textureLoc = scap->TextureUnit[0];
	//textureCount = scap->textureCount;
	//glUniform1i(textureCount,(GLint)1);
	textureMatrix0 = scap->TextureMatrix[0];
	glUniformMatrix4fv(textureMatrix0, 1, GL_FALSE, cursIdentity);

	glUniform1i ( textureLoc, 0 );
	//glDrawElements ( GL_TRIANGLES, 3*2, GL_UNSIGNED_SHORT, ind ); //just render the active ones

	// this more direct hacking also works
	//loc =  glGetAttribLocation ( shader, "fw_ModelViewMatrix" );
	glUniformMatrix4fv(scap->ModelViewMatrix, 1, GL_FALSE, cursIdentity);
	//loc =  glGetAttribLocation ( shader, "fw_ProjectionMatrix" );
	glUniformMatrix4fv(scap->ProjectionMatrix, 1, GL_FALSE, cursIdentity);

	glDrawArrays(GL_TRIANGLES,0,6);

	FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, 0);
	FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, 0);


	glEnable(GL_DEPTH_TEST);
	FW_GL_DEPTHMASK(GL_TRUE);
	restoreGlobalShader();

#endif //NEWWAY_COPIED_FROM_STATUSBARHUD_CURSORDRAW

#ifdef OLDWAY

	FW_GL_DEPTHMASK(GL_FALSE);

    #ifndef GL_ES_VERSION_2_0
	FW_GL_SHADEMODEL(GL_FLAT);
	y += 10;
	#else

    // There is an issue here where Anaglyph rendering gets dinked - see 
    // fwl_RenderSceneUpdateScene() for comments.
    //return;
    

// JAS, trying this GL_PUSH_MATRIX();

	{
		xy = mouse2screen2(x,y);
		//FW_GL_VIEWPORT(0, 0, tg->display.screenWidth, tg->display.screenHeight);
#ifdef OLDGL
		FW_GL_MATRIX_MODE(GL_PROJECTION); //glMatrixMode(GL_PROJECTION);
		FW_GL_LOAD_IDENTITY(); //glLoadIdentity();
		FW_GL_MATRIX_MODE(GL_MODELVIEW); //glMatrixMode(GL_MODELVIEW);
		FW_GL_LOAD_IDENTITY(); //glLoadIdentity();
#endif
		fxy = screen2normalized((GLfloat)xy.x,(GLfloat)xy.y);
#ifdef OLDGL
		FW_GL_TRANSLATE_F((float)fxy.x,(float)fxy.y,0.0f);
#endif
	}
	enableGlobalShader(getMyShader(ONE_TEX_APPEARANCE_SHADER));
	shader = getAppearanceProperties()->currentShaderProperties->myShaderProgram;
	//glEnable(GL_TEXTURE_2D);
	glActiveTexture ( GL_TEXTURE0 );
	glBindTexture ( GL_TEXTURE_2D, p->textureID );
	//SET_TEXTURE_UNIT(0);
				//glActiveTexture(GL_TEXTURE0+c); 
			     /*glUniform1i(loc+c, c); */ 
	loc =  glGetAttribLocation ( shader, "fw_Texture0" );
	glUniform1i(loc,0);
	loc =  glGetAttribLocation ( shader, "fw_Vertex" );
	xy = mouse2screen2(x,y);
	for(i=0;i<6;i++){
		for(j=0;j<3;j++)
			cursorVert2[i*3 + j] = cursorVert[i*3 +j];
		cursorVert2[i*3 +0] += fxy.x;
		cursorVert2[i*3 +1] += fxy.y;

	}
	glVertexAttribPointer ( loc, 3, GL_FLOAT, GL_FALSE, 0, cursorVert2 );
	// Load the texture coordinate
	loc =  glGetAttribLocation ( shader, "fw_TexCoords" );
	glEnableVertexAttribArray ( loc );
	glVertexAttribPointer ( loc, 2, GL_FLOAT, GL_FALSE, 0, cursorTex );  //fails - p->texCoordLoc is 429xxxxx - garbage

	glEnableVertexAttribArray ( loc );



// JAS, trying this GL_POP_MATRIX();

	#endif /* GL_ES_VERSION_2_0 */
	glDisable(GL_DEPTH_TEST);
	
	//xy = mouse2screen2(x,y);
	/* please note that OpenGL ES and OpenGL-3.x does not have the following; here is
	   a hint for future work:
		"If you are using OpenGL ES 2.0, you can use framebuffer objects to render to 
		texture, which is a much better alternative."
	*/
	#ifndef GL_ES_VERSION_2_0
	FW_GL_WINDOWPOS2I(xy.x,xy.y);
	FW_GL_DRAWPIXELS(circleCursor.width,circleCursor.height,GL_BGRA,GL_UNSIGNED_BYTE,circleCursor.pixel_data);
	#else
	if(0){
		// this more direct hacking also works
		loc =  glGetAttribLocation ( shader, "fw_ModelViewMatrix" );
		glUniformMatrix4fv(loc, 1, GL_FALSE, cursIdentity);
		loc =  glGetAttribLocation ( shader, "fw_ProjectionMatrix" );
		glUniformMatrix4fv(loc, 1, GL_FALSE, cursIdentity);
	}
	glDrawArrays(GL_TRIANGLES,0,6);
	#endif /* GL_ES_VERSION_2_0 */

	glEnable(GL_DEPTH_TEST);

	#ifndef GL_ES_VERSION_2_0
	FW_GL_SHADEMODEL(GL_SMOOTH);
	#endif /* GL_ES_VERSION_2_0 */
	FW_GL_DEPTHMASK(GL_TRUE);
	//FW_GL_FLUSH();
#endif //OLDWAY

	return;
}

