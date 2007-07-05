/*******************************************************************
 Copyright (C) 2006 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

#include "headers.h"
#include "EAIheaders.h"
#include "jsUtils.h"
#include "jsNative.h"

void set_one_ECMAtype (uintptr_t tonode, int toname, int dataType, void *Data, unsigned datalen);
void setMFElementtype (uintptr_t num);

/********************************************************************

getField_ToJavascript.

this sends events to scripts that have eventIns defined.

********************************************************************/
void getField_ToJavascript (int num, int fromoffset) {

	#ifdef SETFIELDVERBOSE 
		printf ("CRoutes, sending ScriptEventIn to from offset %d type %d\n",
			fromoffset,JSparamnames[fromoffset].type);  
	#endif

	/* this script initialized yet? */
	initializeScript(num, TRUE);

	/* set the parameter */
	/* see comments in gatherScriptEventOuts to see exact formats */

	switch (JSparamnames[fromoffset].type) {
	case FIELDTYPE_SFBool:
	case FIELDTYPE_SFFloat:
	case FIELDTYPE_SFTime:
	case FIELDTYPE_SFInt32:
	case FIELDTYPE_SFString:
		setScriptECMAtype(num);
		break;
	case FIELDTYPE_SFColor:
	case FIELDTYPE_SFNode:
	case FIELDTYPE_SFVec2f:
	case FIELDTYPE_SFVec3f:
	case FIELDTYPE_SFRotation:
		setScriptMultiElementtype(num);
		break;
	case FIELDTYPE_MFColor:
	case FIELDTYPE_MFVec3f:
	case FIELDTYPE_MFFloat:
	case FIELDTYPE_MFTime:
	case FIELDTYPE_MFInt32:
	case FIELDTYPE_MFString:
	case FIELDTYPE_MFNode:
	case FIELDTYPE_MFRotation:
	case FIELDTYPE_SFImage:
		setMFElementtype(num);
		break;
	default : {
		printf("WARNING: sendScriptEventIn type %s not handled yet\n",
			FIELDTYPES[JSparamnames[fromoffset].type]);
		}
	}
}


/******************************************************************************/

void set_one_ECMAtype (uintptr_t tonode, int toname, int dataType, void *Data, unsigned datalen) {

	char scriptline[100];
	char scl[100];
	jsval retval;
	float fl;
	double dl;
	int il;
	int intval = 0;

	#ifdef SETFIELDVERBOSE
	printf ("set_one_ECMAtype, to %d namepointer %d, fieldname %s, datatype %d length %d\n",
		tonode,toname,JSparamnames[toname].name,dataType,datalen); 
	#endif
	

	switch (dataType) {
		case FIELDTYPE_SFBool:	{	/* SFBool */
			memcpy ((void *) &intval,Data, datalen);
			if (intval == 1) sprintf (scl,"true");
			else sprintf (scl,"false");
			break;
		}

		case FIELDTYPE_SFFloat:	{
			memcpy ((void *) &fl, Data, datalen);
			sprintf (scl,"%f",fl);
			break;
		}
		case FIELDTYPE_SFTime:	{
			memcpy ((void *) &dl, Data, datalen);
			sprintf (scl,"%f", dl);
			break;
		}
		case FIELDTYPE_SFInt32: 	{ 
			memcpy ((void *) &il,Data, datalen);
			sprintf (scl,"%d", il);
			break;
		}

		case FIELDTYPE_SFString: {
			struct Uni_String *ms;
			memcpy((void *) &ms,Data, datalen);
			sprintf (scl,"'%s'",ms->strptr);
			break;
		}
		default: {	printf("WARNING: SHOULD NOT BE HERE! %d\n",JSparamnames[toname].type);
		}
	}

	
	/* and set the value */
	sprintf (scriptline,"%s(%s,%f)",
			 JSparamnames[toname].name,scl, TickTime);
	#ifdef SETFIELDVERBOSE
	printf ("set_one_ECMAtype sending in script %s\n",scriptline);
	#endif
	
	if (!ActualrunScript(tonode, scriptline ,&retval)) {
		printf ("failed to set parameter, line %s\n",scriptline);
	}
}

/* sets a SFBool, SFFloat, SFTime, SFIint32, SFString in a script */
void setScriptECMAtype (uintptr_t num) {
	uintptr_t fn, tn;
	int tptr;
	int len;
	unsigned int to_counter;
	CRnodeStruct *to_ptr = NULL;

	fn = (uintptr_t)(CRoutes[num].fromnode) + (uintptr_t)(CRoutes[num].fnptr);
	len = CRoutes[num].len;

	for (to_counter = 0; to_counter < CRoutes[num].tonode_count; to_counter++) {
		to_ptr = &(CRoutes[num].tonodes[to_counter]);
		tn = (uintptr_t) to_ptr->node;
		tptr = to_ptr->foffset;
		set_one_ECMAtype (tn, tptr, JSparamnames[tptr].type, (void *)fn,len);
	}
}





void setMFElementtype (uintptr_t num) {
	void * fn;
	void * tn;
	uintptr_t tptr, fptr;
	int len;
	unsigned int to_counter;
	CRnodeStruct *to_ptr = NULL;
	char scriptline[20000];
	char sline[100];
	jsval retval;
	int x;
	int elementlen;
	char *pptr;
	float *fp;
	int *ip;
	double *dp;
	struct Multi_Node *mfp;

	/* for MFStrings we have: */
	char *chptr;
	struct Uni_String  **ptr;

	int indexPointer;

	JSContext *_context;
	JSObject *_globalObj;

	/* for PixelTextures we have: */
	struct X3D_PixelTexture *mePix;
	struct Multi_Int32 image;
	int pixcount;

	#ifdef SETFIELDVERBOSE 
		printf("------------BEGIN setMFElementtype ---------------\n");
	#endif

	fn = CRoutes[num].fromnode;
	fptr = CRoutes[num].fnptr;
	
	/* we can do arithmetic on character pointers; so we have to cast void *
	   to char * here */
	pptr = (char *)fn + fptr;

	len = CRoutes[num].len;

	/* is this from a MFElementType? */
	if (len <= 0) {
		#ifdef SETFIELDVERBOSE 
		printf ("len of %d means that this is a MF type\n",len);
		#endif
		mfp = (struct Multi_Node *) pptr;

		/* check Multimemcpy for C to C routing for this type */
		/* get the number of elements */
		len = mfp->n;  
		pptr = (char *) mfp->p; /* pptr is a char * just for math stuff */
		#ifdef SETFIELDVERBOSE 
		printf ("setMFElementtype, len now %d, from %d\n",len,fn);
		#endif
	}

		

	for (to_counter = 0; to_counter < CRoutes[num].tonode_count; to_counter++) {
		to_ptr = &(CRoutes[num].tonodes[to_counter]);
		tn = to_ptr->node;
		tptr = to_ptr->foffset;
		indexPointer = (uintptr_t) tn; /* tn should be a small int here - it is script # */

		#ifdef SETFIELDVERBOSE 
			printf ("got a script event! index %d type %d\n",
					num, CRoutes[num].direction_flag);
			printf ("\tfrom %#x from ptr %#x\n\tto %#x toptr %#x\n",fn,fptr,tn,tptr);
			printf ("\tfrom %d from ptr %d\n\tto %d toptr %d\n",fn,fptr,tn,tptr);
			printf ("\tdata length %d\n",len);
			printf ("and, sending it to %s as type %d\n",JSparamnames[tptr].name,
					JSparamnames[tptr].type);
		#endif

		/* get context and global object for this script */
		_context = (JSContext *) ScriptControl[indexPointer].cx;
		_globalObj = (JSObject *)ScriptControl[indexPointer].glob;

		/* make up the name */
		switch (JSparamnames[tptr].type) {
			case FIELDTYPE_MFVec3f: {
				/*strcpy (scriptline,"xxy = new MFVec3f(new SFVec3f(1,2,3));printValue (xxy)");
					break;*/
				strcpy (scriptline, "xxy = new MFVec3f(");
				elementlen = sizeof (float);
				for (x=0; x<len; x++) {
					fp = (float *)pptr;
					sprintf (sline,"new SFVec3f (%f, ",*fp);
					pptr += elementlen;
					strcat (scriptline,sline);

					fp = (float *)pptr;
					sprintf (sline,"%f, ",*fp);
					pptr += elementlen;
					strcat (scriptline,sline);

					fp = (float *)pptr;
					sprintf (sline,"%f)",*fp);
					pptr += elementlen;
					strcat (scriptline,sline);
					if (x < (len-1)) 
						strcat(scriptline,", ");
				}
				break;
				}
			case FIELDTYPE_MFColor: {
				/*strcpy (scriptline,"xxy = new MFColor(new SFColor(1,2,3));printValue (xxy)");
					break;*/
				strcpy (scriptline, "xxy = new MFColor(");
				elementlen = sizeof (float);
				for (x=0; x<len; x++) {
					fp = (float *)pptr;
					sprintf (sline,"new SFColor (%f, ",*fp);
					pptr += elementlen;
					strcat (scriptline,sline);

					fp = (float *)pptr;
					sprintf (sline,"%f, ",*fp);
					pptr += elementlen;
					strcat (scriptline,sline);

					fp = (float *)pptr;
					sprintf (sline,"%f)",*fp);
					pptr += elementlen;
					strcat (scriptline,sline);
					if (x < (len-1)) 
						strcat(scriptline,", ");
				}
				break;
				}
			case FIELDTYPE_MFFloat: {
				strcpy (scriptline, "xxy = new MFFloat(");
				elementlen = sizeof (float);
				for (x=0; x<len; x++) {
					fp = (float *)pptr;
					sprintf (sline,"%f",*fp);
					if (x < (len-1)) {
						strcat(sline,",");
					}
					pptr += elementlen;
					strcat (scriptline,sline);
				}
				break;
				}
			case FIELDTYPE_MFTime:  {
				strcpy (scriptline, "xxy = new MFTime(");
				elementlen = sizeof (double);
				for (x=0; x<len; x++) {
					dp = (double *)pptr;
					sprintf (sline,"%lf",*dp);
					if (x < (len-1)) {
						strcat(sline,",");
					}
					pptr += elementlen;
					strcat (scriptline,sline);
				}
				break;
				}
			case FIELDTYPE_SFImage:	{

				mePix = (struct X3D_PixelTexture *) fn;
				strcpy (scriptline, "xxy = new SFImage(");
				if (mePix->_nodeType == NODE_PixelTexture) {
					image = mePix->image;
					if (image.n > 2) {
						sprintf (sline, "%d, %d, %d, new MFInt32(",image.p[0], image.p[1], image.p[2]);
						strcat (scriptline,sline);
						for (pixcount = 3; pixcount < image.n; pixcount++) {
							sprintf (sline,"%d",image.p[pixcount]);
							if (pixcount < (image.n-1)) {
								strcat(sline,",");
							}
							strcat (scriptline,sline);
						}

						strcat (scriptline, ")");
					}
				}


				break;
				} 

			case FIELDTYPE_MFInt32: {
				strcpy (scriptline, "xxy = new MFInt32(");
				elementlen = sizeof (int);
				for (x=0; x<len; x++) {
					ip = (int *)pptr;
					sprintf (sline,"%d",*ip);
					if (x < (len-1)) {
						strcat(sline,",");
					}
					pptr += elementlen;
						strcat (scriptline,sline);
				}
				break;
				}
			case FIELDTYPE_MFString:{
				strcpy (scriptline, "xxy = new MFString(");
				ptr = (struct Uni_String **) pptr;
				for (x=0; x<len; x++) {

					chptr = ptr[x]->strptr;
					/* printf ("string might be length %d: %s\n",xx,chptr); */
					strcat (scriptline,"new String('");
					strcat (scriptline,chptr);
					strcat (scriptline,"')");
					if (x < (len-1)) {
						strcat (scriptline,",");
					}
					
				}
				break;
				}
			case FIELDTYPE_MFNode:  {
				strcpy (scriptline, "xxy = new MFNode(");
				elementlen = sizeof (int);
				for (x=0; x<len; x++) {
					ip = (int *)pptr;
					sprintf (sline,"%u",*ip);
					if (x < (len-1)) {
						strcat(sline,",");
					}
					pptr += elementlen;
						strcat (scriptline,sline);
				}
				break;
				}
			case FIELDTYPE_MFRotation: {	
				strcpy (scriptline, "xxy = new MFRotation(");

				elementlen = sizeof (float);
				for (x=0; x<len; x++) {


					fp = (float *)pptr;
					sprintf (sline,"new SFRotation (%f, ",*fp);
					pptr += elementlen;
					strcat (scriptline,sline);

					fp = (float *)pptr;
					sprintf (sline,"%f, ",*fp);
					pptr += elementlen;
					strcat (scriptline,sline);

					fp = (float *)pptr;
					sprintf (sline,"%f, ",*fp);
					pptr += elementlen;
					strcat (scriptline,sline);

					fp = (float *)pptr;
					sprintf (sline,"%f)",*fp);
					pptr += elementlen;
					strcat (scriptline,sline);
					if (x < (len-1)) 
						strcat(scriptline,", ");
				}
				break;
				}
			default: {
					printf ("setMFElement, SHOULD NOT DISPLAY THIS\n");
					strcat (scriptline,"(");
				}
		}

		/* convert these values to a jsval type */
		strcat (scriptline,");");
		strcat (scriptline,JSparamnames[tptr].name);
		strcat (scriptline,"(xxy);");

		#ifdef SETFIELDVERBOSE 
			printf("ScriptLine: %s\n",scriptline);
		#endif

		if (!ActualrunScript(indexPointer,scriptline,&retval))
			printf ("AR failed in setxx\n");


	}
	#ifdef SETFIELDVERBOSE 
		printf("------------END setMFElementtype ---------------\n");
	#endif
}

void set_EAI_MFElementtype (int num, int offset, unsigned char *pptr, int len) {

    int tn, tptr;
    char scriptline[2000];
    char sline[100];
    jsval retval;
    int x;
    int elementlen;
    float *fp;
    int *ip;
    double *dp;

    JSContext *_context;
    JSObject *_globalObj;

    #ifdef SETFIELDVERBOSE 
	printf("------------BEGIN set_EAI_MFElementtype ---------------\n");
    #endif

    tn   = num;
    tptr = offset;

    #ifdef SETFIELDVERBOSE 
	printf ("got a script event! index %d\n",num);
	printf ("\tto %#x toptr %#x\n",tn,tptr);
	printf ("\tdata length %d\n",len);
	printf ("and, sending it to %s\n",JSparamnames[tptr].name);
    #endif

    /* get context and global object for this script */
    _context = (JSContext *) ScriptControl[tn].cx;
    _globalObj = (JSObject *)ScriptControl[tn].glob;

    /* make up the name */
    sprintf (scriptline,"%s(",JSparamnames[tptr].name);
    switch (JSparamnames[tptr].type) {
      case FIELDTYPE_MFVec3f: {
	  strcat (scriptline, "new MFVec3f(");
	  elementlen = sizeof (float) * 3;
	  for (x=0; x<(len/elementlen); x++) {
	      fp = (float *)pptr;
	      sprintf (sline,"%f %f %f",*fp,
		       *(fp+elementlen),
		       *(fp+(elementlen*2)));
	      if (x < ((len/elementlen)-1)) {
		  strcat(sline,",");
	      }
	      pptr += elementlen;
	      strcat (scriptline,sline);
	  }
	  break;
      }
      case FIELDTYPE_MFColor: {
	  strcat (scriptline, "new MFColor(");
	  elementlen = sizeof (float) * 3;
	  for (x=0; x<(len/elementlen); x++) {
	      fp = (float *)pptr;
	      sprintf (sline,"%f %f %f",*fp,
		       *(fp+elementlen),
		       *(fp+(elementlen*2)));
	      if (x < ((len/elementlen)-1)) {
		  strcat(sline,",");
	      }
	      pptr += elementlen;
	      strcat (scriptline,sline);
	  }
	  break;
      }
      case FIELDTYPE_MFFloat: {
	  strcat (scriptline, "new MFFloat(");
	  elementlen = sizeof (float);
	  for (x=0; x<(len/elementlen); x++) {
	      fp = (float *)pptr;
	      sprintf (sline,"%f",*fp);
	      if (x < ((len/elementlen)-1)) {
		  strcat(sline,",");
	      }
	      pptr += elementlen;
	      strcat (scriptline,sline);
	  }

	  break;
      }
      case FIELDTYPE_MFTime:  {
	  strcat (scriptline, "new MFTime(");
	  elementlen = sizeof (double);
	  for (x=0; x<(len/elementlen); x++) {
	      dp = (double *)pptr;
	      sprintf (sline,"%lf",*dp);
	      if (x < ((len/elementlen)-1)) {
		  strcat(sline,",");
	      }
	      pptr += elementlen;
	      strcat (scriptline,sline);
	  }
	  break;
      }
      case FIELDTYPE_MFInt32: {
	  strcat (scriptline, "new MFInt32(");
	  elementlen = sizeof (int);
	  for (x=0; x<(len/elementlen); x++) {
	      ip = (int *)pptr;
	      sprintf (sline,"%d",*ip);
	      if (x < ((len/elementlen)-1)) {
		  strcat(sline,",");
	      }
	      pptr += elementlen;
	      strcat (scriptline,sline);
	  }
	  break;
      }
      case FIELDTYPE_MFString:{
	  strcat (scriptline, "new MFString(");
	  elementlen = sizeof (float);
	  printf ("ScriptAssign, MFString probably broken\n");
	  for (x=0; x<(len/elementlen); x++) {
	      fp = (float *)pptr;
	      sprintf (sline,"%f",*fp);
	      if (x < ((len/elementlen)-1)) {
		  strcat(sline,",");
	      }
	      pptr += elementlen;
	      strcat (scriptline,sline);
	  }
	  break;
      }
      case FIELDTYPE_MFNode:  {
	  strcat (scriptline, "new MFNode(");
	  elementlen = sizeof (int);
	  for (x=0; x<(len/elementlen); x++) {
	      ip = (int *)pptr;
	      sprintf (sline,"%u",*ip);
	      if (x < ((len/elementlen)-1)) {
		  strcat(sline,",");
	      }
	      pptr += elementlen;
	      strcat (scriptline,sline);
	  }
	  break;
      }
      case FIELDTYPE_MFRotation: {	strcat (scriptline, "new MFRotation(");
      elementlen = sizeof (float)*4;
      for (x=0; x<(len/elementlen); x++) {
	  fp = (float *)pptr;
	  sprintf (sline,"%f %f %f %f",*fp,
		   *(fp+elementlen),
		   *(fp+(elementlen*2)),
		   *(fp+(elementlen*3)));
	  sprintf (sline,"%f",*fp);
	  if (x < ((len/elementlen)-1)) {
	      strcat(sline,",");
	  }
	  pptr += elementlen;
	  strcat (scriptline,sline);
      }
      break;
      }
      default: {
	  printf ("setMFElement, SHOULD NOT DISPLAY THIS\n");
	  strcat (scriptline,"(");
      }
    }

    /* convert these values to a jsval type */
    strcat (scriptline,"))");

    #ifdef SETFIELDVERBOSE 
	printf("ScriptLine: %s\n",scriptline);
    #endif


    if (!ActualrunScript(tn,scriptline,&retval))
      printf ("AR failed in setxx\n");

    #ifdef SETFIELDVERBOSE 
	printf("------------END set_EAI_MFElementtype ---------------\n");
    #endif
}


/****************************************************************/
/* sets a SFVec3f and SFColor in a script 			*/
/* sets a SFRotation and SFVec2fin a script 			*/
/*								*/
/* all *Native types have the same structure of the struct -	*/
/* we are just looking for the pointer, thus we can handle	*/
/* multi types here 						*/
/* sets a SFVec3f and SFColor in a script 			*/
/****************************************************************/

/* really do the individual set; used by script routing and EAI sending to a script */
void Set_one_MultiElementtype (uintptr_t tonode, uintptr_t tnfield, void *Data, unsigned dataLen ) {

	char scriptline[100];
	jsval retval;
	SFVec3fNative *_privPtr;

	JSContext *_context;
	JSObject *_TimeObj, *_globalObj, *_sfvec3fObj;
	jsval zimbo;


	/* get context and global object for this script */
	_context = (JSContext *) ScriptControl[tonode].cx;
	_globalObj = (JSObject *)ScriptControl[tonode].glob;

	/* set the time for this script */
        zimbo = DOUBLE_TO_JSVAL(JS_NewDouble(_context, TickTime)); 
        if (!JS_DefineProperty(_context,_globalObj, "__eventInTickTime", zimbo, JS_PropertyStub, JS_PropertyStub, JSPROP_PERMANENT)) { 
                printf( "JS_DefineProperty failed for \"__eventInTickTime\" at %s:%d.\n",__FILE__,__LINE__); 
                return; 
        }

	/* get the variable name to hold the incoming value */
	sprintf (scriptline,"__eventIn_Value_%s", JSparamnames[tnfield].name);

	#ifdef SETFIELDVERBOSE 
	printf ("Set_one_MultiElementType: script %d line %s\n",tonode, scriptline);
	#endif

	if (!JS_GetProperty(_context,_globalObj,scriptline,&retval))
		printf ("JS_GetProperty failed in Set_one_MultiElementtype.\n");

	if (!JSVAL_IS_OBJECT(retval))
		printf ("Set_one_MultiElementtype - not an object\n");

	_sfvec3fObj = JSVAL_TO_OBJECT(retval);

	if ((_privPtr = (SFVec3fNative *)JS_GetPrivate(_context, _sfvec3fObj)) == NULL)
		printf("JS_GetPrivate failed Set_one_MultiElementtype.\n");

	/* copy over the data from the VRML side into the script variable. */
	memcpy ((void *) &_privPtr->v,Data, dataLen);

	/* is the function compiled yet? */
	if (JSparamnames[tnfield].eventInFunction == 0) {
		sprintf (scriptline,"%s(__eventIn_Value_%s,__eventInTickTime)", JSparamnames[tnfield].name,JSparamnames[tnfield].name);
		JSparamnames[tnfield].eventInFunction = (uintptr_t) JS_CompileScript(
			_context, _globalObj, scriptline, strlen(scriptline), "compile eventIn",1);
	}
	/* and run the function */
	#ifdef SETFIELDVERBOSE
	printf ("Set_one_MultiElementtype: running script %s\n",scriptline);
	#endif

	if (!JS_ExecuteScript(_context, _globalObj, JSparamnames[tnfield].eventInFunction, &zimbo)) {
		printf ("failed to set parameter for eventIne %s\n",JSparamnames[tnfield].name);
	}
		
	#ifdef OLDCODE
		if (!ActualrunScript(tonode, scriptline ,&retval)) {
			printf ("failed to set parameter, line %s\n",scriptline);
	}
	#endif
}

void setScriptMultiElementtype (uintptr_t num) {
	void * fn;
	void * tn;
	uintptr_t tptr, fptr;
	unsigned int len;
	unsigned int to_counter;

	uintptr_t indexPointer;

	CRnodeStruct *to_ptr = NULL;

	JSContext *_context;
	JSObject *_globalObj;

	fn = CRoutes[num].fromnode;
	fptr = CRoutes[num].fnptr;
	len = CRoutes[num].len;

	for (to_counter = 0; to_counter < CRoutes[num].tonode_count; to_counter++) {
		to_ptr = &(CRoutes[num].tonodes[to_counter]);

		/* the to_node should be a script number; it will be a small integer */
		tn = to_ptr->node;
		indexPointer = (uintptr_t) tn;
		tptr = to_ptr->foffset;

		#ifdef SETFIELDVERBOSE 
			printf ("got a script event! index %d type %d\n",
					num, CRoutes[num].direction_flag);
			printf ("\tfrom %#x from ptr %#x\n\tto %#x toptr %#x\n",fn,fptr,indexPointer,tptr);
			printf ("\tdata length %d\n",len);
			printf ("setScriptMultiElementtype here indexPointer %d tptr %d len %d\n",indexPointer, tptr,len);
		#endif

		/* get context and global object for this script */
		_context = (JSContext *) ScriptControl[indexPointer].cx;
		_globalObj = (JSObject *)ScriptControl[indexPointer].glob;
		fn += fptr;
		Set_one_MultiElementtype (indexPointer, tptr, fn, len);
	}
}

/* convert a number in memory to a printable type. Used to send back EVents, or replies to
   the Java client program. */

void EAI_Convert_mem_to_ASCII (int id, char *reptype, int type, char *memptr, char *buf) {

	double dval;
	float fl[4];
	float *fp;
	int *ip;
	int ival;
	int row;			/* MF* counter */
	struct Multi_String *MSptr;	/* MFString pointer */
	struct Multi_Node *MNptr;	/* MFNode pointer */
	struct Multi_Color *MCptr;	/* MFColor pointer */
	char *ptr;			/* used for building up return string */
	struct Uni_String *svptr;
	unsigned char *retSFString;

	int numPerRow;			/* 1, 2, 3 or 4 floats per row of this MF? */
	int i;

	/* used because of endian problems... */
	int *intptr;
	intptr = (int *) memptr;

	switch (type) {
		case FIELDTYPE_SFBool: 	{
			if (eaiverbose) { 
			printf ("EAI_SFBOOL\n");
			}

			if (*intptr == 1) sprintf (buf,"%s\n%f\n%d\nTRUE",reptype,TickTime,id);
			else sprintf (buf,"%s\n%f\n%d\nFALSE",reptype,TickTime,id);
			break;
		}

		case FIELDTYPE_SFTime:	{
			if (eaiverbose) { 
			printf ("EAI_SFTIME\n");
			}
			memcpy(&dval,memptr,sizeof(double));
			sprintf (buf, "%s\n%f\n%d\n%lf",reptype,TickTime,id,dval);
			break;
		}

		case FIELDTYPE_SFInt32:	{
			if (eaiverbose) { 
			printf ("EAI_SFINT32 or EAI_SFNODE\n");
			}
			memcpy(&ival,memptr,sizeof(int));
			sprintf (buf, "%s\n%f\n%d\n%d",reptype,TickTime,id,ival);
			break;
		}

		case FIELDTYPE_SFFloat:	{
			if (eaiverbose) { 
			printf ("EAI_SFFLOAT\n");
			}

			memcpy(fl,memptr,sizeof(float));
			sprintf (buf, "%s\n%f\n%d\n%f",reptype,TickTime,id,fl[0]);
			break;
		}

		case FIELDTYPE_SFVec3f:
		case FIELDTYPE_SFColor:	{
			if (eaiverbose) { 
			printf ("EAI_SFCOLOR or EAI_SFVEC3F\n");
			}
			memcpy(fl,memptr,sizeof(float)*3);
			sprintf (buf, "%s\n%f\n%d\n%f %f %f",reptype,TickTime,id,fl[0],fl[1],fl[2]);
			break;
		}

		case FIELDTYPE_SFVec2f:	{
			if (eaiverbose) { 
			printf ("EAI_SFVEC2F\n");
			}
			memcpy(fl,memptr,sizeof(float)*2);
			sprintf (buf, "%s\n%f\n%d\n%f %f",reptype,TickTime,id,fl[0],fl[1]);
			break;
		}

		case FIELDTYPE_SFColorRGBA:
		case FIELDTYPE_SFRotation:	{
			if (eaiverbose) { 
			printf ("EAI_SFROTATION\n");
			}

			memcpy(fl,memptr,sizeof(float)*4);
			sprintf (buf, "%s\n%f\n%d\n%f %f %f %f",reptype,TickTime,id,fl[0],fl[1],fl[2],fl[3]);
			break;
		}

		case FIELDTYPE_SFImage:
		case FIELDTYPE_SFString:	{
			if (eaiverbose) { 
			printf ("EAI_SFSTRING\n");
			}

			svptr = (struct Uni_String *)memptr;
			retSFString = (unsigned char *)svptr->strptr; 
			sprintf (buf, "%s\n%f\n%d\n\"%s\"",reptype,TickTime,id,retSFString);
			break;
		}

		case FIELDTYPE_MFString:	{
			if (eaiverbose) { 
			printf ("EAI_MFSTRING\n");
			}

			/* make the Multi_String pointer */
			MSptr = (struct Multi_String *) memptr;

			/* printf ("EAI_MFString, there are %d strings\n",(*MSptr).n);*/
			sprintf (buf, "%s\n%f\n%d\n",reptype,TickTime,id);
			ptr = buf + strlen(buf);

			for (row=0; row<(*MSptr).n; row++) {
        	        	/* printf ("String %d is %s\n",row,(*MSptr).p[row]->strptr);*/
				if (strlen ((*MSptr).p[row]->strptr) == 0) {
					sprintf (ptr, "\"XyZZtitndi\" "); /* encode junk for Java side.*/
				} else {
					sprintf (ptr, "\"%s\" ",(*MSptr).p[row]->strptr);
				}
				/* printf ("buf now is %s\n",buf);*/
				ptr = buf + strlen (buf);
			}

			break;
		}

		case FIELDTYPE_MFNode: 	{
			MNptr = (struct Multi_Node *) memptr;

			if (eaiverbose) { 
			printf ("EAI_MFNode, there are %d nodes at %d\n",(*MNptr).n,(int) memptr);
			}

			sprintf (buf, "%s\n%f\n%d\n",reptype,TickTime,id);
			ptr = buf + strlen(buf);

			for (row=0; row<(*MNptr).n; row++) {
				sprintf (ptr, "%d ",(uintptr_t) (*MNptr).p[row]);
				ptr = buf + strlen (buf);
			}
			break;
		}

		case FIELDTYPE_MFInt32: {
			MCptr = (struct Multi_Color *) memptr;
			if (eaiverbose) { 
				printf ("EAI_MFColor, there are %d nodes at %d\n",(*MCptr).n,(int) memptr);
			}

			sprintf (buf, "%s\n%f\n%d\n%d \n",reptype,TickTime,id,(*MCptr).n);
			ptr = buf + strlen(buf);

			ip = (int *) (*MCptr).p;
			for (row=0; row<(*MCptr).n; row++) {
				sprintf (ptr, "%d \n",*ip); 
				ip++;
				/* printf ("line %d is %s\n",row,ptr);  */
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
				printf ("EAI_MFColor, there are %d nodes at %d\n",(*MCptr).n,(int) memptr);
			}

			sprintf (buf, "%s\n%f\n%d\n%d \n",reptype,TickTime,id,(*MCptr).n);
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
				/* printf ("line %d is %s\n",row,ptr); */
				ptr = buf + strlen (buf);
			}

			break;
		}
		default: {
			printf ("EAI, type %c not handled yet\n",type);
		}


/*XXX	case EAI_MFTIME:	{handleptr = &handleEAI_MFTIME_Listener;break;}*/
	}
}

