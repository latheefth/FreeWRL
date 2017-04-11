/*


X3D parser functions.

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


#ifndef __FREEWRL_X3D_PARSER_H__
#define __FREEWRL_X3D_PARSER_H__

// used by EAIEventsIn.c, XMLParser.c
char *X3DParser_getNameFromNode(struct X3D_Node* myNode);
struct X3D_Node *X3DParser_getNodeFromName(const char *name);

int X3DParse (struct X3D_Node* ectx, struct X3D_Node* myParent, const char *inputstring);
void QAandRegister_parsedRoute_B(struct X3D_Proto *context, char* fnode, char* ffield, char* tnode, char* tfield);
void kill_X3DDefs(void);
int freewrl_XML_GetCurrentLineNumber(void);

// the following are actually used elsewhere, but some of the params are specific;
// grep for the user files...
//void endCDATA (void *ud, const xmlChar *string, int len);
//void X3DParser_clear(struct tX3DParser *t);
//void X3DParser_init(struct tX3DParser *t);

#endif /*  __FREEWRL_X3D_PARSER_H__ */
