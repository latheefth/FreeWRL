/* androidHelper.c */

#if defined( _ANDROID )

#include <stddef.h>
/* possibly also stdlib.h, stdio.h...*/

# include <stdbool.h>
#include <stdio.h>
#include <string.h>


#include <libFreeWRL.h>
#include <jni.h>
#include <android/log.h>

#include <pthread.h>

// from orig system

#include <config.h>
//JAS #include <system.h>
//JAS #include <internal.h>

#include <libFreeWRL.h>
#include <list.h>
#include <resources.h>

#include "display.h"
#include "vrml_parser/Structs.h"

#include <jni.h>
#include <android/log.h>

#include <pthread.h>
#include <resources.h>
#include <iglobal.h>

// end from orig system



#define TRUE (1==1)
#define FALSE (1==0)
#define  LOG_TAG    "FreeWRL-androidHelper-"

// globals

static JavaVM* g_jvm = NULL;
static jclass fileCallbackClass = NULL;
static jmethodID fileLoadCallback = NULL;
static jmethodID startRenderCallback = NULL;
void DROIDDEBUG( const char*pFmtStr, ...);

static char* initialFile = NULL;

// keep sequence here so that we know if we have a restart, or just a refresh
static int mapTexture = -1;
static int confidenceCone = -1;

static ttglobal *pGlobal=NULL;

/********************************************
 Initializer thread 
*********************************************/

pthread_t loadFileThread = (pthread_t)0;

void fileLoadThread(void* param) {

	DROIDDEBUG("------------------LOAD THREAD-----------------------");
	if (initialFile == NULL) { DROIDDEBUG("..... iniitialFile NULL");} else {DROIDDEBUG("+++++++++ initialfile NOT null");}
	fwl_OSX_initializeParameters(initialFile);
	DROIDDEBUG("------------------FIN LOAD THREAD-----------------------");
}


/********************************************
*********************************************/


void DROIDDEBUG( const char*pFmtStr, ...)
{
	static char zLog[500];
	
	va_list mrk;
	va_start(mrk,pFmtStr);
	vsprintf(zLog,pFmtStr, mrk);
	va_end(mrk);
	__android_log_print(ANDROID_LOG_INFO,LOG_TAG,zLog);
}

JNIEXPORT jint JNICALL JNI_OnLoad( JavaVM* vm, void* reserved )
{
	DROIDDEBUG("------------------ON LOAD-----------------------");
	g_jvm = vm;
	JNIEnv* ioEnv = NULL;


        if( pGlobal == NULL ) {
                pGlobal = (ttglobal*)fwl_init_instance();
        }


	DROIDDEBUG("------------------FIN ON LOAD-----------------------");
	return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL Java_org_freewrl_FreeWRLLib_initialFile(JNIEnv * env, jobject obj, jstring passedInitialFile)
{
	DROIDDEBUG("------------------INITIAL FILE-----------------------");
	if (initialFile == NULL) { DROIDDEBUG("..... iniitialFile NULL");} else {DROIDDEBUG("+++++++++ initialfile NOT null");}

	const char *cFilename = (*env)->GetStringUTFChars(env, passedInitialFile, NULL);

	/* save a copy of this filename - we can probably free this after the initial request from the NDK rendering
	   library, as it will cache it as well */

	if (initialFile != NULL) free(initialFile);
	initialFile = strdup(cFilename);


	//DROIDDEBUG("cFilename is :%s:",initialFile);

	// step 1:
	fwl_initializeRenderSceneUpdateScene();

        // step 2:  create a thread to handle the file load requests from the library
        if( 0 != pthread_create(&loadFileThread, NULL, (void*)fileLoadThread, (void*)initialFile) )
        {
                DROIDDEBUG("!!Error creating fileloadedThread");
                return;
        }


	// step 3:
	fv_display_initialize();
	(*env)->ReleaseStringUTFChars(env, initialFile, cFilename);
	DROIDDEBUG("------------------END INITIAL FILE-----------------------");
}


JNIEXPORT void JNICALL Java_org_freewrl_FreeWRLLib_init(JNIEnv * env, jobject obj,  jint width, jint height)
{
	DROIDDEBUG("------------------LIB INIT-----------------------");
	if (initialFile == NULL) { DROIDDEBUG("..... iniitialFile NULL");} else {DROIDDEBUG("+++++++++ initialfile NOT null");}

	fwl_setScreenDim(width, height);

	DROIDDEBUG("------------------FIN LIB INIT-----------------------");
}


/* do we WANT a file? return yes/no */
JNIEXPORT jboolean JNICALL Java_org_freewrl_FreeWRLLib_resourceWanted(JNIEnv * env, jobject obj) {
	return fwg_frontEndWantsFileName()!=NULL;
}

/* return the NAME of the resource we want... */
JNIEXPORT jstring JNICALL Java_org_freewrl_FreeWRLLib_resourceNameWanted(JNIEnv *env, jobject obj) {
	DROIDDEBUG("------------------RESOURCE NAME WANTED CALLED----------------------");
	if (initialFile == NULL) { DROIDDEBUG("..... iniitialFile NULL");} else {DROIDDEBUG("+++++++++ initialfile NOT null");}
	DROIDDEBUG(fwg_frontEndWantsFileName());
	return (*env)->NewStringUTF(env,fwg_frontEndWantsFileName());
}

/* return the data associated with the name */
JNIEXPORT void JNICALL Java_org_freewrl_FreeWRLLib_resourceData(JNIEnv * env, jobject this, jstring logThis)
{
	jboolean isCopy;
	const char * szLogThis = (*env)->GetStringUTFChars(env, logThis, &isCopy);

	fwg_frontEndReturningData(szLogThis,strlen(szLogThis));
	(*env)->ReleaseStringUTFChars(env, logThis, szLogThis);
}  

/* return fileDescriptor, offset and length, and read the file here */


#define SUCCESS			         			0
#define ERROR_CODE_CANNOT_OPEN_MYFILE         			100
#define ERROR_CODE_CANNOT_GET_DESCRIPTOR_FIELD			101
#define ERROR_CODE_CANNOT_GET_FILE_DESCRIPTOR_CLASS		102

JNIEXPORT jint JNICALL Java_org_freewrl_FreeWRLLib_resourceFile (JNIEnv * env, jclass thiz, jobject fd_sys, jint off, jint len) {
	jclass fdClass = (*env)->FindClass(env,"java/io/FileDescriptor");
	if (fdClass != NULL){
		jfieldID fdClassDescriptorFieldID = (*env)->GetFieldID(env,fdClass, "descriptor", "I");
		if (fdClassDescriptorFieldID != NULL && fd_sys != NULL){
			jint fd = (*env)->GetIntField(env,fd_sys, fdClassDescriptorFieldID);
			int myfd = dup(fd);
			FILE* myFile = fdopen(myfd, "rb");
			if (myFile){
				char myString[2000];
				unsigned char *myFileData = malloc (len+1);
				size_t frv;
				fseek(myFile, off, SEEK_SET);
				frv = fread (myFileData, (size_t)len, (size_t)1, myFile);

				/* null terminate this; note that for textures, file is from 0 to (len-1) 
				   so final trailing null is of no consequence */
				myFileData[len] = '\0'; 
				fwg_frontEndReturningData(myFileData,len);
				return (jint)SUCCESS;
			}
			else {
				return (jint) ERROR_CODE_CANNOT_OPEN_MYFILE;
			}
		}
		else {
			return (jint)ERROR_CODE_CANNOT_GET_DESCRIPTOR_FIELD;
		}
	}
	else {
		return (jint)ERROR_CODE_CANNOT_GET_FILE_DESCRIPTOR_CLASS;
	}
}

/* do a call of the scenegraph. */
JNIEXPORT void JNICALL Java_org_freewrl_FreeWRLLib_step(JNIEnv * env, jobject obj)
{
    	fwl_RenderSceneUpdateScene();
}


/* reload assets when onSurfaceCreated, but system already loaded */
JNIEXPORT void JNICALL Java_org_freewrl_FreeWRLLib_nextViewpoint(JNIEnv * env, jobject obj)
{
    	fwl_Next_ViewPoint();
}

/* reload assets when onSurfaceCreated, but system already loaded */
JNIEXPORT void JNICALL Java_org_freewrl_FreeWRLLib_reloadAssets(JNIEnv * env, jobject obj)
{
    	fwl_Android_reloadAssets();
}
/* handle touch */
JNIEXPORT void JNICALL Java_org_freewrl_FreeWRLLib_setButDown(JNIEnv *env, jobject obj, int but, int state)
{
        fwl_setButDown(but,state);
}

JNIEXPORT void JNICALL Java_org_freewrl_FreeWRLLib_setLastMouseEvent(JNIEnv *env, jobject obj, int state)
{
        fwl_setLastMouseEvent(state);
}

JNIEXPORT void JNICALL Java_org_freewrl_FreeWRLLib_handleAqua(JNIEnv *env, jobject obj, int but, int state, int x, int y)
{
        fwl_handle_aqua(but,state,x,y);
}

#endif
