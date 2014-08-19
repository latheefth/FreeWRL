/*


General functions declarations.

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


#ifndef __FREEWRL_PRODCON_MAIN_H__
#define __FREEWRL_PRODCON_MAIN_H__


void registerBindable(struct X3D_Node *);
extern char *EAI_Flag;
void frontend_dequeue_get_enqueue();
int frontendGetsFiles();
void resitem_queue_flush();
void resitem_queue_exit();
void resitem_enqueue_tg(s_list_t *item, void* tg);
bool parser_process_res_VRML_X3D(resource_item_t *res);

#endif /* __FREEWRL_PRODCON_MAIN_H__ */
