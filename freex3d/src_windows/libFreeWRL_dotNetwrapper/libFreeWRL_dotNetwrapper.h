// libFreeWRL_dotNetwrapper.h

#pragma once
#include < stdio.h >
#include < stdlib.h >
#include<vcclr.h>
using namespace System;

using namespace System::Runtime::InteropServices;
#include "dllFreeWRL.h"
[DllImport("Kernel32.dll")]    
extern int AllocConsole();    
[DllImport("Kernel32.dll")]    
extern int FreeConsole();    
[DllImport("Kernel32.dll")]    
extern int AttachConsole(UInt32 dwProcessId);


namespace libFreeWRL_dotNetwrapper {

	public ref class FreewrlLib
	{
	private:
		CdllFreeWRL *dllfreewrl;
		char * string2chars(System::String^ sstring){
			//const char* str2 = (char*)(void*)Marshal::StringToHGlobalAnsi(sstring);
			const char* str2 = (char*)(void*)Marshal::StringToHGlobalAnsi(sstring);
			int len = sstring->Length;
			char* temp = (char*)malloc(len+1);
			for(int i=0;i<len;i++)
			{
				temp[i] = str2[i];
			}
			temp[len] = 0;
			return temp;
		}

public: 
		// TODO: Add your methods for this class here.

		enum class KeyAction {KEYDOWN=2,KEYUP=3,KEYPRESS=1};
		enum class MouseAction {MOUSEMOVE=6,MOUSEDOWN=4,MOUSEUP=5};
		enum class MouseButton {LEFT=1,MIDDLE=2,RIGHT=3,NONE=0};
		System::String ^message;
		FreewrlLib(int width, int height, IntPtr windowhandle, bool bEai)
		{
			//void *handle;
			//handle = (void*)windowhandle.ToInt32;
			dllfreewrl = new CdllFreeWRL();
			dllfreewrl->onInit(width, height, windowhandle.ToPointer(), bEai, false);
			//message = handle.ToString();
			// Hide the console window    FreeConsole();
			//AllocConsole();    
			Console::WriteLine("Write to the console!");   
			Console::WriteLine("and again");
		}
		void onLoad(String^ Scene_url)
		{
			char *chars = string2chars(Scene_url);
			dllfreewrl->onLoad(chars); 
			if(chars) free(chars);
		}
        void onResize(int width,int height)
		{
			dllfreewrl->onResize(width,height);
		}
        int onMouse(MouseAction a,MouseButton b,int x, int y)
		{
			message = x.ToString()+y.ToString();
			return dllfreewrl->onMouse((int)a,(int)b,x,y);
		}
        void onKey(KeyAction a,int keyValue)
		{
			int act = (int)a;
			if(keyValue != 'q')
				dllfreewrl->onKey(act,keyValue);
		}
        void onTick(int interval)
		{
			//dllfreewrl->onTick(interval);
		}
		void commandline(String^ cmdline){
			char *chars = string2chars(cmdline);
			dllfreewrl->commandline(chars);
			if(chars) free(chars);
		}
		// http://msdn.microsoft.com/en-us/library/ms177197.aspx
		// somehow we need to deterministically release unmanaged resources
		// is this the right way, or will this way skip Dispose?
		~FreewrlLib() 
		{
			this->!FreewrlLib();
		}
		!FreewrlLib() 
		{
			//Flush();
			//fclose(file);
			message += "closing";
			dllfreewrl->onClose();
			//delete dllfreewrl;
		}
	};
}
