/*

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

#if !defined(AQUA)
#ifdef OLDCODE
void kill_status (void) {
}
void update_status(char* msg)
{
}
void hudSetConsoleMessage(char *buffer){}
void handleButtonOver(){}
void handleOptionPress(){}
void handleButtonPress(){}

void setMenuButton_collision(int val){}
void setMenuButton_texSize(int size){}
void setMenuButton_headlight(int val){}
void setMenuButton_navModes(int type){}
int handleStatusbarHud(int mev, int* clipplane)
{return 0;}
#endif
void statusbar_init(struct tstatusbar *t){
}
void statusbar_set_window_size(int width, int height)
{
	fwl_setScreenDim(width, height);
}
void statusbar_handle_mouse(int mev, int butnum, int mouseX, int mouseY)
{
	fwl_handle_aqua(mev, butnum, mouseX, mouseY); /* ,gcWheelDelta); */
}

void drawStatusBar()
{
}

#endif //AQUA
