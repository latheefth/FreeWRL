/*


???

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



// JAS - OLDCODE #ifndef REWIRE
#include <config.h>
#include <system.h>
#include <libFreeWRL.h>
// JAS - OLDCODE #endif
#include <display.h>
#include <internal.h>


#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../world_script/JScript.h"
#include "../world_script/CScripts.h"
#include "../vrml_parser/CParseParser.h"
#include "../vrml_parser/CParseLexer.h"
#include "EAIHeaders.h"
#include "EAIHelpers.h"

/* TODO: clean-up Rewire */
// JAS - OLDCODE #ifdef REWIRE
// JAS - OLDCODE # include "../../libeai/EAI_C.h"
// JAS - OLDCODE # define ADD_PARENT(a,b)
// JAS - OLDCODE #endif

/* assume eaiverbose is false, unless told otherwise */
//int eaiverbose = FALSE;
typedef struct pEAI_C_CommonFunctions{
	struct VRMLParser *parser; // = NULL;
}* ppEAI_C_CommonFunctions;
void *EAI_C_CommonFunctions_constructor()
{
	void *v = MALLOCV(sizeof(struct pEAI_C_CommonFunctions));
	memset(v,0,sizeof(struct pEAI_C_CommonFunctions));
	return v;
}

void EAI_C_CommonFunctions_init(struct tEAI_C_CommonFunctions* t){
	//public
	t->eaiverbose = FALSE;
	//private
	t->prv = EAI_C_CommonFunctions_constructor();
	//just a pointer - null init ok
}

#define PST_MF_STRUCT_ELEMENT(type1,type2) \
	case FIELDTYPE_MF##type1: { \
		struct Multi_##type1 *myv; \
		myv = (struct Multi_##type1 *) nst; \
		/* printf ("old val p= %u, n = %d\n",myv->p, myv->n); */\
		myv->p = myVal.mf##type2.p; \
		myv->n = myVal.mf##type2.n; \
		/* printf ("PST_MF_STRUCT_ELEMENT, now, element count %d\n",myv->n); */ \
		break; }


#define PST_SF_SIMPLE_ELEMENT(type1,type2,size3) \
	case FIELDTYPE_SF##type1: { \
		memcpy(nst, &myVal.sf##type2, size3); \
		break; }


/* create a structure to hold a string; it has a length, and a string pointer */
struct Uni_String *newASCIIString(char *str) {
	struct Uni_String *retval;
	int len;
	int eaiverbose = gglobal()->EAI_C_CommonFunctions.eaiverbose;

	if (eaiverbose) {
	printf ("newASCIIString for :%s:\n",str);
	}

	/* the returning Uni_String is here. Make blank struct */
	retval = MALLOC (struct Uni_String *, sizeof (struct Uni_String));
	len = (int) strlen(str);

	retval->strptr  = MALLOC (char *, sizeof(char) * len+1);
	strncpy(retval->strptr,str,len+1);
	retval->len = len+1;
	retval->touched = 1; /* make it 1, to signal that this is a NEW string. */

	/* printf ("newASCIIString, returning UniString %x, strptr %u for string :%s:\n",retval, retval->strptr,str); */

	return retval;
}



void clearASCIIString(struct Uni_String *us);
void freeASCIIString(struct Uni_String *us);
void clearMFString(struct Multi_String *ms);
void freeMFString(struct Multi_String **ms);

void clearASCIIString(struct Uni_String *us){
	if(us){
		FREE_IF_NZ(us->strptr);
		us->strptr = NULL;
		us->len = 0;
	}
}
void freeASCIIString(struct Uni_String *us){
	clearASCIIString(us);
	FREE_IF_NZ(us);
}
void clearMFString(struct Multi_String *ms){
	if(ms){
		int i;
		//printf("ms.n=%d\n",ms->n);
		for(i=0;i<ms->n;i++){
			struct Uni_String *us = ms->p[i];
			//printf("us[%d]='%s'\n",i,us->strptr);
			freeASCIIString(ms->p[i]);
		}
		ms->n = 0;
		FREE_IF_NZ(ms->p);
	}
}
void freeMFString(struct Multi_String **ms){
	clearMFString(*ms);
	FREE_IF_NZ(*ms);
}

/* do these strings differ?? If so, copy the new string over the old, and 
touch the touched flag */
void verify_Uni_String(struct  Uni_String *unis, char *str) {
	char *ns;
	char *os;
	size_t len;
	// JASint eaiverbose = gglobal()->EAI_C_CommonFunctions.eaiverbose;

	/* bounds checking */
	if (unis == NULL) {
		printf ("Warning, verify_Uni_String, comparing to NULL Uni_String, %s\n",str);
		return;
	}

	/* are they different? */
	if (strcmp(str,unis->strptr)!= 0) {
		os = unis->strptr;
		len = strlen(str);
		ns = MALLOC (char *,len+1);
		strncpy(ns,str,len+1);
		unis->strptr = ns;
		FREE_IF_NZ (os);
		unis->touched++;
	}
}
		



/* get how many bytes in the type */
int  returnElementLength(int type) {
	  switch (type) {
		case FIELDTYPE_SFVec2d:
		case FIELDTYPE_MFVec2d:
		case FIELDTYPE_MFVec3d:
		case FIELDTYPE_SFVec3d:
		case FIELDTYPE_MFDouble:
		case FIELDTYPE_SFDouble:
		case FIELDTYPE_SFVec4d:
		case FIELDTYPE_MFVec4d:
		case FIELDTYPE_SFMatrix3d:
		case FIELDTYPE_MFMatrix3d:
		case FIELDTYPE_SFMatrix4d:
		case FIELDTYPE_MFMatrix4d:
		case FIELDTYPE_SFTime :
    		case FIELDTYPE_MFTime : return (int) sizeof(double); break;
			case FIELDTYPE_SFImage:
    		case FIELDTYPE_MFInt32: return (int) sizeof(int)   ; break;
		case FIELDTYPE_FreeWRLPTR:
		case FIELDTYPE_MFString:
		case FIELDTYPE_SFString:
    		case FIELDTYPE_SFNode :
    		case FIELDTYPE_MFNode : return (int) sizeof(void *); break;
	  	default     : {}
	}
	return (int) sizeof(float) ; /* turn into byte count */
}

/* for passing into CRoutes/CRoutes_Register */
/* lengths are either positive numbers, or, if there is a complex type, a negative number. If positive,
   in routing a memcpy is performed; if negative, then some inquiry is required to get correct length
   of both source/dest during routing. */

int returnRoutingElementLength(int type) {
	  switch (type) {
		case FIELDTYPE_SFTime:	return (int) sizeof(double); break;
		case FIELDTYPE_SFBool:
		case FIELDTYPE_SFInt32:	return (int) sizeof(int); break;
		case FIELDTYPE_SFFloat:	return (int) sizeof (float); break;
		case FIELDTYPE_SFVec2f:	return (int) sizeof (struct SFVec2f); break;
		case FIELDTYPE_SFVec3f:
		case FIELDTYPE_SFColor: 	return (int) sizeof (struct SFColor); break;
		case FIELDTYPE_SFVec3d: return (int) sizeof (struct SFVec3d); break;
		case FIELDTYPE_SFColorRGBA:
		case FIELDTYPE_SFRotation:return (int) sizeof (struct SFRotation); break;
		case FIELDTYPE_SFNode:	return (int) ROUTING_SFNODE; break;
		case FIELDTYPE_SFMatrix3f: return (int) sizeof (struct SFMatrix3f); break;
		case FIELDTYPE_SFMatrix3d: return (int) sizeof (struct SFMatrix3d); break;
/* FIXME FIND DEF FOR SFVEC4F */
// JAS - OLDCODE #ifndef REWIRE
		case FIELDTYPE_SFVec4f: return (int) sizeof (struct SFVec4f) ; break;
// JAS - OLDCODE #endif
		case FIELDTYPE_SFMatrix4f: return (int) sizeof (struct SFMatrix4f); break;
		case FIELDTYPE_SFVec2d: return (int) sizeof (struct SFVec2d); break;
		case FIELDTYPE_SFDouble: return (int) sizeof (double); break;
		case FIELDTYPE_SFVec4d: return (int) sizeof (struct SFVec4d); break;

		case FIELDTYPE_SFString: return (int) ROUTING_SFSTRING; break;
		case FIELDTYPE_SFImage:	return (int) ROUTING_SFIMAGE; break;

		case FIELDTYPE_MFNode:	return (int) ROUTING_MFNODE; break;
		case FIELDTYPE_MFString: 	return (int) ROUTING_MFSTRING; break;
		case FIELDTYPE_MFFloat:	return (int) ROUTING_MFFLOAT; break;
		case FIELDTYPE_MFColorRGBA:
		case FIELDTYPE_MFRotation: return (int) ROUTING_MFROTATION; break;
		case FIELDTYPE_MFBool:
		case FIELDTYPE_MFInt32:	return (int) ROUTING_MFINT32; break;
		case FIELDTYPE_MFColor:	return (int) ROUTING_MFCOLOR; break;
		case FIELDTYPE_MFVec2f:	return (int) ROUTING_MFVEC2F; break;
		case FIELDTYPE_MFVec3f:	return (int) ROUTING_MFVEC3F; break;
		case FIELDTYPE_MFVec3d: return (int) ROUTING_MFVEC3D; break;
		case FIELDTYPE_MFDouble: return (int) ROUTING_MFDOUBLE; break;
		case FIELDTYPE_MFTime: return (int) ROUTING_MFDOUBLE; break;
		case FIELDTYPE_MFMatrix4f: return (int) ROUTING_MFMATRIX4F; break;
		case FIELDTYPE_MFMatrix4d: return (int) ROUTING_MFMATRIX4D; break;
		case FIELDTYPE_MFVec2d: return (int) ROUTING_MFVEC2D; break;
		case FIELDTYPE_MFVec4f: return (int) ROUTING_MFVEC4F; break;
		case FIELDTYPE_MFVec4d: return (int) ROUTING_MFVEC4D; break;
		case FIELDTYPE_MFMatrix3f: return (int) ROUTING_MFMATRIX3F; break;
		case FIELDTYPE_MFMatrix3d: return (int) ROUTING_MFMATRIX3D; break;

                default:{
			printf ("warning - returnRoutingElementLength not a handled type, %d\n",type);
		}
	}
	return (int) sizeof(int);
} 



/* how many numbers/etc in an array entry? eg, SFVec3f = 3 - 3 floats */
/*		"" ""			eg, MFVec3f = 3 - 3 floats, too! */
int returnElementRowSize (int type) {
	switch (type) {
		case FIELDTYPE_SFVec2f:
		case FIELDTYPE_MFVec2f:
		case FIELDTYPE_SFVec2d:
		case FIELDTYPE_MFVec2d:
			return 2;
		case FIELDTYPE_SFColor:
		case FIELDTYPE_MFColor:
		case FIELDTYPE_SFVec3f:
		case FIELDTYPE_SFVec3d:
		case FIELDTYPE_MFVec3f:
		case FIELDTYPE_MFVec3d:
		case FIELDTYPE_SFImage: /* initialization - we can have a "0,0,0" for no texture */
			return 3;
		case FIELDTYPE_SFRotation:
		case FIELDTYPE_MFRotation:
		case FIELDTYPE_SFVec4d:
		case FIELDTYPE_SFVec4f:
		case FIELDTYPE_MFVec4d:
		case FIELDTYPE_MFVec4f:
		case FIELDTYPE_SFColorRGBA:
		case FIELDTYPE_MFColorRGBA:
			return 4;
		case FIELDTYPE_MFMatrix3f:
		case FIELDTYPE_SFMatrix3f:
		case FIELDTYPE_MFMatrix3d:
		case FIELDTYPE_SFMatrix3d:
			return 9;
		case FIELDTYPE_MFMatrix4f:
		case FIELDTYPE_SFMatrix4f:
		case FIELDTYPE_MFMatrix4d:
		case FIELDTYPE_SFMatrix4d:
			return 16;
	}
	return 1;

}



int mf2sf(int itype){
	//luckily the fieldtype defines are consistently mf = sf+1
	//return convertToSFType(itype); //this is more reliable -converts and sf to itself- but bulky
	if(itype == FIELDTYPE_SFImage)
		return FIELDTYPE_SFInt32;
	return itype -1;
}
int sf2mf(int itype){
	//luckily the fieldtype defines are consistently mf = sf+1
	return itype +1;
}

/*we seem to be missing something in generated code/structs that would allow me to
  look up how big something is. I suspect it's ##MACRO-ized elsewhere.
*/


int isSForMFType(int itype){
	//sadly the fieldtype defines aren't reliably even or odd for sf or mf, so we'll do a switch case
	//-1 unknown /not a fieldtype
	//0 SF
	//1 MF
	int iret;
	switch(itype){
		case FIELDTYPE_SFFloat: 
		case FIELDTYPE_SFRotation:	
		case FIELDTYPE_SFVec3f:	
		case FIELDTYPE_SFBool:	
		case FIELDTYPE_SFInt32:	
		case FIELDTYPE_SFNode:	
		case FIELDTYPE_SFColor:	
		case FIELDTYPE_SFColorRGBA:	
		case FIELDTYPE_SFTime:	
		case FIELDTYPE_SFString: 
		case FIELDTYPE_SFVec2f:	
		//case FIELDTYPE_SFImage:
		case FIELDTYPE_SFVec3d:	
		case FIELDTYPE_SFDouble: 
		case FIELDTYPE_SFMatrix3f: 
		case FIELDTYPE_SFMatrix3d: 
		case FIELDTYPE_SFMatrix4f: 
		case FIELDTYPE_SFMatrix4d: 
		case FIELDTYPE_SFVec2d: 
		case FIELDTYPE_SFVec4f:	
		case FIELDTYPE_SFVec4d:	
			iret = 0;
			break;

		case FIELDTYPE_MFFloat: 
		case FIELDTYPE_MFRotation:	
		case FIELDTYPE_MFVec3f:	
		case FIELDTYPE_MFBool:	
		case FIELDTYPE_MFInt32:	
		case FIELDTYPE_MFNode:	
		case FIELDTYPE_MFColor:	
		case FIELDTYPE_MFColorRGBA:	
		case FIELDTYPE_MFTime:	
		case FIELDTYPE_MFString: 
		case FIELDTYPE_MFVec2f:	
		case FIELDTYPE_SFImage: //
		case FIELDTYPE_MFVec3d:	
		case FIELDTYPE_MFDouble: 
		case FIELDTYPE_MFMatrix3f: 
		case FIELDTYPE_MFMatrix3d: 
		case FIELDTYPE_MFMatrix4f: 
		case FIELDTYPE_MFMatrix4d: 
		case FIELDTYPE_MFVec2d: 
		case FIELDTYPE_MFVec4f:	
		case FIELDTYPE_MFVec4d:	
			iret = 1; break;
		default:
			iret = -1; break;
	}
	return iret;
}
int type2SF(int itype){
	//unconditionally returns sf type
	int jtype, sformf = isSForMFType(itype);
	if(sformf < 0) return -1;
	jtype = itype;
	if(sformf == 1) jtype = mf2sf(itype);
	return jtype;
}
int isSFType(int itype){
	return (isSForMFType(itype) == 0) ? 1 : 0;
}
#define FIELDTYPE_MFImage	43 
int sizeofSForMF(int itype){
	//goal get the offset for MF.p[i] in bytes
	//or also this is the 'shallow size' for field copying
	int iz;
	switch(itype){
	case FIELDTYPE_SFFloat: iz = sizeof(float); break;
	case FIELDTYPE_SFRotation:	iz = sizeof(struct SFRotation); break;
	case FIELDTYPE_SFVec3f:	iz = sizeof(struct SFVec3f);break;
	case FIELDTYPE_SFBool:	iz = sizeof(int); break;
	case FIELDTYPE_SFInt32:	iz = sizeof(int); break;
	case FIELDTYPE_SFNode:	iz = sizeof(void*); break;
	case FIELDTYPE_SFColor:	iz = sizeof(struct SFColor); break;
	case FIELDTYPE_SFColorRGBA:	iz = sizeof(struct SFColorRGBA); break;
	case FIELDTYPE_SFTime:	iz = sizeof(double); break;
	case FIELDTYPE_SFString: iz = sizeof(struct Uni_String *); break;  //sizeof(void *) because nodes that have a string field declare it struct Uni_String *, so when copying to a node, you copy sizeof(void*). H: if the char *string is const, then uni_string is const (they may hang out as pals for life, or char *string may outlive its uni_string pal
	case FIELDTYPE_SFVec2f:	iz = sizeof(struct SFVec2f); break;
	//case FIELDTYPE_SFImage:	iz = sizeof(void*); break;
	case FIELDTYPE_SFVec3d:	iz = sizeof(struct SFVec3d); break;
	case FIELDTYPE_SFDouble: iz = sizeof(double); break;
	case FIELDTYPE_SFMatrix3f: iz = sizeof(struct SFMatrix3f); break;
	case FIELDTYPE_SFMatrix3d: iz = sizeof(struct SFMatrix3d); break;
	case FIELDTYPE_SFMatrix4f: iz = sizeof(struct SFMatrix4f); break;
	case FIELDTYPE_SFMatrix4d: iz = sizeof(struct SFMatrix4d); break;
	case FIELDTYPE_SFVec2d: iz = sizeof(struct SFVec2d); break;
	case FIELDTYPE_SFVec4f:	iz = sizeof(struct SFVec4f); break;
	case FIELDTYPE_SFVec4d:	iz = sizeof(struct SFVec4d); break;

	case FIELDTYPE_SFImage: //same as MFInt32
	case FIELDTYPE_MFFloat: 
	case FIELDTYPE_MFRotation:	
	case FIELDTYPE_MFVec3f:	
	case FIELDTYPE_MFBool:	
	case FIELDTYPE_MFInt32:	
	case FIELDTYPE_MFNode:	
	case FIELDTYPE_MFColor:	
	case FIELDTYPE_MFColorRGBA:	
	case FIELDTYPE_MFTime:	
	case FIELDTYPE_MFString: 
	case FIELDTYPE_MFVec2f:	
	case FIELDTYPE_MFImage:
	case FIELDTYPE_MFVec3d:	
	case FIELDTYPE_MFDouble: 
	case FIELDTYPE_MFMatrix3f: 
	case FIELDTYPE_MFMatrix3d: 
	case FIELDTYPE_MFMatrix4f: 
	case FIELDTYPE_MFMatrix4d: 
	case FIELDTYPE_MFVec2d: 
	case FIELDTYPE_MFVec4f:	
	case FIELDTYPE_MFVec4d:	
		iz = sizeof(struct Multi_Node);
		break;
	default:
		//unknown
		iz = sizeof(void*);
		break;
	}
	return iz;
}
int sizeofSF(int itype){
	int jtype;
	int sformf = isSForMFType(itype);
	if( sformf < 0) return 0;
	jtype = itype;
	if( sformf == 1 ) jtype = mf2sf(itype);
	return sizeofSForMF(jtype);
}


//static struct VRMLParser *parser = NULL;

/* from the XML parser, for instance, we can call this on close to delete memory and memory tables */
void Parser_deleteParserForScanStringValueToMem(void) {
	ppEAI_C_CommonFunctions p = (ppEAI_C_CommonFunctions)gglobal()->EAI_C_CommonFunctions.prv;
	if (p==NULL) return;
	if (p->parser != NULL) {
		lexer_destroyData(p->parser->lexer);
		deleteParser(p->parser);
		p->parser = NULL;
	}
}


void Parser_scanStringValueToMem(struct X3D_Node *node, size_t coffset, indexT ctype, char *value, int isXML) {
	void *nst;                      /* used for pointer maths */
	union anyVrml myVal;
	char *mfstringtmp = NULL;
	int oldXMLflag;
	struct X3D_Node *np;
	struct VRMLParser *parser = ((ppEAI_C_CommonFunctions)gglobal()->EAI_C_CommonFunctions.prv)->parser;
	#ifdef SETFIELDVERBOSE
	printf ("\nPST, for %s we have %s strlen %lu\n",stringFieldtypeType(ctype), value, strlen(value));
	#endif
	np = NULL;
	/* if this is the first time through, create a new parser, and tell it:
	      - that we are using X3D formatted field strings, NOT "VRML" ones;
	      - that the destination node is not important (the NULL, offset 0) */

	if (parser == NULL) {
		parser=newParser(NULL,NULL, 0, TRUE);
		//ConsoleMessage ("Parser_ScanStringValueToMem, new parser created");
		// save it
		((ppEAI_C_CommonFunctions)gglobal()->EAI_C_CommonFunctions.prv)->parser = parser;
	}

	lexer_forceStringCleanup(parser->lexer);

	/* October 20, 2009; XML parsing should not go through here; XML encoded X3D should not have a "value=" field, but
	   have the SFNode or MFNode as part of the syntax, eg <field ...> <Box/> </field> */

	if (isXML) {
		/* printf ("we have XML parsing for type %s, string :%s:\n",stringFieldtypeType(ctype),value); */
		if ((ctype==FIELDTYPE_SFNode) || (ctype==FIELDTYPE_MFNode)) {
			/* printf ("returning\n"); */
			lexer_forceStringCleanup(parser->lexer);
			return;
		}

	}

	/* there is a difference sometimes, in the XML format and VRML classic format. The XML
	   parser will use xml format, scripts and EAI will use the classic format */
	oldXMLflag = parser->parsingX3DfromXML;
	parser->parsingX3DfromXML = isXML;

	/* we NEED MFStrings to have quotes on; so if this is a MFString, ensure quotes are ok */
	if (ctype == FIELDTYPE_MFString) {
		#ifdef SETFIELDVERBOSE
		printf ("parsing type %s, string :%s:\n",stringFieldtypeType(ctype),value); 
		#endif

		/* go to the first non-space character, and see if this is required;
		   sometimes people will encode mfstrings as:
			url=' "images/earth.gif" "http://ww
		   note the space in the value */
		while ((*value == ' ') && (*value != '\0')) value ++;

		/* now, does the value string need quoting? */
		if ((*value != '"') && (*value != '\'') && (*value != '[')) {
			size_t len;
			 /* printf ("have to quote this string\n"); */
			len = strlen(value);
			mfstringtmp = MALLOC (char *, sizeof (char *) * len + 10);
			memcpy (&mfstringtmp[1],value,len);
			mfstringtmp[0] = '"';
			mfstringtmp[len+1] = '"';
			mfstringtmp[len+2] = '\0';
			/* printf ("so, mfstring is :%s:\n",mfstringtmp); */ 
			
		} else {
			mfstringtmp = STRDUP(value);
		}
		parser_fromString(parser,mfstringtmp);
		/* FREE_IF_NZ(mfstringtmp); */
	} else if (ctype == FIELDTYPE_SFNode) {
		/* Need to change index to proper node ptr */
		np = getEAINodeFromTable(atoi(value), -1);
	} else if (ctype == FIELDTYPE_SFString) {
		if(isXML){
			/* double quotes " are unique to x3d values and must be \" to pass javascript compiling */
			int nq = 0;
			char *mv, *pv, *v = value;
			while (*v && *v != '\0')
			{	
				if(*v == '"') nq++;
				v++;
			}
			mfstringtmp = (char *)MALLOC(void *, strlen(value)+nq+1);
			v = value;
			pv = NULL;
			mv = mfstringtmp;
			while(*v && *v != '\0')
			{
				if(*v == '"'){
					if(!(pv && *pv == '\\')){
						*mv = '\\';
						mv++;
					}
				}
				*mv = *v;
				mv++;
				pv = v;
				v++;
			}
			*mv = '\0';
		}else{
			mfstringtmp = STRDUP(value);
		}
		parser_fromString(parser,mfstringtmp);
	} else {
		mfstringtmp = STRDUP(value);
		parser_fromString(parser,mfstringtmp);
		/* FREE_IF_NZ(mfstringtmp); */
	}

	ASSERT(parser->lexer);
	FREE_IF_NZ(parser->lexer->curID);

	if (ctype == FIELDTYPE_SFNode) {
		struct X3D_Node* oldvalue;
		nst = offsetPointer_deref(void *,node,coffset);
		memcpy (&oldvalue, nst, sizeof(struct X3D_Node*));
		if (oldvalue) {
			remove_parent(oldvalue, node);
		}
		if(np){
			memcpy(nst, (void*)&np, sizeof(struct X3D_Node*));
			add_parent(np, node, "sarah's add", 0);
		}
	} else if (parseType(parser, ctype, &myVal)) {
		/* printf ("parsed successfully\n");  */
		nst = offsetPointer_deref(void *,node,coffset);


/*
MF_TYPE(MFNode, mfnode, Node)
*/
		switch (ctype) {

			PST_MF_STRUCT_ELEMENT(Vec2f,vec2f)
			PST_MF_STRUCT_ELEMENT(Vec3f,vec3f)
			PST_MF_STRUCT_ELEMENT(Vec3d,vec3d)
			PST_MF_STRUCT_ELEMENT(Vec4d,vec4d)
			PST_MF_STRUCT_ELEMENT(Vec2d,vec2d)
			PST_MF_STRUCT_ELEMENT(Color,color)
			PST_MF_STRUCT_ELEMENT(ColorRGBA,colorrgba)
			PST_MF_STRUCT_ELEMENT(Int32,int32)
			PST_MF_STRUCT_ELEMENT(Float,float)
			PST_MF_STRUCT_ELEMENT(Double,double)
			PST_MF_STRUCT_ELEMENT(Bool,bool)
			PST_MF_STRUCT_ELEMENT(Time,time)
			PST_MF_STRUCT_ELEMENT(Rotation,rotation)
			PST_MF_STRUCT_ELEMENT(Matrix3f,matrix3f)
			PST_MF_STRUCT_ELEMENT(Matrix3d,matrix3d)
			PST_MF_STRUCT_ELEMENT(Matrix4f,matrix4f)
			PST_MF_STRUCT_ELEMENT(Matrix4d,matrix4d)
			PST_MF_STRUCT_ELEMENT(String,string)

			PST_SF_SIMPLE_ELEMENT(Float,float,sizeof(float))
			PST_SF_SIMPLE_ELEMENT(Time,time,sizeof(double))
			PST_SF_SIMPLE_ELEMENT(Double,double,sizeof(double))
			PST_SF_SIMPLE_ELEMENT(Int32,int32,sizeof(int))
			PST_SF_SIMPLE_ELEMENT(Bool,bool,sizeof(int))
			PST_SF_SIMPLE_ELEMENT(Node,node,sizeof(void *))
			PST_SF_SIMPLE_ELEMENT(Vec2f,vec2f,sizeof(struct SFVec2f))
			PST_SF_SIMPLE_ELEMENT(Vec2d,vec2d,sizeof(struct SFVec2d))
			PST_SF_SIMPLE_ELEMENT(Vec3f,vec3f,sizeof(struct SFColor))
			PST_SF_SIMPLE_ELEMENT(Vec3d,vec3d,sizeof(struct SFVec3d))
			PST_SF_SIMPLE_ELEMENT(Vec4d,vec4d,sizeof(struct SFVec4d))
			PST_SF_SIMPLE_ELEMENT(Rotation,rotation,sizeof(struct SFRotation))
			PST_SF_SIMPLE_ELEMENT(Color,color,sizeof(struct SFColor))
			PST_SF_SIMPLE_ELEMENT(ColorRGBA,colorrgba,sizeof(struct SFColorRGBA))
			PST_SF_SIMPLE_ELEMENT(Matrix3f,matrix3f,sizeof(struct SFMatrix3f))
			PST_SF_SIMPLE_ELEMENT(Matrix4f,matrix4f,sizeof(struct SFMatrix4f))
			PST_SF_SIMPLE_ELEMENT(Matrix3d,matrix3d,sizeof(struct SFMatrix3d))
			PST_SF_SIMPLE_ELEMENT(Matrix4d,matrix4d,sizeof(struct SFMatrix4d))
			PST_SF_SIMPLE_ELEMENT(Image,image,sizeof(struct Multi_Int32))

			case FIELDTYPE_SFString: {
					//struct Uni_String *mptr;
					memcpy(nst, &myVal.sfstring, sizeof(struct Uni_String*));
					//mptr = * (struct Uni_String **)nst;
					//if (!mptr) {
					//	ERROR_MSG("Parser_scanStringValueToMem: is nst (Uni_String) supposed to hold a NULL value ?");
					//} else {
					//	FREE_IF_NZ(mptr->strptr);
					//	mptr->strptr = myVal.sfstring->strptr;
					//	mptr->len = myVal.sfstring->len;
					//	mptr->touched = myVal.sfstring->touched;
					//}
				break; }

			default: {
				printf ("unhandled type, in EAIParse  %s\n",stringFieldtypeType(ctype));
				lexer_forceStringCleanup(parser->lexer);
				return;
			}
		}

	} else {
		if (strlen (value) > 50) {
			value[45] = '.';
			value[46] = '.';
			value[47] = '.';
			value[48] = '\0';
		}
		ConsoleMessage ("parser problem on parsing fieldType %s, string :%s:", stringFieldtypeType(ctype),value);
	}

	/* tell the parser that we have done with the input - it will FREE the data */
	lexer_forceStringCleanup(parser->lexer);

	/* and, reset the XML flag */
	parser->parsingX3DfromXML = oldXMLflag;
}


void Parser_scanStringValueToMem_B(union anyVrml* any, indexT ctype, char *value, int isXML) 
{
	//dug9 Feb 2013: same as Parser_scanStringValueToMem except:
	// - puts it into *anyVrml instead of (node,offset)
	// - doesn't update parents for SFNode, MFNode fields (just zeros it) - that's done outside
	void *nst;                      /* used for pointer maths */
	union anyVrml myVal;
	char *mfstringtmp = NULL;
	int oldXMLflag;
	struct X3D_Node *np = NULL;
	struct VRMLParser *parser = ((ppEAI_C_CommonFunctions)gglobal()->EAI_C_CommonFunctions.prv)->parser;
	#ifdef SETFIELDVERBOSE
	printf ("\nPST, for %s we have %s strlen %lu\n",stringFieldtypeType(ctype), value, strlen(value));
	#endif

	/* if this is the first time through, create a new parser, and tell it:
	      - that we are using X3D formatted field strings, NOT "VRML" ones;
	      - that the destination node is not important (the NULL, offset 0) */

	if (parser == NULL) {
		parser=newParser(NULL,NULL, 0, TRUE);
		//ConsoleMessage ("Parser_ScanStringValueToMem, new parser created");
		// save it
		((ppEAI_C_CommonFunctions)gglobal()->EAI_C_CommonFunctions.prv)->parser = parser;
	}

	lexer_forceStringCleanup(parser->lexer);

	/* October 20, 2009; XML parsing should not go through here; XML encoded X3D should not have a "value=" field, but
	   have the SFNode or MFNode as part of the syntax, eg <field ...> <Box/> </field> */

	if (isXML) {
		/* printf ("we have XML parsing for type %s, string :%s:\n",stringFieldtypeType(ctype),value); */
		if ((ctype==FIELDTYPE_SFNode) || (ctype==FIELDTYPE_MFNode)) {
			/* printf ("returning\n"); */
			lexer_forceStringCleanup(parser->lexer);
			return;
		}

	}

	/* there is a difference sometimes, in the XML format and VRML classic format. The XML
	   parser will use xml format, scripts and EAI will use the classic format */
	oldXMLflag = parser->parsingX3DfromXML;
	parser->parsingX3DfromXML = isXML;

	/* we NEED MFStrings to have quotes on; so if this is a MFString, ensure quotes are ok */
	if (ctype == FIELDTYPE_MFString) {
		#ifdef SETFIELDVERBOSE
		printf ("parsing type %s, string :%s:\n",stringFieldtypeType(ctype),value); 
		#endif

		/* go to the first non-space character, and see if this is required;
		   sometimes people will encode mfstrings as:
			url=' "images/earth.gif" "http://ww
		   note the space in the value */
		while ((*value == ' ') && (*value != '\0')) value ++;

		/* now, does the value string need quoting? */
		if ((*value != '"') && (*value != '\'') && (*value != '[')) {
			static int MFS_warning_given = 0;
			size_t len;
			 /* printf ("have to quote this string\n"); */
			len = strlen(value);
			mfstringtmp = MALLOC (char *, sizeof (char *) * len + 10);
			memcpy (&mfstringtmp[1],value,len);
			mfstringtmp[0] = '"';
			mfstringtmp[len+1] = '"';
			mfstringtmp[len+2] = '\0';
			if(!MFS_warning_given){
				ConsoleMessage("Warning - an MFString needs internal quotes ie '%s' should be '%s'\n",value,mfstringtmp);
				MFS_warning_given = 1;
			}
			/* printf ("so, mfstring is :%s:\n",mfstringtmp); */ 
			
		} else {
			mfstringtmp = STRDUP(value);
		}
		parser_fromString(parser,mfstringtmp);
		/* FREE_IF_NZ(mfstringtmp); */
	} else if (ctype == FIELDTYPE_SFNode) {
		/* Need to change index to proper node ptr */
		np = getEAINodeFromTable(atoi(value), -1);
	} else if (ctype == FIELDTYPE_SFString) {
		if(isXML){
			/* double quotes " are unique to x3d values and must be \" to pass javascript compiling */
			int nq = 0;
			char *mv, *pv, *v = value;
			while (*v && *v != '\0')
			{	
				if(*v == '"') nq++;
				v++;
			}
			mfstringtmp = (char *)MALLOC(void *, strlen(value)+nq+1);
			v = value;
			pv = NULL;
			mv = mfstringtmp;
			while(*v && *v != '\0')
			{
				if(*v == '"'){
					if(!(pv && *pv == '\\')){
						*mv = '\\';
						mv++;
					}
				}
				*mv = *v;
				mv++;
				pv = v;
				v++;
			}
			*mv = '\0';
		}else{
			mfstringtmp = STRDUP(value);
		}
		parser_fromString(parser,mfstringtmp);
	} else {
		mfstringtmp = STRDUP(value);
		parser_fromString(parser,mfstringtmp);
		/* FREE_IF_NZ(mfstringtmp); */
	}

	ASSERT(parser->lexer);
	FREE_IF_NZ(parser->lexer->curID);

	if (ctype == FIELDTYPE_SFNode) {
		//struct X3D_Node* oldvalue;
		//nst = offsetPointer_deref(void *,node,coffset);
		//memcpy (&oldvalue, any, sizeof(struct X3D_Node*));
		//if (oldvalue) {
		//	remove_parent(oldvalue, node);
		//}
		memcpy(any, (void*)&np, sizeof(struct X3D_Node*));
		any->sfnode->_parentVector = NULL;
		//add_parent(np, node, "sarah's add", 0);
	} else if (parseType(parser, ctype, &myVal)) {
		/* printf ("parsed successfully\n");  */
		//nst = offsetPointer_deref(void *,node,coffset);
		nst = any;

/*
MF_TYPE(MFNode, mfnode, Node)
*/
		switch (ctype) {

			PST_MF_STRUCT_ELEMENT(Vec2f,vec2f)
			PST_MF_STRUCT_ELEMENT(Vec3f,vec3f)
			PST_MF_STRUCT_ELEMENT(Vec3d,vec3d)
			PST_MF_STRUCT_ELEMENT(Vec4d,vec4d)
			PST_MF_STRUCT_ELEMENT(Vec2d,vec2d)
			PST_MF_STRUCT_ELEMENT(Color,color)
			PST_MF_STRUCT_ELEMENT(ColorRGBA,colorrgba)
			PST_MF_STRUCT_ELEMENT(Int32,int32)
			PST_MF_STRUCT_ELEMENT(Float,float)
			PST_MF_STRUCT_ELEMENT(Double,double)
			PST_MF_STRUCT_ELEMENT(Bool,bool)
			PST_MF_STRUCT_ELEMENT(Time,time)
			PST_MF_STRUCT_ELEMENT(Rotation,rotation)
			PST_MF_STRUCT_ELEMENT(Matrix3f,matrix3f)
			PST_MF_STRUCT_ELEMENT(Matrix3d,matrix3d)
			PST_MF_STRUCT_ELEMENT(Matrix4f,matrix4f)
			PST_MF_STRUCT_ELEMENT(Matrix4d,matrix4d)
			PST_MF_STRUCT_ELEMENT(String,string)

			PST_SF_SIMPLE_ELEMENT(Float,float,sizeof(float))
			PST_SF_SIMPLE_ELEMENT(Time,time,sizeof(double))
			PST_SF_SIMPLE_ELEMENT(Double,double,sizeof(double))
			PST_SF_SIMPLE_ELEMENT(Int32,int32,sizeof(int))
			PST_SF_SIMPLE_ELEMENT(Bool,bool,sizeof(int))
			PST_SF_SIMPLE_ELEMENT(Node,node,sizeof(void *))
			PST_SF_SIMPLE_ELEMENT(Vec2f,vec2f,sizeof(struct SFVec2f))
			PST_SF_SIMPLE_ELEMENT(Vec2d,vec2d,sizeof(struct SFVec2d))
			PST_SF_SIMPLE_ELEMENT(Vec3f,vec3f,sizeof(struct SFColor))
			PST_SF_SIMPLE_ELEMENT(Vec3d,vec3d,sizeof(struct SFVec3d))
			PST_SF_SIMPLE_ELEMENT(Vec4d,vec4d,sizeof(struct SFVec4d))
			PST_SF_SIMPLE_ELEMENT(Rotation,rotation,sizeof(struct SFRotation))
			PST_SF_SIMPLE_ELEMENT(Color,color,sizeof(struct SFColor))
			PST_SF_SIMPLE_ELEMENT(ColorRGBA,colorrgba,sizeof(struct SFColorRGBA))
			PST_SF_SIMPLE_ELEMENT(Matrix3f,matrix3f,sizeof(struct SFMatrix3f))
			PST_SF_SIMPLE_ELEMENT(Matrix4f,matrix4f,sizeof(struct SFMatrix4f))
			PST_SF_SIMPLE_ELEMENT(Matrix3d,matrix3d,sizeof(struct SFMatrix3d))
			PST_SF_SIMPLE_ELEMENT(Matrix4d,matrix4d,sizeof(struct SFMatrix4d))
			PST_SF_SIMPLE_ELEMENT(Image,image,sizeof(struct Multi_Int32))

			case FIELDTYPE_SFString: {
					//struct Uni_String *mptr;
					memcpy(nst, &myVal.sfstring, sizeof(struct Uni_String*));
					//mptr = * (struct Uni_String **)nst;
					//if (!mptr) {
					//	ERROR_MSG("Parser_scanStringValueToMem: is nst (Uni_String) supposed to hold a NULL value ?");
					//} else {
					//	FREE_IF_NZ(mptr->strptr);
					//	mptr->strptr = myVal.sfstring->strptr;
					//	mptr->len = myVal.sfstring->len;
					//	mptr->touched = myVal.sfstring->touched;
					//}
				break; }

			default: {
				printf ("unhandled type, in EAIParse  %s\n",stringFieldtypeType(ctype));
				lexer_forceStringCleanup(parser->lexer);
				return;
			}
		}

	} else {
		if (strlen (value) > 50) {
			value[45] = '.';
			value[46] = '.';
			value[47] = '.';
			value[48] = '\0';
		}
		ConsoleMessage ("parser problem on parsing fieldType %s, string :%s:", stringFieldtypeType(ctype),value);
	}

	/* tell the parser that we have done with the input - it will FREE the data */
	lexer_forceStringCleanup(parser->lexer);

	FREE_IF_NZ(mfstringtmp);

	/* and, reset the XML flag */
	parser->parsingX3DfromXML = oldXMLflag;
}

void Parser_scanStringValueToMem_C0(struct VRMLParser *parser, union anyVrml* any, indexT ctype, char *value, int isXML) 
{
	//dug9 Apr 2013: same as Parser_scanStringValueToMemB except:
	// - you create or remember your parser* outside, so no static or gglobal in here, so can be called from libeai.
	// - puts it into *anyVrml instead of (node,offset)
	// - doesn't update parents for SFNode, MFNode fields (just zeros it) - that's done outside
	void *nst;                      /* used for pointer maths */
	union anyVrml myVal;
	char *mfstringtmp = NULL;
	int oldXMLflag;
	struct X3D_Node *np;
	#ifdef SETFIELDVERBOSE
	printf ("\nPST, for %s we have %s strlen %lu\n",stringFieldtypeType(ctype), value, strlen(value));
	#endif

	/* if this is the first time through, create a new parser, and tell it:
	      - that we are using X3D formatted field strings, NOT "VRML" ones;
	      - that the destination node is not important (the NULL, offset 0) */

	if (parser == NULL) {
		parser=newParser(NULL,NULL, 0, TRUE);
		//ConsoleMessage ("Parser_ScanStringValueToMem, new parser created");
	}

	lexer_forceStringCleanup(parser->lexer);

	/* October 20, 2009; XML parsing should not go through here; XML encoded X3D should not have a "value=" field, but
	   have the SFNode or MFNode as part of the syntax, eg <field ...> <Box/> </field> */

	if (isXML) {
		/* printf ("we have XML parsing for type %s, string :%s:\n",stringFieldtypeType(ctype),value); */
		if ((ctype==FIELDTYPE_SFNode) || (ctype==FIELDTYPE_MFNode)) {
			/* printf ("returning\n"); */
			lexer_forceStringCleanup(parser->lexer);
			return;
		}

	}

	/* there is a difference sometimes, in the XML format and VRML classic format. The XML
	   parser will use xml format, scripts and EAI will use the classic format */
	oldXMLflag = parser->parsingX3DfromXML;
	parser->parsingX3DfromXML = isXML;

	/* we NEED MFStrings to have quotes on; so if this is a MFString, ensure quotes are ok */
	if (ctype == FIELDTYPE_MFString) {
		#ifdef SETFIELDVERBOSE
		printf ("parsing type %s, string :%s:\n",stringFieldtypeType(ctype),value); 
		#endif

		/* go to the first non-space character, and see if this is required;
		   sometimes people will encode mfstrings as:
			url=' "images/earth.gif" "http://ww
		   note the space in the value */
		while ((*value == ' ') && (*value != '\0')) value ++;

		/* now, does the value string need quoting? */
		if ((*value != '"') && (*value != '\'') && (*value != '[')) {
			size_t len;
			 /* printf ("have to quote this string\n"); */
			len = strlen(value);
			mfstringtmp = MALLOC (char *, sizeof (char *) * len + 10);
			memcpy (&mfstringtmp[1],value,len);
			mfstringtmp[0] = '"';
			mfstringtmp[len+1] = '"';
			mfstringtmp[len+2] = '\0';
			/* printf ("so, mfstring is :%s:\n",mfstringtmp); */ 
			
		} else {
			mfstringtmp = STRDUP(value);
		}
		parser_fromString(parser,mfstringtmp);
		/* FREE_IF_NZ(mfstringtmp); */
	} else if (ctype == FIELDTYPE_SFNode) {
		/* Need to change index to proper node ptr */
		np = getEAINodeFromTable(atoi(value), -1);
	} else if (ctype == FIELDTYPE_SFString) {
		if(isXML){
			/* double quotes " are unique to x3d values and must be \" to pass javascript compiling */
			int nq = 0;
			char *mv, *pv, *v = value;
			while (*v && *v != '\0')
			{	
				if(*v == '"') nq++;
				v++;
			}
			mfstringtmp = (char *)MALLOC(void *, strlen(value)+nq+1);
			v = value;
			pv = NULL;
			mv = mfstringtmp;
			while(*v && *v != '\0')
			{
				if(*v == '"'){
					if(!(pv && *pv == '\\')){
						*mv = '\\';
						mv++;
					}
				}
				*mv = *v;
				mv++;
				pv = v;
				v++;
			}
			*mv = '\0';
		}else{
			mfstringtmp = STRDUP(value);
		}
		parser_fromString(parser,mfstringtmp);
	} else {
		mfstringtmp = STRDUP(value);
		parser_fromString(parser,mfstringtmp);
		/* FREE_IF_NZ(mfstringtmp); */
	}

	ASSERT(parser->lexer);
	FREE_IF_NZ(parser->lexer->curID);

	if (ctype == FIELDTYPE_SFNode) {
		//struct X3D_Node* oldvalue;
		//nst = offsetPointer_deref(void *,node,coffset);
		//memcpy (&oldvalue, any, sizeof(struct X3D_Node*));
		//if (oldvalue) {
		//	remove_parent(oldvalue, node);
		//}
		memcpy(any, (void*)&np, sizeof(struct X3D_Node*));
		any->sfnode->_parentVector = NULL;
		//add_parent(np, node, "sarah's add", 0);
	} else if (parseType(parser, ctype, &myVal)) {
		/* printf ("parsed successfully\n");  */
		//nst = offsetPointer_deref(void *,node,coffset);
		nst = any;

/*
MF_TYPE(MFNode, mfnode, Node)
*/
		switch (ctype) {

			PST_MF_STRUCT_ELEMENT(Vec2f,vec2f)
			PST_MF_STRUCT_ELEMENT(Vec3f,vec3f)
			PST_MF_STRUCT_ELEMENT(Vec3d,vec3d)
			PST_MF_STRUCT_ELEMENT(Vec4d,vec4d)
			PST_MF_STRUCT_ELEMENT(Vec2d,vec2d)
			PST_MF_STRUCT_ELEMENT(Color,color)
			PST_MF_STRUCT_ELEMENT(ColorRGBA,colorrgba)
			PST_MF_STRUCT_ELEMENT(Int32,int32)
			PST_MF_STRUCT_ELEMENT(Float,float)
			PST_MF_STRUCT_ELEMENT(Double,double)
			PST_MF_STRUCT_ELEMENT(Bool,bool)
			PST_MF_STRUCT_ELEMENT(Time,time)
			PST_MF_STRUCT_ELEMENT(Rotation,rotation)
			PST_MF_STRUCT_ELEMENT(Matrix3f,matrix3f)
			PST_MF_STRUCT_ELEMENT(Matrix3d,matrix3d)
			PST_MF_STRUCT_ELEMENT(Matrix4f,matrix4f)
			PST_MF_STRUCT_ELEMENT(Matrix4d,matrix4d)
			PST_MF_STRUCT_ELEMENT(String,string)

			PST_SF_SIMPLE_ELEMENT(Float,float,sizeof(float))
			PST_SF_SIMPLE_ELEMENT(Time,time,sizeof(double))
			PST_SF_SIMPLE_ELEMENT(Double,double,sizeof(double))
			PST_SF_SIMPLE_ELEMENT(Int32,int32,sizeof(int))
			PST_SF_SIMPLE_ELEMENT(Bool,bool,sizeof(int))
			PST_SF_SIMPLE_ELEMENT(Node,node,sizeof(void *))
			PST_SF_SIMPLE_ELEMENT(Vec2f,vec2f,sizeof(struct SFVec2f))
			PST_SF_SIMPLE_ELEMENT(Vec2d,vec2d,sizeof(struct SFVec2d))
			PST_SF_SIMPLE_ELEMENT(Vec3f,vec3f,sizeof(struct SFColor))
			PST_SF_SIMPLE_ELEMENT(Vec3d,vec3d,sizeof(struct SFVec3d))
			PST_SF_SIMPLE_ELEMENT(Vec4d,vec4d,sizeof(struct SFVec4d))
			PST_SF_SIMPLE_ELEMENT(Rotation,rotation,sizeof(struct SFRotation))
			PST_SF_SIMPLE_ELEMENT(Color,color,sizeof(struct SFColor))
			PST_SF_SIMPLE_ELEMENT(ColorRGBA,colorrgba,sizeof(struct SFColorRGBA))
			PST_SF_SIMPLE_ELEMENT(Matrix3f,matrix3f,sizeof(struct SFMatrix3f))
			PST_SF_SIMPLE_ELEMENT(Matrix4f,matrix4f,sizeof(struct SFMatrix4f))
			PST_SF_SIMPLE_ELEMENT(Matrix3d,matrix3d,sizeof(struct SFMatrix3d))
			PST_SF_SIMPLE_ELEMENT(Matrix4d,matrix4d,sizeof(struct SFMatrix4d))
			PST_SF_SIMPLE_ELEMENT(Image,image,sizeof(struct Multi_Int32))

			case FIELDTYPE_SFString: {
					//struct Uni_String *mptr;
					memcpy(nst, &myVal.sfstring, sizeof(struct Uni_String*));
					//mptr = * (struct Uni_String **)nst;
					//if (!mptr) {
					//	ERROR_MSG("Parser_scanStringValueToMem: is nst (Uni_String) supposed to hold a NULL value ?");
					//} else {
					//	FREE_IF_NZ(mptr->strptr);
					//	mptr->strptr = myVal.sfstring->strptr;
					//	mptr->len = myVal.sfstring->len;
					//	mptr->touched = myVal.sfstring->touched;
					//}
				break; }

			default: {
				printf ("unhandled type, in EAIParse  %s\n",stringFieldtypeType(ctype));
				lexer_forceStringCleanup(parser->lexer);
				return;
			}
		}

	} else {
		if (strlen (value) > 50) {
			value[45] = '.';
			value[46] = '.';
			value[47] = '.';
			value[48] = '\0';
		}
		ConsoleMessage ("parser problem on parsing fieldType %s, string :%s:", stringFieldtypeType(ctype),value);
	}

	/* tell the parser that we have done with the input - it will FREE the data */
	lexer_forceStringCleanup(parser->lexer);

	/* and, reset the XML flag */
	parser->parsingX3DfromXML = oldXMLflag;
}
void Parser_scanStringValueToMem_C(void *any0, int ctype0, char *value, int isXML)
//void Parser_scanStringValueToMem_C(union anyVrml* any, indexT ctype, char *value, int isXML) 
{
	struct VRMLParser *parser;
	union anyVrml* any;
	indexT ctype;
	any = (union anyVrml*)any0;
	ctype = (indexT)ctype0;
	parser=newParser(NULL,NULL, 0, TRUE);
	Parser_scanStringValueToMem_C0(parser, any, ctype, value, isXML);
	if (parser != NULL) {
		lexer_destroyData(parser->lexer);
		deleteParser(parser);
		parser = NULL;
	}
	return;
}
