/*
 * $Id$
 */

#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "npapi.h" /* netscape plugin api */
#include "nputils.h" /* netscape plugin utilities */

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#define _DEBUG 0

/*
 * Signal and sockets code based on Unix Network Programming
 * by W. Richard Stevens.
 *
 * XSub code based on perldoc documentation.
 * Use perldoc perlxs, perlxstut!!!
 */

MODULE = VRML::PluginGlue PACKAGE = VRML::PluginGlue
PROTOTYPES: ENABLE

char*
plugin_connect(fd, plugin_instance, url)
    int fd
    unsigned int plugin_instance
    const char *url
    CODE:
    {
	size_t len = 0;
	size_t bytes = 0;
	char cache_url[FILENAME_MAX];
	urlRequest request;

	request.instance = (NPP) plugin_instance;
	request.notifyCode = 0; /* not currently used */

	len = (strlen(url) + 1) * (sizeof(char));

	bzero(request.url, len);
	bytes = sizeof(request);

	/* safe string copying */
	memmove(request.url, url, len);

	if (write(fd, (urlRequest *) &request, bytes) < 0) {
	    perror("Call to write failed");
	    RETVAL = NULL;
	}

	bzero(cache_url, FILENAME_MAX);
	
	if (read(fd, cache_url, FILENAME_MAX) < 0) {
#if _DEBUG
	    perror("Call to read failed");
#else
	    /* If blocked or interrupted, be silent. */
  	    if (errno != EAGAIN && errno != EINTR) {
		perror("Call to read failed");
  	    }
#endif
	    RETVAL = NULL;
	}
	else {
	    RETVAL = cache_url;
	}
    }
    OUTPUT:
	RETVAL

void
close_fd(fd)
    int fd
    CODE:
    {
	close(fd);
    }
