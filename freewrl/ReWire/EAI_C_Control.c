
#include "Eai_C.h"
pthread_t readThread;
int readThreadInitialized = FALSE;

void X3D_initialize(char *hostname) {
	struct sockaddr_in serv_addr;
	struct hostent *server;
	int iret1;

	_X3D_FreeWRL_FD = socket(AF_INET, SOCK_STREAM, 0);
	if (_X3D_FreeWRL_FD < 0) X3D_error("ERROR opening socket");

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
	if (connect(_X3D_FreeWRL_FD,&serv_addr,sizeof(serv_addr)) < 0) 
		X3D_error("ERROR connecting");

	/* start up read thread */
	iret1 = pthread_create( &readThread, NULL, freewrlReadThread, NULL);
}



/* tell FreeWRL to shut down; don't worry about the return value */
void X3D_shutdown() {
	_X3D_makeShortCommand(STOPFREEWRL);
}
