/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * $Id$
 *
 * ***** BEGIN LICENSE BLOCK *****
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

/*
 * The signal handling code is based on the work of
 * W. Richard Stevens from Unix Network Programming,
 * Networking APIs: Sockets and XTI.
 */

#include "npFreeWRLInstance.h"


// add timer to periodically update

// handle fullsize


// Unix needs this
#ifdef XP_UNIX

#define MIME_TYPES_HANDLED "model/vrml:wrl:FreeWRL VRML Browser;x-world/x-vrml:wrl:FreeWRL VRML Browser"
#define PLUGIN_NAME "FreeWRL VRML Browser"
#define PLUGIN_DESCRIPTION "Implements VRML Browsing on Mozilla/Netscape"

// not in npp_gate.cpp
char* NPP_GetMIMEDescription()
{
    return(MIME_TYPES_HANDLED);
}

#endif // XP_UNIX

npFreeWRLInstance::npFreeWRLInstance(NPP instance) :
    npInstanceBase(),
    mInstance(instance),
    mInitialized(FALSE),
    mFreeWRLAlive(FALSE),
    freeWRLPid(0),
    socketDesc(0)
{

#ifdef XP_UNIX

    mPlatform.window = 0;
    mPlatform.appWindow = 0;
    mPlatform.appWidget = 0;
    mPlatform.display = 0;
    mPlatform.widget = NULL;
    mPlatform.gc = NULL;
    mPlatform.x = 0;
    mPlatform.y = 0;
    mPlatform.width = 0;
    mPlatform.height = 0;

	addr.sin_family = 0;
	addr.sin_addr.s_addr = 0;
	addr.sin_port = 0;

#endif // XP_UNIX
    instance->pdata = this;
}

npFreeWRLInstance::~npFreeWRLInstance()
{
    std::cerr << "npFreeWRLInstance destructor!" << endl;

#ifdef XP_UNIX

    if (mPlatform.widget) {
        XtRemoveEventHandler(mPlatform.widget,
                             ExposureMask,
                             FALSE,
                             (XtEventHandler) &exposeHandler,
                             (XtPointer) this);

        XtRemoveEventHandler(mPlatform.widget,
                          StructureNotifyMask,
                          FALSE,
                          (XtEventHandler) &structHandler,
                          (XtPointer) this);

        XtRemoveEventHandler(mPlatform.widget,
                          SubstructureNotifyMask,
                          FALSE,
                          (XtEventHandler) &substructHandler,
                          (XtPointer) this);

    }

#endif // XP_UNIX
    if (mInitialized) { mInitialized = FALSE; }
    if (mFreeWRLAlive) { mFreeWRLAlive = FALSE; }
}


NPBool
npFreeWRLInstance::init(NPWindow *pNPWindow)
{
#if _DEBUG
    std::cerr << "npFreeWRLInstance init!" << endl;
#endif // _DEBUG
    
    if (pNPWindow == NULL || pNPWindow->window == NULL) {
        return FALSE;
    }


#ifdef XP_UNIX

#if _DEBUG
    std::cerr << "\txid = "
              << (XID) pNPWindow->window
              << ", x = " << pNPWindow->x
              << ", y = " << pNPWindow->y
              << ", W = " << pNPWindow->width
              << ",  H = " << pNPWindow->height
              << " under Unix!" << endl;
#endif // _DEBUG

        
    mPlatform.window = (XID) pNPWindow->window;
    mPlatform.display = ((NPSetWindowCallbackStruct *) pNPWindow->ws_info)->display;
    mPlatform.x = pNPWindow->x;
    mPlatform.y = pNPWindow->y;
    mPlatform.width = pNPWindow->width;
    mPlatform.height = pNPWindow->height;

    mPlatform.widget = XtWindowToWidget(mPlatform.display, mPlatform.window);
        

    if (mPlatform.widget) {
        
        // event masks from X11/X.h
        XtAddEventHandler(mPlatform.widget,
                          ExposureMask,
                          FALSE,
                          (XtEventHandler) &exposeHandler,
                          (XtPointer) this);

        XtAddEventHandler(mPlatform.widget,
                          StructureNotifyMask,
                          FALSE,
                          (XtEventHandler) &structHandler,
                          (XtPointer) this);

        XtAddEventHandler(mPlatform.widget,
                          SubstructureNotifyMask,
                          FALSE,
                          (XtEventHandler) &substructHandler,
                          (XtPointer) this);

    } else {
        std::cerr << "XtWindowToWidget failed!" << endl;
        return FALSE;
    }

#endif // XP_UNIX
    // add MacOS code later

    // handle signals...
    if (signal(SIGIO, (__sighandler_t) &npFreeWRLInstance::signalHandler) == SIG_ERR) {
        std::cerr << "signal with SIGIO failed." << endl;
        return FALSE;
    }

    if (signal(SIGPIPE, (__sighandler_t) &npFreeWRLInstance::signalHandler) == SIG_ERR) {
        std::cerr << "signal with SIGPIPE failed." << endl;
        return FALSE;
    }

#if _DEBUG
    if (signal(SIGBUS, (__sighandler_t) &npFreeWRLInstance::signalHandler) == SIG_ERR) {
        std::cerr << "signal with SIGPIPE failed." << endl;
        return FALSE;
    }
#endif // _DEBUG

    if (signal(SIGHUP, (__sighandler_t) &npFreeWRLInstance::signalHandler) == SIG_ERR) {
        std::cerr << "signal with SIGHUP failed." << endl;
        return FALSE;
    }

    if (signal(SIGUSR2, (__sighandler_t) &npFreeWRLInstance::signalHandler) == SIG_ERR) {
        std::cerr << "signal with SIGUSR2 failed." << endl;
        return FALSE;
    }


    if ((socketDesc = pluginBind(&addr)) < 0) {
        std::cerr << "pluginBind failed." << endl;
        return FALSE;
    }

    pid_t pid = getpid();
    if (setIOOptions(socketDesc, pid, TRUE, FALSE) < 0) {
        std::cerr << "setIOOptions failed." << endl;
        return FALSE;
    }
	
    mInitialized = TRUE;
    return TRUE;
}

void
npFreeWRLInstance::shut()
{
#if _DEBUG
    std::cerr << "npFreeWRLInstance shut!" << endl;
#endif // _DEBUG

    int status, options = WUNTRACED;

    if (kill(freeWRLPid, SIGUSR2) < 0) {
        printCError("kill failed", errno);
    }
    
    if (waitpid(freeWRLPid, &status, options) < 0) {
        printCError("waitpid failed", errno);
    }
    
#if _DEBUG
    printWaitpidStatus(status);
#endif // _DEBUG

    if (mPlatform.appWidget) {
        XtUnmanageChild(mPlatform.appWidget);
        XtDestroyWidget(mPlatform.appWidget);
    }
 
    mInitialized = FALSE;
    mFreeWRLAlive = FALSE;
}

#if _DEBUG
// for debugging
void
printWaitpidStatus(int status)
{
    if (WIFEXITED(status)) {
        std::cerr << "FreeWRL exited normally with return code "
                  << (int) WEXITSTATUS(status) << "." << endl;
    } else if (WIFSIGNALED(status)) {
        std::cerr << "FreeWRL exited because of uncaught signal "
                  << WTERMSIG(status) << "." << endl;
    } else if (WIFSTOPPED(status)) {
        std::cerr << "FreeWRL process was stopped by signal "
                  << WSTOPSIG(status) << "." << endl;
    } else {
#ifdef WCOREDUMP
        if (WCOREDUMP(status)) {
            std::cerr << "FreeWRL dumped core." << endl;
        }
#endif // WCOREDUMP
    }
}
#endif // _DEBUG

void
npFreeWRLInstance::clear()
{
#if _DEBUG
    std::cerr << "npFreeWRLInstance clear!" << endl;
#endif // _DEBUG
}

NPBool
npFreeWRLInstance::isInitialized()
{
#if _DEBUG
    std::cerr << "npFreeWRLInstance isInitialized!" << endl;
#endif // _DEBUG
    return mInitialized;
}


PlatformInfo *
npFreeWRLInstance::getPlatformInfo() { return &mPlatform; }


void
npFreeWRLInstance::getVersion(char **aVersion)
{
    const char *ua = NPN_UserAgent(mInstance);
    char*& version = *aVersion;
    version = (char*) NPN_MemAlloc(1 + strlen(ua));
    if (version)
        strcpy(version, ua);
}
 
void
npFreeWRLInstance::showVersion() 
{}

#ifdef XP_UNIX

// from the simple sample plugin:
NPError
npFreeWRLInstance::resize()
{
#if _DEBUG
    std::cerr << "npFreeWRLInstance resize ( xid = "
                  << (XID) mPlatform.window
                  << ", x = " << mPlatform.x
                  << ", y = " << mPlatform.y
                  << ", W = " << mPlatform.width
                  << ",  H = " << mPlatform.height
                  << " )!" << endl;
#endif // _DEBUG

    int rv = NPERR_NO_ERROR;
    return rv;
}

void
structHandler(Widget widget,
              XtPointer data,
              XEvent* event,
              Boolean* dispatchContinue)
{
#if _DEBUG
    std::cerr << "plugin X structHandler: " << XEventToString(event->type)
              << "!" << endl;
#endif // _DEBUG
    UNUSED(widget);
    UNUSED(dispatchContinue);
    UNUSED(data);
    UNUSED(event);
}

void
substructHandler(Widget widget,
                XtPointer data,
                XEvent* event,
                Boolean* dispatchContinue)
{
#if _DEBUG
    std::cerr << "plugin X substructHandler: " << XEventToString(event->type)
              << "!" << endl;
#endif // _DEBUG
    UNUSED(widget);
    UNUSED(dispatchContinue);
    
    npFreeWRLInstance *pInstance = (npFreeWRLInstance *) data;
    PlatformInfo *pInfo = (PlatformInfo *) pInstance->getPlatformInfo();

    if (XSendEvent(pInfo->display,
                   pInfo->appWindow,
                   FALSE,
                   StructureNotifyMask,
                   event) == 0) {
        std::cerr << "XSendEvent failed!" << endl;
    }
}

void
exposeHandler(Widget widget,
            XtPointer data,
            XEvent* event,
            Boolean* dispatchContinue)
{
#if _DEBUG
    std::cerr << "plugin X exposeHandler: "
              << XEventToString(event->type) << "!" << endl;
#endif // _DEBUG
    UNUSED(widget);
    UNUSED(data);
    UNUSED(dispatchContinue);
    UNUSED(event);
}

#endif // XP_UNIX

/*
 * Note: NPWindow structure is essentially equivalent to
 * nsPluginWindow (see nsplugindefs.h)
 */
NPError
npFreeWRLInstance::SetWindow(NPWindow* pNPWindow)
{
    uint32 i, j, k;
    if (pNPWindow == NULL) {
        std::cerr << "npFreeWRLInstance SetWindow null window!" << endl;
        return NPERR_GENERIC_ERROR;
    }

#if _DEBUG
    std::cerr << "npFreeWRLInstance SetWindow!" << endl;
#endif // _DEBUG

    if (pNPWindow->window == NULL) {
         // release resources used by window
        std::cerr << "npFreeWRLInstance SetWindow null pNPWindow->window!" << endl;
    } else {

#ifdef XP_UNIX

#if _DEBUG
        std::cerr << "npFreeWRLInstance SetWindow set window ( xid = "
                  << (XID) pNPWindow->window
                  << ", x = " << pNPWindow->x
                  << ", y = " << pNPWindow->y
                  << ", W = " << pNPWindow->width
                  << ",  H = " << pNPWindow->height
                  << " ) under Unix!" << endl;
#endif // _DEBUG

        
        if (mPlatform.window != (XID) pNPWindow->window) {
            mPlatform.window =
                (XID) pNPWindow->window;
        }

        if (mPlatform.display != ((NPSetWindowCallbackStruct *) pNPWindow->ws_info)->display) {
            mPlatform.display =
                ((NPSetWindowCallbackStruct *) pNPWindow->ws_info)->display;
        }


        if (mFreeWRLAlive && !mPlatform.appWindow) {
            Window rootReturn, parentReturn, *childList = NULL;
            uint32 childCount = 0;
            char pidStr[SMALLSTRINGSIZE];
            char *windowName = NULL;
            bool found = FALSE;

            /*
             * Corresponds to setting the window title in GLBackEnd.pm
             * and in OpenGL/OpenGL.xs.
             */
            memset(pidStr, 0, SMALLSTRINGSIZE);
            sprintf(pidStr,"fw%d", freeWRLPid);

            if (XQueryTree(mPlatform.display,
// RootWindowOfScreen(XtScreen(mPlatform.widget)),
                           mPlatform.window,
                           &rootReturn,
                           &parentReturn,
                           &childList,
                           &childCount) != 0) {
#if _DEBUG
                std::cerr << "\tgot " << childCount << " child(ren)!" << endl;
#endif // _DEBUG
                // locate FreeWRL window
                for (i = 0; i < childCount && !found; i++) {
                    if (XFetchName(mPlatform.display,
                                   childList[i],
                                   &windowName) != 0) {

                        if (strncmp(pidStr, windowName, strlen(pidStr)) == 0) {
#if _DEBUG
                            std::cerr << "\twindow " << windowName
                                      << " found!" << endl;
#endif // _DEBUG
                            found = TRUE;
                            mPlatform.appWindow = childList[i];
                        } else {
#if _DEBUG
                            std::cerr << "\twindow " << windowName
                                      << " did not match " << pidStr
                                      << "!" << endl;
#endif // _DEBUG
                        }
                        XFree(windowName);
                    }
                }
                XFree(childList);
            } else {
                std::cerr << "XQueryTree failed." << endl;
            }
        }

        mPlatform.x = pNPWindow->x;
        mPlatform.y = pNPWindow->y;
        mPlatform.width = pNPWindow->width;
        mPlatform.height = pNPWindow->height;

#endif // XP_UNIX
// add MacOS code later
    }

    return NPERR_NO_ERROR;
}

NPError
npFreeWRLInstance::NewStream(NPMIMEType type, // NPMIMEType is char *
                             NPStream *stream,
                             NPBool seekable,
                             uint16 *stype)
{
#if _DEBUG
    std::cerr << "npFreeWRLInstance NewStream, url = "
              << stream->url << "!" << endl;
#endif // _DEBUG

    if (stream->url == NULL) {
        std::cerr << "npFreeWRLInstance NewStream null stream->url!" << endl;
		return NPERR_INVALID_INSTANCE_ERROR;
    }
    UNUSED(type);

	// save this to a file.
	*stype = NP_ASFILEONLY;
	seekable = FALSE;

    return NPERR_NO_ERROR;
}

NPError
npFreeWRLInstance::DestroyStream(NPStream* stream, NPError reason)
{
#if _DEBUG
    std::cerr << "npFreeWRLInstance DestroyStream!" << endl;
#endif // _DEBUG

    UNUSED(stream);
    UNUSED(reason);
    
    return NPERR_NO_ERROR;
}


NPError
npFreeWRLInstance::runFreeWRL(const char *fileName)
{
#if _DEBUG
    std::cerr << "npFreeWRLInstance runFreeWRL, file = "
              << fileName << "!" << endl;
#endif // _DEBUG

    XID parent = 0;

    int  childPid; // pid of forked process
    char *cmdLineStr[16]; // build command line string to exec FreeWRL
    char winGeomStr[SMALLSTRINGSIZE];
    char pidStr[SMALLSTRINGSIZE];
    char instanceStr[LARGESTRINGSIZE];
    char parentWinStr[LARGESTRINGSIZE];
    char socketStr[LARGESTRINGSIZE];
    char fName[FILENAME_MAX];

    // EAI runs locally and port 2000 is reserved.
    char *const eaiAddress = "localhost:2000";

#ifdef XP_UNIX
    childPid = (int) fork();
    if (childPid < 0) {
        printCError("fork failed", errno);
        return NPERR_GENERIC_ERROR;
    } else if (childPid == 0) { // child's thread of execution (FreeWRL)
        freeWRLPid = (int) getpid();

        memset(winGeomStr, 0, SMALLSTRINGSIZE);
        memset(pidStr, 0, SMALLSTRINGSIZE);
        memset(instanceStr, 0, LARGESTRINGSIZE);
        memset(parentWinStr, 0, LARGESTRINGSIZE);
        memset(socketStr, 0, LARGESTRINGSIZE);
        memset(fName, 0, FILENAME_MAX);

        sprintf(winGeomStr,"%dx%d", mPlatform.width, mPlatform.height);
        sprintf(pidStr,"fw%d", freeWRLPid);
        sprintf(socketStr, "%d", socketDesc);

        // legacy requirement!
        sprintf(instanceStr, "%u", (uint) mInstance);

        sprintf(fName, "%s", fileName);
//         parent = (XID) RootWindowOfScreen(XtScreen(mPlatform.widget));
        parent = (XID) mPlatform.window;
        sprintf(parentWinStr, "%lu", parent);

        /*
         * Invoke FreeWRL:
         * freewrl fileName  -geom wwxhh
         *                   -plugin pid
         *                   -fileDesc socketfd
         *                   -instance instancePtr
         *                   -parent parentWinXID
         *
         *
         * A request for an EAI connection is also made - find
         * a better way to handle this!
         */

        // 'nice' utility modifies scheduling priority for FreeWRL command
        cmdLineStr[0] = "nice";
        cmdLineStr[1] = "freewrl";
        cmdLineStr[2] = fName;
        cmdLineStr[3] = "-geom";
        cmdLineStr[4] = winGeomStr;
        cmdLineStr[5] = "-plugin";
        cmdLineStr[6] = pidStr;

        /*
         * Hard-code request for EAI connection
         * until a better way to determine if EAI is
         * needed can be found.
         */
        cmdLineStr[7] = "-eai";
        cmdLineStr[8] = eaiAddress;

        cmdLineStr[9] = "-fd";
        cmdLineStr[10] = socketStr;
        cmdLineStr[11] = "-instance";
        cmdLineStr[12] = instanceStr;
        cmdLineStr[13] = "-parent";
        cmdLineStr[14] = parentWinStr;
        cmdLineStr[15] = NULL;


#if _DEBUG
        std::cerr << "Calling";
        for (int i = 0; i < 15; i++) { std::cerr << " " << cmdLineStr[i] << " "; }
        std::cerr << endl;
#endif // _DEBUG
            
        // execute 'nice', which in turn runs FreeWRL
        if (execvp(cmdLineStr[0], cmdLineStr) < 0) {
            printCError("execvp failed", errno);
        }
    } else { // parent's thread of execution (Plugin)
        freeWRLPid = (int) childPid;

        // decrease reference count by 1
        if (close(socketDesc) < 0) {
            printCError("close failed", errno);
        }
    }

#endif // XP_UNIX
    return NPERR_NO_ERROR;
}

/*
 * FreeWRL process gets forked/exec'd from here, just like in the Netscape
 * 4.7x plugin.
 */
void
npFreeWRLInstance::StreamAsFile(NPStream* stream, const char *fileName)
{
#if _DEBUG
    std::cerr << "npFreeWRLInstance StreamAsFile: " << fileName << "!" << endl;
#endif // _DEBUG
    NPError rv = NPERR_NO_ERROR;
    size_t bytes = 0;

    UNUSED(stream);

#ifdef XP_UNIX
    if (!mFreeWRLAlive) {
        if ((rv = runFreeWRL(fileName)) != NPERR_NO_ERROR) {
            std::cerr << "runFreeWRL failed with error number "
                      << rv << "!" << endl;
            mFreeWRLAlive = FALSE;
            return;
        } else {
            mFreeWRLAlive = TRUE;
        }
    } else {
        std::cerr << "\tFreeWRL is alive!" << endl;
        if (fileName) {
            bytes = (strlen(fileName) + 1) * sizeof(char);
            if (send(socketDesc, fileName, bytes, 0) < 0) {
                printCError("send failed", errno); 
            }
        } else {
            std::cerr << "File could not be found in npFreeWRLInstance::StreamAsFile."
                      << endl;
        }
    }
#endif // XP_UNIX
}

int32
npFreeWRLInstance::WriteReady(NPStream* stream)
{
    std::cerr << "npFreeWRLInstance WriteReady!" << endl;

    UNUSED(stream);

    return STREAMBUFSIZE;
}

int32
npFreeWRLInstance::Write(NPStream* stream, int32 offset, int32 len, void* buffer)
{
    UNUSED(stream);
    UNUSED(offset);
    UNUSED(buffer);

    return len;
}

/*
 * The FreeWRL plugin is normally full page, but it's better to let
 * the browser handle printing.
 */
void
npFreeWRLInstance::Print(NPPrint* printInfo)
{
     // from npshell.c in the default plugin sample:  
    if (printInfo == NULL) {
        std::cerr << "npFreeWRLInstance Print, null printInfo!" << endl;
        return;
    }
 
    if (printInfo->mode == NP_FULL) {
        std::cerr << "npFreeWRLInstance Print, NP_FULL mode!" << endl;
        /*
         * the default - allows the browser to handle printing, which
         * means that NPP_Print (which uses this function) will be
         * called again using printInfo->mode set to NP_EMBED
         */
        printInfo->print.fullPrint.pluginPrinted = FALSE;
    } else { // printInfo->mode == NP_EMBED
        std::cerr << "npFreeWRLInstance Print, NP_EMBED mode!" << endl;

         // print code ???

//         NPWindow* printWindow =
//             &(printInfo->print.embedPrint.window);
//         void* platformPrint =
//             printInfo->print.embedPrint.platformPrint;
        
    }
}

uint16
npFreeWRLInstance::HandleEvent(void* event)
{
    UNUSED(event);
    return NPERR_NO_ERROR; 
}

void
npFreeWRLInstance::URLNotify(const char* url, NPReason reason, void* notifyData)
{
    UNUSED(url);
    UNUSED(reason);
    UNUSED(notifyData);
}

NPError
npFreeWRLInstance::GetValue(NPPVariable variable, void* value)
{
#if _DEBUG
    std::cerr << "npFreeWRLInstance GetValue!" << endl;
#endif // _DEBUG
    NPError rv = NPERR_NO_ERROR;

     // not handling scriptable at this point, if ever

    switch (variable) {
        case NPPVpluginScriptableInstance: {
         // addref happens in getter, so we don't addref here
//         nsISimplePlugin * scriptablePeer = getScriptablePeer();
//         if (scriptablePeer) {
//             *(nsISupports **)aValue = scriptablePeer;
//         } else
//             rv = NPERR_OUT_OF_MEMORY_ERROR;
        }
        break;

        case NPPVpluginScriptableIID: {
//         static nsIID scriptableIID = NS_ISIMPLEPLUGIN_IID;
//         nsIID* ptr = (nsIID *)NPN_MemAlloc(sizeof(nsIID));
//         if (ptr) {
//             *ptr = scriptableIID;
//             *(nsIID **)aValue = ptr;
//         } else
//             rv = NPERR_OUT_OF_MEMORY_ERROR;
        }
        break;

#ifdef XP_UNIX
         // Unix needs some additional cases
        case NPPVpluginNameString:
            *((char **) value) = PLUGIN_NAME;
        break;
        case NPPVpluginDescriptionString:
            *((char **) value) = PLUGIN_DESCRIPTION;
        break;
#endif //XP_UNIX

        default:
        break;
    }

    return rv;
}

NPError
npFreeWRLInstance::SetValue(NPNVariable variable, void* value)
{
#if _DEBUG
    std::cerr << "npFreeWRLInstance SetValue!" << endl;
#endif // _DEBUG

    UNUSED(variable);
    UNUSED(value);

    return NPERR_NO_ERROR;
}

void
npFreeWRLInstance::signalHandler(int signo)
{
#if _DEBUG
    std::cerr << "npFreeWRLInstance signalHandler signal number = "
              << signo << endl;
#endif // _DEBUG

    switch(signo) {
        case SIGIO:
#if _DEBUG
            std::cerr << "\tplugin caught SIGIO!" << endl;
#endif // _DEBUG
            if (receiveUrl(socketDesc, &lastRequest) != NPERR_NO_ERROR) {
                std::cerr << "receiveUrl returned with error!" << endl;
            } else {
                if (NPN_GetURL((NPP) lastRequest.instance,
                               lastRequest.url,
                               NULL) != NO_ERROR) {
                    fprintf(stderr, "NPN_GetURL failed.\n");
                }
            }
            break;
#if _DEBUG
        case SIGBUS:
            std::cerr << "\tplugin caught SIGPIPE!" << endl;
            break;
#endif // _DEBUG
        case SIGPIPE:
#if _DEBUG
            std::cerr << "\tplugin caught SIGPIPE!" << endl;
#endif // _DEBUG
            break;
        case SIGHUP:
#if _DEBUG
            std::cerr << "\tplugin caught SIGHUP!" << endl;
#endif // _DEBUG
            break;
        case SIGUSR2:
#if _DEBUG
            std::cerr << "\tplugin caught SIGUSR2!" << endl;
#endif // _DEBUG
            break;
        default:
            break;
    }
}

// prototypes in npInstanceBase.h, functions called by various in
// np_entry.cpp and npp_gate.cpp

// nsIServiceManager *gServiceManager is global in npFreeWRLInstance.h

npInstanceBase *
NS_NewPluginInstance(npCreateData* pCreate)
{
    if(!pCreate)
        return NULL;

#if _DEBUG
    std::cerr << "NS_NewPluginInstance for MIME type = "
              << pCreate->type << ", ";
    if (pCreate->mode == NP_EMBED) {
        std::cerr << "mode = NP_EMBED, ";
    } else if (pCreate->mode == NP_FULL){
        std::cerr << "mode = NP_FULL, ";
    }

    if (pCreate->instance) {
        std::cerr << "with instance data, ";
        if (pCreate->instance->pdata) {
            std::cerr << "plugin instance private data = "
                      << pCreate->instance->pdata << ", ";
        }
    } else {
        std::cerr << "with no instance data, ";
    }

    if (pCreate->saved) {
        std::cerr << "with saved data, ";
    } else {
        std::cerr << "with no saved data, ";
    }

    std::cerr << "with " << pCreate->argc << " args" << endl;
    for (int i = 0; i < pCreate->argc; i++) {
        std::cerr << "\t" << pCreate->argv[i] << endl;
    }
#endif // _DEBUG

    npFreeWRLInstance *plugin = new npFreeWRLInstance(pCreate->instance);
    return plugin;
}


void
NS_DestroyPluginInstance(npInstanceBase* aPlugin)
{
#if _DEBUG
    std::cerr << "NS_DestroyPluginInstance!" << endl;
#endif // _DEBUG
    if (aPlugin)
        delete aPlugin;
}


// global plugin initialization and shutdown

NPError
NS_PluginInitialize()
{
#if _DEBUG
    std::cerr << "NS_PluginInitialize!" << endl;
#endif // _DEBUG

     // from sdk simple sample plugin:
     // this is probably a good place to get the service manager
     // note that Mozilla will add reference, so do not forget to release
//     nsISupports *sm = NULL;
//     NPN_GetValue(NULL, NPNVserviceManager, &gSupports);

     // Mozilla returns nsIServiceManagerObsolete which can be queried for nsIServiceManager
//     if (gSupports) {
//         gSupports->QueryInterface(NS_GET_IID(nsIServiceManager), (void**) &gServiceManager);
//         NS_RELEASE(sm);
//     }

    return NPERR_NO_ERROR;
}


void
NS_PluginShutdown()
{

#if _DEBUG
    std::cerr << "NS_PluginShutdown!" << endl;
#endif // _DEBUG
     // from sdk simple sample plugin:

     // we should release the service manager
//     NS_IF_RELEASE(gServiceManager);
//     NS_IF_RELEASE(gSupports);
//     gServiceManager = NULL;
//     gSupports = NULL;
}

/*
 * User-defined signal system call
 * (see Steven's book mentioned above):
 */
__sighandler_t
signal(int signo, __sighandler_t func)
{
#ifdef XP_UNIX
    struct sigaction action;
    struct sigaction old_action;
    /*
     * Initialize action's signal set as empty set
     * (see man page sigsetops(3)).
     */
    memset(&action, 0, sizeof(action));
    memset(&old_action, 0, sizeof(old_action));
    sigemptyset(&action.sa_mask);

    // don't bother with SIGCHLD or zombies 
    action.sa_flags = SA_NOCLDSTOP | SA_RESTART | SA_ONSTACK;
    action.sa_handler = func;

    if (sigaction(signo, &action, &old_action) < 0) {
        printCError("sigaction failed", errno);
        return SIG_ERR;
    }
    // Return the old signal handler or SIG_ERR.
    return old_action.sa_handler;    

#else // no other platforms supported at the moment
    return NULL;
#endif // XP_UNIX
}


void
printCError(const char *msg, int err)
{
    std::cerr << msg << ": " << strerror(err) << endl;
}
