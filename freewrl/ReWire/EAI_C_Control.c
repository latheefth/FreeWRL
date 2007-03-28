
#include "Eai_C.h"
#include <stdio.h>
#include <sys/errno.h>

pthread_t readThread;
int readThreadInitialized = FALSE;

void X3D_initialize(char *hostname) {
	struct sockaddr_in serv_addr; 
	struct hostent *server;
	int iret1;
	int loopCount;
	int constat;

	loopCount = 0;
	while ((_X3D_FreeWRL_FD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf ("socket %d waiting...\n",_X3D_FreeWRL_FD);
		usleep (100000);
		loopCount ++;
		if (loopCount >= 10000) {
			X3D_error("ERROR opening socket");
			return;
		}
	}

	printf ("socket %d\n",_X3D_FreeWRL_FD);
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
printf ("connecting to socket\n");
	while ((constat = connect(_X3D_FreeWRL_FD,(struct sockaddr *) &serv_addr,sizeof(serv_addr))) < 0) {
printf ("connection failed; errno %d\n",errno);
		usleep (100000);
		loopCount ++;
		if (loopCount >= 10000) {
			X3D_error("ERROR connecting to socket - FreeWRL not there?");
			return;
		}
	}

printf ("now starting freewrlreadthread\n");
	/* start up read thread */
	iret1 = pthread_create( &readThread, NULL, freewrlReadThread, NULL);
}



/* tell FreeWRL to shut down; don't worry about the return value */
void X3D_shutdown() {
	_X3D_makeShortCommand(STOPFREEWRL);
}
