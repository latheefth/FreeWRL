/*
 * $Id$
 *
 * Common functions used by Mozilla and Netscape plugins...(maybe
 * PluginGlue too?)
 */

#include "PluginSocket.h"

int
createUDPSocket()
{
	int sockDesc = 0;
	
    if ((sockDesc = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket failed");
		return SOCKET_ERROR;
    }

	return sockDesc;
}


int
setIOOptions(int sockDesc,
			 pid_t pid,
			 int nonblock,
			 int timeout)
{
	struct timespec ts;
    int signo = 0; /* int io_flags = 0; */
	const int on = 1;

	ts.tv_sec = PLUGIN_TIMEOUT_SEC;
	ts.tv_nsec = PLUGIN_TIMEOUT_NSEC;


	if (nonblock) {
		/* use signals */
		#ifndef __APPLE__
		if (fcntl(sockDesc, F_GETSIG, signo) < 0) {
			perror("fcntl with command F_GETSIG failed");
			return SOCKET_ERROR;
		}

		if (signo != SIGIO || signo != 0) {
			if (fcntl(sockDesc, F_SETSIG, SIGIO) < 0) {
				perror("fcntl with command F_SETSIG failed");
				return SOCKET_ERROR;
			}
		}
		#endif

		/* F_SETOWN is specific to BSD and Linux. */
		if (fcntl(sockDesc, F_SETOWN, (pid_t) pid) < 0) {
			perror("fcntl with command F_SETOWN failed");
			 return SOCKET_ERROR;
		 }

		 if (ioctl(sockDesc, FIOASYNC, &on) < 0) {
			 perror("ioctl with request FIOASYNC failed");
			 return SOCKET_ERROR;
		 }

		 if (ioctl(sockDesc, FIONBIO, &on) < 0) {
			 perror("ioctl with request FIONBIO failed");
			 return SOCKET_ERROR;
		 }
	 }

	if (timeout) {
		if (setsockopt(sockDesc, SOL_SOCKET, SO_RCVTIMEO, &ts, sizeof(ts)) < 0) {
			perror("setsockopt with option SO_RCVTIMEO failed");
			return SOCKET_ERROR;
		}

		if (setsockopt(sockDesc, SOL_SOCKET, SO_SNDTIMEO, &ts, sizeof(ts)) < 0) {
			perror("setsockopt with option SO_SNDTIMEO failed");
			return SOCKET_ERROR;
		}
	}

    return NO_ERROR;
}

char *
requestUrlfromPlugin(int sockDesc,
		   unsigned int plugin_instance,
		   const char *url)
{
	int flags = 0;
	size_t len = 0, ulen = 0, bytes = 0;
	char return_url[FILENAME_MAX];
	urlRequest request;

	request.instance = (void *) plugin_instance;
	request.notifyCode = 0; /* not currently used */

	len = FILENAME_MAX * sizeof(char);
	memset(request.url, 0, len);
	memset(return_url, 0, len);

	ulen = strlen(url) + 1;
	memmove(request.url, url, ulen);
	bytes = sizeof(urlRequest);


#if FALSE
#ifdef MSG_CONFIRM
	flags |= MSG_CONFIRM;
#endif /* MSG_CONFIRM */
#endif /* FALSE */

	if (write(sockDesc, (urlRequest *) &request, bytes) < 0) {
		perror("write failed in requestUrlfromPlugin");
		/* return SOCKET_ERROR; */
		return NULL;
	}

#if FALSE
	flags |= MSG_WAITALL;
#endif /* FALSE */
	
	//if (read(sockDesc, (char *) return_url, FILENAME_MAX) < 0) {
	if (read(sockDesc, (char *) return_url, len) < 0) {
		perror("read failed in requestUrlfromPlugin");
 		 /* If blocked or interrupted... */
/* 		if (errno != EAGAIN && errno != EINTR) { */
/* 			return SOCKET_ERROR; */
/* 		} */
fprintf(stderr, "Testing: error from read -- returned url is %s.\n", return_url);
		/* return SOCKET_ERROR; */
		return NULL;
	}

	return return_url;
}

int
receiveUrl(int sockDesc, urlRequest *request)
{
    sigset_t newmask, oldmask;
	size_t len = 0, request_size = 0;
	
	
	len = FILENAME_MAX * (sizeof(char));
	memset(request->url, 0, len);
    request->instance = 0;
    request->notifyCode = 0; // unused
    request_size = sizeof(urlRequest);

    /*
     * The signal handling code is based on the work of
     * W. Richard Stevens from Unix Network Programming,
     * Networking APIs: Sockets and XTI.
     */

    // Init. the signal sets as empty sets.
    if (sigemptyset(&newmask) < 0) {
        perror("sigemptyset with arg newmask failed");
        return SIGNAL_ERROR;
    }
    
    if (sigemptyset(&oldmask) < 0) {
        perror("sigemptyset with arg oldmask failed");
        return SIGNAL_ERROR;
    }

    if (sigaddset(&newmask, SIGIO) < 0) {
        perror("sigaddset failed");
        return SIGNAL_ERROR;
    }

    /* Code to block SIGIO while saving the old signal set. */
    if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0) {
        perror("sigprocmask failed");
        return SIGNAL_ERROR;
    }
    
    if (read(sockDesc, (urlRequest *) request, request_size) < 0) {
		perror("recv failed");
        /* If blocked or interrupted... */
/*     	if (errno != EINTR && errno != EAGAIN) { */
/* 			return SOCKET_ERROR; */
/*     	} */
		return SOCKET_ERROR;
    }


    /* Restore old signal set, which unblocks SIGIO. */
    if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
        perror("sigprocmask failed");
        return SIGNAL_ERROR;
    }
    
    return NO_ERROR;
}


int
pluginBind(struct sockaddr_in *addr)
{
	int sockDesc;
	// socklen_t addrLen = 0;
	int addrLen = 0;

    if ((sockDesc = createUDPSocket()) < 0) {
        fprintf(stderr, "createUDPSocket failed.\n");
        return SOCKET_ERROR;
    }

	memset(addr, 0, sizeof(struct sockaddr_in));

	addr->sin_family = PF_INET;
	addr->sin_addr.s_addr = htonl(INADDR_ANY);
	addr->sin_port = htons(PLUGIN_PORT);

	addrLen = sizeof(struct sockaddr_in);

    if (bind(sockDesc, (struct sockaddr *) addr, addrLen) < 0) {
		perror("bind failed");
		return SOCKET_ERROR;
    }

	return sockDesc;
}


int
connectToPlugin(const char *host)
{
	int sockDesc;
	struct sockaddr_in addr;
	struct hostent *he;
	struct in_addr in;

    if ((sockDesc = createUDPSocket()) < 0) {
        fprintf(stderr, "createUDPSocket failed.\n");
        return SOCKET_ERROR;
    }

	if ((he = gethostbyname(host)) == NULL) {
		perror("gethostbyname failed");
        return SOCKET_ERROR;
	}

	/* use the first address */
	memcpy(&in.s_addr, *(he->h_addr_list), sizeof(struct in_addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PLUGIN_PORT);
	addr.sin_addr = in;

	if (connect(sockDesc, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("connect failed");
        return SOCKET_ERROR;
	}

	return sockDesc;
}
