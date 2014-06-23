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

void kill_javascript(void);
void JSInit(struct Shader_Script *script); //int num);
void SaveScriptText(int num, const char *text);
void process_eventsProcessed();
void js_cleanup_script_context(int counter);
int jsActualrunScript(int num, char *script);
void JSInitializeScriptAndFields (int num);
void JSCreateScriptContext(int num);
void SaveScriptField (int num, indexT kind, indexT type, const char* field, union anyVrml value);

void SaveScriptField (int num, indexT kind, indexT type, const char* field, union anyVrml value);
void js_setField_javascriptEventOut_B(union anyVrml* any, int fieldType, unsigned len, int extraData, int actualscript);
void js_setField_javascriptEventOut(struct X3D_Node *tn,unsigned int tptr,  int fieldType, unsigned len, int extraData, int actualscript);

void setScriptECMAtype(int num);
int get_valueChanged_flag (int fptr, int actualscript);
void resetScriptTouchedFlag(int actualscript, int fptr);
void set_one_ECMAtype (int tonode, int toname, int dataType, void *Data, int datalen);
void set_one_MultiElementType (int tonode, int tnfield, void *Data, int dataLen);
void set_one_MFElementType(int tonode, int toname, int dataType, void *Data, int datalen);

int jsIsRunning();
void jsShutdown();
void JSDeleteScriptContext(int num);
void InitScriptField(int num, indexT kind, indexT type, const char* field, union anyVrml value);
void jsClearScriptControlEntries(int num); //struct CRscriptStruct *ScriptControl);


#endif /* __FREEWRL_JS_JSCRIPT_H__ */
