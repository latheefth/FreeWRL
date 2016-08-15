#ifdef OLDCODE
OLDCODE
OLDCODE#include <config.h>
OLDCODE
OLDCODE#if !defined(IPHONE) && !defined(_ANDROID) && !defined(GLES2) && !defined(AQUA)
OLDCODE
OLDCODE/* DJTRACK_OSCSENSORS */
OLDCODE
OLDCODE/* 
OLDCODE * NOTE -- this file is #include'd into Component_Networking.c , it exists solely to separate out the handler code
OLDCODE * in order to make it easier to override with custom implementations and not muddle Component_Networking.c itself.
OLDCODE *
OLDCODE * Do not add this file into a Makefile or build system's source list.
OLDCODE */
OLDCODE
OLDCODE
OLDCODEtypedef int (*functions)(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data) ;
OLDCODE
OLDCODEint nullOSC_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data) ;
OLDCODEint defaultOSC_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data) ;
OLDCODE
OLDCODEint nullOSC_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data)
OLDCODE{
OLDCODE	struct X3D_OSC_Sensor *realnode ;
OLDCODE	realnode = (struct X3D_OSC_Sensor *) user_data ;
OLDCODE
OLDCODE	printf("nullOSC_handler (%s,%d) : description='%s'\n",__FILE__,__LINE__, realnode->description->strptr) ;
OLDCODE	printf("nullOSC_handler (%s,%d) : filter=%s\n",__FILE__,__LINE__, realnode->filter->strptr) ;
OLDCODE	printf("nullOSC_handler (%s,%d) : listenfor=%s\n",__FILE__,__LINE__, realnode->listenfor->strptr);
OLDCODE
OLDCODE	printf("%s (%d,%s) <-", path, argc,types);
OLDCODE	int i ;
OLDCODE	for (i=0 ; i < argc ; i++) {
OLDCODE		switch (types[i]) {
OLDCODE			case 'f':
OLDCODE				printf(" %c:%f", types[i], argv[i]->f) ;
OLDCODE				break;
OLDCODE			case 'i':
OLDCODE				printf(" %c:%d", types[i], argv[i]->i) ;
OLDCODE				break;
OLDCODE			default:
OLDCODE				printf(" %c:??", types[i]) ;
OLDCODE				break;
OLDCODE		}
OLDCODE	}
OLDCODE	printf("\n\n") ;
OLDCODE	fflush(stdout);
OLDCODE
OLDCODE	return 0;
OLDCODE}
OLDCODEint defaultOSC_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data)
OLDCODE{
OLDCODE	struct X3D_OSC_Sensor *realnode ;
OLDCODE	realnode = (struct X3D_OSC_Sensor *) user_data ;
OLDCODE
OLDCODE	int willOverflow = 0;
OLDCODE	int freeCount;
OLDCODE	int iFltCount;
OLDCODE	int iIntCount;
OLDCODE	int iStrCount;
OLDCODE	int iBlobCount;
OLDCODE	int iMidiCount;
OLDCODE	int iOtherCount;
OLDCODE	/* Get the incoming counts */
OLDCODE	utilOSCcounts((char*) types,&iIntCount,&iFltCount,&iStrCount,&iBlobCount,&iMidiCount,&iOtherCount);
OLDCODE
OLDCODE	/*
OLDCODE	 * If we have FIFOs operative, then use them...
OLDCODE	 * We need to do an atomic transaction, ie do not push
OLDCODE	 * any values unless there is space for all the values.
OLDCODE	 */
OLDCODE	if (realnode->FIFOsize > 0) {
OLDCODE		freeCount = RingBuffer_freeLen(realnode->_floatInpFIFO) ;
OLDCODE		if (iFltCount > freeCount) {willOverflow++;}
OLDCODE
OLDCODE		freeCount = RingBuffer_freeLen(realnode->_int32InpFIFO ) ;
OLDCODE		if ((iIntCount+iMidiCount) > freeCount) {willOverflow++;}
OLDCODE
OLDCODE		freeCount = RingBuffer_freeLen(realnode->_stringInpFIFO) ;
OLDCODE		if ((iStrCount+iBlobCount+iOtherCount) > freeCount) {willOverflow++;}
OLDCODE	}
OLDCODE
OLDCODE/*
OLDCODE	printf("defaultOSC_handler : description='%s'\n", realnode->description->strptr) ;
OLDCODE	printf("defaultOSC_handler : filter=%s\n", realnode->filter->strptr) ;
OLDCODE	printf("defaultOSC_handler : listenfor=%s (got %s)\n", realnode->listenfor->strptr,types);
OLDCODE	printf("defaultOSC_handler : enabled=%d\n", realnode->enabled);
OLDCODE	printf("defaultOSC_handler : gotEvents=%d\n", realnode->gotEvents);
OLDCODE	printf("defaultOSC_handler : FIFOsize=%d\n", realnode->FIFOsize);
OLDCODE	printf("defaultOSC_handler : _status=%d\n", realnode->_status);
OLDCODE*/
OLDCODE
OLDCODE/*
OLDCODE	printf("defaultOSC_handler int _renderFlags=%d\n", realnode->_renderFlags);
OLDCODE	printf("defaultOSC_handler int _hit=%d\n", realnode->_hit);
OLDCODE	printf("defaultOSC_handler int _change=%d\n", realnode->_change);
OLDCODE	printf("defaultOSC_handler int _nparents=%d\n", realnode->_nparents);
OLDCODE	printf("defaultOSC_handler int _nparalloc=%d\n", realnode->_nparalloc);
OLDCODE	printf("defaultOSC_handler int _ichange=%d\n", realnode->_ichange);
OLDCODE	printf("defaultOSC_handler int _nodeType=%d\n", realnode->_nodeType);
OLDCODE	printf("defaultOSC_handler int referenceCount=%d\n", realnode->referenceCount);
OLDCODE	printf("defaultOSC_handler int _defaultContainer=%d\n", realnode->_defaultContainer);
OLDCODE*/
OLDCODE
OLDCODE/*
OLDCODE	printf("defaultOSC_handler struct Multi_Float _floatInpFIFO;
OLDCODE	printf("defaultOSC_handler struct Multi_Float _floatOutFIFO;
OLDCODE	printf("defaultOSC_handler struct Multi_Int32 _int32InpFIFO;
OLDCODE	printf("defaultOSC_handler struct Multi_Int32 _int32OutFIFO;
OLDCODE	printf("defaultOSC_handler struct Multi_Node _nodeInpFIFO;
OLDCODE	printf("defaultOSC_handler struct Multi_Node _nodeOutFIFO;
OLDCODE	printf("defaultOSC_handler struct Multi_String _stringInpFIFO;
OLDCODE	printf("defaultOSC_handler struct Multi_String _stringOutFIFO;
OLDCODE*/
OLDCODE	if (willOverflow > 0) {
OLDCODE		printf("defaultOSC_handler would overflow in %s,%d\n", __FILE__,__LINE__);
OLDCODE	} else {
OLDCODE                /* stringInp */
OLDCODE		#if TRACK_OSC_MSG
OLDCODE		printf("%s (%d,%s) <-", path, argc,types);
OLDCODE		#endif
OLDCODE		int i ;
OLDCODE		int pushBuffError = 0 ;
OLDCODE		for (i=0 ; i < argc ; i++) {
OLDCODE			switch (types[i]) {
OLDCODE				case 'f':
OLDCODE					#if TRACK_OSC_MSG
OLDCODE					printf(" %c:%f", types[i], argv[i]->f) ;
OLDCODE					#endif
OLDCODE					realnode->floatInp = (argv[i]->f) ;
OLDCODE					if (realnode->FIFOsize > 0) {
OLDCODE						#if TRACK_OSC_MSG
OLDCODE						printf("_floatInpFIFO = %p\n",realnode->_floatInpFIFO) ;
OLDCODE						#endif
OLDCODE						pushBuffError =  RingBuffer_pushFloat(realnode->_floatInpFIFO, argv[i]->f) ;
OLDCODE					}
OLDCODE					break;
OLDCODE				case 'i':
OLDCODE					#if TRACK_OSC_MSG
OLDCODE					printf(" %c:%d", types[i], argv[i]->i) ;
OLDCODE					#endif
OLDCODE					realnode->int32Inp = (argv[i]->i) ;
OLDCODE					if (realnode->FIFOsize > 0) {
OLDCODE						#if TRACK_OSC_MSG
OLDCODE						printf("_int32InpFIFO = %p\n",realnode->_int32InpFIFO) ;
OLDCODE						#endif
OLDCODE						pushBuffError =  RingBuffer_pushInt(realnode->_int32InpFIFO, argv[i]->i) ;
OLDCODE					}
OLDCODE					break;
OLDCODE				case 's':
OLDCODE					#if TRACK_OSC_MSG
OLDCODE					printf(" %c:%s", types[i], (char *)argv[i]) ;
OLDCODE					#endif
OLDCODE					if (realnode->stringInp != NULL) {FREE(realnode->stringInp);}
OLDCODE					realnode->stringInp = newASCIIString((char *)argv[i]);
OLDCODE					if (realnode->FIFOsize > 0) {
OLDCODE						#if TRACK_OSC_MSG
OLDCODE						printf("_stringInpFIFO = %p\n",realnode->_stringInpFIFO) ;
OLDCODE						#endif
OLDCODE						pushBuffError =  RingBuffer_pushPointer(realnode->_stringInpFIFO, (newASCIIString((char *)argv[i]))->strptr);
OLDCODE					}
OLDCODE					break;
OLDCODE				default:
OLDCODE					printf(" %c:??", types[i]) ;
OLDCODE					lo_arg_pp(types[i], argv[i]);
OLDCODE					break;
OLDCODE			}
OLDCODE			#if TRACK_OSC_MSG
OLDCODE			printf(" ");
OLDCODE			#endif
OLDCODE		}
OLDCODE		#if TRACK_OSC_MSG
OLDCODE		printf("\n\n") ;
OLDCODE		#endif
OLDCODE		fflush(stdout);
OLDCODE
OLDCODE		if (realnode->enabled) {
OLDCODE			realnode->gotEvents += 1;
OLDCODE			MARK_EVENT (X3D_NODE(realnode), offsetof(struct X3D_OSC_Sensor, gotEvents));
OLDCODE		}
OLDCODE	}
OLDCODE	#if TRACK_OSC_MSG
OLDCODE	printf("\n");
OLDCODE	printf("defaultOSC_handler : description='%s'\n", realnode->description->strptr) ;
OLDCODE	printf("defaultOSC_handler : int32Inp=%d\n", realnode->int32Inp);
OLDCODE	printf("defaultOSC_handler : floatInp=%f\n", realnode->floatInp);
OLDCODE	printf("defaultOSC_handler : stringInp=%s\n", realnode->stringInp->strptr);
OLDCODE	printf("\n");
OLDCODE
OLDCODE	if (realnode->FIFOsize > 0) {
OLDCODE		int qLen , iTemp ;
OLDCODE		float fTemp ;
OLDCODE		char * sTemp ;
OLDCODE		
OLDCODE		qLen = RingBuffer_qLen(realnode->_floatInpFIFO) ;
OLDCODE		if (qLen > 0) {
OLDCODE			fTemp = RingBuffer_peekUnion(realnode->_floatInpFIFO)->f ;
OLDCODE			printf("%d : float length=%d , head=%f\n",__LINE__,qLen,fTemp);
OLDCODE		}
OLDCODE
OLDCODE		qLen = RingBuffer_qLen(realnode->_int32InpFIFO) ;
OLDCODE		if (qLen > 0) {
OLDCODE			iTemp = RingBuffer_peekUnion(realnode->_int32InpFIFO)->i ;
OLDCODE			printf("%d : int length=%d , head=%d\n",__LINE__,qLen,iTemp);
OLDCODE		}
OLDCODE
OLDCODE		qLen = RingBuffer_qLen(realnode->_stringInpFIFO) ;
OLDCODE		if (qLen > 0) {
OLDCODE			sTemp = (char *)RingBuffer_peekUnion(realnode->_stringInpFIFO)->p ;
OLDCODE			printf("%d : string length=%d , head=%s\n",__LINE__,qLen,sTemp);
OLDCODE		}
OLDCODE	}
OLDCODE	printf("\n");
OLDCODE	#endif
OLDCODE
OLDCODE	return 0; /* Tell OSC we have swallowed the packet and that it should NOT try any other handlers */
OLDCODE}
OLDCODE
OLDCODE#define OSCfuncCount  2
OLDCODEfunctions OSCcallbacks[OSCfuncCount] = {nullOSC_handler,defaultOSC_handler};
OLDCODEchar *OSCfuncNames[OSCfuncCount] = { "", "default" };
OLDCODE
OLDCODE#endif /* IPHONE */
#endif //OLDCODE

