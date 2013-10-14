// freeWRLCtrl.cpp : Implementation of the CfreeWRLCtrl ActiveX Control class.

#include "stdafx.h"
#include <dllFreeWRL.h>
#include "freeWRLAx.h"
#include "freeWRLCtrl.h"
#include "freeWRLPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE(CfreeWRLCtrl, COleControl)



// Message map

BEGIN_MESSAGE_MAP(CfreeWRLCtrl, COleControl)
	ON_OLEVERB(AFX_IDS_VERB_EDIT, OnEdit)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
	ON_WM_MOUSEMOVE()
	//ON_WM_MOUSEWHEEL()  
	ON_WM_LBUTTONDOWN()  
	ON_WM_LBUTTONUP()  
	//ON_WM_LBUTTONDBLCLK() 
	ON_WM_RBUTTONDOWN()  
	ON_WM_RBUTTONUP()  
	//ON_WM_RBUTTONDBLCLK()  
	ON_WM_MBUTTONDOWN() 
	ON_WM_MBUTTONUP()  
	//ON_WM_MBUTTONDBLCLK() 
	ON_WM_KEYDOWN() 
	ON_WM_KEYUP() 
	ON_WM_CHAR() 
	//ON_WM_TIMER()
END_MESSAGE_MAP()



// Dispatch map

BEGIN_DISPATCH_MAP(CfreeWRLCtrl, COleControl)
END_DISPATCH_MAP()



// Event map

BEGIN_EVENT_MAP(CfreeWRLCtrl, COleControl)
END_EVENT_MAP()



// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CfreeWRLCtrl, 1)
	PROPPAGEID(CfreeWRLPropPage::guid)
END_PROPPAGEIDS(CfreeWRLCtrl)



// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CfreeWRLCtrl, "freewrl.freewrl.2", //FREEWRLAX.freeWRLCtrl.2", //version 2  //"FREEWRLAX.freeWRLCtrl.1", //version 1.22.12_pre2
	0x4e814fce, 0xb546, 0x4d91, 0x8e, 0xa8, 0x35, 0x82, 0x64, 0xe5, 0xd4, 0x23)
	//0x582c9301, 0xa2c8, 0x45fc, 0x83, 0x1b, 0x65, 0x4d, 0xe7, 0xf3, 0xaf, 0x11) //version 1.22.12_pre2



// Type library ID and version

IMPLEMENT_OLETYPELIB(CfreeWRLCtrl, _tlid, _wVerMajor, _wVerMinor)



// Interface IDs

const IID BASED_CODE IID_DfreeWRLAx =
		{ 0xb066822f, 0xed4c, 0x4cf0, { 0x8e, 0x83, 0x10, 0x3b, 0x72, 0x9a, 0x38, 0x3 } };
		//{ 0xC3C32307, 0x89C2, 0x4C1A, { 0xB7, 0xF1, 0xD5, 0x61, 0xAA, 0x6, 0x53, 0xA3 } }; //version 1.22.12_pre2
const IID BASED_CODE IID_DfreeWRLAxEvents =
		{ 0x637caf03, 0xa846, 0x4eb1, { 0xa7, 0xa6, 0xc6, 0x56, 0x26, 0x88, 0xdf, 0xf7 } }; //version 2.0
		//{ 0x91DEB0AA, 0x9E7D, 0x43F4, { 0x80, 0xC4, 0x12, 0x5C, 0x53, 0x5B, 0x30, 0xEC } };//version 1.22.12_pre2



// Control type information

static const DWORD BASED_CODE _dwfreeWRLAxOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CfreeWRLCtrl, IDS_FREEWRLAX, _dwfreeWRLAxOleMisc)



// CfreeWRLCtrl::CfreeWRLCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CfreeWRLCtrl

BOOL CfreeWRLCtrl::CfreeWRLCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegInsertable | afxRegApartmentThreading to afxRegInsertable.

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_FREEWRLAX,
			IDB_FREEWRLAX,
			afxRegInsertable | afxRegApartmentThreading,
			_dwfreeWRLAxOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}



// CfreeWRLCtrl::CfreeWRLCtrl - Constructor

CfreeWRLCtrl::CfreeWRLCtrl()
{
	InitializeIIDs(&IID_DfreeWRLAx, &IID_DfreeWRLAxEvents);
	// TODO: Initialize your control's instance data here.
    m_cstrFileName = "";
	m_initialized = 0; //0 means we haven't instanced dllfreewrl yet - its null
	m_Hwnd = (void *)0; //initialize to unlikely void* value, 
	// so if onMouse is called before onDraw > onInit, then setHandle will return 0
	// and onMouse will be skipped (versus setHandle seeing a 0 handle, and finding
	// a non-main thread
	//m_Hwnd = (void *)this->GetHwnd();
	//m_dllfreewrl = new CdllFreeWRL(100,100,m_Hwnd,false);
	//m_initialized = 0;
#ifdef _DEBUG
  ::MessageBoxA(NULL,"You may now attach a debugger.\n Press OK when you want to proceed.","freeWRLAx plugin process(1)",MB_OK);
#endif

}

// CfreeWRLCtrl::~CfreeWRLCtrl - Destructor

CfreeWRLCtrl::~CfreeWRLCtrl()
{
	// TODO: Cleanup your control's instance data here.
	m_initialized = 0;
	m_dllfreewrl->onClose(); //m_Hwnd); //this is null on exit: (void*)this->GetHwnd() so use last known handle
	//AfxMessageBox("Destructor"); 
}



// CfreeWRLCtrl::OnDraw - Drawing function

void CfreeWRLCtrl::OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	if (!pdc)
		return;
	//if(m_initialized ==0)return;
	if(m_initialized % 10 == 0 )
	{
		//m_initialized = 4; //just so no one does anything while we initialize
		m_Hwnd = (void*)this->GetHwnd(); //we just need the real hWnd for onInit -gl / DC stuff
		// after that it's just an instance ID
		m_dllfreewrl = new CdllFreeWRL(m_rcBounds.right-rcBounds.left,rcBounds.bottom-rcBounds.top,m_Hwnd,false);
		m_initialized += 1;
		this->Invalidate();
	}
	if(m_initialized % 10 == 1 && m_initialized / 10 == 1){
		m_dllfreewrl->onLoad(m_cstrFileName.GetBuffer());
		m_initialized += 1;
	}
	if(m_initialized % 10 == 2){
		m_dllfreewrl->onResize(rcBounds.right-rcBounds.left,rcBounds.bottom-rcBounds.top);//,m_Hwnd
	}
}

void CfreeWRLCtrl::DoPropExchange(CPropExchange* pPX)
{
#define MESSBOX 1
#undef MESSBOX
#ifdef MESSBOX //_DEBUGn
		AfxMessageBox("doPropExchange m_initialized="+m_initialized); //"DoPropExchange");
#endif
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);
	USES_CONVERSION;


	//PX_String(pPX, "SRC", m_cstrFileName); 

	// TODO: Call PX_ functions for each persistent custom property.
	//HTML <OBJECT> tends to generate two DoPropExchanges, HREF and EMBED just one, 
	// so we'll fetch SRC on the first
	if(m_initialized / 10 == 0) 
	{

		// MimeType sample program says SRC property is where Mime handler 
		//   passes in URL
	  if(PX_String(pPX, "SRC", m_cstrFileName)){
		//AfxMessageBox(m_cstrFileName);

		// http://support.microsoft.com/kb/181678 How to Retrieve the URL of a Web Page from an ActiveX Control
		//m_cstrContainerURL = NULL; //C++ initializes these I think.
		LPOLECLIENTSITE pClientSite = this->GetClientSite();
		if (pClientSite != NULL)
		{
			// Obtain URL from container moniker.
			CComPtr<IMoniker> spmk;
			LPOLESTR pszDisplayName;

			if (SUCCEEDED(pClientSite->GetMoniker(
											OLEGETMONIKER_TEMPFORUSER,
											OLEWHICHMK_CONTAINER,
											&spmk)))
			{
				if (SUCCEEDED(spmk->GetDisplayName(
										NULL, NULL, &pszDisplayName)))
				{

					CComBSTR bstrURL;
					bstrURL = pszDisplayName;
					//AfxMessageBox(OLE2T(bstrURL));
					m_cstrContainerURL = OLE2T(bstrURL);
					//ATLTRACE("The current URL is %s\n", OLE2T(bstrURL));
					CoTaskMemFree((LPVOID)pszDisplayName);
				}
			}
		}
		//AfxMessageBox("containerURL="+m_cstrContainerURL);
		//AfxMessageBox("fileName="+m_cstrFileName); //"DoPropExchange");
		if(m_cstrContainerURL != m_cstrFileName)
		{
			//they are different, so concatonate
			int lastSlash = m_cstrContainerURL.ReverseFind('/');
			m_cstrContainerURL = m_cstrContainerURL.Left(lastSlash);
			if(m_cstrContainerURL.GetLength() > 0)
				m_cstrFileName = m_cstrContainerURL + "/" + m_cstrFileName;
		}
#define MESSBOX 1
#undef MESSBOX
#ifdef MESSBOX //_DEBUGn
		AfxMessageBox(m_cstrFileName); //"DoPropExchange");
#endif
		//m_Hwnd = (void *)0; //an unlikely real handle value, and not null either 
		m_initialized += 10; //10 means we have a filename
	 // }else{
		//AfxMessageBox("get SRC failed");
	  }
	}
}



// CfreeWRLCtrl::OnResetState - Reset control to default state

void CfreeWRLCtrl::OnResetState()
{
	//AfxMessageBox("onResetState"); 
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}
void CfreeWRLCtrl::OnLButtonDown(UINT nFlags,CPoint point)
{
	if(m_initialized % 10)
	m_dllfreewrl->onMouse(4, 1,point.x,point.y); //m_Hwnd, 
	COleControl::OnLButtonDown(nFlags,point);
}
void CfreeWRLCtrl::OnLButtonUp(UINT nFlags,CPoint point)
{
	if(m_initialized % 10)
	m_dllfreewrl->onMouse(5, 1,point.x,point.y);//m_Hwnd, 
	COleControl::OnLButtonUp(nFlags,point);
}
void CfreeWRLCtrl::OnMButtonDown(UINT nFlags,CPoint point)
{
	if(m_initialized % 10)
	m_dllfreewrl->onMouse(4, 2,point.x,point.y); //m_Hwnd, 
	COleControl::OnMButtonDown(nFlags,point);
}
void CfreeWRLCtrl::OnMButtonUp(UINT nFlags,CPoint point)
{
	if(m_initialized % 10)
	m_dllfreewrl->onMouse(5, 2,point.x,point.y);//m_Hwnd, 
	COleControl::OnMButtonUp(nFlags,point);
}
void CfreeWRLCtrl::OnRButtonDown(UINT nFlags,CPoint point)
{
	if(m_initialized % 10)
	m_dllfreewrl->onMouse(4, 3,point.x,point.y);//m_Hwnd, 
	COleControl::OnRButtonDown(nFlags,point);
}
void CfreeWRLCtrl::OnRButtonUp(UINT nFlags,CPoint point)
{
	if(m_initialized % 10)
	m_dllfreewrl->onMouse(5, 3,point.x,point.y); //m_Hwnd, 
	COleControl::OnRButtonUp(nFlags,point);
}

void CfreeWRLCtrl::OnMouseMove(UINT nFlags,CPoint point)
{
//#define KeyChar         1
//#if defined(AQUA) || defined(WIN32)
//#define KeyPress        2
//#define KeyRelease      3
//#define ButtonPress     4
//#define ButtonRelease   5
//#define MotionNotify    6
//#define MapNotify       19
	/* butnum=1 left butnum=3 right (butnum=2 middle, not used by freewrl) */

	//m_dllfreewrl.onMouse(int mouseAction,int mouseButton,int x, int y);
	//if(m_initialized == 11 ){ //malloced freewrl and we have a filename
	//	m_dllfreewrl->onLoad(m_cstrFileName.GetBuffer()); //m_Hwnd,
	//	m_initialized += 10; //shut off double-load
	//}
	if(m_initialized % 10)
		m_dllfreewrl->onMouse(6, 0,point.x,point.y);//m_Hwnd, 
	COleControl::OnMouseMove(nFlags,point);
}

void CfreeWRLCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(m_initialized % 10)
		m_dllfreewrl->onKey(m_dllfreewrl->KEYDOWN,nChar); //m_Hwnd,
	COleControl::OnKeyDown(nChar,nRepCnt,nFlags);
}
void CfreeWRLCtrl::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(m_initialized % 10)
		m_dllfreewrl->onKey(m_dllfreewrl->KEYUP,nChar); //m_Hwnd,
	COleControl::OnKeyUp(nChar,nRepCnt,nFlags);
}
void CfreeWRLCtrl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(m_initialized % 10)
		m_dllfreewrl->onKey(m_dllfreewrl->KEYPRESS,nChar); //m_Hwnd,
	COleControl::OnChar(nChar,nRepCnt,nFlags);
}
//void CfreeWRLCtrl::

// CfreeWRLCtrl message handlers
