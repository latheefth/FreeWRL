/*
 * $Id$
 */

#ifndef __pluginSocket_h__
#define __pluginSocket_h__


#ifdef XP_UNIX

#include <unistd.h>

#endif /* XP_UNIX */

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "pluginUtils.h"


#ifdef __cplusplus
extern "C" {
#endif

	
int
createUDPSocket();

int
setIOOptions(int sockDesc,
			 pid_t pid,
			 int nonblock,
			 int timeout);

int
requestUrl(int sockDesc,
		   unsigned int plugin_instance,
		   const char *url,
		   char *return_url);

int
receiveUrl(int sockDesc, urlRequest *request);


int
pluginBind(struct sockaddr_in *addr);

/* use a "connected" UDP socket */ 
int
connectToPlugin(const char *server);


#ifdef __cplusplus
}
#endif

	
#endif /*__pluginSocket_h__ */
