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

// based on plugin.h
// from mozilla plugin sdk 

#ifndef _npFreeWRL_h_
#define _npFreeWRL_h_

#include <string>
#include <iostream>
#include <cstdlib>
#include <csignal>
#include <cstdio>

#ifdef XP_UNIX
#include <sys/wait.h>
#endif // XP_UNIX

// #include "nsWindow.h"

#include "pluginSocket.h"

#include "nsISupports.h"
#include "nsIServiceManager.h"
#include "nsIMemory.h" // needed ???

#include "npInstanceBase.h"

// from mozilla plugin sdk and samples - plugin.h

class npFreeWRLInstance : public npInstanceBase
{
public:
	npFreeWRLInstance(NPP instance);
	npFreeWRLInstance();
	virtual ~npFreeWRLInstance();
	
	virtual NPBool init(NPWindow *aWindow);
	virtual NPBool isInitialized();
	virtual void shut();

    virtual NPError SetWindow(NPWindow *pNPWindow);
    

    virtual NPError NewStream(NPMIMEType type,
                              NPStream *stream,
                              NPBool seekable,
                              uint16 *stype);
    

    virtual NPError DestroyStream(NPStream *stream,
                                  NPError reason);
    

    virtual void StreamAsFile(NPStream *stream,
                              const char *fname);
    

    virtual int32 WriteReady(NPStream *stream);
    

    virtual int32 Write(NPStream *stream,
                        int32 offset,
                        int32 len,
                        void *buffer);
    

    virtual void Print(NPPrint *printInfo);
    

    virtual uint16 HandleEvent(void *event);
    

    virtual void URLNotify(const char *url,
                           NPReason reason,
                           void *notifyData);
    

    virtual NPError GetValue(NPPVariable variable,
                             void *value);
    

    virtual NPError SetValue(NPNVariable variable,
                             void *value);
    
#ifdef XP_UNIX

    NPError resize(void);

#endif // XP_UNIX

	void getVersion(char **aVersion);
	void showVersion();
	void clear();
    void signalHandler(int signo);
    PlatformInfo * getPlatformInfo();

private:
	NPP mInstance;
    PlatformInfo mPlatform;

	NPBool mInitialized;
	NPBool mFreeWRLAlive;

    pid_t freeWRLPid;
    urlRequest lastRequest;
    struct sockaddr_in addr;
    int socketDesc;

    NPError runFreeWRL(const char *fileName);
};

// static nsIServiceManager *gServiceManager = NULL;
// static nsISupports *gSupports = NULL;


static void exposeHandler(Widget widget,
                          XtPointer closure,
                          XEvent* event,
                          Boolean* dispatchContinue);

static void structHandler(Widget widget,
                             XtPointer data,
                             XEvent* event,
                             Boolean* dispatchContinue);

static void substructHandler(Widget widget,
                             XtPointer data,
                             XEvent* event,
                             Boolean* dispatchContinue);

static void printCError(const char *msg, int err);
#if _DEBUG
static void printWaitpidStatus(int status);
#endif // _DEBUG

#endif // _npFreeWRL_h_
