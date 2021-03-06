
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


// for compiler warning mitigation, we require the following few lines.
#define UNUSED(v) ((void) v)


/* function protos */
#ifndef WIN32
#include "config.h"
#include "system.h"
#endif
#include "EAI_C.h"


/* get a node pointer */
X3DNode *X3D_getNode (char *name) {
	char *ptr;
	int adr;
	X3DNode *retval;

	retval = malloc (sizeof(X3DNode));
	retval->X3D_SFNode.type = FIELDTYPE_SFNode;
	/* retval->X3D_SFNode.SFNodeType = '\0'; */

	retval->X3D_SFNode.adr = 0;

	/* get the node address. save the address. */
	ptr = _X3D_make1StringCommand(GETNODE,name);

	if (sscanf (ptr,"%d", &adr) != 1) {
		printf ("error getting %s\n",name);
	} else {
		#ifdef VERBOSE
		printf ("X3D_getNode, ptr %s\n",ptr);
		printf ("adr %d\n",adr);
		#endif

		if (adr == 0) {
			printf ("node %s does not exist\n",name);
			return 0;
		}

		
		retval->X3D_SFNode.adr = adr;
	}
	REMOVE_EOT
	return retval;
}

//writes all the parents of a given node into an allocated array and passes the address of it to the outParentArray argument. Caller should free the array.
//returns the number of parents, "0" if none found, "-1" if an error occurred
int X3D_getParentNodes(X3DNode* child, X3DNode** outParentArray)
{
	char *ptr;
	int adr;
	char arg[10];
	X3DNode* tmpBuf;
	X3DNode* tmpReallocBuf;
	char* parentToken;
	char* next_token;
	int parentAdr;
	int parentCount;

	parentToken=NULL;
	next_token=NULL;

	if((child->X3D_SFNode.type != FIELDTYPE_SFNode) && (child->X3D_MFNode.type != FIELDTYPE_MFNode)) {
		printf ("X3D_getParentNodes, expected a node, got a %s\n",FIELDTYPES[child->X3D_SFNode.type]);		
		return -1;
	}

	if (child->X3D_SFNode.type == FIELDTYPE_SFNode) {
		adr = child->X3D_SFNode.adr;
	} else {
		if (child->X3D_MFNode.n != 1) {
			printf ("warning - will only get event for first child = have %d nodes\n",child->X3D_MFNode.n);
		}
		/* get the first address in the list */
		adr = child->X3D_MFNode.p[5].adr; 
	}

	snprintf(arg, 10, "%d", adr);
	
	ptr = _X3D_make1StringCommand(GETNODEPARENTS,arg);

	REMOVE_EOT;

	//start tokenizer
	parentToken = STRTOK_S(ptr," ",&next_token);

	sscanf(parentToken,"%d",&parentAdr);

	//parent search did not found parents or encountered an error?
	if(parentAdr <= 0)
	{
		*outParentArray = NULL;
		return parentAdr;
	}		

	tmpBuf = (X3DNode*) calloc(10,sizeof(X3DNode));
	parentCount=0;

	while(parentToken)
	{
		//parentCount is a multiply of 10?
		if(parentCount && parentCount%10==0)
		{
			//reallocate the buffer with 10 more slots
			tmpReallocBuf =(X3DNode*) realloc(tmpBuf,(sizeof(X3DNode)*(parentCount+10)));

			//buffer was not reallocated, we exit
			if(!tmpReallocBuf)
			{
				*outParentArray = tmpBuf;
				return -1;
			}

			tmpBuf = tmpReallocBuf;
		}
				
		tmpBuf[parentCount].X3D_SFNode.type = FIELDTYPE_SFNode;

		sscanf(parentToken,"%d",&parentAdr);

		//parent search did not found parents or encountered an error?
		if(parentAdr <= 0)
		{
			*outParentArray = tmpBuf;
			return parentAdr;
		}

		tmpBuf[parentCount].X3D_SFNode.adr = parentAdr;

		parentCount++;		

		//find next parent address
		parentToken = STRTOK_S(NULL," ",&next_token);
	}

	*outParentArray = tmpBuf;

	return parentCount;
}

//same as above, but also get the child node using the DEF name passed and copy the address of it into the outChildNode argument.
int X3D_getParentNodesbyName(char *childName, X3DNode** outChildNode, X3DNode** outParentArray)
{
	*outChildNode = X3D_getNode(childName);

	if(*outChildNode)
		return X3D_getParentNodes(*outChildNode, outParentArray);
	else
		return -1;
}


/* get an eventIn */

X3DEventIn *_X3D_getEvent(X3DNode *node, char *name, int into) {
	char *ptr;
	int origPtr;
	int offset;
	int nds;
	X3DEventIn *retval;
	int adr;

        retval = malloc (sizeof (struct _intX3DEventIn));
	retval->offset = 0;
	retval->nodeptr = 0;
	retval->datasize = 0;
	retval->field = NULL;
	retval->scripttype = 0;

	if((node->X3D_SFNode.type != FIELDTYPE_SFNode) && (node->X3D_MFNode.type != FIELDTYPE_MFNode)) {
		printf ("X3D_getEvent, expected a node, got a %s\n",FIELDTYPES[node->X3D_SFNode.type]);
		free(retval);
		return 0;
	}

	if (node->X3D_SFNode.type == FIELDTYPE_SFNode) {
		adr = node->X3D_SFNode.adr;
	} else {
		if (node->X3D_MFNode.n != 1) {
			int i;
			printf ("warning - will only get event for first node = have %d nodes\n",node->X3D_MFNode.n);
			for (i=0; i<node->X3D_MFNode.n; i++) {
				printf ("... node %d nodetype %d\n",i,node->X3D_MFNode.p[i].adr);
			}
		}
		/* get the first address in the list */
		adr = node->X3D_MFNode.p[0].adr; 
	}

	/* printf ("getting eventin for address %d, field %s\n",adr, name); */
	//if (into) ptr = _X3D_Browser_SendEventType(adr, name, "eventIn");
	//else ptr = _X3D_Browser_SendEventType(adr, name, "eventOut");

	switch(into)
	{
	case 0:	//specifing a "eventOut" access method
		ptr = _X3D_Browser_SendEventType(adr, name, "eventOut");
		break;
	case 1:	//specifing a "eventIn" access method
		ptr = _X3D_Browser_SendEventType(adr, name, "eventIn");
		break;
	case -1: //we do not want to specify an access method, we only ask for the field definition
		ptr = _X3D_Browser_SendEventType(adr, name, "");
		break;
	default:
		//TODO: FOR NOW WE FORCE TO AN "EVENTIN" CASE. IT WOULD BE BETTER TO THROW AN ERROR
		ptr = _X3D_Browser_SendEventType(adr, name, "eventIn");
		break;
	}

	/* ptr should point to , for example: 161412616 116 0 q 0 eventIn
	   where we have 
			newnodepoiner,
			 field offset, 
			node datasize,
			field type (as an ascii char - eg 'q' for X3D_MFNODE
			ScriptType,
			and whether this is an eventIn or not.
	printf ("getEvent: ptr is %s\n",ptr);
	*/

	/* eg: ptr is 161412616 116 0 q 0 eventIn */
	if (sscanf(ptr,"%d %d %d", &origPtr, &offset, &nds) != 3) {
		printf ("error in getEventIn\n");
		free(retval);
		return 0;
	}

	/* do the pointers match? they should.. */
	if (origPtr !=  adr) {
		printf ("error in getEventIn, origptr and node ptr do not match\n");
		free(retval);
		return 0;
	}

	/* save the first 3 fields */
	retval->nodeptr = origPtr;
	retval->offset = offset;
	retval->datasize = nds;

	/* go to the data type. will be a character, eg 'q' */
	SKIP_CONTROLCHARS	/* should now be at the copy of the memory pointer */
	SKIP_IF_GT_SPACE	/* should now be  at end of memory pointer */
	SKIP_CONTROLCHARS	/* should now be at field offset */
	SKIP_IF_GT_SPACE	/* should now be at end of field offset */
	SKIP_CONTROLCHARS	/* should now be at the nds parameter */
	SKIP_IF_GT_SPACE	/* should now be at ehd of the nds parameter */
	SKIP_CONTROLCHARS	/* should now be at start of the type */


	retval->datatype = mapEAItypeToFieldType(*ptr);
	SKIP_IF_GT_SPACE
	SKIP_CONTROLCHARS

	/* should be sitting at field type */
	if (sscanf (ptr,"%d",&(retval->scripttype)) != 1) {
		printf ("No Event %s found!\n", name);
		free(retval);
		return 0;
	}

	SKIP_IF_GT_SPACE
	SKIP_CONTROLCHARS

/*
	if (into) {
		if (strncmp(ptr,"eventIn",strlen("eventIn")) != 0) 
			if (strncmp(ptr,"exposedField",strlen("exposedField")) != 0) 
	} else {
		if (strncmp(ptr,"eventOut",strlen("eventOut")) != 0) 
			if (strncmp(ptr,"exposedField",strlen("exposedField")) != 0) 
				printf ("WARNING: expected for field %seventOut or exposedField, got %s\n",name,ptr);
	}
*/


	retval->field = strdup(name);
	
	REMOVE_EOT
    return retval;
}


X3DEventIn *X3D_getEventIn(X3DNode *node, char *name) {
	X3DEventIn *retval;
	retval = _X3D_getEvent(node, name,TRUE);
	return retval;
}

//#define SYSTEMROOTNODE "_Sarah_this_is_the_FreeWRL_System_Root_Node"
/* get the scene root node */
X3DNode* X3D_getExecutionContext()
{
	return X3D_getNode (SYSTEMROOTNODE);
}

void X3D_rootNodeHandling(X3DNode *scene, X3DNode *node, int action)
{
	if(action == 1){
		X3DEventIn *addChildren = X3D_getEventIn(scene,"addChildren");
		X3D_setValue(addChildren, node);
	}else if(action ==2){
		X3DEventIn *removeChildren = X3D_getEventIn(scene,"removeChildren");
		X3D_setValue(removeChildren, node);
	}
}

X3DEventOut *X3D_getEventOut(X3DNode *node, char *name) {
	X3DEventOut *retval;
	retval = _X3D_getEvent(node, name,FALSE);
	return retval;

}

X3DFieldDef* X3D_getFieldDef(X3DNode *node, char *name){
	X3DEventOut *retval;
	retval = _X3D_getEvent(node, name,-1);
	return retval;
}

//gets the list of fields of the standard node
char* X3D_getFieldDefs(int nodeAdr)
{
	char myline[200];
	char *ptr;
	char *res;
	sprintf (myline,"%d",nodeAdr);
	
	// INCOMPLETE: we get a string containing the fields but we actually do nothing with it.	
	ptr = _X3D_make1StringCommand(GETFIELDDEFS,myline);

	res = strdup(ptr);

	return res;
}

void X3D_addRoute (X3DEventOut *from, X3DEventIn *to) {
	char myline[200];
	char *ptr;
	UNUSED(ptr); // for compiler warning mitigation

	sprintf (myline,"%d %s %d %s",from->nodeptr,from->field,to->nodeptr,to->field);
	ptr = _X3D_make1StringCommand(ADDROUTE,myline);
}

void X3D_deleteRoute (X3DEventOut *from, X3DEventIn *to) {
	char myline[200];
	char *ptr;
	UNUSED(ptr); // for compiler warning mitigation

	sprintf (myline,"%d %s %d %s",from->nodeptr,from->field,to->nodeptr,to->field);
	ptr = _X3D_make1StringCommand(DELETEROUTE,myline);
}

X3DNode* X3D_getValue (X3DEventOut *src) {
	char myline[4128];
	char tstring[1024];
	int num;
	int retvals;
	int adr;
	char* ptr;
	float a, b, c, d;
	double db;
	int i, j;
	char ttok[4126];
	int val;
	float fval;
	int mytmp;
	char* temp;
	int len;

	X3DNode* value;
	value = malloc (sizeof(X3DNode));
	bzero(value, sizeof(X3DNode));
	value->type = src->datatype;
	sprintf(myline, "%d %d %c %d", src->nodeptr, src->offset, mapFieldTypeToEAItype(src->datatype), src->datasize);
	ptr = _X3D_make1StringCommand(GETVALUE, myline);

	switch (src->datatype) {
                case FIELDTYPE_SFVec3f:
			sscanf(ptr, "%f %f %f", &a, &b, &c);

			value->X3D_SFVec3f.c[0] = a;
			value->X3D_SFVec3f.c[1] = b;
			value->X3D_SFVec3f.c[2] = c;

			break;
                case FIELDTYPE_SFColor:

			sscanf(ptr, "%f %f %f", &a, &b, &c);
	
	        	value->X3D_SFColor.c[0] = a;
	        	value->X3D_SFColor.c[1] = b;
	        	value->X3D_SFColor.c[2] = c;

                	break;

		case FIELDTYPE_SFVec2f:

			sscanf(ptr, "%f %f", &a, &b);

			value->X3D_SFVec2f.c[0] = a;
			value->X3D_SFVec2f.c[1] = b;
			
			break;

		case FIELDTYPE_SFRotation:

			sscanf(ptr, "%f %f %f %f", &a, &b, &c, &d);
	
	        	value->X3D_SFRotation.r[0] = a;
	        	value->X3D_SFRotation.r[1] = b;
	        	value->X3D_SFRotation.r[2] = c;
	        	value->X3D_SFRotation.r[3] = d;
	
			break;

		case FIELDTYPE_SFColorRGBA:

			sscanf(ptr, "%f %f %f %f", &a, &b, &c, &d);
	
	        	value->X3D_SFColorRGBA.r[0] = a;
	        	value->X3D_SFColorRGBA.r[1] = b;
	        	value->X3D_SFColorRGBA.r[2] = c;
	        	value->X3D_SFColorRGBA.r[3] = d;
			
			break;

		case FIELDTYPE_SFBool:
		
			sscanf(ptr, "%s", tstring);

			if (!strcmp(tstring, "TRUE"))
				value->X3D_SFBool.value = 1;
			else
				value->X3D_SFBool.value = 0;

			break;

		case FIELDTYPE_SFFloat:

			sscanf(ptr, "%f", &a);

			value->X3D_SFFloat.value = a;

			break;

		case FIELDTYPE_SFTime:

			sscanf(ptr, "%lf", &db);

			value->X3D_SFTime.value = db;

			break;

                case FIELDTYPE_SFInt32:

                        sscanf(ptr, "%d", &i);

                        value->X3D_SFInt32.value = i;

                        break;

		case FIELDTYPE_SFString:

			sscanf(ptr, "\"%s\"", tstring);
			len = strlen(tstring);
			len--;
			tstring[len] = '\0';

			value->X3D_SFString.strptr = malloc ((strlen(tstring)+1) * sizeof(char));
			strncpy(value->X3D_SFString.strptr,tstring, strlen(tstring) + 1);
			value->X3D_SFString.len = strlen(tstring);
			
			break;

		case FIELDTYPE_MFString:

			bzero(ttok, sizeof(ttok));
			/* changes from Doug Sanden */
			temp = strtok(ptr, "\r\n"); /* we will parse manually within "a line" "because we " dont trust blanks */
					
			j = 0;
			while (strncmp(temp, "RE_EOT", 6) && (temp != NULL)) {
			/* changes from Doug Sanden */
				/*pre process to get the "" strings*/
				int start, istart;
				int stop;
				int i;
				istart = 0;
				do
				{
					start = 0;
					stop = 0;
					/* find the starting " */
					for(i=istart;i<strlen(temp);i++)
					{
						if( temp[i] == '"' )
						{
							if( i > 0 )
							{
								if( temp[i-1] != '\\' ) start = i+1;  /* the special \" case - ignor as literal */
							} else {
								start = i+1;
							}
						}
						if( start ) break;
					}
					/* find the stopping " */
					if(start)
					{
						for(i=start;i<strlen(temp);i++)
						{
							if( temp[i] == '"' )
								if( temp[i-1] != '\\' ) stop = i-1;
							if( stop ) break;
						}
					}
					if( start && stop )
					{
						len = stop-start+1;
						strncpy(tstring,&temp[start],len);
						tstring[len] = '\0';
						strcat(ttok, tstring);
						strcat(ttok, "\r"); /*good delimeter needed for loop below - vrml doesn't allow \r within ""? or other like ~ */
						j++;
						istart = stop + 2;
					}
				}while(start && stop);
			 	temp = strtok(NULL, "\r\n");	
			}

			value->X3D_MFString.n = j;
			value->X3D_MFString.p = malloc(j*sizeof(X3DNode));

			/* changes from Doug Sanden */
			temp = strtok(ttok, "\r");
			if (temp != NULL) {
				value->X3D_MFString.p[0].len = strlen(temp);
				value->X3D_MFString.p[0].strptr = malloc(sizeof(char)*(STRLEN));
				strncpy(value->X3D_MFString.p[0].strptr, temp, STRLEN);
			/* changes from Doug Sanden */
				value->X3D_MFString.p[0].type = FIELDTYPE_SFString;
			}

			for (i = 1; i < j; i++) {
			/* changes from Doug Sanden */
				temp = strtok(NULL, "\r");
				value->X3D_MFString.p[i].len = strlen(temp);
				value->X3D_MFString.p[i].strptr = malloc(STRLEN);
				strncpy(value->X3D_MFString.p[i].strptr, temp, STRLEN);

				/* changes from Doug Sanden */
				value->X3D_MFString.p[i].type = FIELDTYPE_SFString;
			}
			break;

		case FIELDTYPE_MFInt32:

			temp = strtok(ptr, " \r\n");
			num = atoi(temp);
			value->X3D_MFInt32.n = num;
			value->X3D_MFInt32.p = malloc(num * sizeof(X3DNode));

			for (i = 0; i < num; i++) {
				temp = strtok(NULL, " \r\n");
				val = atoi(temp);
				value->X3D_MFInt32.p[i].value = val;
				value->X3D_MFInt32.p[i].type= FIELDTYPE_SFInt32;
			}

			break;

		case FIELDTYPE_MFBool:

			temp = strtok(ptr, " \r\n");
			num = atoi(temp);
			value->X3D_MFBool.n = num;
			value->X3D_MFBool.p = malloc(num * sizeof(X3DNode));

			for (i = 0; i < num; i++) {
				temp = strtok(NULL, " \r\n");
				if (!strcmp(temp, "TRUE")) {
					value->X3D_MFBool.p[i].value = 1;
				} else {
					value->X3D_MFBool.p[i].value = 0;
				}
				value->X3D_MFBool.p[i].type= FIELDTYPE_SFBool;
			}

			break;

		case FIELDTYPE_MFFloat:

			temp = strtok(ptr, " \r\n");
			num = atoi(temp);
			value->X3D_MFFloat.n = num;
			value->X3D_MFFloat.p = malloc(num * sizeof(X3DNode));

			for (i = 0; i < num; i++) {
				temp = strtok(NULL, " \r\n");
				fval = atof(temp);
				value->X3D_MFFloat.p[i].value = fval;
				value->X3D_MFFloat.p[i].type= FIELDTYPE_SFFloat;
			}

			break;

		case FIELDTYPE_MFVec3f:
			temp = strtok(ptr, " \r\n");
			num = atoi(temp);

			value->X3D_MFVec3f.n = num;
			value->X3D_MFVec3f.p = malloc(num * sizeof(X3DNode));

			for (i = 0; i < num; i++) {
				for (j=0; j < 3; j++) {
					temp = strtok(NULL, " \r\n");
					fval = atof(temp);	
					value->X3D_MFVec3f.p[i].c[j] = fval;
				}	
				value->X3D_MFVec3f.p[i].type = FIELDTYPE_SFVec3f;
			}

			break;

		case FIELDTYPE_MFVec3d:
			temp = strtok(ptr, " \r\n");
			num = atoi(temp);

			value->X3D_MFVec3d.n = num;
			value->X3D_MFVec3d.p = malloc(num * sizeof(X3DNode));

			for (i = 0; i < num; i++) {
				for (j=0; j < 3; j++) {
					temp = strtok(NULL, " \r\n");
					fval = atof(temp);	
					value->X3D_MFVec3d.p[i].c[j] = (double) fval;
				}	
				value->X3D_MFVec3d.p[i].type = FIELDTYPE_SFVec3d;
			}

			break;

		case FIELDTYPE_MFColor:
			temp = strtok(ptr, " \r\n");
			num = atoi(temp);

			value->X3D_MFColor.n = num;
			value->X3D_MFColor.p = malloc(num * sizeof(X3DNode));

			for (i = 0; i < num; i++) {
				for (j=0; j < 3; j++) {
					temp = strtok(NULL, " \r\n");
					fval = atof(temp);	
					value->X3D_MFColor.p[i].c[j] = fval;
				}	
				value->X3D_MFColor.p[i].type = FIELDTYPE_SFColor;
			}

			break;

		case FIELDTYPE_MFVec2f:
			temp = strtok(ptr, " \r\n");
			num = atoi(temp);

			value->X3D_MFVec2f.n = num;
			value->X3D_MFVec2f.p = malloc(num * sizeof(X3DNode));

			for (i = 0; i < num; i++) {
				for (j=0; j < 2; j++) {
					temp = strtok(NULL, " \r\n");
					fval = atof(temp);	
					value->X3D_MFVec2f.p[i].c[j] = fval;
				}	
				value->X3D_MFVec2f.p[i].type = FIELDTYPE_SFVec2f;
			}

			break;

		case FIELDTYPE_MFRotation:
			temp = strtok(ptr, " \r\n");
			num = atoi(temp);

			value->X3D_MFRotation.n = num;
			value->X3D_MFRotation.p = malloc(num * sizeof(X3DNode));

			for (i = 0; i < num; i++) {
				for (j=0; j < 4; j++) {
					temp = strtok(NULL, " \r\n");
					fval = atof(temp);	
					value->X3D_MFRotation.p[i].r[j] = fval;
				}	
				value->X3D_MFRotation.p[i].type = FIELDTYPE_SFRotation;
			}

			break;

		case FIELDTYPE_MFColorRGBA:
			temp = strtok(ptr, " \r\n");
			num = atoi(temp);

			value->X3D_MFColorRGBA.n = num;
			value->X3D_MFColorRGBA.p = malloc(num * sizeof(X3DNode));

			for (i = 0; i < num; i++) {
				for (j=0; j < 4; j++) {
					temp = strtok(NULL, " \r\n");
					fval = atof(temp);	
					value->X3D_MFColorRGBA.p[i].r[j] = fval;
				}	
				value->X3D_MFColorRGBA.p[i].type = FIELDTYPE_SFColorRGBA;
			}
			
			break;

                case FIELDTYPE_SFNode:


                        sscanf(ptr, "%d", &adr);

                        value->X3D_SFNode.adr= adr;

                break;

		case FIELDTYPE_MFNode:

			retvals = _X3D_countWords(ptr);
			retvals--;
        		value->X3D_MFNode.p = malloc (retvals/2 * sizeof (X3DNode));
        		value->X3D_MFNode.n = retvals;

        		for (i= 0; i< (retvals); i++) {

                		/* skip to the memory pointer */
                		SKIP_CONTROLCHARS

                		/* read in the memory pointer */
                		sscanf (ptr,"%d",&mytmp); /* changed for 1.18.15 JAS */
				value->X3D_MFNode.p[i].adr = mytmp;
	
        		        /* skip past this number now */
                		SKIP_IF_GT_SPACE
        		}
	

		break;

		default: 
			printf("XXX - getValue, not implemented yet for type '%s'\n", FIELDTYPES[src->datatype]);
			return 0;
	}

	return value;
}


void X3D_setValue (X3DEventIn *dest, X3DNode *node) {
	char myline[2048];
	int count;
	int i;
	/* unused int ptr; */
	char tstring[2048];
	
	/* sanity check */
	if (dest->datatype != node->X3D_SFNode.type) {
		printf ("X3D_setValue mismatch: event type %s, value type %s (%d, %d)\n", 
				FIELDTYPES[(int)dest->datatype], FIELDTYPES[node->X3D_SFNode.type],
					dest->datatype,node->X3D_SFNode.type);
		return;
	}


	switch (dest->datatype) {
		default:
		printf ("XXX - setValue, not implemented yet for type '%s'\n",FIELDTYPES[dest->datatype]);
		return;

		case FIELDTYPE_SFVec3d:
			sprintf (myline, "%c %d %d %d %f %f %f\n",
				mapFieldTypeToEAItype(dest->datatype),
				dest->nodeptr, dest->offset, dest->scripttype,
				node->X3D_SFVec3d.c[0],
				node->X3D_SFVec3d.c[1],
				node->X3D_SFVec3d.c[2]);
			_X3D_sendEvent (SENDEVENT,myline);
		break;


		case FIELDTYPE_SFVec3f:
		case FIELDTYPE_SFColor:
			sprintf (myline, "%c %d %d %d %f %f %f\n",
				mapFieldTypeToEAItype(dest->datatype),
				dest->nodeptr, dest->offset, dest->scripttype,
				node->X3D_SFVec3f.c[0],
				node->X3D_SFVec3f.c[1],
				node->X3D_SFVec3f.c[2]);
			_X3D_sendEvent (SENDEVENT,myline);
		break;

		case FIELDTYPE_SFVec2f:
			sprintf (myline, "%c %d %d %d %f %f\n",
				mapFieldTypeToEAItype(dest->datatype),
				dest->nodeptr, dest->offset, dest->scripttype,
				node->X3D_SFVec2f.c[0],
				node->X3D_SFVec2f.c[1]);
			_X3D_sendEvent (SENDEVENT,myline);
		break;

		case FIELDTYPE_SFRotation:
		case FIELDTYPE_SFColorRGBA:
			sprintf (myline, "%c %d %d %d %f %f %f %f\n",
				mapFieldTypeToEAItype(dest->datatype),
				dest->nodeptr, dest->offset, dest->scripttype,
				node->X3D_SFRotation.r[0],
				node->X3D_SFRotation.r[1],
				node->X3D_SFRotation.r[2],
				node->X3D_SFRotation.r[3]);
			_X3D_sendEvent (SENDEVENT,myline);
		break;

		case FIELDTYPE_SFBool:
			if (node->X3D_SFBool.value) {
				sprintf (myline, "%c %d %d %d TRUE\n",
					mapFieldTypeToEAItype(dest->datatype),
					dest->nodeptr, dest->offset, dest->scripttype);
			} else { 
				sprintf (myline, "%c %d %d %d FALSE\n",
					mapFieldTypeToEAItype(dest->datatype),
					dest->nodeptr, dest->offset, dest->scripttype);
			}
			_X3D_sendEvent (SENDEVENT,myline);
			break;
		case FIELDTYPE_SFInt32:
			sprintf (myline, "%c %d %d %d %d\n",
				mapFieldTypeToEAItype(dest->datatype),
				dest->nodeptr, dest->offset, dest->scripttype,
				node->X3D_SFBool.value);
			_X3D_sendEvent (SENDEVENT,myline);
		break;

		case FIELDTYPE_SFFloat:
			sprintf (myline, "%c %d %d %d %f\n",
				mapFieldTypeToEAItype(dest->datatype),
				dest->nodeptr, dest->offset, dest->scripttype,
				node->X3D_SFFloat.value);
			_X3D_sendEvent (SENDEVENT,myline);
		break;

		case FIELDTYPE_SFTime:
			sprintf (myline, "%c %d %d %d %lf\n",
				mapFieldTypeToEAItype(dest->datatype),
				dest->nodeptr, dest->offset, dest->scripttype,
				node->X3D_SFTime.value);
			_X3D_sendEvent (SENDEVENT,myline);
		break;

		case FIELDTYPE_SFString:
			sprintf (myline, "%c %d %d %d %d:%s\n",
				mapFieldTypeToEAItype(dest->datatype),
				dest->nodeptr, dest->offset, dest->scripttype,
				node->X3D_SFString.len, node->X3D_SFString.strptr);
			_X3D_sendEvent (SENDEVENT,myline);
		break;

		case FIELDTYPE_MFInt32:
			sprintf(myline, "%c %d %d %d [",
				mapFieldTypeToEAItype(dest->datatype),
				dest->nodeptr, dest->offset, dest->scripttype
				);
			
			for (i = 0; i < node->X3D_MFInt32.n; i++) {
				sprintf(tstring, "%d", node->X3D_MFInt32.p[i].value);
				strcat(myline, tstring);
				strcat(myline, ", ");
			}
			strcat(myline, "]");
			_X3D_sendEvent (SENDEVENT, myline);

		break;
			
		case FIELDTYPE_MFBool:
			sprintf(myline, "%c %d %d %d [",
				mapFieldTypeToEAItype(dest->datatype),
				dest->nodeptr, dest->offset, dest->scripttype
				);
			
			for (i = 0; i < node->X3D_MFBool.n; i++) {
				if (node->X3D_MFBool.p[i].value) {
					strcat(myline, "TRUE");
				} else {
					strcat(myline, "FALSE");
				}
				strcat(myline, ", ");
			}
			strcat(myline, "]");
			_X3D_sendEvent (SENDEVENT, myline);

		break;
			
		case FIELDTYPE_MFFloat:
			sprintf(myline, "%c %d %d %d [",
				mapFieldTypeToEAItype(dest->datatype),
				dest->nodeptr, dest->offset, dest->scripttype
				);
			
			for (i = 0; i < node->X3D_MFFloat.n; i++) {
				sprintf(tstring, "%f", node->X3D_MFFloat.p[i].value);
				strcat(myline, tstring);
				strcat(myline, ", ");
			}
			strcat(myline, "]");
			_X3D_sendEvent (SENDEVENT, myline);

		break;

		case FIELDTYPE_MFVec3f:
			sprintf(myline, "%c %d %d %d [",
				mapFieldTypeToEAItype(dest->datatype),
				dest->nodeptr, dest->offset, dest->scripttype
				);

			for (i = 0; i < node->X3D_MFVec3f.n; i++) {
				sprintf(tstring, "%f %f %f, ", node->X3D_MFVec3f.p[i].c[0], node->X3D_MFVec3f.p[i].c[1], node->X3D_MFVec3f.p[i].c[2]);
				strcat(myline, tstring);
			}
			strcat(myline, "]");
			_X3D_sendEvent(SENDEVENT, myline);

		break;

		case FIELDTYPE_MFVec3d:
			sprintf(myline, "%c %d %d %d [",
				mapFieldTypeToEAItype(dest->datatype),
				dest->nodeptr, dest->offset, dest->scripttype
				);

			for (i = 0; i < node->X3D_MFVec3d.n; i++) {
				sprintf(tstring, "%lf %lf %lf, ", node->X3D_MFVec3d.p[i].c[0], node->X3D_MFVec3d.p[i].c[1], node->X3D_MFVec3d.p[i].c[2]);
				strcat(myline, tstring);
			}
			strcat(myline, "]");
			_X3D_sendEvent(SENDEVENT, myline);

		break;

		case FIELDTYPE_MFColor:
			sprintf(myline, "%c %d %d %d [",
				mapFieldTypeToEAItype(dest->datatype),
				dest->nodeptr, dest->offset, dest->scripttype
				);

			for (i = 0; i < node->X3D_MFColor.n; i++) {
				sprintf(tstring, "%f %f %f, ", node->X3D_MFColor.p[i].c[0], node->X3D_MFColor.p[i].c[1], node->X3D_MFColor.p[i].c[2]);
				strcat(myline, tstring);
			}
			strcat(myline, "]");
			_X3D_sendEvent(SENDEVENT, myline);

		break;

		case FIELDTYPE_MFVec2f:
			sprintf(myline, "%c %d %d %d [",
				mapFieldTypeToEAItype(dest->datatype),
				dest->nodeptr, dest->offset, dest->scripttype
				);

			for (i = 0; i < node->X3D_MFVec2f.n; i++) {
				sprintf(tstring, "%f %f, ", node->X3D_MFVec2f.p[i].c[0], node->X3D_MFVec2f.p[i].c[1]);
				strcat(myline, tstring);
			}
			strcat(myline, "]");
			_X3D_sendEvent(SENDEVENT, myline);

		break;

		case FIELDTYPE_MFRotation:
			sprintf(myline, "%c %d %d %d [",
				mapFieldTypeToEAItype(dest->datatype),
				dest->nodeptr, dest->offset, dest->scripttype
				);

			for (i = 0; i < node->X3D_MFRotation.n; i++) {
				sprintf(tstring, "%f %f %f %f, ", node->X3D_MFRotation.p[i].r[0], node->X3D_MFRotation.p[i].r[1], node->X3D_MFRotation.p[i].r[2], node->X3D_MFRotation.p[i].r[3]);
				strcat(myline, tstring);
			}
			strcat(myline, "]");
			_X3D_sendEvent(SENDEVENT, myline);

		break;

		case FIELDTYPE_MFColorRGBA:
			sprintf(myline, "%c %d %d %d [",
				mapFieldTypeToEAItype(dest->datatype),
				dest->nodeptr, dest->offset, dest->scripttype
				);

			for (i = 0; i < node->X3D_MFColorRGBA.n; i++) {
				sprintf(tstring, "%f %f %f %f, ", node->X3D_MFColorRGBA.p[i].r[0], node->X3D_MFColorRGBA.p[i].r[1], node->X3D_MFColorRGBA.p[i].r[2], node->X3D_MFColorRGBA.p[i].r[3]);
				strcat(myline, tstring);
			}
			strcat(myline, "]");
			_X3D_sendEvent(SENDEVENT, myline);

		break;

		case FIELDTYPE_MFString:
			sprintf(myline, "%c %d %d %d [ ",
				mapFieldTypeToEAItype(dest->datatype),
				dest->nodeptr, dest->offset, dest->scripttype
				);

			for (i = 0; i < node->X3D_MFString.n; i++) {
				strcat(myline, "\"");
				strcat(myline, node->X3D_MFString.p[i].strptr);
				strcat(myline, "\" ");
			}
			strcat(myline, "]");
/*
			int len = strlen(myline);
			len--;
			myline[len] = '\0';
*/
			_X3D_sendEvent(SENDEVENT, myline);

		break;

		case FIELDTYPE_MFNode:
			#ifdef VERBOSE
			printf ("sending in %d nodes\n",node->X3D_MFNode.n);
			#endif

			for (count = 0; count < node->X3D_MFNode.n; count ++) {
				sprintf (myline,"%d %d %s %d\n",
					dest->nodeptr,
					dest->offset,
					dest->field,
					node->X3D_MFNode.p[count].adr);

				_X3D_sendEvent (SENDCHILD,myline);
			}

		break;

		case FIELDTYPE_SFNode:
			sprintf (myline, "%c %d %d %d %d\n",
				mapFieldTypeToEAItype(dest->datatype),
				dest->nodeptr, dest->offset, dest->scripttype,
				node->X3D_SFNode.adr);

			_X3D_sendEvent (SENDEVENT,myline);

	}

}


/*****************************************************************************/

#ifdef WIN32
void X3D_lastViewpoint(){}
void X3D_firstViewpoint(){}
void X3D_previousViewpoint(){}
void X3D_nextViewpoint(){}


#endif
char *descrip = NULL;

void X3D_setDescription(char *newDesc) {
	/* description is held locally. */
	if (descrip != NULL) 
		free (descrip);
	descrip  = strdup(newDesc);
}

char *X3D_getDescription() {

	/* description is held locally. */
	if (descrip == NULL) {
		descrip = strdup("in X3D");
	}
	return descrip;
}

char *X3D_getName() {
	char *ptr;
	ptr = strdup(_X3D_makeShortCommand(GETNAME));
	REMOVE_EOT
	return ptr;
}
char *X3D_getVersion() {
	char *ptr;
	ptr = strdup(_X3D_makeShortCommand(GETVERSION));
	REMOVE_EOT
	return ptr;
}
char *X3D_getWorldURL() {
	char *ptr;
	ptr = strdup(_X3D_makeShortCommand(GETURL));
	REMOVE_EOT
	return ptr;
}



float X3D_getCurrentSpeed() {
	char *ptr;
	float curspeed;
	ptr = _X3D_makeShortCommand(GETCURSPEED);
	if (sscanf(ptr,"%f",&curspeed) == 0) {
		printf ("client, error - problem reading float from %s\n",ptr);
		exit(0);
	}
	return curspeed;
}


float X3D_getCurrentFrameRate() {
	char *ptr;
	float curframe;
	ptr = _X3D_makeShortCommand(GETFRAMERATE);
	if (sscanf(ptr,"%f",&curframe) == 0) {
		printf ("client, error - problem reading float from %s\n",ptr);
		exit(0);
	}
	return curframe;
}

X3DNode *X3D_createVrmlFromString(char *str) {
	X3DNode *retval;
	char *ptr;
	int retvals;
	int count;
	int mytmp;
	
        retval = malloc (sizeof(X3DNode));
	retval->X3D_MFNode.type = FIELDTYPE_MFNode;
	retval->X3D_MFNode.n = 0;

	#ifdef VERBOSE
	printf ("X3D_createVrmlFromString  - string %s\n",str);
	#endif

	ptr = _X3D_make2StringCommand(CREATEVS,str,"\nEOT\n");
	
	#ifdef VERBOSE
	printf ("return pointer is %s\n",ptr);
	#endif

	/* now, how many numbers did it return? */
	retvals = _X3D_countWords(ptr);
	retval->X3D_MFNode.p = malloc (retvals * sizeof (X3DNode));
	retval->X3D_MFNode.n = retvals;

	for (count = 0; count < retvals; count++) {
		/* skip to the memory pointer */
		SKIP_CONTROLCHARS

		/* read in the memory pointer */
		sscanf (ptr,"%d",&mytmp); /* changed for 1.18.15 JAS */

		retval->X3D_MFNode.p[count].adr = mytmp;

		/* skip past this number now */
		SKIP_IF_GT_SPACE
	}
	#ifdef VERBOSE
	printf ("X3D_createVrmlFromString, found %d pointers, they are:\n",retval->X3D_MFNode.n);
	for (count=0; count<retval->X3D_MFNode.n; count++) {
		printf ("	%d\n",(int) retval->X3D_MFNode.p[count].adr);
	}
	printf ("(end oflist)\n");
	#endif
	return retval;	
}
X3DNode *X3D_createX3DFromString(char *str) {
	X3DNode *retval;
	char *ptr;
	int retvals;
	int count;
	int mytmp;
	
        retval = malloc (sizeof(X3DNode));
	retval->X3D_MFNode.type = FIELDTYPE_MFNode;
	retval->X3D_MFNode.n = 0;

	#ifdef VERBOSE
	printf ("X3D_createX3DFromString  - string %s\n",str);
	#endif

	ptr = _X3D_make2StringCommand(CREATEXS,str,"\nEOT\n");
	
	#ifdef VERBOSE
	printf ("return pointer is %s\n",ptr);
	#endif

	/* now, how many numbers did it return? */
	retvals = _X3D_countWords(ptr);
	retval->X3D_MFNode.p = malloc (retvals * sizeof (X3DNode));
	retval->X3D_MFNode.n = retvals;

	for (count = 0; count < retvals; count++) {
		/* skip to the memory pointer */
		SKIP_CONTROLCHARS

		/* read in the memory pointer */
		sscanf (ptr,"%d",&mytmp); /* changed for 1.18.15 JAS */

		retval->X3D_MFNode.p[count].adr = mytmp;

		/* skip past this number now */
		SKIP_IF_GT_SPACE
	}
	#ifdef VERBOSE
	printf ("X3D_createVrmlFromString, found %d pointers, they are:\n",retval->X3D_MFNode.n);
	for (count=0; count<retval->X3D_MFNode.n; count++) {
		printf ("	%d\n",(int) retval->X3D_MFNode.p[count].adr);
	}
	printf ("(end oflist)\n");
	#endif
#undef VERBOSE
	return retval;	
}

