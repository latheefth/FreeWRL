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


#define  LOG_TAG    "WRL-"


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


static struct X3D_Group *l_nRN = NULL;


JNIEXPORT void JNICALL Java_awila_com_libWrapper_initLib(JNIEnv* ioEnv, jobject ioThis, int wid, int hei)
{
	DROIDDEBUG(">initLib() : %d,%d",wid,hei);
	
DROIDDEBUG("--setscreendim");
	fwl_setScreenDim(wid,hei);
	
DROIDDEBUG("--display_init");
	if( fwl_display_initialize() == 0 )
	{
		DROIDDEBUG("--Error in fwl_display_initialize" );
		
	}
	
	DROIDDEBUG("<<leaving initLib");
}



JNIEXPORT int JNICALL Java_awila_com_libWrapper_parseX3DFrom(JNIEnv* ioEnv, jobject ioThis, jstring pX3DString)
{
	
	l_nRN = (struct X3D_Group*) createNewX3DNode(NODE_Group);
	
	if( l_nRN == NULL ) return -1;
	
	return X3DParse( l_nRN, pX3DString );
}


JNIEXPORT void JNICALL Java_awila_com_libWrapper_renderScene(JNIEnv* ioEnv, jobject ioThis)
{
	if( NULL != frontEndWantsFileName() )
	{
#define MYSTRING \
"#VRML V2.0 utf8\n" \
"Background {skyAngle        [ 1.07 1.45 1.52 1.57 ]skyColor        [ 0.00 0.00 0.30 0.00 0.00 0.80 0.45 0.70 0.80 0.70 0.50 0.00 1.00 0.00 0.00 ] groundAngle     1.57 groundColor     [ 0.0 0.0 0.0, 0.0 0.7 0.0 ]}" \
        " Shape { appearance Appearance { material Material {} } geometry Cone {}  }"
        
	    fwl_frontEndReturningData(MYSTRING, strlen(MYSTRING));	
	}
	
	RenderSceneUpdateScene();	
}


// Call once at start up
JNIEXPORT void JNICALL Java_awila_com_libWrapper_initScene(JNIEnv* ioEnv, jobject ioThis)
{
	DROIDDEBUG(">>initScene");
	fwl_initializeRenderSceneUpdateScene();
	
    
	DROIDDEBUG(">>initParams");
    fwl_OSX_initializeParameters("http://freewrl.sourceforge.net/test3.wrl");
}


// Call on termination
JNIEXPORT void JNICALL Java_awila_com_libWrapper_termScene(JNIEnv* ioEnv, jobject ioThis)
{
	DROIDDEBUG(">>termScene");
	finalizeRenderSceneUpdateScene();
}



#endif
