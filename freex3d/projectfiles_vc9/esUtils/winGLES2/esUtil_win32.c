//
// Book:      OpenGL(R) ES 2.0 Programming Guide
// Authors:   Aaftab Munshi, Dan Ginsburg, Dave Shreiner
// ISBN-10:   0321502795
// ISBN-13:   9780321502797
// Publisher: Addison-Wesley Professional
// URLs:      http://safari.informit.com/9780321563835
//            http://www.opengles-book.com
//

// esUtil_win32.c
//
//    This file contains the Win32 implementation of the windowing functions. 


///
// Includes
//
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "esUtil.h"

//////////////////////////////////////////////////////////////////
//
//  Private Functions
//
//

///
//  ESWindowProc()
//
//      Main window procedure
//
#define ButtonPress     4
#define ButtonRelease   5
#define MotionNotify    6
#define KEYPRESS 1
#define KEYDOWN 2
#define KEYUP 3

static int mouseX=0, mouseY=0;
LRESULT WINAPI ESWindowProc ( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) 
{
   LRESULT  lRet = 1; 
	int mev, butnum;
	mev = butnum = 0;

   switch (uMsg) 
   { 
      case WM_CREATE:
         break;

      case WM_PAINT:
         {
            ESContext *esContext = (ESContext*)(LONG_PTR) GetWindowLongPtr ( hWnd, GWL_USERDATA );
            
            if ( esContext && esContext->drawFunc )
               esContext->drawFunc ( esContext );
            
            ValidateRect( esContext->hWnd, NULL );
         }
         break;

      case WM_DESTROY:
         PostQuitMessage(0);             
         break; 
	  case WM_SIZE:
		  {
			  RECT rect;
            ESContext *esContext = (ESContext*)(LONG_PTR) GetWindowLongPtr ( hWnd, GWL_USERDATA );
			GetClientRect(hWnd, &rect); 
            
            if ( esContext && esContext->resizeFunc )
               esContext->resizeFunc ( esContext, (int)rect.right, (int) rect.bottom );

			//fwl_setScreenDim(rect.right,rect.bottom);
			//fwl_setp_width(rect.right);
			//fwl_setp_height(rect.bottom);
			break;
		  }

		case WM_KEYDOWN:
		case WM_KEYUP: 
			{
			int updown;
		    char kp;
			static int shiftState = 0;

            ESContext *esContext = (ESContext*)(LONG_PTR) GetWindowLongPtr ( hWnd, GWL_USERDATA );
            

			//lkeydata = lParam;
			updown = KEYDOWN;
			if(uMsg==WM_KEYUP) updown = KEYUP;
			if(updown==KEYDOWN)
				if(lParam & 1 << 30) 
					break; //ignor - its an auto-repeat
			//kp = (char)wParam; 
			kp = (char)wParam; 
			//if(kp >= 'A' && kp <= 'Z' && shiftState ==0 ) kp = (char)tolower(wParam); //the F1 - F12 are small case ie y=121=F1
			//printf("      wParam %d %x\n",wParam, wParam);
			//x3d specs http://www.web3d.org/x3d/specifications/ISO-IEC-19775-1.2-X3D-AbstractSpecification/index.html
			//section 21.4.1 has a table of KeySensor ActionKey values which we must map to at some point
			// http://msdn.microsoft.com/en-us/library/ms646268(VS.85).aspx windows keyboard messages
			switch (wParam) 
			{ 
				//case VK_LEFT: 
				//	kp = 'j';//19;
				//	break; 
				//case VK_RIGHT: 
				//	kp = 'l';//20;
				//	break; 
				//case VK_UP: 
				//	kp = 'p';//17;
				//	break; 
				//case VK_DOWN: 
				//	kp =  ';';//18;
				//	break; 
				//case -70:
				//	kp = ';';
				//	break;
				case VK_SHIFT: //0x10
					if(updown==KEYDOWN) shiftState = 1;
					if(updown==KEYUP) shiftState = 0;
				case VK_OEM_1:
					kp = ';'; //could be : or ; but tolower won't lowercase it, but returns same character if it can't
					break;
				default:
					break;
			}
			//fwl_do_keyPress(kp, updown); 
            if ( esContext && esContext->keyFunc )
	            esContext->keyFunc ( esContext, (unsigned char) kp,
		                             updown, shiftState );

			}
		break; 
      
      case WM_CHAR:
         {
            //POINT      point;
            ESContext *esContext = (ESContext*)(LONG_PTR) GetWindowLongPtr ( hWnd, GWL_USERDATA );
            
            //GetCursorPos( &point );
			//point.x = mouseX;
			//point.y = mouseY;

            if ( esContext && esContext->keyFunc )
	            esContext->keyFunc ( esContext, (unsigned char) wParam,
		                             (int) KEYPRESS, (int) 0 );
		}
         break;


	/* Mouse events, processed */
    case WM_LBUTTONDOWN:
		butnum = 1;
		mev = ButtonPress;
		break;
    case WM_MBUTTONDOWN:
		butnum = 2;
		mev = ButtonPress;
		break;
    case WM_RBUTTONDOWN:
		butnum = 3;
		mev = ButtonPress;
		break;
    case WM_LBUTTONUP:
		butnum = 1;
		mev = ButtonRelease;
		break;
    case WM_MBUTTONUP:
		butnum = 2;
		mev = ButtonRelease;
		break;
    case WM_RBUTTONUP:
		butnum = 3;
		mev = ButtonRelease;
		break;
    case WM_MOUSEMOVE:
    {
		POINTS pz;
		pz= MAKEPOINTS(lParam); 
		mouseX = pz.x;
		mouseY = pz.y;
    } 
    mev = MotionNotify;
    break;
         
      default: 
         lRet = DefWindowProc (hWnd, uMsg, wParam, lParam); 
         break; 
   } 
    if(mev)
    {
		POINT point;
		ESContext *esContext = (ESContext*)(LONG_PTR) GetWindowLongPtr ( hWnd, GWL_USERDATA );
		//GetCursorPos( &point );
		point.x = mouseX;
		point.y = mouseY;
		if ( esContext && esContext->mouseFunc )
	            esContext->mouseFunc ( esContext, mev, butnum, 
		                             (int) point.x, (int) point.y );
    }

   return lRet; 
}

//////////////////////////////////////////////////////////////////
//
//  Public Functions
//
//

///
//  WinCreate()
//
//      Create Win32 instance and window
//
#include <stdio.h>
GLboolean WinCreate ( ESContext *esContext, const char *title )
{
   WNDCLASS wndclass = {0}; 
   DWORD    wStyle   = 0;
   RECT     windowRect;
   /*HWND ghWnd;*/
   HINSTANCE hInstance = GetModuleHandle(NULL);


   wndclass.style         = CS_OWNDC;
   wndclass.lpfnWndProc   = (WNDPROC)ESWindowProc; 
   wndclass.hInstance     = hInstance; 
   wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); 
   wndclass.lpszClassName = "opengles2.0"; 

   if (!RegisterClass (&wndclass) ) 
   {
      return FALSE; 
	  printf("Ouch - can't RegisterClass\n");
   }

   wStyle = WS_VISIBLE | WS_POPUP | WS_BORDER | WS_SYSMENU | WS_CAPTION;
   wStyle |= WS_SIZEBOX; //makes it resizable 

   // Adjust the window rectangle so that the client area has
   // the correct number of pixels
   windowRect.left = 0;
   windowRect.top = 0;
   windowRect.right = esContext->width;
   windowRect.bottom = esContext->height;

   AdjustWindowRect ( &windowRect, wStyle, FALSE );

   esContext->hWnd = CreateWindow(
                         "opengles2.0",
                         title,
                         wStyle,
                         0,
                         0,
                         windowRect.right - windowRect.left,
                         windowRect.bottom - windowRect.top,
                         NULL,
                         NULL,
                         hInstance,
                         NULL);


   // Set the ESContext* to the GWL_USERDATA so that it is available to the 
   // ESWindowProc
   SetWindowLongPtr (  esContext->hWnd, GWL_USERDATA, (LONG) (LONG_PTR) esContext );


   if ( esContext->hWnd == NULL )
      return GL_FALSE;

   ShowWindow ( esContext->hWnd, TRUE );
   //printf("showing window\n");
   return GL_TRUE;
}

///
//  winLoop()
//
//      Start main windows loop
//
void WinLoop ( ESContext *esContext )
{
   MSG msg = { 0 };
   int done = 0;
   DWORD lastTime = GetTickCount();
   
   while (!done)
   {
      int gotMsg = (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0);
      DWORD curTime = GetTickCount();
      float deltaTime = (float)( curTime - lastTime ) / 1000.0f;
      lastTime = curTime;

      if ( gotMsg )
      {
         if (msg.message==WM_QUIT)
         {
             done=1; 
         }
         else
         {
             TranslateMessage(&msg); 
             DispatchMessage(&msg); 
         }
      }
      else
         SendMessage( esContext->hWnd, WM_PAINT, 0, 0 );

      // Call update function if registered
      if ( esContext->updateFunc != NULL )
         esContext->updateFunc ( esContext, deltaTime );
   }
}
//#include <windows.h> //Commdlg.h
#include <Commdlg.h>
#include <Cderr.h>
#include <malloc.h>
char *strBackslash2fore(char *str);
char* esPickFile(HWND hwnd)
{
//	http://msdn.microsoft.com/en-us/library/windows/desktop/ms646927(v=vs.85).aspx
	char * fname;
	OPENFILENAME *LPOPENFILENAME;
	//               123456789 123456789 123456 789 1234567 8 9
	char * filter = "web3d Files (*.x3d;*.wrl) \0*.x3d;*.wrl\0\0";
	fname = (char *)malloc(4096);
	memset(fname,0,4096);
	LPOPENFILENAME = (OPENFILENAME *)malloc(sizeof(OPENFILENAME));
	memset(LPOPENFILENAME,0,sizeof(OPENFILENAME));

	LPOPENFILENAME->lStructSize = sizeof(OPENFILENAME);
	LPOPENFILENAME->lpstrFilter = filter;
	LPOPENFILENAME->lpstrFile = (LPSTR)fname;
	LPOPENFILENAME->nMaxFile = (DWORD)(1024 - 1);

	if(GetOpenFileName(LPOPENFILENAME))
	{
		fname = strBackslash2fore(fname);
	}else{
		DWORD error = CommDlgExtendedError();
		switch(error){
			case CDERR_DIALOGFAILURE: printf("CDERR_DIALOGFAILURE");break;
			case CDERR_FINDRESFAILURE: printf("CDERR_FINDRESFAILURE");break;
			case CDERR_INITIALIZATION: printf("CDERR_INITIALIZATION");break;
			case CDERR_LOADRESFAILURE: printf("CDERR_LOADRESFAILURE");break;
			case CDERR_LOADSTRFAILURE: printf("CDERR_LOADSTRFAILURE");break;
			case CDERR_LOCKRESFAILURE: printf("CDERR_LOCKRESFAILURE");break;
			case CDERR_MEMALLOCFAILURE: printf("CDERR_MEMALLOCFAILURE");break;
			case CDERR_MEMLOCKFAILURE: printf("CDERR_MEMLOCKFAILURE");break;
			case CDERR_NOHINSTANCE: printf("CDERR_NOHINSTANCE");break;
			case CDERR_NOHOOK: printf("CDERR_NOHOOK");break;
			case CDERR_NOTEMPLATE: printf("CDERR_NOTEMPLATE");break;
			case CDERR_STRUCTSIZE: printf("CDERR_STRUCTSIZE");break;
			case FNERR_BUFFERTOOSMALL: printf("FNERR_BUFFERTOOSMALL");break;
			case FNERR_INVALIDFILENAME: printf("FNERR_INVALIDFILENAME");break;
			case FNERR_SUBCLASSFAILURE: printf("FNERR_SUBCLASSFAILURE");break;
			default: printf("default");break;
		}
		free(fname);
		fname = 0;
	}
	return fname;
}
#include "resource.h"
//message handler for URL dialog
static char szItemName[2048]; // receives name of item to delete. 
 
BOOL CALLBACK URLItemProc(HWND hwndDlg, 
                             UINT message, 
                             WPARAM wParam, 
                             LPARAM lParam) 
{ 
    switch (message) 
    { 
		case WM_INITDIALOG:
			SetDlgItemText(hwndDlg,IDC_EDIT_URL,szItemName);
			return TRUE;

        case WM_COMMAND: 
            switch (LOWORD(wParam)) 
            { 
                case IDOK: 
                    if (!GetDlgItemText(hwndDlg, IDC_EDIT_URL, szItemName, 2048))
					{
                         szItemName[0]=0; 
						 return FALSE;
					}
                    EndDialog(hwndDlg, wParam); 
					return IDOK;
 
                case IDCANCEL: 
                    EndDialog(hwndDlg, wParam); 
                    return IDCANCEL; 
            } 
    } 
    return FALSE; 
} 

char * esPickURL(HWND hWnd)
{
	HINSTANCE hInst;
	char *fname;
    hInst = (HINSTANCE)GetModuleHandle(NULL); 

	fname = NULL;
	if(DialogBox(hInst, MAKEINTRESOURCE(IDD_ENTER_URL), hWnd, (DLGPROC)URLItemProc)==IDOK)
	{
		fname = malloc(strlen(szItemName)+1);
		strcpy(fname,szItemName);
	}else{
		fname = NULL;
	}
	//printf("hi from esPickURL URL=%s\n",fname);
	return fname;
}
