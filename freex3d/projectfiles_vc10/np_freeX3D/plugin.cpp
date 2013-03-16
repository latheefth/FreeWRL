/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

//////////////////////////////////////////////////
//
// CPlugin class implementation
//
#include <assert.h>
#include "plugin.h"
#include "npfunctions.h"
#include "BasePlugin.h"
#include "dllFreeWRL.h"
//extern "C"
//{
//	#include "libFreeWRL.h"
//	//#include <internal.h>
//	//#include <iglobal.h>
//}

#ifdef XP_WIN
#include <windows.h>
#include <windowsx.h>
#include <string>
#endif

#ifdef XP_MAC
#include <TextEdit.h>
#endif

#ifdef XP_UNIX
#include <string.h>
#endif


static char *fontPath = NULL; //once per process for the DLL should do it
void setFontPath()
{
	/* deployed system (with intalled fonts) - use system fonts  
	we plan to use a professional installer to install the fonts to %windir%\Fonts directory 
	where all the system fonts already are.
	Then in this program we will get the %windir%\Fonts directory, and set it as temporary
	environment variable for InputFunctions.C > makeFontsDirectory() to fetch.
	*/
	static char *fdir;
	char *syspath;
	if(fontPath == NULL){
		syspath = getenv("windir");
		printf("windir path=[%s]\n",syspath);
		fdir = (char *)malloc(1024); 
		strcpy(fdir,"FREEWRL_FONTS_DIR=");
		strcat(fdir,syspath);
		strcat(fdir,"\\Fonts");
		_putenv( fdir );
		fontPath = fdir;
	}
}

///
///CPlugin class.constructor. Base initialization goes here.
///
CPlugin::CPlugin(NPP pNPInstance) :
  m_pNPInstance(pNPInstance),
  m_pNPStream(NULL),
  m_bInitialized(FALSE),
  m_pScriptableObject(NULL)
{
#ifdef XP_WIN
  m_hWnd = NULL;
#endif
  m_pfreeWRLPlayer = NULL;
  
  m_pfreeWRLPlayer = new CdllFreeWRL();

  assert(m_pfreeWRLPlayer);

  m_sceneUrl = NULL;

  NPN_GetValue(m_pNPInstance, NPNVWindowNPObject, &sWindowObj); 
}

CPlugin::~CPlugin()
{
	if (m_pScriptableObject)
		NPN_ReleaseObject(m_pScriptableObject);

	if (sWindowObj)
		NPN_ReleaseObject(sWindowObj);
	
	sWindowObj = 0;
}

#ifdef XP_WIN
static LRESULT CALLBACK PluginWinProc(HWND, UINT, WPARAM, LPARAM);
static WNDPROC lpOldProc = NULL;
#endif

NPBool CPlugin::init(NPWindow* pNPWindow)
{
  if(pNPWindow == NULL)
    return FALSE;

  m_Window = pNPWindow;

#ifdef XP_WIN
  m_hWnd = (HWND)pNPWindow->window;
  if(m_hWnd == NULL)
    return FALSE;

  // subclass window so we can intercept window messages and
  // do our drawing to it
  lpOldProc = SubclassWindow(m_hWnd, (WNDPROC)PluginWinProc);
   
  // associate window with our CPlugin object so we can access 
  // it in the window procedure
  SetWindowLong(m_hWnd, GWL_USERDATA, (LONG)this);
#else
  m_hWnd = (long int*) pNPWindow->window;
    
#endif

  m_pfreeWRLPlayer->onInit(pNPWindow->window,pNPWindow->width,pNPWindow->height);

  m_pfreeWRLPlayer->onLoad(pNPWindow->window,m_sceneUrl);
    

  m_bInitialized = TRUE;
  return TRUE;
}

void CPlugin::shut()
{
	m_pfreeWRLPlayer->onClose(m_Window->window);
	//TODO: since we are closing a bunch of threads, maybe should be better to set a check on them.

#ifdef XP_WIN
  // subclass it back
	SubclassWindow(m_hWnd, lpOldProc);
	m_hWnd = NULL;	
#endif

	if(m_sceneUrl)
		free(m_sceneUrl);


	delete m_pfreeWRLPlayer;
	
	m_bInitialized = FALSE;
}

NPBool CPlugin::isInitialized()
{
  return m_bInitialized;
}

int16_t CPlugin::handleEvent(void* event)
{
#ifdef XP_MAC
  NPEvent* ev = (NPEvent*)event;
  if (m_Window) {
    Rect box = { m_Window->y, m_Window->x,
                 m_Window->y + m_Window->height, m_Window->x + m_Window->width };
    if (ev->what == updateEvt) {
      ::TETextBox(m_String, strlen(m_String), &box, teJustCenter);
    }
  }
#endif
  return 0;
}

// this will force to draw a version string in the plugin window
void CPlugin::showVersion()
{
  const char *ua = NPN_UserAgent(m_pNPInstance);
  strcpy_s(m_String, ua);

#ifdef XP_WIN
  InvalidateRect(m_hWnd, NULL, TRUE);
  UpdateWindow(m_hWnd);
#endif

  /*if (m_Window) {
    NPRect r =
      {
        (uint16_t)m_Window->y, (uint16_t)m_Window->x,
        (uint16_t)(m_Window->y + m_Window->height),
        (uint16_t)(m_Window->x + m_Window->width)
      };

    NPN_InvalidateRect(m_pNPInstance, &r);
  }*/
}

// this will clean the plugin window
void CPlugin::clear()
{
  strcpy_s(m_String, "");

#ifdef XP_WIN
  InvalidateRect(m_hWnd, NULL, TRUE);
  UpdateWindow(m_hWnd);
#endif
}

void CPlugin::getVersion(char* *aVersion)
{
  const char *ua = NPN_UserAgent(m_pNPInstance);
  char*& version = *aVersion;
  version = (char*)NPN_MemAlloc(1 + strlen(ua));
  if (version)
    strcpy_s(version, sizeof(version), ua);
}

NPObject *
CPlugin::GetScriptableObject()
{
  if (!m_pScriptableObject) {
    m_pScriptableObject =
      NPN_CreateObject(m_pNPInstance,
                       GET_NPOBJECT_CLASS(BasePlugin));
  }

  if (m_pScriptableObject) {
    NPN_RetainObject(m_pScriptableObject);
  }

  return m_pScriptableObject;
}

LRESULT CPlugin::handleWindowEvents(HWND hWnd, UINT eventmsg, WPARAM wParam, LPARAM lParam)
{
	switch(eventmsg)
	{
	case WM_LBUTTONDOWN:
		{
			m_pfreeWRLPlayer->onMouse(hWnd, 4, 1,GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			return 0;
		}
	case WM_LBUTTONUP:
		{
			m_pfreeWRLPlayer->onMouse(hWnd, 5, 1,GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			return 0;
		}
	case WM_MBUTTONDOWN:
		{
			m_pfreeWRLPlayer->onMouse(hWnd, 4, 2,GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			return 0;
		}
	case WM_MBUTTONUP:
		{
			m_pfreeWRLPlayer->onMouse(hWnd, 5, 2,GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			return 0;
		}
	case WM_RBUTTONDOWN:
		{
			m_pfreeWRLPlayer->onMouse(hWnd, 4, 3,GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			return 0;
		}
	case WM_RBUTTONUP:
		{
			m_pfreeWRLPlayer->onMouse(hWnd, 5, 3,GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			return 0;
		}
	case WM_MOUSEMOVE:
		{
			m_pfreeWRLPlayer->onMouse(hWnd, 6, 0,GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			return 0;
		}
	case WM_KEYDOWN:
		{
			m_pfreeWRLPlayer->onKey(hWnd,CdllFreeWRL::KEYDOWN,wParam);
			return 0;
		}
	case WM_KEYUP:
		{
			m_pfreeWRLPlayer->onKey(hWnd,CdllFreeWRL::KEYUP,wParam);
			return 0;
		}
	case WM_CHAR:
		{
			m_pfreeWRLPlayer->onKey(hWnd,CdllFreeWRL::KEYPRESS,wParam);
			return 0;
		}
	case WM_DESTROY:
		{
			
			break;
		}
		
	default:
		break;
	}

	return 0;
}

void CPlugin::setSceneUrl(char* sceneUrl)
{
	//char* buf = (char*) NPN_MemAlloc(strlen(sceneUrl)+1);
	size_t buffersize = strlen(sceneUrl)+1;

	char* buf = (char*) calloc(buffersize,sizeof(char));

	strcpy_s(buf,buffersize,sceneUrl);

	m_sceneUrl = buf; 
}


#ifdef XP_WIN
static LRESULT CALLBACK PluginWinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CPlugin *plugin = (CPlugin *)(LONG_PTR) GetWindowLong(hWnd, GWL_USERDATA);

	if (!plugin){
		  switch (msg) {
			case WM_PAINT:
			  {		       
				 std::string errormsg("Instance Error: Plugin has not been initialized");

				PAINTSTRUCT ps;
				RECT rc;
				HDC hdc = BeginPaint(hWnd, &ps);

				GetClientRect(hWnd, &rc);
				FrameRect(hdc, &rc, GetStockBrush(BLACK_BRUSH));
				DrawText(hdc, (LPCTSTR) errormsg.c_str(), errormsg.length(), &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
				EndPaint(hWnd, &ps);
			  }
			  break;
			default:
			  break;
		  }
	}
	else
	{
		plugin->handleWindowEvents(hWnd,msg,wParam,lParam);
	}
  return DefWindowProc(hWnd, msg, wParam, lParam);
}
#endif
