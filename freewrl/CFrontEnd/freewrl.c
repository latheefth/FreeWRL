/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/* beginnings of FreeWRL in C */
#include "EXTERN.h"
#include "perl.h"
#include <Structs.h>
#include <pthread.h>
#include <headers.h>

#ifdef AQUA

#include <gl.h>
#include <glu.h>
#include <glext.h>

#else

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <GL/glext.h>

#endif

#ifdef printf
#undef printf
#endif

// display and win are opened here, then pointers passed to
// freewrl. We just use these as unsigned, because we just
// pass the pointers along; we do not care what type they are.
Display *Disp;
Window Win;
GLXContext globalContext;

/* threading variables for event loop */
static pthread_t *loopthread;
char *threadmsg = "eventloop";
pthread_t thread1;


/* function prototypes */
//extern void xs_init(void);
extern void openMainWindow(Display *dpy, Window *win,
		GLXContext *glocx);
extern unsigned rootNode;
void displayThread();
extern void glpOpenGLInitialize(void);
extern void new_tessellation(void);
extern char *BrowserURL;
extern produceTask(unsigned a,char *b,unsigned ptr, unsigned ofs);
extern PerlInterpreter *my_perl;

int main (int argc, char **argv, char **env) {
	int retval;
	int count;

	BrowserURL = "./";
	/* create the display thread. */
	pthread_create (&thread1, NULL, (void *)&displayThread, (void *)threadmsg);

	/* create the initial scene, from the file passed in
	and place it as a child of the rootNode. */

	produceTask(FROMURL, 
		"./s.wrl",
		rootNode, offsetof (struct VRML_Group, children));

	/* now wait around until something kills this thread. */
	pthread_join(thread1, NULL);

	perl_destruct(my_perl);
	perl_free(my_perl);
}


/* handle all the displaying and event loop stuff. */
void displayThread() {
	int count;
	openMainWindow(Disp,&Win,&globalContext);
	glpOpenGLInitialize();
	new_tessellation();

	while (1==1) {
		EventLoop();
	}
}
