/*
=INSERT_TEMPLATE_HERE=

$Id$

???

*/

#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../main/Snapshot.h"
#include "../scenegraph/Collision.h"
#include "../scenegraph/quaternion.h"
#include "../scenegraph/Viewer.h"
#include "../input/SensInterps.h"
#include "../input/EAIheaders.h"
#include "../input/EAIHelpers.h"
#include "../x3d_parser/Bindable.h"

#include "CScripts.h"
#include "jsUtils.h"
#include "jsNative.h"
#include "jsVRMLClasses.h"
#include "fieldSet.h"
#include "fieldGet.h"


/*******************************************************************

A group of routines to SET a field in memory - in the FreeWRL
scene graph. 

Different methods are used, depending on the format of the call.

*********************************************************************/

/* copy new scanned in data over to the memory area in the scene graph. */
static uintptr_t Multi_Struct_memptr (int type, void *memptr) {
	struct Multi_Vec3f *mp;
	uintptr_t retval;

	/* is this a straight copy, or do we have a struct to send to? */
	/* now, some internal reps use a structure defined as:
	   struct Multi_Vec3f { int n; struct SFColor  *p; };
	   so, we have to put the data in the p pointer, so as to
	   not overwrite the data. */

	retval = (uintptr_t) memptr;

	switch (type) {
		case FIELDTYPE_MFInt32:
		case FIELDTYPE_MFFloat:
		case FIELDTYPE_MFRotation:
		case FIELDTYPE_MFVec3f:
		case FIELDTYPE_MFColorRGBA:
		case FIELDTYPE_MFColor:
		case FIELDTYPE_MFString:
		case FIELDTYPE_MFVec2f:
			mp = (struct Multi_Vec3f*) memptr;
			/* printf ("Multi_Struct_memptr, have multi thing, have %d elements, pointer %u\n",mp->n, mp->p); */
			retval = (uintptr_t) (mp->p);

		default: {}
		}
	return retval;
}


/*
SF_TYPE(SFBool, sfbool, Bool)
MF_TYPE(MFBool, mfbool, Bool)
SF_TYPE(SFColor, sfcolor, Color)
MF_TYPE(MFColor, mfcolor, Color)
SF_TYPE(SFColorRGBA, sfcolorrgba, ColorRGBA)
MF_TYPE(MFColorRGBA, mfcolorrgba, ColorRGBA)
*/

/* how many rows are in this data type? SF nodes have 1, other nodes... */
int returnNumberOfRows(int datatype,union anyVrml *memptr) {
	switch (datatype) {

  #define SF_TYPE(fttype, type, ttype) \
   case FIELDTYPE_##fttype: return 1; break; 

  #define MF_TYPE(fttype, type, ttype) \
   case FIELDTYPE_##fttype: return memptr->type.n; break; 

#include "../vrml_parser/VrmlTypeList.h"

#undef SF_TYPE
#undef MF_TYPE

  default:
   parseError("Unsupported type in defaultValue!");



	}
	return 1;
}



/* set a field; used in JavaScript, and in the Parser VRML parser 

	fields are:
		ptr:		pointer to a node (eg, X3D_Box)
		field:		string field name (eg, "size")
		value:		string of value (eg "2 2 2");

This is used mainly in parsing */

void setField_fromJavascript (struct X3D_Node *node, char *field, char *value, int isXML) {
	int foffset;
	int coffset;
	int ctype;
	int ctmp;

	struct X3D_Group *group;

	#ifdef SETFIELDVERBOSE
	printf ("\nsetField_fromJavascript, node %d field %s value %s\n", node, field, value);
	#endif
	
	/* is this a valid field? */
	foffset = findRoutedFieldInFIELDNAMES(node,field,1);	
	if (foffset < 0) {
		ConsoleMessage ("field %s is not a valid field of a node %s",field,stringNodeType(node->_nodeType));
		printf ("field %s is not a valid field of a node %s\n",field,stringNodeType(node->_nodeType));
		return;
	}

	/* get offsets for this field in this nodeType */
	#ifdef SETFIELDVERBOSE
	printf ("getting nodeOffsets for type %s field %s value %s\n",stringNodeType(node->_nodeType),field,value); 
	#endif

	findFieldInOFFSETS(node->_nodeType, foffset, &coffset, &ctype, &ctmp);

	#ifdef SETFIELDVERBOSE
	printf ("so, offset is %d, type %d value %s\n",coffset, ctype, value);
	#endif

	if (coffset <= 0) {
		printf ("setField_fromJavascript, trouble finding field %s in node %s\n",field,stringNodeType(node->_nodeType));
		printf ("is this maybe a PROTO?? if so, it will be a Group node with FreeWRL__protoDef set to an index\n");
		if (node->_nodeType == NODE_Group) {
			group = (struct X3D_Group *)node;
			printf ("it IS a group...\n");
			if (group->FreeWRL__protoDef!= INT_ID_UNDEFINED) {
				printf ("and, this is a PROTO...have to go through PROTO defs to get to it\n");
			}
		}
	}

	Parser_scanStringValueToMem(node, coffset, ctype, value, isXML);
}


/* and incoming EAI event has come in, and the destination is an inputOnly field of a script.
   Make It So. This mimics the routing function "getField_ToJavascript" except that we do not
   have a routing entry for the from address and size and type, so we have to do this by hand.
*/

static unsigned int setField_FromEAI_ToScript(uintptr_t tonode, int toname,
		int datatype, void *data, unsigned rowcount) {
	unsigned datalen;

	#ifdef SETFIELDVERBOSE
	printf ("doing setField_FromEAI_ToScript, for script %u, nameIndex %u, type %s\n",tonode, toname, stringFieldtypeType(datatype));
	#endif

        switch (datatype) {
        case FIELDTYPE_SFBool:
        case FIELDTYPE_SFFloat:
        case FIELDTYPE_SFTime:
        case FIELDTYPE_SFInt32:
        case FIELDTYPE_SFString:

		/* this one expects datalen to be in bytes */
		datalen = returnElementLength(datatype) * returnElementRowSize(datatype);
		#ifdef SETFIELDVERBOSE
		printf ("SFSingle in setField_FromEAI_ToScript, setting script, dataLength is made up of %d x %d x %d for %s\n",
			returnElementLength(datatype), returnElementRowSize(datatype), rowcount, stringFieldtypeType(datatype));
		#endif

                set_one_ECMAtype (tonode, toname, datatype, data, datalen);
                break;
        case FIELDTYPE_SFColor:
        case FIELDTYPE_SFVec2f:
        case FIELDTYPE_SFVec3f:
        case FIELDTYPE_SFVec3d:
        case FIELDTYPE_SFRotation:
		/* this one expects datalen to be in bytes */
		datalen = returnElementLength(datatype) * returnElementRowSize(datatype);
		#ifdef SETFIELDVERBOSE
		printf ("SFColor-style in setField_FromEAI_ToScript, setting script, dataLength is made up of %d x %d x %d for %s\n",
			returnElementLength(datatype), returnElementRowSize(datatype), rowcount, stringFieldtypeType(datatype));
		#endif
		set_one_MultiElementType (tonode, toname, data, datalen);
                break;
        case FIELDTYPE_SFNode:
		#ifdef SETFIELDVERBOSE
		printf ("SFNode copy, tonode %u...\n",tonode);
		#endif

		printf ("SFNode copy, tonode %u...\n",tonode);
		datalen = returnElementLength(FIELDTYPE_SFNode);
		set_one_MultiElementType (tonode, toname, data, datalen);
		printf ("SFNode done copy, tonode %u...\n",tonode);
                break;



        case FIELDTYPE_MFColor:
        case FIELDTYPE_MFVec3f:
        case FIELDTYPE_MFVec3d:
        case FIELDTYPE_MFVec2f:
        case FIELDTYPE_MFFloat:
        case FIELDTYPE_MFTime:
        case FIELDTYPE_MFInt32:
        case FIELDTYPE_MFString:
        case FIELDTYPE_MFNode:
        case FIELDTYPE_MFRotation:
        case FIELDTYPE_SFImage:
		#ifdef SETFIELDVERBOSE
		printf ("going to call MF types in  set_one_MFElementType rowcount %d\n",rowcount);
		#endif
		set_one_MFElementType(tonode, toname, datatype, data, rowcount);
                break;
        default : {
                printf("WARNING: setField_FromEAI_ToScript,  type %s not handled yet\n",
			stringFieldtypeType(datatype));
                }
        }

	return TRUE;
}



/* an incoming EAI/CLASS event has come in, convert the ASCII characters
 * to an internal representation, and act upon it */

unsigned int setField_FromEAI (char *ptr) {
	unsigned char nt;
	int datatype;
	int nodeIndex, fieldIndex;
	uintptr_t nodeptr;
	uintptr_t offset;
	uintptr_t myoffset;
	unsigned int scripttype;
	char *eol;

	uintptr_t memptr = 0;
	uintptr_t myptr = 0;

	int valIndex;
	struct Multi_Color *tcol;
	int retint; 			/* used to get return value of sscanf */

	union anyVrml myAnyValue;

	#ifdef SETFIELDVERBOSE
	printf ("\nsetField_FromEAI, string :%s:\n",ptr);
	#endif
	
	/* we have an event, get the data properly scanned in from the ASCII string. */

	/* node type */
	while (*ptr==' ')ptr++; 	/* remove blank space at front of string */
	nt = *ptr; ptr++;		/* get the ASCII indication of node type */
	datatype = mapEAItypeToFieldType(nt);

	/* blank space */
	ptr++;

	/* nodeptr, offset */
	retint=sscanf (ptr, "%d %d %d",&nodeIndex, &fieldIndex, &scripttype);
	if (retint != 3) ConsoleMessage ("setField_FromEAI: error reading 3 numbers from the string :%s:\n",ptr);

	while ((*ptr) > ' ') ptr++; 	/* node ptr */
	while ((*ptr) == ' ') ptr++;	/* inter number space(s) */
	while ((*ptr) > ' ') ptr++;	/* node offset */
	while ((*ptr) == ' ') ptr++;	/* inter number space(s) */
	while ((*ptr) > ' ') ptr++;	/* script type */

	#ifdef SETFIELDVERBOSE
	{
		struct X3D_Node *np;
		int nt;
		/* get the actual node pointer from this index */
		np = getEAINodeFromTable(nodeIndex,fieldIndex);
		nt = getEAINodeTypeFromTable(nodeIndex);

		 printf ("EAI_SendEvent, type %s, nodeptr (index %d) %u offset %d script type %d ",
				 stringFieldtypeType(datatype),nodeIndex, np->_nodeType, fieldIndex, scripttype);
		printf ("np->_nodeType %s\n",stringNodeType(np->_nodeType));

		if (nt == EAI_NODETYPE_SCRIPT) printf ("setField_FromEAI - sending to a script node!\n");
		else if (nt == EAI_NODETYPE_PROTO) printf ("setField_FromEAI - sending to a script node!\n");
		else if(nt == EAI_NODETYPE_STANDARD) printf ("setField_FromEAI - sending to a standard node!\n");
		else printf ("setField_FromEAI - unknown type!\n");
		}
	#endif


	/* We have either a event to a memory location, or to a script. */
	/* the field scripttype tells us whether this is true or not.   */

	if (scripttype == EAI_NODETYPE_SCRIPT) {
		/* a local temporary area for us */
		memptr = (uintptr_t) &myAnyValue;
	} else {
		memptr = (uintptr_t) getEAIMemoryPointer (nodeIndex,fieldIndex);
	}

	offset = getEAIActualOffset(nodeIndex, fieldIndex);
	myoffset = offset;
	nodeptr = (uintptr_t) getEAINodeFromTable(nodeIndex,fieldIndex);
	myptr = nodeptr;

	/* now, we are at start of data. */
	/* lets go to the first non-blank character in the string */
	while (*ptr == ' ') ptr++;

	#ifdef SETFIELDVERBOSE 
	printf ("setField_FromEAI EAI_SendEvent, event string now is :%s:\n",ptr);
	#endif

	/* is this a MF node, that has floats or ints, and the set1Value method is called? 	*/
	/* check out the java external/field/MF*java files for the string "ONEVAL "		*/
	if (strncmp("ONEVAL ",ptr, strlen("ONEVAL ")) == 0) {
		ptr += strlen ("ONEVAL ");

		/* find out which element the user wants to set - that should be the next number */
		while (*ptr==' ')ptr++;
		retint=sscanf (ptr,"%d",&valIndex);
		if (retint != 1) ConsoleMessage ("setField_FromEAI: error reading 1 numbers from the string :%s:\n",ptr);
		while (*ptr>' ')ptr++; /* past the number */
		while (*ptr==' ')ptr++;

		/* lets do some bounds checking here. */
		tcol = (struct Multi_Color *) memptr;
		if (tcol->n <= valIndex) {
			printf ("Error, setting 1Value of %d, max in scenegraph is %d\n",valIndex,tcol->n);
			return FALSE;
		}


		/* if this is a struct Multi* node type, move the actual memory pointer to the data */
		memptr = Multi_Struct_memptr(datatype, (void *) memptr);

		/* and index into that array; we have the index, and sizes to worry about 	*/

		memptr += valIndex * returnElementLength(datatype) *  returnElementRowSize(datatype);

		/* and change the nodetype to reflect this change */
		datatype = convertToSFType(datatype);

		/* For ONEVAL need to pass memptr, not nodeptr */
		myptr = memptr;
		myoffset = 0;
	}

	/* lets replace the end of the string with a NULL, for parsing purposes */
	eol = strchr (ptr,'\n'); if (eol != NULL) *eol = '\0';

	/* at this point, we have:
		memptr = pointer to memory location to start scanning; 
		offset = actual offset in node, or 0 if ONEVAL invoked;
		nodeptr = actual memory pointer of X3D_Node* */

	/* first, parse the value into the local variable */
	Parser_scanStringValueToMem(myptr,myoffset,datatype,ptr,FALSE);

	if (scripttype == EAI_NODETYPE_SCRIPT) {
		struct Shader_Script * sp;
		int rowCount;

		/* we send along the script number, not the node pointer */
		sp = (struct Shader_Script *) (X3D_SCRIPT(nodeptr)->__scriptObj);

		mark_script (sp->num);

		/* now, send the number of rows along; SFs return 1, MFS return rows */
		rowCount = returnNumberOfRows(datatype,memptr);

		/* inch the type along, to the data pointer */
		memptr = Multi_Struct_memptr(datatype, (void *) memptr);

		setField_FromEAI_ToScript(sp->num,offset,datatype,memptr,rowCount);
	} else {
		/* if this is a geometry, make it re-render.
		   Some nodes (PROTO interface params w/o IS's)
		   will have an offset of zero, and are thus not
		   "real" nodes, only memory locations
		*/

		update_node ((void *)nodeptr);

		/* if anything uses this for routing, tell it that it has changed */
		MARK_EVENT (X3D_NODE(nodeptr),offset);
	}

	/* replace the end of the line with a newline */
	if (eol != NULL) *eol = '\n';
	return TRUE;

}

void setField_javascriptEventOut(struct X3D_Node *tn,unsigned int tptr,  int fieldType, unsigned len, int extraData, uintptr_t cx) {
        int ival;
        double tval;
        float fl[4];
	JSContext *scriptContext;
	char *memptr;
        JSString *strval; /* strings */
	char *strp;


	/* set up a pointer to where to put this stuff */
	memptr = (char *)tn;
	memptr += tptr;

	/* not all files know what a JSContext is, so we just pass it around as a uintptr_t type */
	scriptContext = (JSContext *) cx;


	#ifdef SETFIELDVERBOSE
	strval = JS_ValueToString(scriptContext, JSglobal_return_val);
       	strp = JS_GetStringBytes(strval);
	printf ("start of setField_javascriptEventOut, to %d:%d = %d, fieldtype %d string %s\n",tn, tptr, memptr, fieldType, strp);
	#endif


#define GETJSVAL_TYPE_A(thistype,field) \
		case FIELDTYPE_##thistype: { \
			/* printf ("doing TYPEA memcpy to %u, from %u, len %d\n",(void *)memptr, (void *) &(((thistype##Native *)JSSFpointer)->field),len); */ \
			memcpy ((void *)memptr, (void *) &(((thistype##Native *)JSSFpointer)->field),len); \
			break; \
		} 

#define GETJSVAL_TYPE_MF_A(MFtype,SFtype) \
		case FIELDTYPE_##MFtype: {getJSMultiNumType (scriptContext, (struct Multi_Vec3f *)memptr,FIELDTYPE_##SFtype); break;}

	switch (fieldType) {
                        GETJSVAL_TYPE_A(SFRotation,v)
                        /* GETJSVAL_TYPE_A(SFNode,handle) */
                        GETJSVAL_TYPE_A(SFVec2f,v)
                        GETJSVAL_TYPE_A(SFVec3f,v)
                        GETJSVAL_TYPE_A(SFColor,v)
                        GETJSVAL_TYPE_A(SFColorRGBA,v)

                        GETJSVAL_TYPE_MF_A(MFRotation,SFRotation)
                        GETJSVAL_TYPE_MF_A(MFVec2f,SFVec2f)
                        GETJSVAL_TYPE_MF_A(MFVec3f,SFVec3f)
                        GETJSVAL_TYPE_MF_A(MFColor,SFColor)
                        GETJSVAL_TYPE_MF_A(MFColorRGBA,SFColorRGBA)


		case FIELDTYPE_SFInt32: 
		case FIELDTYPE_SFBool:	{	/* SFBool */
			if (!JS_ValueToInt32(scriptContext, JSglobal_return_val,&ival)) {
				printf ("error\n");
				ival=0;
			}
			memcpy ((void *)memptr, (void *)&ival,len);
			break;
		}

		case FIELDTYPE_SFTime: {
			if (!JS_ValueToNumber(scriptContext, JSglobal_return_val,&tval)) tval=0.0;
			memcpy ((void *)memptr, (void *)&tval,len);
			break;
		}

		case FIELDTYPE_SFFloat: {
			if (!JS_ValueToNumber(scriptContext, JSglobal_return_val,&tval)) tval=0.0;
			/* convert double precision to single, for X3D */
			fl[0] = (float) tval;
			memcpy ((void *)memptr, (void *)fl,len);
			break;
		}

		case FIELDTYPE_SFImage: {
			/* the string should be saved as an SFImage */
			strval = JS_ValueToString(scriptContext, JSglobal_return_val);
	        	strp = JS_GetStringBytes(strval);

			Parser_scanStringValueToMem(tn, tptr, FIELDTYPE_SFImage, strp, FALSE);
			break;
		}

		case FIELDTYPE_SFString: {
			struct Uni_String *ms;
			uintptr_t *newptr;

			strval = JS_ValueToString(scriptContext, JSglobal_return_val);
	        	strp = JS_GetStringBytes(strval);

			/* copy the string over, delete the old one, if need be */
			/* printf ("fieldSet SFString, tn %d tptr %d offset from struct %d\n",
				tn, tptr, offsetof (struct X3D_TextureCoordinateGenerator, mode)); */
			newptr = (uintptr_t *)memptr;
			ms = (struct Uni_String*) *newptr;
			verify_Uni_String (ms,strp);
			break;
		}


			/* a series of Floats... */
		case FIELDTYPE_MFFloat: {getJSMultiNumType (scriptContext, (struct Multi_Vec3f *)memptr,FIELDTYPE_SFFloat); break;}
		case FIELDTYPE_MFInt32: {getJSMultiNumType (scriptContext, (struct Multi_Vec3f *)memptr,FIELDTYPE_SFInt32); break;}
		case FIELDTYPE_MFTime: {getJSMultiNumType (scriptContext, (struct Multi_Vec3f *)memptr,FIELDTYPE_SFTime); break;}
		case FIELDTYPE_MFNode: {
				strval = JS_ValueToString(scriptContext, JSglobal_return_val);
	        		strp = JS_GetStringBytes(strval);
				getMFNodetype (strp,(struct Multi_Node *)memptr,X3D_NODE(tn),extraData); break;
		}
		case FIELDTYPE_MFString: {
			getMFStringtype (scriptContext, (jsval *)JSglobal_return_val,(struct Multi_String *)memptr);
			break;
		}

		case FIELDTYPE_SFNode: {

				/* printf ("doing TYPEA memcpy to %u, from %u, len %d\n",(void *)memptr, (void *) &(((SFNodeNative *)JSSFpointer)->handle),returnElementLength(FIELDTYPE_SFNode));*/
			memcpy ((void *)memptr, (void *) &(((SFNodeNative *)JSSFpointer)->handle),returnElementLength(FIELDTYPE_SFNode)); 

				break;
		}


		default: {	printf("WARNING: unhandled from type %s\n", stringFieldtypeType(fieldType));
		}
	}

	#ifdef SETFIELDVERBOSE
	printf ("done setField_javascriptEventOut\n");
	if (fieldType == FIELDTYPE_MFInt32) {
		printf ("setField_javascriptEventOut, checking the pointers...\n");
		printf ("node type is %s\n",stringNodeType(X3D_NODE(tn)->_nodeType));
	}
	
	#endif
}

/* find the ASCII string name of this field of this node */
char *findFIELDNAMESfromNodeOffset(struct X3D_Node *node, int offset) {
	int* np;
	if (node == 0) return "unknown";

	np = (int *) NODE_OFFSETS[node->_nodeType];  /* it is a const int* type */
	np++;  /* go to the offset field */

	while ((*np != -1) && (*np != offset)) np +=5;
	
	if (*np == -1) return "fieldNotFound";
	
	/* go back to the field name */
	np --;
	return ((char *) FIELDNAMES[*np]);
}

/* go through the generated table FIELDTYPES, and find the int of this string, returning it, or -1 on error 
	or if it is an "internal" field */
int findFieldInARR(const char* field, const char** arr, size_t cnt)
{
	int x;
	int mystrlen;
	
	if (field == NULL) return -1;

	#ifdef SETFIELDVERBOSE
	if (field[0] == '_') {
		printf ("findFieldInFIELDNAMES - internal field %s\n",field);
	}
	#endif
	
	mystrlen = strlen(field);
	/* printf ("findFieldInFIELDNAMES, string :%s: is %d long\n",field,mystrlen);  */
	for (x=0; x!=cnt; ++x) {
		if (strlen(arr[x]) == mystrlen) {
			if (strcmp(field, arr[x])==0) return x;
		} 
	}
	return -1;

}
#define DEF_FINDFIELD(arr) \
 int findFieldIn##arr(const char* field) \
 { \
  return findFieldInARR(field, arr, arr##_COUNT); \
 }
DEF_FINDFIELD(FIELDNAMES)
DEF_FINDFIELD(FIELD)
DEF_FINDFIELD(EXPOSED_FIELD)
DEF_FINDFIELD(EVENT_IN)
DEF_FINDFIELD(EVENT_OUT)
DEF_FINDFIELD(KEYWORDS)
DEF_FINDFIELD(PROTOKEYWORDS)
DEF_FINDFIELD(NODES)
DEF_FINDFIELD(PROFILES)
DEF_FINDFIELD(COMPONENTS)
DEF_FINDFIELD(FIELDTYPES)
DEF_FINDFIELD(X3DSPECIAL)
DEF_FINDFIELD(GEOSPATIAL)

/* lets see if this node has a routed field  fromTo  = 0 = from node, anything else = to node */
/* returns the FIELDNAMES index. */
/* for user-fields, the additional check is skipped */
int findRoutedFieldInARR (struct X3D_Node * node, const char *field, int fromTo,
  const char** arr, size_t cnt, BOOL user) {
	int retval;
	char mychar[200];
	int a,b,c;
	
	retval = -1;

#define FIELDCHECK(fld) \
	if (retval >= 0) { \
	  if (user) return retval; \
	  {int fieldNamesIndex = findIndexInFIELDNAMES(retval, arr, cnt); \
	  if (fieldNamesIndex >= 0) { \
	    findFieldInOFFSETS (node->_nodeType, fieldNamesIndex,\
	      &a, &b, &c); \
	    /* did this return any of the ints as != -1? */ \
	    /* printf ("     findRoutedField for field %s, nodetype %s is %d\n",  fld,stringNodeType(node->_nodeType),a); */ \
	    if (a >= 0) return retval;  /* found it! */ \
	  }} \
	} 


	/* step try the field as is. */
	retval = findFieldInARR(field, arr, cnt);
	/* printf ("findRoutedField, field %s retval %d\n",field,retval); */
	FIELDCHECK (field)

	/* try ADDING the "set_" or "_changed" */
	strncpy (mychar, field, 100);
	if (fromTo != 0) {
		if (strlen(field) > strlen("set_"))
			retval=findFieldInARR(mychar+strlen("set_"), arr, cnt);
	} else {
		if (strlen(field) > strlen("_changed")) {
			mychar[strlen(field) - strlen("_changed")] = '\0';
			retval = findFieldInARR(mychar, arr, cnt);
		}
	}
	/* printf ("findRoutedField, mychar %s retval %d\n",mychar,retval); */
	FIELDCHECK (mychar)

	/* try ADDING the "set_" or "_changed"  some nodes have fields ending in "_changed" - maybe the
  	   user forgot about that? (eg, ProximitySensor) */
	if (fromTo != 0) {
		strcpy (mychar,"set_");
		strncat (mychar, field,100);
		retval=findFieldInARR(mychar, arr, cnt);
	} else {
		strncpy (mychar, field, 100);
		strcat (mychar,"_changed");
		retval = findFieldInARR(mychar, arr, cnt);
	}
	/* printf ("findRoutedField, mychar %s retval %d\n",mychar,retval); */
	FIELDCHECK (mychar)


	return retval;
}
#define DEF_FINDROUTEDFIELD(arr) \
 int findRoutedFieldIn##arr(struct X3D_Node* node, const char* field, int fromTo) \
 { \
  return findRoutedFieldInARR(node, field, fromTo, arr, arr##_COUNT, FALSE); \
 }
DEF_FINDROUTEDFIELD(FIELDNAMES)
DEF_FINDROUTEDFIELD(EXPOSED_FIELD)
DEF_FINDROUTEDFIELD(EVENT_IN)
DEF_FINDROUTEDFIELD(EVENT_OUT)


/* go through the OFFSETS for this node, looking for field, and return offset, type, and kind */
void findFieldInOFFSETS(int nodeType, const int field, int *coffset, int *ctype, int *ckind) {
	int *x;
	int X3DLevel;
	int mask = 0;

	x = (int *) NODE_OFFSETS[nodeType];

	#ifdef SETFIELDVERBOSE
	printf ("findFieldInOFFSETS, nodeType %s\n",stringNodeType(nodeType));
	printf ("findFieldInOffsets, comparing %d to %d\n",*x, field); 
	#endif

	while ((*x != field) && (*x != -1)) {
		x += 5;
	}
	if (*x == field) {
		x++; *coffset = *x; x++; *ctype = *x; x++; *ckind = *x; x++; X3DLevel = *x;

		#ifdef SETFIELDVERBOSE
		printf ("found field, coffset %d ctype %d ckind %d X3DLevel %x\n",*coffset, *ctype, *ckind, X3DLevel); 
		#endif

		/* do we care if, maybe, this field is not correct for requested version of FreeWRL? */
		if (global_strictParsing) {
			if (inputFileVersion[0] == 2) { /* VRML 2.0 */
				if ((X3DLevel & SPEC_VRML) == SPEC_VRML) {
					return; /* field ok */
				}
			} else if (inputFileVersion[0] == 3) { /* X3D V3.x */
				switch (inputFileVersion[1]) {
					case 0: mask = SPEC_X3D30; break;
					case 1: mask = SPEC_X3D31; break;
					case 2: mask = SPEC_X3D32; break;
					case 3: mask = SPEC_X3D33; break;
					case 4: mask = SPEC_X3D34; break;
					default: {printf ("unknown X3D level %d\n",inputFileVersion[1]);
							mask = SPEC_X3D33;
					}
				}
				if ((X3DLevel & mask) == mask) {
					return; /* field ok */
				}
			} else {
				printf ("unknown input file version %d for strictParsing! help!\n",inputFileVersion[0]);
			}
			ConsoleMessage ("strictParsing, Node %s field %s is not valid for X3D version %d.%d",
				stringNodeType(nodeType),stringFieldType(field),inputFileVersion[0],inputFileVersion[1]);
		}

		return;
	}
	if (*x == -1) {
		#ifdef  SETFIELDVERBOSE
		printf ("did not find field %d in OFFSETS\n",field);
		#endif

		*coffset = -1; *ctype = -1, *ckind = -1;
		return;
	}
}


/****************************************************************/
/* a Jscript is returning a Multi-number type; copy this from 	*/
/* the Jscript return string to the data structure within the	*/
/* freewrl C side of things.					*/
/*								*/
/* note - this cheats in that the code assumes that it is 	*/
/* a series of Multi_Vec3f's while in reality the structure	*/
/* of the multi structures is the same - so we "fudge" things	*/
/* to make this multi-purpose.					*/
/****************************************************************/
void getJSMultiNumType (JSContext *cx, struct Multi_Vec3f *tn, int eletype) {
	float *fl;
	int *il;
	double *dl;
	uintptr_t *nl;

	double dtmp;
	jsval mainElement;
	int len;
	int i;
	char *strp;
	int elesize;
	SFVec2fNative *sfvec2f;
	SFVec3fNative *sfvec3f;
	SFRotationNative *sfrotation;
	struct Uni_String * *ms;
	jsval myJSVal;


	/* get size of each element, used for MALLOCing memory  - eg, this will
	   be sizeof(float) * 3 for a SFColor */
	elesize = returnElementLength(eletype) * returnElementRowSize(eletype);

	/* rough check of return value */
	/* where did this come from? Was it from a script execution, or from an assignment from within a script?? */
	#ifdef SETFIELDVERBOSE
	printf ("getJSMultiNumType, JSCreate_global_return_val %u, JSglobal_return_val %u\n",JSCreate_global_return_val, JSglobal_return_val);
	#endif

	if (JSCreate_global_return_val!= INT_TO_JSVAL(0)) {
		myJSVal = JSCreate_global_return_val;
		JSCreate_global_return_val = INT_TO_JSVAL(0);

		#ifdef SETFIELDVERBOSE
		printf ("getJSMultiNumType: using JSCreate_global_return_val\n");
		#endif
	} else {
		#ifdef SETFIELDVERBOSE
		printf ("getJSMultiNumType: using JSglobal_return_val\n");
		#endif

		myJSVal = JSglobal_return_val;
	}

	if (!JSVAL_IS_OBJECT(myJSVal)) {
		printf ("getJSMultiNumType - did not get an object\n");
		return;
	}

	#ifdef SETFIELDVERBOSE
	printf ("getJSMultiNumType, tn %d dest has  %s size %d\n",tn,stringFieldtypeType(eletype), elesize); 

	printf("getJSMulitNumType, node type of myJSVal is :");
	printJSNodeType (cx,myJSVal);
	#endif

	if (!JS_GetProperty(cx, (JSObject *)myJSVal, "length", &mainElement)) {
		printf ("JS_GetProperty failed for \"length\" in getJSMultiNumType\n");
		return;
	}
	len = JSVAL_TO_INT(mainElement);
	#ifdef SETFIELDVERBOSE
	printf ("getmuiltie length of grv is %d old len is %d\n",len,tn->n);
	#endif

	/* do we have to realloc memory? */
	if (len != tn->n) {

		tn->n = 0;
		/* yep... */
			/* printf ("old pointer %d\n",tn->p); */
		FREE_IF_NZ (tn->p);
		tn->p = (struct SFColor *)MALLOC ((unsigned)(elesize*len));

		#ifdef SETFIELDVERBOSE 
		printf ("MALLOCing memory for elesize %d len %d new pointer now is %d\n",elesize,len,tn->p);
		#endif

		/* if this is an MFString, we should set each element to a null string */
		if (eletype == FIELDTYPE_SFString) {
			#ifdef SETFIELDVERBOSE
			printf ("getJSMultiNumType, this is a MFString, so making tiny strings for now\n");
			#endif

			ms = (struct Uni_String * *) tn->p;
			for (i=0; i<len; i++) {
				*ms = newASCIIString ("(getJSMultiNumType null)");
				ms ++;
			}
		}
	}

	/* set these three up, but we only use one of them */
	fl = (float *) tn->p;
	il = (int *) tn->p;
	dl = (double *) tn->p;
	nl = (uintptr_t *) tn->p;
	ms = (struct Uni_String * *) tn->p;

	/* go through each element of the main array. */
	for (i = 0; i < len; i++) {
		if (!JS_GetElement(cx, (JSObject *)myJSVal, i, &mainElement)) {
			printf ("WARNING: JS_GetElement failed for %d in getJSMultiNumType\n",i);
			switch (eletype) {
			case FIELDTYPE_SFNode:
				*nl = 0; nl++; break;
			case FIELDTYPE_SFInt32: 
				*il=0; il++; break;
			case FIELDTYPE_SFTime: 
				*dl=0.0; dl++; break;
			case FIELDTYPE_SFFloat: 
				*fl = (float) 0.0;  fl++; break; 
			case FIELDTYPE_SFVec2f: 
				*fl = (float) 0.0;  fl++; *fl = (float) 0.0;  fl++; break; 
			case FIELDTYPE_SFVec3f:
	                case FIELDTYPE_SFColor: 
				*fl = (float) 0.0;  fl++; *fl = (float) 0.0;  fl++; *fl = (float) 0.0;  fl++; break; 
			case FIELDTYPE_SFRotation: 
				*fl = (float) 0.0;  fl++; *fl = (float) 0.0;  fl++; *fl = (float) 0.0;  fl++; *fl = (float) 0.0;  fl++; break; 
			case FIELDTYPE_SFString: 
	                        verify_Uni_String (*ms,"(empty value)"); ms++; break;
	
			default : {printf ("getJSMultiNumType unhandled eletype: %d\n",
					eletype);
				   return;
				}
			}
		} else {
			#ifdef SETFIELDVERBOSE
			JSString *_tmpStr;

	                _tmpStr = JS_ValueToString(cx, mainElement);
			strp = JS_GetStringBytes(_tmpStr);
	                printf ("sub element %d is \"%s\" \n",i,strp);  

			if (JSVAL_IS_OBJECT(mainElement)) printf ("sub element %d is an OBJECT\n",i);
			if (JSVAL_IS_PRIMITIVE(mainElement)) printf ("sub element %d is an PRIMITIVE\n",i);
			#endif
	
			/* code is pretty much same as SF* values in setField_javascriptEventOut */
			switch (eletype) {
			case FIELDTYPE_SFNode: {
				int xxx;
				if (RUNNINGONAMD64) printf ("FIELDTYPE_SFNode *may* not work on 64 bit computers...\n");
				if (!JS_ValueToInt32(cx, mainElement ,&xxx)) {
					printf ("error\n");
					*nl=0;
				}
				/* *nl = (uintptr_t * *)xxx; */
				*nl = (uintptr_t)xxx;
				nl++;
				break;
			}
			case FIELDTYPE_SFInt32: {
				if (!JS_ValueToInt32(cx, mainElement ,il)) {
					printf ("error\n");
					*il=0;
				}
				il++;
				break;
			}
			case FIELDTYPE_SFTime: {
				if (!JS_ValueToNumber(cx, mainElement ,dl)) *dl=0.0;
				dl++;
				break;
			}
			case FIELDTYPE_SFFloat: {
				if (!JS_ValueToNumber(cx, mainElement, &dtmp)) dtmp=0.0;
				/* convert double precision to single, for X3D */
				*fl = (float) dtmp;
				fl++;
				break;
			}
			case FIELDTYPE_SFVec2f: {
				if (JSVAL_IS_OBJECT(mainElement)) {
	                        	if ((sfvec2f = (SFVec2fNative *)JS_GetPrivate(cx, (JSObject *)mainElement)) == NULL) {
	                                	printf( "JS_GetPrivate failed for obj in setField_javascriptEventOut.\n");
	                                	return;
	                        	}
	                        	memcpy ((void *)fl, (void *)&(sfvec2f->v),2*sizeof(float));
					fl += 2;
				} else {
					/* we are working in a value that kind of exists, but is undefined */
					*fl = (float) 0.0; fl++; *fl = (float) 0.0; fl++;	
				}
	                        break;
			}
			case FIELDTYPE_SFVec3f:
	                case FIELDTYPE_SFColor: {       /* SFColor */
				if (JSVAL_IS_OBJECT(mainElement)) {
	                        	if ((sfvec3f = (SFVec3fNative *)JS_GetPrivate(cx, (JSObject *)mainElement)) == NULL) {
	                        	        printf( "JS_GetPrivate failed for obj in setField_javascriptEventOut.\n");
	                        	        return;
	                        	}
	                        	memcpy ((void *)fl, (void *)&(sfvec3f->v),3*sizeof(float));
					fl += 3;
				} else {
					/* we are working in a value that kind of exists, but is undefined */
					*fl = (float) 0.0; fl++;	
					*fl = (float) 0.0; fl++;	
					*fl = (float) 0.0; fl++;	
				}
	                        break;
			}
			case FIELDTYPE_SFRotation: {
				if (JSVAL_IS_OBJECT(mainElement)) {
	                        	if ((sfrotation = (SFRotationNative *)JS_GetPrivate(cx, (JSObject *)mainElement)) == NULL) {
	                        	        printf( "JS_GetPrivate failed for obj in setField_javascriptEventOut.\n");
	                        	        return;
	                        	}
	                        	memcpy ((void *)fl, (void *)&(sfrotation->v),4*sizeof(float));
					fl += 4;
				} else {
					/* we are working in a value that kind of exists, but is undefined */
					*fl = (float) 0.0; fl++;	*fl = (float) 0.0; fl++;	
					*fl = (float) 0.0; fl++;	*fl = (float) 0.0; fl++;	
				}
	                        break;
			}
	
			case FIELDTYPE_SFString: {
				JSString *strval;
	
	                        strval = JS_ValueToString(cx, mainElement);
	                        strp = JS_GetStringBytes(strval);
	
				#ifdef SETFIELDVERBOSE
				printf ("getJSMultiNumType, got string %s\n",strp); 
				#endif
	
	                        /* copy the string over, delete the old one, if need be */
	                        verify_Uni_String (*ms,strp);
				ms++;
	                        break;
			}
	
			default : {printf ("getJSMultiNumType unhandled eletype: %d\n",
					eletype);
				   return;
				}
			}
		}

	}
	#ifdef SETFIELDVERBOSE
	printf ("getJSMultiNumType, setting old length %d to length %d\n",tn->n, len);
	#endif

	tn->n = len;
}

/****************************************************************/
/* a script is returning a MFString type; add this to the C	*/
/* children field						*/
/****************************************************************/
void getMFStringtype (JSContext *cx, jsval *from, struct Multi_String *to) {
	int oldlen, newlen;
	jsval _v;
	JSObject *obj;
	int i;
	char *valStr, *OldvalStr;
	struct Uni_String **svptr;
	struct Uni_String **newp, **oldp;
	int count;

	JSString *strval; /* strings */

	oldlen = to->n;
	svptr = to->p;
	newlen=0;

	if (!JS_ValueToObject(cx, (jsval) from, &obj))
		printf ("JS_ValueToObject failed in getMFStringtype\n");

	if (!JS_GetProperty(cx, obj, "length", &_v)) {
		printf ("JS_GetProperty failed for \"length\" in getMFStringtype.\n");
        }

	newlen = JSVAL_TO_INT(_v);

	/* printf ("getMFStringType, newlen %d oldlen %d\n",newlen,oldlen); */


	/*  if we have to expand size of SV... */
	if (newlen > oldlen) {
		oldp = to->p; /* same as svptr, assigned above */
		to->n = newlen;
		to->p = (struct Uni_String**)MALLOC(newlen * sizeof(to->p));
		newp = to->p;

		/* copy old values over */
		for (count = 0; count <oldlen; count ++) {
			/*printf ("copying over element %d\n",count); */
			*newp = *oldp;
			newp++;
			oldp++;
		}

		/* zero new entries */
		for (count = oldlen; count < newlen; count ++) {
			/* make the new SV */
			*newp = (struct Uni_String *)MALLOC (sizeof (struct Uni_String));
			

			/* now, make it point to a blank string */
			*newp = newASCIIString("");
			newp ++;
		}
		FREE_IF_NZ (svptr);
		svptr = to->p;
	} else {
		/* possibly truncate this, but leave the memory alone. */
		to->n = newlen;
	}

	/* printf ("verifying structure here\n");
	for (i=0; i<(to->n); i++) {
		printf ("indx %d flag %x string :%s: len1 %d len2 %d\n",i,
				(svptr[i])->sv_flags,
	}
	printf ("done\n");
	*/


	for (i = 0; i < newlen; i++) {
		/* get the old string pointer */
		OldvalStr = svptr[i]->strptr;
		/* printf ("old string at %d is %s len %d\n",i,OldvalStr,strlen(OldvalStr)); */

		/* get the new string pointer */
		if (!JS_GetElement(cx, obj, i, &_v)) {
			fprintf(stderr,
				"JS_GetElement failed for %d in getMFStringtype\n",i);
			return;
		}
		strval = JS_ValueToString(cx, _v);
		valStr = JS_GetStringBytes(strval);

		/* printf ("new string %d is %s\n",i,valStr); */

		/*  if the strings are different... */
		if (strcmp(valStr,OldvalStr) != 0) {
			/* MALLOC a new string, of correct len for terminator */
			svptr[i] =  newASCIIString(valStr);
		}
	}
	/*
	printf ("\n new structure: %d %d\n",svptr,newlen);
	for (i=0; i<newlen; i++) {
		printf ("indx %d string :%s: len1 %d len2 %d\n",i,
				mypv->xpv_pv, mypv->xpv_cur,mypv->xpv_len);
	}
	*/
	
}


/************************************************************************/
/* a script is returning a MFNode type; add or remove this to the C	*/
/* children field							*/
/* note params - tn is the address of the actual field, parent is parent*/
/* structure								*/
/************************************************************************/

void getMFNodetype (char *strp, struct Multi_Node *tn, struct X3D_Node *parent, int ar) {
	uintptr_t newptr;
	int newlen;
	char *cptr;
	void *newmal;
	uintptr_t *tmpptr;

	/* is this 64 bit compatible? - unsure right now. */
	if (sizeof(void *) != sizeof (unsigned int))
		printf ("getMFNodetype - unverified that this works on 64 bit machines\n");

	#ifdef SETFIELDVERBOSE 
		printf ("getMFNodetype, %s ar %d\n",strp,ar);
		printf ("getMFNodetype, parent %d has %d nodes currently\n",tn,tn->n);
	#endif

	newlen=0;

	/* this string will be in the form "[ CNode addr CNode addr....]" */
	/* count the numbers to add  or remove */
	if (*strp == '[') { strp++; }
	while (*strp == ' ') strp++; /* skip spaces */
	cptr = strp;

	while (sscanf (cptr,"%ld",&newptr) == 1) {
		newlen++;
		/* skip past this number */
		while (isdigit(*cptr) || (*cptr == ',') || (*cptr == '-')) cptr++;
		while (*cptr == ' ') cptr++; /* skip spaces */
	}
	cptr = strp; /* reset this pointer to the first number */

	/* printf ("newlen HERE is %d\n",newlen); */

	/* create the list to send to the AddRemoveChildren function */
	newmal = MALLOC (newlen*sizeof(void *));
	tmpptr = (uintptr_t*)newmal;

	/* scan through the string again, and get the node numbers. */
	while (sscanf (cptr,"%ld", (uintptr_t *)tmpptr) == 1) {
		/* printf ("just scanned in %d, which is a %s\n",*tmpptr, 
			stringNodeType((X3D_NODE(*tmpptr))->_nodeType)); */

		/* skip past this number */
		while (isdigit(*cptr) || (*cptr == ',') || (*cptr == '-')) cptr++;
		while (*cptr == ' ') cptr++; /* skip spaces */
		tmpptr = (uintptr_t*) (tmpptr + sizeof (void *));
	}

	/* now, perform the add/remove */
	AddRemoveChildren (parent, tn, newmal, newlen, ar,__FILE__,__LINE__);
}


/* Map the given index into arr to an index into FIELDNAMES or -1, if the
 * string in question isn't there. */
int findIndexInFIELDNAMES(int index, const char** arr, size_t arrCnt) {
  int i;

  /* If this is already FIELDNAMES, return index. */
  if(arr==FIELDNAMES)
    return index;

  /* Look for the string */
  for(i=0; i!=FIELDNAMES_COUNT; ++i) {
    if(!strcmp(FIELDNAMES[i], arr[index]))
      return i;
  }

  /* Not found */
  return -1;
}
