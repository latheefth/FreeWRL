/*


Javascript C language binding.

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
#include <system_threads.h>
#include <system_js.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../vrml_parser/CRoutes.h"
#include "../main/headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../main/Snapshot.h"
#include "../scenegraph/Collision.h"
#include "../scenegraph/quaternion.h"
#include "../scenegraph/Viewer.h"
#include "../input/EAIHeaders.h"
#include "../input/EAIHelpers.h"	/* resolving implicit declarations */
#include "../input/SensInterps.h"
#include "../x3d_parser/Bindable.h"

#include "JScript.h"
#include "CScripts.h"
#include "fieldSet.h"
#include "fieldGet.h"

#ifdef HAVE_JAVASCRIPT
/********************************************************************

getField_ToJavascript.

this sends events to scripts that have eventIns defined.

********************************************************************/

void getField_ToJavascript (int num, int fromoffset) {
	int ignored;
	struct CRjsnameStruct *JSparamnames = getJSparamnames();

	UNUSED(ignored); // compiler warning mitigation

	#ifdef SETFIELDVERBOSE
	printf ("getField_ToJavascript, from offset %d name %s type %d num=%d\n",
	        fromoffset,JSparamnames[fromoffset].name,JSparamnames[fromoffset].type,num);
        #endif
	/* set the parameter */
	/* see comments in gatherScriptEventOuts to see exact formats */

	switch (JSparamnames[fromoffset].type) {
	case FIELDTYPE_SFBool:
	case FIELDTYPE_SFFloat:
	case FIELDTYPE_SFTime:
	case FIELDTYPE_SFDouble:
	case FIELDTYPE_SFInt32:
	case FIELDTYPE_SFString:
		setScriptECMAtype(num);
		break;
	case FIELDTYPE_SFColor:
	case FIELDTYPE_SFNode:
	case FIELDTYPE_SFVec2f:
	case FIELDTYPE_SFVec3f:
	case FIELDTYPE_SFVec3d:
	case FIELDTYPE_SFRotation:
		setScriptMultiElementtype(num);
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
		ignored = setMFElementtype(num);
		break;
	default : {
		printf("WARNING: sendScriptEventIn type %s not handled yet\n",
			FIELDTYPES[JSparamnames[fromoffset].type]);
		}
	}
}

//void getField_ToJavascript_B(int num, int fromoffset) {
void getField_ToJavascript_B(int shader_num, int fieldOffset, int type, union anyVrml *any, int len) {

	#ifdef SETFIELDVERBOSE
	struct CRjsnameStruct *JSparamnames = getJSparamnames();

	printf ("getField_ToJavascript, from offset %d type %d num=%d\n",
		fieldOffset,JSparamnames[fieldOffset].type,shader_num);
	#endif

	/* set the parameter */
	/* see comments in gatherScriptEventOuts to see exact formats */

	switch (type) {
	case FIELDTYPE_SFBool:
	case FIELDTYPE_SFFloat:
	case FIELDTYPE_SFTime:
	case FIELDTYPE_SFDouble:
	case FIELDTYPE_SFInt32:
	case FIELDTYPE_SFString:
		//setScriptECMAtype(num);
		set_one_ECMAtype(shader_num, fieldOffset, type, any, len);
		break;
	case FIELDTYPE_SFColor:
	case FIELDTYPE_SFNode:
	case FIELDTYPE_SFVec2f:
	case FIELDTYPE_SFVec3f:
	case FIELDTYPE_SFVec3d:
	case FIELDTYPE_SFRotation:
		set_one_MultiElementType(shader_num, fieldOffset, any, len);
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

		set_one_MFElementType(shader_num, fieldOffset, type, (void *)any,len);

		break;
	default : {
		printf("WARNING: sendScriptEventIn type %s not handled yet\n",
			FIELDTYPES[type]);
		}
	}
}




/* setMFElementtype called by getField_ToJavascript for
         case FIELDTYPE_MFColor:
        case FIELDTYPE_MFVec3f:
        case FIELDTYPE_MFVec2f:
        case FIELDTYPE_MFFloat:
        case FIELDTYPE_MFTime:
        case FIELDTYPE_MFInt32:
        case FIELDTYPE_MFString:
        case FIELDTYPE_MFNode:
        case FIELDTYPE_MFRotation:
        case FIELDTYPE_SFImage:
*/


int setMFElementtype (int num) {
	void * fn;
	int fptr;
	int len;
	int to_counter;
	CRnodeStruct *to_ptr = NULL;
	char *pptr;
	struct Multi_Node *mfp;
	struct CRStruct *CRoutes = getCRoutes();
	struct CRjsnameStruct *JSparamnames = getJSparamnames();


	#ifdef SETFIELDVERBOSE
		printf("------------BEGIN setMFElementtype ---------------\n");
	#endif


	fn = (void *)CRoutes[num].routeFromNode;
	fptr = CRoutes[num].fnptr;

	/* we can do arithmetic on character pointers; so we have to cast void *
	   to char * here */
	pptr = offsetPointer_deref (char *, fn, fptr);

	len = CRoutes[num].len;

	/* is this from a MFElementType? positive lengths in routing table  == easy memcpy types */
	if (len <= 0) {
		mfp = (struct Multi_Node *) pptr;

		/* check Multimemcpy for C to C routing for this type */
		/* get the number of elements */
		len = mfp->n;
		pptr = (char *) mfp->p; /* pptr is a char * just for math stuff */
		#ifdef SETFIELDVERBOSE
		printf ("setMFElementtype, len now %d, from %d\n",len,fn);
		#endif
	} else {
		/* SFImages will have a length of greater than zero */
		/* printf ("setMFElementtype, length is greater than 0 (%d), how can this be?\n",len); */
	}

	/* go through all the nodes that this script sends to for this entry in the CRoutes table */
	for (to_counter = 0; to_counter < CRoutes[num].tonode_count; to_counter++) {
                struct Shader_Script *myObj;

		to_ptr = &(CRoutes[num].tonodes[to_counter]);
                myObj = X3D_SCRIPT(to_ptr->routeToNode)->__scriptObj;

		#ifdef SETFIELDVERBOSE
			printf ("got a script event! index %d type %d\n",
					num, CRoutes[num].direction_flag);
/*
			printf ("\tfrom %#x from ptr %#x\n\tto %#x toptr %#x\n",fn,fptr,tn,to_ptr->foffset);
			printf ("\tfrom %d from ptr %d\n\tto %d toptr %d\n",fn,fptr,tn,to_ptr->foffset);
*/
			printf ("\tdata length %d\n",len);
			printf ("and, sending it to %s as type %d\n",JSparamnames[to_ptr->foffset].name,
					JSparamnames[to_ptr->foffset].type);
		#endif

		set_one_MFElementType(myObj->num, to_ptr->foffset, JSparamnames[to_ptr->foffset].type, (void *)pptr,len);
	}


	#ifdef SETFIELDVERBOSE
		printf("------------END setMFElementtype ---------------\n");
	#endif
	return FALSE; /* return value never checked; #defines expect a return value */
}


/* setScriptMultiElementtype called by getField_ToJavascript for
        case FIELDTYPE_SFColor:
        case FIELDTYPE_SFNode:
        case FIELDTYPE_SFVec2f:
        case FIELDTYPE_SFVec3f:
        case FIELDTYPE_SFRotation:
*/

void setScriptMultiElementtype (int num)
{
	int tptr, fptr;
	int len;
	int to_counter;
	void *fn;

	CRnodeStruct *to_ptr = NULL;
	struct CRStruct *CRoutes = getCRoutes();


	fn = (void *)CRoutes[num].routeFromNode;
	fptr = CRoutes[num].fnptr;
	if (CRoutes[num].len == ROUTING_SFNODE) len = returnElementLength(FIELDTYPE_SFNode);
	else if (CRoutes[num].len < 0) {
		ConsoleMessage ("setScriptMultiElementtype - len of %d unhandled\n",CRoutes[num].len);
		return;
	} else {
		len = CRoutes[num].len;
	}

	for (to_counter = 0; to_counter < CRoutes[num].tonode_count; to_counter++) {
                struct Shader_Script *myObj;

                to_ptr = &(CRoutes[num].tonodes[to_counter]);
                myObj = X3D_SCRIPT(to_ptr->routeToNode)->__scriptObj;

		/* the to_node should be a script number; it will be a small integer */
		tptr = to_ptr->foffset;

		#ifdef SETFIELDVERBOSE
			printf ("got a script event! index %d type %d\n",
					num, CRoutes[num].direction_flag);
			printf ("\tfrom %#x from ptr %#x\n\tto %#x toptr %#x\n",fn,fptr,myObj->num,tptr);
			printf ("\tdata length %d\n",len);
			printf ("setScriptMultiElementtype here script number  %d tptr %d len %d\n",myObj->num, tptr,len);
		#endif

		fn = offsetPointer_deref(void*,fn,fptr); /*fn += fptr;*/

		set_one_MultiElementType (myObj->num, tptr, fn, len);
	}
}


#endif /* HAVE_JAVASCRIPT */

/* convert a number in memory to a printable type. Used to send back EVents, or replies to
   the SAI/EAI client program. */

void EAI_Convert_mem_to_ASCII (int id, char *reptype, int type, char *memptr, char *buf) {

	char utilBuf[EAIREADSIZE];
	int errcount;
	memset(utilBuf,'\0',sizeof(utilBuf));

	errcount = UtilEAI_Convert_mem_to_ASCII (type,memptr, utilBuf);
	if (0 == errcount) {
		sprintf (buf,"%s\n%f\n%d\n%s",reptype,TickTime(),id, utilBuf);
	} else {
		sprintf (buf,"%s\n%f\n%d\n%s",reptype,TickTime(),id, "indeterminate....");
	}
}

/* Utility routine to convert a value in memory to a printable type. */

int UtilEAI_Convert_mem_to_ASCII (int type, char *memptr, char *buf) { /* Returns errcount */

	double dval;
	float fl[4];
	double dl[4];
	float *fp;
	int *ip;
	int ival;
	struct X3D_Node *uval;
	int row;			/* MF* counter */
	struct Multi_String *MSptr;	/* MFString pointer */
	struct Multi_Node *MNptr;	/* MFNode pointer */
	struct Multi_Color *MCptr;	/* MFColor pointer */
	char *ptr;			/* used for building up return string */
	struct Uni_String *svptr;
	char *retSFString;

	int numPerRow;			/* 1, 2, 3 or 4 floats per row of this MF? */
	int i, errcount;

	/* used because of endian problems... */
	int *intptr;
	int eaiverbose;
	eaiverbose = gglobal()->EAI_C_CommonFunctions.eaiverbose;
	intptr = (int *) memptr;

/* printf("%s,%d UtilEAI_Convert_mem_to_ASCII (type=%d , memptr=%p intptr=%p ....)\n",__FILE__,__LINE__,type,memptr,intptr); */

	errcount=0;
	switch (type) {
		case FIELDTYPE_SFBool: 	{
			if (eaiverbose) {
			printf ("UtilEAI_Convert_mem_to_ASCII: EAI_SFBOOL - value %d; TRUE %d false %d\n",*intptr,TRUE,FALSE);
			}

			if (*intptr == 1) sprintf (buf,"TRUE");
			else sprintf (buf,"FALSE");
			break;
		}

		case FIELDTYPE_SFDouble:
		case FIELDTYPE_SFTime:	{
			if (eaiverbose) {
			printf ("UtilEAI_Convert_mem_to_ASCII: EAI_SFTIME\n");
			}
			memcpy(&dval,memptr,sizeof(double));
			sprintf (buf, "%lf",dval);
			break;
		}

		case FIELDTYPE_SFInt32:	{
			if (eaiverbose) {
			printf ("UtilEAI_Convert_mem_to_ASCII: EAI_SFINT32\n");
			}
			memcpy(&ival,memptr,sizeof(int));
			sprintf (buf, "%d",ival);
			break;
		}

		case FIELDTYPE_SFNode:	{
			if (eaiverbose) {
			printf ("UtilEAI_Convert_mem_to_ASCII: EAI_SFNODE\n");
			}
			memcpy((void *)&uval,(void *)memptr,sizeof(void *));
			sprintf (buf, "%u",registerEAINodeForAccess(X3D_NODE(uval)));
			break;
		}

		case FIELDTYPE_SFFloat:	{
			if (eaiverbose) {
			printf ("UtilEAI_Convert_mem_to_ASCII: EAI_SFFLOAT\n");
			}

			memcpy(fl,memptr,sizeof(float));
			sprintf (buf, "%f",fl[0]);
			break;
		}

		case FIELDTYPE_SFVec3f:
		case FIELDTYPE_SFColor:	{
			if (eaiverbose) {
			printf ("UtilEAI_Convert_mem_to_ASCII: EAI_SFCOLOR or EAI_SFVEC3F\n");
			}
			memcpy(fl,memptr,sizeof(float)*3);
			sprintf (buf, "%f %f %f",fl[0],fl[1],fl[2]);
			break;
		}

		case FIELDTYPE_SFVec3d:	{
			if (eaiverbose) {
			printf ("UtilEAI_Convert_mem_to_ASCII: EAI_SFVEC3D\n");
			}
			memcpy(dl,memptr,sizeof(double)*3);
			sprintf (buf, "%lf %lf %lf",dl[0],dl[1],dl[2]);
			break;
		}

		case FIELDTYPE_SFVec2f:	{
			if (eaiverbose) {
			printf ("UtilEAI_Convert_mem_to_ASCII: EAI_SFVEC2F\n");
			}
			memcpy(fl,memptr,sizeof(float)*2);
			sprintf (buf, "%f %f",fl[0],fl[1]);
			break;
		}

		case FIELDTYPE_SFColorRGBA:
		case FIELDTYPE_SFRotation:	{
			if (eaiverbose) {
			printf ("UtilEAI_Convert_mem_to_ASCII: EAI_SFROTATION\n");
			}

			memcpy(fl,memptr,sizeof(float)*4);
			sprintf (buf, "%f %f %f %f",fl[0],fl[1],fl[2],fl[3]);
			break;
		}

		case FIELDTYPE_SFImage:
		case FIELDTYPE_SFString:	{
			uintptr_t *xx;

			if (eaiverbose) {
			printf ("UtilEAI_Convert_mem_to_ASCII: EAI_SFSTRING\n");
			}

			/* get the pointer to the string, do this in a couple of steps... */
			svptr = (struct Uni_String *)memptr;
			xx= (uintptr_t *) memptr;
			svptr = (struct Uni_String *) *xx;

			retSFString = (char *)svptr->strptr;
			sprintf (buf, "\"%s\"",retSFString);
			break;
		}

		case FIELDTYPE_MFString:	{
			if (eaiverbose) {
			printf ("UtilEAI_Convert_mem_to_ASCII: EAI_MFSTRING\n");
			}

			/* make the Multi_String pointer */
			MSptr = (struct Multi_String *) memptr;

			/* printf ("UtilEAI_Convert_mem_to_ASCII: EAI_MFString, there are %d strings\n",(*MSptr).n);*/
			ptr = buf + strlen(buf);

			for (row=0; row<(*MSptr).n; row++) {
        	        	/* printf ("UtilEAI_Convert_mem_to_ASCII: String %d is %s\n",row,(*MSptr).p[row]->strptr); */
				if (strlen ((*MSptr).p[row]->strptr) == 0) {
					sprintf (ptr, "\"\" "); /* encode junk for Java side.*/
				} else {
					sprintf (ptr, "\"%s\" ",(*MSptr).p[row]->strptr);
				}
				/* printf ("UtilEAI_Convert_mem_to_ASCII: buf now is %s\n",buf); */
				ptr = buf + strlen (buf);
			}

			break;
		}

		case FIELDTYPE_MFNode: 	{
			MNptr = (struct Multi_Node *) memptr;

			if (eaiverbose) {
			printf ("UtilEAI_Convert_mem_to_ASCII: EAI_MFNode, there are %d nodes at %p\n",(*MNptr).n,memptr);
			}

			ptr = buf + strlen(buf);

			for (row=0; row<(*MNptr).n; row++) {
				sprintf (ptr, "%d ",registerEAINodeForAccess(X3D_NODE((*MNptr).p[row])));
				ptr = buf + strlen (buf);
			}
			break;
		}

		case FIELDTYPE_MFInt32: {
			MCptr = (struct Multi_Color *) memptr;
			if (eaiverbose) {
				printf ("UtilEAI_Convert_mem_to_ASCII: EAI_MFColor, there are %d nodes at %p\n",(*MCptr).n,memptr);
			}

			sprintf (buf, "%d \n",(*MCptr).n);
			ptr = buf + strlen(buf);

			ip = (int *) (*MCptr).p;
			for (row=0; row<(*MCptr).n; row++) {
				sprintf (ptr, "%d \n",*ip);
				ip++;
				/* printf ("UtilEAI_Convert_mem_to_ASCII: line %d is ",row,ptr);  */
				ptr = buf + strlen (buf);
			}

			break;
		}

		case FIELDTYPE_MFFloat:
		case FIELDTYPE_MFVec2f:
		case FIELDTYPE_MFVec3f:
		case FIELDTYPE_MFRotation:
		case FIELDTYPE_MFColorRGBA:
		case FIELDTYPE_MFColor: {
			numPerRow=3;
			if (type==FIELDTYPE_MFFloat) {numPerRow=1;}
			else if (type==FIELDTYPE_MFVec2f) {numPerRow=2;}
			else if (type==FIELDTYPE_MFRotation) {numPerRow=4;}
			else if (type==FIELDTYPE_MFColorRGBA) {numPerRow=4;}

			MCptr = (struct Multi_Color *) memptr;
			if (eaiverbose) {
				printf ("UtilEAI_Convert_mem_to_ASCII: EAI_MFColor, there are %d nodes at %p\n",(*MCptr).n,memptr);
			}

			sprintf (buf, "%d \n",(*MCptr).n);
			ptr = buf + strlen(buf);


			fp = (float *) (*MCptr).p;
			for (row=0; row<(*MCptr).n; row++) {
				for (i=0; i<numPerRow; i++) {
					fl[i] = *fp; fp++;
				}
				switch (numPerRow) {
					case 1:
						sprintf (ptr, "%f \n",fl[0]); break;
					case 2:
						sprintf (ptr, "%f %f \n",fl[0],fl[1]); break;
					case 3:
						sprintf (ptr, "%f %f %f \n",fl[0],fl[1],fl[2]); break;
					case 4:
						sprintf (ptr, "%f %f %f %f \n",fl[0],fl[1],fl[2],fl[3]); break;
				}
				/* printf ("UtilEAI_Convert_mem_to_ASCII: line %d is ",row,ptr); */
				ptr = buf + strlen (buf);
			}

			break;
		}
		default: {
			errcount++;
			printf ("UtilEAI_Convert_mem_to_ASCII: EAI, type %d (%s) not handled yet\n",type,stringFieldtypeType (type));
		}


	}
	return errcount ;
}
