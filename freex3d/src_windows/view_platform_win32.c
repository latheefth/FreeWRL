#include <Windows.h>

static int Console_writePrimitive = 0; //for msc_ver, but will test in shared code
int consolefileOpened;// = 0;
//FILE* consolefile;
HANDLE hStdErr; // = NULL;

#ifdef _MSC_VER
#if _MSC_VER < 1500
#define HAVE_VSCPRINTF
#endif

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <wtypes.h>
//a console window should be a single shared process resource
//static HANDLE hStdErr = NULL;
void fwl_setConsole_writePrimitive(int ibool)
{
	//this function used by the dll wrapper in win32 for 
	//some types of applications (not the console program as of Jun1/2011)
	//ppConsoleMessage p = (ppConsoleMessage)gglobal()->ConsoleMessage.prv;
	//if(ibool) p->Console_writePrimitive = 1;
	//else  p->Console_writePrimitive = 0;
	Console_writePrimitive = ibool;
}
void initConsoleH(DWORD pid)
{
	//ppConsoleMessage p = (ppConsoleMessage)gglobal()->ConsoleMessage.prv;
#if _MSC_VER >= 1500

	//p->hStdErr = GetStdHandle(STD_ERROR_HANDLE);
	//if(!p->hStdErr)
	hStdErr = GetStdHandle(STD_ERROR_HANDLE);
	if (!hStdErr)
	if (!AttachConsole(pid))
	{
		DWORD dw = GetLastError();
		if (dw == ERROR_ACCESS_DENIED)
			printf("attachconsole access denied\n");
		else if (dw == ERROR_INVALID_HANDLE)
			printf("attachconsole invalid handle\n");
		else if (dw == ERROR_GEN_FAILURE)
			printf("attachconsole gen failure\n");
		AllocConsole();
	}
	hStdErr = GetStdHandle(STD_ERROR_HANDLE);
	//p->hStdErr = GetStdHandle(STD_ERROR_HANDLE);
	//if(!p->hStdErr) p->hStdErr = -1;
#endif
}

static void initConsole(void)
{
	BOOL ac;
	//ppConsoleMessage p = (ppConsoleMessage)gglobal()->ConsoleMessage.prv;

	//hStdErr = GetStdHandle(STD_ERROR_HANDLE);
	//if(p->hStdErr == NULL)
	if (hStdErr == NULL)
	{
#ifndef ATTACH_PARENT_PROCESS
#define ATTACH_PARENT_PROCESS ((DWORD)-1)
#endif
#if _MSC_VER >= 1500
		if (!AttachConsole(ATTACH_PARENT_PROCESS))
		{
			DWORD dw = GetLastError();
			if (dw == ERROR_ACCESS_DENIED)
				printf("attachconsole access denied\n");
			else if (dw == ERROR_INVALID_HANDLE)
				printf("attachconsole invalid handle\n");
			else if (dw == ERROR_GEN_FAILURE)
				printf("attachconsole gen failure\n");
			ac = AllocConsole();
		}
#endif
		//p->hStdErr = GetStdHandle(STD_ERROR_HANDLE);
		hStdErr = GetStdHandle(STD_ERROR_HANDLE);
	}
}
void writeToWin32Console(char *buff)
{
	//ppConsoleMessage p = (ppConsoleMessage)gglobal()->ConsoleMessage.prv;

	DWORD cWritten;
	//if (p->hStdErr == NULL)
	if (hStdErr == NULL)
		initConsole();
	/* not C console - more low level windows SDK API */
	//WriteConsoleA(p->hStdErr, buff, strlen(buff),&cWritten, NULL);
	WriteConsoleA(hStdErr, buff, strlen(buff), &cWritten, NULL);
}
//stub for vc7
int DEBUG_FPRINTF(const char *fmt, ...)
{
	return 0;
}

#endif

//from dllfreewrl init

#ifdef CONSOLE
fwl_setConsole_writePrimitive(1);
DWORD pid = GetCurrentProcessId();
initConsoleH(pid);
swDebugf("after fwl_initFreeWRL\n");
#endif
