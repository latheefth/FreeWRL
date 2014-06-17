/*


???

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


#ifndef __FREEWRL_JS_JSCRIPT_H__
#define __FREEWRL_JS_JSCRIPT_H__

//#include <system_js.h>

void kill_javascript(void);
void cleanupDie(int num, const char *msg);
void JSMaxAlloc(void);
void JSInit(int num);
void SaveScriptText(int num, const char *text);
void JSInitializeScriptAndFields (int num);
void JSCreateScriptContext(int num);
void process_eventsProcessed();

int isScriptControlOK(int actualscript);
int isScriptControlInitialized(int actualscript);
void js_cleanup_script_context(int counter);
struct CRscriptStruct *getScriptControlIndex(int actualscript);
void initializeAnyScripts();



void * SFNodeNativeNew(void);
int SFNodeNativeAssign(void *top, void *fromp);
void * SFColorRGBANativeNew(void);
void SFColorRGBANativeAssign(void *top, void *fromp);
void * SFColorNativeNew(void);
void SFColorNativeAssign(void *top, void *fromp);
void * SFImageNativeNew(void);
void SFImageNativeAssign(void *top, void *fromp);
void * SFRotationNativeNew(void);
void SFRotationNativeAssign(void *top, void *fromp);
void * SFVec2fNativeNew(void);
void SFVec2fNativeAssign(void *top, void *fromp);
void * SFVec3fNativeNew(void);
void SFVec3fNativeAssign(void *top, void *fromp);
void * SFVec3dNativeNew(void);
void SFVec3dNativeAssign(void *top, void *fromp);
void * SFVec4fNativeNew(void);
void SFVec4fNativeAssign(void *top, void *fromp);
void * SFVec4dNativeNew(void);
void SFVec4dNativeAssign(void *top, void *fromp);
void SaveScriptField (int num, indexT kind, indexT type, const char* field, union anyVrml value);


#endif /* __FREEWRL_JS_JSCRIPT_H__ */
