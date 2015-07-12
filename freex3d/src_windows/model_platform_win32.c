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
	// /* the windows getlocaltime has a granularity of 1ms at best. 
	// There are a gazillion time functions in windows so I isolated it here in case I got it wrong*/
	///* win32 there are some higher performance timer functions (win95-vista)
	//but a system might not support it - lpFrequency returns 0 if not supported
	//BOOL QueryPerformanceFrequency( LARGE_INTEGER *lpFrequency );
	//BOOL QueryPerformanceCounter( LARGE_INTEGER *lpPerformanceCount );
	//*/
	SYSTEMTIME mytimet; /*winNT and beyond */

#ifdef TRUE //HAVE_ULARGE_INTEGER
	if(1){
		/*freewrl blackscreens overnight, the '/' viewpoint and world coordinates go to NaNs:
			Position[0.0000, 0.0000, 10.0000]
			Quaternion[-1.#IND, -1.#IND, -1.#IND, -1.#IND]
			Orientation[-1.#IND, -1.#IND, -1.#IND, -1.#IND]
			World Coordinates of Avatar [1.#QNB, 1.#QNB -1.#IND]
		one hypothesis: -ve delta time or zero time throws off calculation at midnight
		MS docs say don't subtract filetimes - convert to __int64 first
		*/
		FILETIME mytimef;
		ULARGE_INTEGER mytimeu;
		static ULARGE_INTEGER mystarttimeu = {0};
		double dtime;

		GetSystemTime(&mytimet);
		SystemTimeToFileTime(&mytimet,&mytimef);
		//milli 10-3 micro 10-6 nano 10-9 pico 10-12 femto 10-15 atto 10-18
		//filetime is 64bit int in 100 nano seconds since year 1600
		mytimeu.HighPart = mytimef.dwHighDateTime;
		mytimeu.LowPart = mytimef.dwLowDateTime;
		if(mystarttimeu.QuadPart == 0){
			mystarttimeu.QuadPart = mytimeu.QuadPart;
		}
		mytimeu.QuadPart -= mystarttimeu.QuadPart;
		dtime = (double)(mytimeu.QuadPart);
		dtime = dtime * .0000001; //100 nano to seconds
		return dtime;
	}
#endif

	 GetLocalTime(&mytimet);
	 return (double) mytimet.wHour*3600.0 + (double)mytimet.wMinute*60.0 + (double)mytimet.wSecond + (double)mytimet.wMilliseconds/1000.0;
	//FILETIME ft;
	//ULARGE_INTEGER ul;
	//GetSystemTimeAsFileTime(&ft); //higher resolution 100nanosec but systemtime can change with internet updates
	//ul.LowPart = ft.dwLowDateTime;
	//ul.HighPart = ft.dwHighDateTime;
	//return ((double)(ul.QuadPart))*.0000001; //they say 100 nanosecond resolution, but .1 nanoseconds seems to work
//#if _MSC_VER > 1500
//	//win vista,7,8.. should have 64
//	return ((double)(GetTickCount64())) * .001; //winxp doesn't have this in millisec with 10 to 16ms resolution, lower resolution but no internet updates
//#else
//	//win XP and lower 
//	return ((double)(GetTickCount())) * .001; //
//#endif
}


