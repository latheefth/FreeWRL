#include <config.h>
#if defined(JAVASCRIPT_STUB)

typedef int indexT;
typedef union anyVrml{
	int nothing;
} anyVrml;
#include "JScript.h"
void JScript_init(void *t){}
void jsVRMLBrowser_init(void *t){}
void jsUtils_init(void *t){}
void jsVRMLClasses_init(void *t){}

/* stubs, when you don't have a javascript engine */
//void kill_javascript(void){return;}
//void JSInit(int num){return;}
//void SaveScriptText(int num, const char *text){return;}
void process_eventsProcessed(){return;}
void js_cleanup_script_context(int counter){return;}
int jsActualrunScript(int num, char *script){return 0;}
//void JSInitializeScriptAndFields (int num){return;}
void JSCreateScriptContext(int num){return;}

//void SaveScriptField (int num, indexT kind, indexT type, const char* field, union anyVrml value){return;}
void js_setField_javascriptEventOut_B(union anyVrml* any, int fieldType, unsigned len, int extraData, int actualscript){return;}
void js_setField_javascriptEventOut(struct X3D_Node *tn,unsigned int tptr,  int fieldType, unsigned len, int extraData, int actualscript){return;}

void setScriptECMAtype(int num){return;}
int get_valueChanged_flag (int fptr, int actualscript){return 0;}
void resetScriptTouchedFlag(int actualscript, int fptr){return;}
void set_one_ECMAtype (int tonode, int toname, int dataType, void *Data, int datalen){return;}
void set_one_MultiElementType (int tonode, int tnfield, void *Data, int dataLen){return;}
void set_one_MFElementType(int tonode, int toname, int dataType, void *Data, int datalen){return;}
int jsIsRunning(){return 1;}
void JSDeleteScriptContext(int num){return;}
void jsShutdown(){return;}
void InitScriptField(int num, indexT kind, indexT type, const char* field, union anyVrml value)
{return;}
void jsClearScriptControlEntries(int num){return;}

#endif /* defined(JAVASCRIPT_STUB) */
