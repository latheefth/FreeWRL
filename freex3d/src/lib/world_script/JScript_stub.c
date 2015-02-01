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
#if defined(JAVASCRIPT_STUB)

typedef int indexT;
typedef union anyVrml{
	int nothing;
} anyVrml;
typedef struct X3D_Node;
typedef struct X3D_Proto;
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
void JSInitializeScriptAndFields (int num) {return;}
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
void SaveScriptField (int num, indexT kind, indexT type, const char* field, union anyVrml value){return;}
int runQueuedDirectOutputs(){
	//stub for SM and STUBS (DUK has it)
	return 0;
}
#endif /* defined(JAVASCRIPT_STUB) */
