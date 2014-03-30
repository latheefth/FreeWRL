#include <windows.h>

#include <windows.h>
void usleep(int us){
	Sleep((us) / 1000);
}
void sleep(int ms){
	Sleep(ms);
}

double Time1970sec()
{
	// SYSTEMTIME mytimet; /*winNT and beyond */
	// /* the windows getlocaltime has a granularity of 1ms at best. 
	// There are a gazillion time functions in windows so I isolated it here in case I got it wrong*/
	///* win32 there are some higher performance timer functions (win95-vista)
	//but a system might not support it - lpFrequency returns 0 if not supported
	//BOOL QueryPerformanceFrequency( LARGE_INTEGER *lpFrequency );
	//BOOL QueryPerformanceCounter( LARGE_INTEGER *lpPerformanceCount );
	//*/

	// GetLocalTime(&mytimet);
	// return (double) mytimet.wHour*3600.0 + (double)mytimet.wMinute*60.0 + (double)mytimet.wSecond + (double)mytimet.wMilliseconds/1000.0;
	//FILETIME ft;
	//ULARGE_INTEGER ul;
	//GetSystemTimeAsFileTime(&ft); //higher resolution 100nanosec but systemtime can change with internet updates
	//ul.LowPart = ft.dwLowDateTime;
	//ul.HighPart = ft.dwHighDateTime;
	//return ((double)(ul.QuadPart))*.0000001; //they say 100 nanosecond resolution, but .1 nanoseconds seems to work
#if _MSC_VER > 1500
	//win vista,7,8.. should have 64
	return ((double)(GetTickCount64())) * .001; //winxp doesn't have this in millisec with 10 to 16ms resolution, lower resolution but no internet updates
#else
	//win XP and lower 
	return ((double)(GetTickCount())) * .001; //
#endif
}


