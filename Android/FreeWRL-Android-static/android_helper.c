/* androidHelper.c */

#if defined( _ANDROID )

#include <config.h>
#include <system.h>
#include <internal.h>

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


#define  LOG_TAG    "WRL-"

// globals

static JavaVM* g_jvm = NULL;
static jclass fileCallbackClass = NULL;
static jmethodID fileLoadCallback = NULL;
static jmethodID startRenderCallback = NULL;
static ttglobal *pGlobal=NULL;



void _fileLoadThread(void* param);


void DROIDDEBUG( const char*pFmtStr, ...)
{
	static char zLog[500];
	
	va_list mrk;
	va_start(mrk,pFmtStr);
	vsprintf(zLog,pFmtStr, mrk);
	va_end(mrk);
	__android_log_print(ANDROID_LOG_INFO,LOG_TAG,zLog);
}

void checkGlError(const char* op) 
{
	GLint error;
	
    for (error = glGetError(); error; error = glGetError())
    {
        DROIDDEBUG("after %s() glError (0x%x)\n", op, error);
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad( JavaVM* vm, void* reserved )
{
	DROIDDEBUG("------------------ON LOAD-----------------------");
	g_jvm = vm;
	JNIEnv* ioEnv = NULL;

	DROIDDEBUG("Registering callbacks...");
	if( (*g_jvm)->GetEnv(g_jvm,(void**)&ioEnv, JNI_VERSION_1_6) == JNI_OK )
	{
		fileCallbackClass = (*ioEnv)->FindClass(ioEnv,"awila/com/Callbacks");
		
		if( fileCallbackClass!=NULL )
		{
			fileLoadCallback = (*ioEnv)->GetStaticMethodID(ioEnv, fileCallbackClass, "getLocalFileForRemote", "(Ljava/lang/String;)Ljava/lang/String;");
			startRenderCallback = (*ioEnv)->GetStaticMethodID(ioEnv, fileCallbackClass, "startRender", "()V");
			
			if( fileLoadCallback == NULL )
			{
				DROIDDEBUG( "Could not GetMethodID for (s) getLocaFileForRemote(s)");
			}
			else
			{
				DROIDDEBUG( "have getLocalFileForRemote method!");
				
			}
		}
		else
		{
			DROIDDEBUG( "Could not find class 'awila/com/Callbacks'");
		}
	}
	else
	{
		DROIDDEBUG( "Failed to get environment.");
	}
	
	return JNI_VERSION_1_6;
}


JNIEXPORT void JNICALL Java_awila_com_libWrapper_initLib(JNIEnv* ioEnv, jobject ioThis, int wid, int hei)
{
	ttglobal *pGlobal=NULL;
	
	if( pGlobal == NULL ) {
		pGlobal = (ttglobal*)fwl_init_instance();
	}
	
	DROIDDEBUG(">initLib() : %d,%d",wid,hei);

	fwl_setp_width(wid);
	fwl_setp_height(hei);
	
	fwl_setScreenDim(wid,hei);
	
	if( fv_display_initialize() == 0 )
	{
		DROIDDEBUG("--Error in fwl_display_initialize" );
		
	}
	
	DROIDDEBUG("<<leaving initLib");
}


JNIEXPORT void JNICALL Java_awila_com_libWrapper_renderScene(JNIEnv* ioEnv, jobject ioThis)
{
	fwl_RenderSceneUpdateScene();	
}


// Call once at start up
JNIEXPORT void JNICALL Java_awila_com_libWrapper_initScene(JNIEnv* ioEnv, jobject ioThis)
{
	DROIDDEBUG(">>initScene");
	fwl_initializeRenderSceneUpdateScene();
	
    
	DROIDDEBUG(">>initialize");
    //fwl_OSX_initializeParameters("http://freewrl.sourceforge.net/test3.wrl");
    
    // added ANDROID specific initialisation as we don't want the render thread loading files...
    fwl_ANDROID_initialize();
    
}


// Call on termination
JNIEXPORT void JNICALL Java_awila_com_libWrapper_termScene(JNIEnv* ioEnv, jobject ioThis)
{
	DROIDDEBUG(">>termScene");
	finalizeRenderSceneUpdateScene();
}

pthread_t loadFileThread = (pthread_t)0;

JNIEXPORT void JNICALL Java_awila_com_libWrapper_loadFile( JNIEnv* ioEnv, jobject ioThis, jstring sFilename)
{
	resource_item_t* res;
	char* pFilename = NULL;
	jboolean jb;
	
	pFilename = (char*)((*ioEnv)->GetStringUTFChars(ioEnv, sFilename, &jb));
	DROIDDEBUG(">>loadFile( %s )", pFilename );
	
	if( 0 != loadFileThread )
	{
		DROIDDEBUG("Attempt to load a new file while waiting for a file to load!");
		(*ioEnv)->ReleaseStringUTFChars(ioEnv, sFilename, pFilename );
		return;
	}
		
    /* Give the main argument to the resource handler */
    res = resource_create_single(pFilename);

	// create a thread to handle the file load requests from the library
	if( 0 != pthread_create(&loadFileThread, NULL, (void*)_fileLoadThread, res ) )
	{
		DROIDDEBUG("!!Error creating fileloadedThread");
		(*ioEnv)->ReleaseStringUTFChars(ioEnv, sFilename, pFilename );
		return;
	}

    send_resource_to_parser(res);
    
	(*ioEnv)->ReleaseStringUTFChars(ioEnv, sFilename, pFilename );
}

void _fileLoadThread(void* param)
{
	jboolean jb;
	resource_item_t* res = (resource_item_t*)param;
	JNIEnv* ioEnv = NULL;

    while ((!res->complete) && (res->status != ress_failed) && (res->status != ress_not_loaded))
    {
    	const char* pFilename = (const char*)fwg_frontEndWantsFileName();
    	if( NULL != pFilename )
    	{
    		DROIDDEBUG( "Want file: %s", pFilename);
    		if( (*g_jvm)->AttachCurrentThread(g_jvm, &ioEnv,NULL) == JNI_OK )
    		{
				jstring strFilename;
				jstring result;
				
	    		strFilename = (*ioEnv)->NewStringUTF(ioEnv, pFilename);
		    		
	    		result = (*ioEnv)->CallStaticObjectMethod(ioEnv,fileCallbackClass,fileLoadCallback,strFilename );
		    		
	    		
	    		const char* pCacheFile = ((*ioEnv)->GetStringUTFChars(ioEnv, result, &jb));
					    		
	    		DROIDDEBUG( "Returning cached local file: %s", pCacheFile );
	    		
	    		fwg_frontEndReturningLocalFile( (char*)pCacheFile, 1);
	    		
	    		(*ioEnv)->ReleaseStringUTFChars(ioEnv, result, pCacheFile );
		    }
    		else
    		{
    			DROIDDEBUG( "Error obtaining environment");
    			return;
    		}    		
    	}
    	else
    	{
			usleep(500);
		}
    }

    /* did this load correctly? */
    if (res->status == ress_not_loaded) {
DROIDDEBUG("-- it's not loaded!");    
		ConsoleMessage ("FreeWRL: Problem loading file \"%s\"", res->request);
    }

    if (res->status == ress_failed) {
		printf("load failed %s\n", res->request);
		ConsoleMessage ("FreeWRL: unknown data on command line: \"%s\"", res->request);
    } else {
    	/* tell the new world which viewpoint to go to */
    	if (res->afterPoundCharacters != NULL) {
			fwl_gotoViewpoint(res->afterPoundCharacters);
			/* Success! 
			printf("loaded %s\n", initialURL); */
		}
    }
    loadFileThread = (pthread_t)0;
    
	(*ioEnv)->CallStaticVoidMethod(ioEnv,fileCallbackClass,startRenderCallback);
	
	(*g_jvm)->DetachCurrentThread(g_jvm);
    
    return;
}

JNIEXPORT void JNICALL Java_awila_com_libWrapper_cachedFile(JNIEnv* ioEnv, jobject ioThis, jstring filename)
{
	jboolean jb;
	FILE *fp = NULL;
	struct stat st;
	int len = 0;
	
	const char* pCacheFile = ((*ioEnv)->GetStringUTFChars(ioEnv, filename, &jb));
		    		
/*
	DROIDDEBUG( ">>cachedFile is : %s", pCacheFile );

	stat(pCacheFile, &st);
	len = (int)st.st_size;		

	DROIDDEBUG("File length is %d", len );
	
	
	if( (fp=fopen(pCacheFile,"rb")) != NULL )
	{
		char* data = (char*)malloc(len+1);
		
		fread(data,1,len,fp);
		DROIDDEBUG("read the file" );
		
		fclose(fp);
		
		DROIDDEBUG("Submitting data");

		fwg_frontEndReturningData( data,len );
	}
*/
		
	fwg_frontEndReturningLocalFile( (char*)pCacheFile, 1);
	
//	(*ioEnv)->ReleaseStringUTFChars(ioEnv, filename, pCacheFile );
}


JNIEXPORT void JNICALL Java_awila_com_libWrapper_getViewerPosition(JNIEnv* ioEnv, jobject ioThis, jobject jfaXYZ)
{
	float * pXYZ = (float*)(*ioEnv)->GetDirectBufferAddress(ioEnv,jfaXYZ);
	
	if( pXYZ != NULL )
	{
//		mainloop_getPos(pXYZ);
	}
	else
	{
		DROIDDEBUG("!!ERROR in GetDirectBufferAddress");
	}
}

JNIEXPORT void JNICALL Java_awila_com_libWrapper_setViewerPosition(JNIEnv* ioEnv, jobject ioThis, float x, float y, float z)
{
//	mainloop_setPos(x,y,z);
}

#endif
