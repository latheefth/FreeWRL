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
#include "main/headers.h"
#include "vrml_parser/Structs.h"
#include "scenegraph/Viewer.h"
#include "scenegraph/Component_Shape.h"
#include "opengl/Textures.h"
#include "opengl/LoadTextures.h"
#include "main/MainLoop.h"


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
	GLint textureID;
	int done;
}* ppCursorDraw;
void *CursorDraw_constructor(){
	void *v = malloc(sizeof(struct pCursorDraw));
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
	xy.y = gglobal()->display.screenHeight -y;
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

/* the slave cursor method emulates a multitouch, but doesn't suppress 
   the regular mouse cursor, so don't draw ID=0 
   currently no use of angle
   as of March 14, 2012 I'm using this only in stereovision mode, to draw
   viewport alignment fiducials
   */
void cursorDraw(int ID, int x, int y, float angle) 
{
	XY xy;
	FXY fxy;
	GLint shader, loc;
	ppCursorDraw p;
	ttglobal tg = gglobal();
	p = (ppCursorDraw)tg->CursorDraw.prv;

	//if( ID == 0 )return;

#ifdef GL_ES_VERSION_2_0
    // There is an issue here where Anaglyph rendering gets dinked - see 
    // fwl_RenderSceneUpdateScene() for comments.
    return;
#endif //GL_ES_VERSION_2_0




	FW_GL_DEPTHMASK(GL_FALSE);

    #ifndef GL_ES_VERSION_2_0
	FW_GL_SHADEMODEL(GL_FLAT);
	y += 10;
	#else

    // There is an issue here where Anaglyph rendering gets dinked - see 
    // fwl_RenderSceneUpdateScene() for comments.
    return;
    

// JAS, trying this GL_PUSH_MATRIX();

	if(!p->done)
	{
		glGenTextures(1, &p->textureID);
		glBindTexture(GL_TEXTURE_2D, p->textureID);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, circleCursor.width, circleCursor.height, 0, GL_RGBA , GL_UNSIGNED_BYTE, circleCursor.pixel_data);
		p->done = 1; 
	}
	{
		ttglobal tg = gglobal();
		xy = mouse2screen2(x,y);
		FW_GL_VIEWPORT(0, 0, tg->display.screenWidth, tg->display.screenHeight);
		FW_GL_MATRIX_MODE(GL_PROJECTION); //glMatrixMode(GL_PROJECTION);
		FW_GL_LOAD_IDENTITY(); //glLoadIdentity();
		FW_GL_MATRIX_MODE(GL_MODELVIEW); //glMatrixMode(GL_MODELVIEW);
		FW_GL_LOAD_IDENTITY(); //glLoadIdentity();
		fxy = screen2normalized((GLfloat)xy.x,(GLfloat)xy.y);
		FW_GL_TRANSLATE_F((float)fxy.x,(float)fxy.y,0.0f);
	}
	enableGlobalShader(getMyShader(ONE_TEX_APPEARANCE_SHADER));
	shader = getAppearanceProperties()->currentShaderProperties->myShaderProgram;
	//FW_GL_ENABLE(GL_TEXTURE_2D);
	glActiveTexture ( GL_TEXTURE0 );
	glBindTexture ( GL_TEXTURE_2D, p->textureID );
	//SET_TEXTURE_UNIT(0);
				//glActiveTexture(GL_TEXTURE0+c); 
			     /*glUniform1i(loc+c, c); */ 
	loc =  glGetAttribLocation ( shader, "fw_Texture0" );
	glUniform1i(loc,0);
	loc =  glGetAttribLocation ( shader, "fw_Vertex" );
	glVertexAttribPointer ( loc, 3, GL_FLOAT, GL_FALSE, 0, cursorVert );
	// Load the texture coordinate
	loc =  glGetAttribLocation ( shader, "fw_TexCoords" );
	glEnableVertexAttribArray ( loc );
	glVertexAttribPointer ( loc, 2, GL_FLOAT, GL_FALSE, 0, cursorTex );  //fails - p->texCoordLoc is 429xxxxx - garbage

	glEnableVertexAttribArray ( loc );

	//FW_GL_ENABLECLIENTSTATE (GL_TEXTURE_COORD_ARRAY);
	//FW_GL_ENABLECLIENTSTATE(GL_VERTEX_ARRAY);
	//FW_GL_VERTEX_POINTER (3,GL_FLOAT,0,cursorVert);
	//FW_GL_TEXCOORD_POINTER (2,GL_FLOAT,0,cursorTex);

// JAS, trying this GL_POP_MATRIX();

	#endif /* GL_ES_VERSION_2_0 */
	FW_GL_DISABLE(GL_DEPTH_TEST);
	
	xy = mouse2screen2(x,y);
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
	//FW_GL_DRAWARRAYS (GL_TRIANGLES, 0, 6);
	glDrawArrays(GL_TRIANGLES,0,6);
	#endif /* GL_ES_VERSION_2_0 */

	FW_GL_ENABLE(GL_DEPTH_TEST);

	#ifndef GL_ES_VERSION_2_0
	FW_GL_SHADEMODEL(GL_SMOOTH);
	#endif /* GL_ES_VERSION_2_0 */
	FW_GL_DEPTHMASK(GL_TRUE);
	//FW_GL_FLUSH();
	return;
}

