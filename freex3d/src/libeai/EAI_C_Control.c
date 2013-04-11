
/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#include "EAI_C.h"
#include "config.h"
#include "system.h"
#include <stdio.h>
#ifndef WIN32
#include <sys/errno.h>
#endif

pthread_t readThread;
pthread_t swigThread;
int readThreadInitialized = FALSE;

void X3D_initialize(char *hostname) {
	struct sockaddr_in serv_addr; 
	struct hostent *server;
	int iret1;
	#ifdef WIN32
		int iret2;
		WORD wVersionRequested;
		WSADATA wsaData;
		int err;
	#endif
	int loopCount;
	int constat;
		
#ifdef WIN32
	wVersionRequested = MAKEWORD( 2, 2 );
	 
	err = WSAStartup( wVersionRequested, &wsaData );
	
	if ( err != 0 ) 
	{
		int socket_error = WSAGetLastError();

		switch(socket_error)
		{
		case WSASYSNOTREADY:
			//The underlying network subsystem is not ready for network communication.
			break;
		case WSAVERNOTSUPPORTED:
			//The version of Windows Sockets support requested is not provided by this particular Windows Sockets implementation.
			break;
		case WSAEINPROGRESS:
			//A blocking Windows Sockets 1.1 operation is in progress.
			break;
		case WSAEPROCLIM:
			//A limit on the number of tasks supported by the Windows Sockets implementation has been reached.
			break;
		case WSAEFAULT:
			//The lpWSAData parameter is not a valid pointer.
			break;
		default:
			break;
		}
	}
#endif

	loopCount = 0;

	while ((_X3D_FreeWRL_FD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {

#ifdef WIN32
		int socket_error = WSAGetLastError();

		switch(socket_error)
		{
		case (WSANOTINITIALISED):
			//The network subsystem or the associated service provider has failed.
			break;
		case (WSAENETDOWN):
			//The network subsystem or the associated service provider has failed.
			break;
		case (WSAEAFNOSUPPORT):
			//The specified address family is not supported.
			break;
		case (WSAEINPROGRESS):
			//A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.
			break;
		case (WSAEMFILE):
			//No more socket descriptors are available.
			break;
		case (WSAENOBUFS):
			//No buffer space is available. The socket cannot be created.
			break;
		case (WSAEPROTONOSUPPORT):
			//The specified protocol is not supported.
			break;
		case (WSAEPROTOTYPE):
			//The specified protocol is the wrong type for this socket.
			break;
		case (WSAESOCKTNOSUPPORT):
			//The specified socket type is not supported in this address family.
			break;
		default:
			//unspecified error...
			break;
		}
#endif

		usleep (100000);
		loopCount ++;
		if (loopCount >= 10000) {
			X3D_error("ERROR opening socket");
			return;
		}
	}

	usleep (10000);	/* let remote end settle down to this interruption */

	if (strlen(hostname) == 0) hostname = "localhost";

	server = gethostbyname(hostname);
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host :%s:\n",hostname);
		exit(0);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, 
		 (char *)&serv_addr.sin_addr.s_addr,
		 server->h_length);

		serv_addr.sin_port = htons(EAIBASESOCKET);

	loopCount = 0;
	while ((constat = connect(_X3D_FreeWRL_FD,(struct sockaddr *) &serv_addr,sizeof(serv_addr))) < 0) {
		usleep (100000);
		loopCount ++;
		if (loopCount >= 10000) {
			X3D_error("ERROR connecting to socket - FreeWRL not there?");
			return;
		}
	}


	/* start up read thread */
	iret1 = pthread_create( &readThread, NULL, freewrlReadThread, NULL);

	/* start up thread to allow connections from SWIG (or similar) */
	/* iret2 = pthread_create(&swigThread, NULL, freewrlSwigThread, NULL); */
//#ifdef WIN32
//	iret2 = pthread_create(&swigThread, NULL, freewrlSwigThread, NULL); 
//#endif
}



/* tell FreeWRL to shut down; don't worry about the return value */
void X3D_shutdown() {
	_X3D_makeShortCommand(STOPFREEWRL);
#ifdef WIN32
	/* cleanup I suspect it never gets here */
    closesocket(_X3D_FreeWRL_FD);
    WSACleanup();
#endif
}
