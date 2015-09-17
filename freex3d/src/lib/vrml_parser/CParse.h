/*


VRML-parsing routines in C.

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


#ifndef __FREEWRL_CPARSE_H__
#define __FREEWRL_CPARSE_H__


/* for scanning and determining whether a character is part of a valid X3D name 
http://www.web3d.org/documents/specifications/19776-2/V3.2/Part02/grammar.html#Nodes
http://www.web3d.org/documents/specifications/14772/V2.0/part1/grammar.html  //older allows ':' in DEF/USE ids
See IdFirstChar IdRestChar
Rest char forbids:
0x3a : COLON // just the newer V3.2 disallows, the older 2.0 permits. We will permit
0x0-0x20 NUL - SPACE
0x22,0x23 " #
0x27 '
0x2c,0x2e ,.
0x5b,0x5c,0x5d [\]
0x7b,0x7d,0x7f {} DEL

FirstChar = RestChar minus:
0X30-0X39 - digits
0x2b,0x2d +-
Sept 2015 - we now allow 0x3a colon : in First and Rest - && c!=0x3a
*/
#define IS_ID_REST(c) \
 (c>0x20 && c!=0x22 && c!=0x23 && c!=0x27 && c!=0x2C && c!=0x2E  && c!=0x5B && \
  c!=0x5C && c!=0x5D && c!=0x7B && c!=0x7D && c!=0x7F )
#define IS_ID_FIRST(c) \
 (IS_ID_REST(c) && (c<0x30 || c>0x39) && c!=0x2B && c!=0x2D )

BOOL cParse(void *ectx, void* ptr, unsigned offset, const char* cdata);

/* Destroy all data associated with the currently parsed world kept. */
#define destroyCParserData(me) \
 parser_destroyData(me)

/* Some accessor-methods */
struct X3D_Node* parser_getNodeFromName(const char*);
char* parser_getNameFromNode(struct X3D_Node*);
char* parser_getPROTONameFromNode(struct X3D_Node*);
//extern struct VRMLParser* globalParser;

/* tie assert in here to give better failure methodology */
/* #define ASSERT(cond) if(!(cond)){fw_assert(__FILE__,__LINE__);} */
/* void fw_assert(char *,int); */


#endif /* __FREEWRL_CPARSE_H__ */
