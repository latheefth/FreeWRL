/*
 * $Id$
 */

#include "pluginSocket.h"

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

/*
 * Signal and sockets code based on Unix Network Programming
 * by W. Richard Stevens.
 *
 * XSub code based on perldoc documentation.
 * Use perldoc perlxs, perlxstut!!!
 */


MODULE = VRML::PluginGlue PACKAGE = VRML::PluginGlue
PROTOTYPES: ENABLE


int
requestUrl(fd, plugin_instance, url, return_url)
	int fd
	unsigned int plugin_instance
	const char *url
	char *return_url

int
connectToPlugin(server)
    const char *server

int
setIOOptions(sockDesc, pid, nonblock, timeout)
	int sockDesc
	pid_t pid
	int nonblock
	int timeout

void
closeFileDesc(fd)
	 int fd
CODE:
{
	close(fd);
}
