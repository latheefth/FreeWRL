
/*	win32: include the headers of your operating system that define the size_t, fd_set, socklen_t and struct sockaddr data types and define MHD_PLATFORM_H
	linux: don't define MHD_PLATFORM_H and it will figure it out
	license MIT or similar - hack away, have fun
	SSR - server-side rendering - the user drags on a webgl canvas featuring a simple grid, and when
		they lift their mousebutton/touchfinger the client code ajaxes (asynchronous javascript and xml, via xmlhttprequest)
		the viewpoint pose to the server
		the server reponds by correcting the pose for gravity/collision and sends back
		then the client sends the corrected pose and asks via  for a snapshot
		the server takes a screen snapshot and sends back to clien
		the client shows the snapshot over the grid and waits for the next mousedown/touch to show the grid again
	SSRServer.exe this commandline program that takes Port (ie 8080 or your choice) on the command line,
		- runs linked-in libmicrohttpd C web server to listen and call functions below
		- you need to run this program in src/SSR/public to server SSRClient.html
		- serves SSRClient.html when in a browser you go: http://localhost:8080/SSRClient.html or -via your noip/dyndns hostname http://myhostname:8080/SSRClient.html
	Current state Feb 25, 2015:
		* works a bit on win32 and in up-to-date desktop web browsers IE, Chrome, FF
		x doesn't work in iPhone, BBPlaybook webbrowsers 
				- needs shims/polyfills in SSRClient.html for a few modern things window.requestNextAnimationFrame and xhr2 xmlhttprequest JSON/blob payloads
		* tested with free noip.com account, which re-directed to home server computer behind DSL modem
			- DSL modem had settings Advanced > DDNS (dynamic dns) > no-ip auto-update feature,
				and PortForwarding (set to our chosen PORT  8080 on WAN and LAN)
*/


#ifdef _MSC_VER
//WIN32
#define MHD_PLATFORM_H WIN32
#include <sys/types.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <ws2tcpip.h>
#define ssize_t size_t
#define off_t ptrdiff_t
#include <sys/stat.h>
#include <limits.h> 
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#define R_OK 4
#define X_OK 4
#define O_NONBLOCK 0
#define O_BLOCK 0
#define SSIZE_MAX 100000000L
#define close _close
#define open _open
#define read _read
#define write _write
//END WIN32
#endif
// in theory scene_path could/should be a command line arg, or something the server helps the user upload or select on another page
#define scene_path "http://dug9.users.sourceforge.net/web3d/townsite_2014/townsite.x3d" //"C:/Users/Public/dev/source2/placeholders/townsite_2014/townsite.x3d"
#include <cdllFreeWRL.h>
#define SSR_SERVER 1
#include "../lib/SSRhelper.h"
int run_fw = 1;
void *fwctx = NULL;
int runFW(char *url){
	if(run_fw){
		fwctx = dllFreeWRL_dllFreeWRL();
		dllFreeWRL_onInit(fwctx,400,300,NULL,FALSE,FALSE);
		dllFreeWRL_onLoad(fwctx,url); 
	}
	return 0;
}
int stopFW(){
	if(run_fw)
		dllFreeWRL_onClose(fwctx);
	return 0;
}



#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PAGE "<html><head><title>libmicrohttpd demo</title>"\
			"</head><body>libmicrohttpd demo</body></html>"

//the iterate_post, request_completed functions and connection_info_struct 
// were copied from the POST tutorial at:
// http://www.gnu.org/software/libmicrohttpd/tutorial.html
#define PORT            8888
#define POSTBUFFERSIZE  512
#define MAXNAMESIZE     20
#define MAXANSWERSIZE   512

#define GET             0
#define POST            1

struct connection_info_struct
{
	int connectiontype;
	char *answerstring;
	int len;
	struct MHD_PostProcessor *postprocessor;
};

const char *askpage = "<html><body>\
                       What's your name, Sir?<br>\
                       <form action=\"/namepost\" method=\"post\">\
                       <input name=\"name\" type=\"text\"\
                       <input type=\"submit\" value=\" Send \"></form>\
                       </body></html>";

const char *greetingpage =
  "<html><body><h1>Welcome, %s!</center></h1></body></html>";

const char *errorpage =
  "<html><body>This doesn't seem to be right.</body></html>";


static int
send_page (struct MHD_Connection *connection, const char *page, int len)
{
	int ret;
	struct MHD_Response *response;


	response = MHD_create_response_from_buffer (len, (void *) page, MHD_RESPMEM_PERSISTENT);
	if (!response)
		return MHD_NO;

	ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
	MHD_destroy_response (response);

	return ret;
}
#define MAXPOSESIZE 255
static void jsonPose2double(double *quat4, double *vec3, char *data){
	sscanf(data,"[%lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf]",
		&quat4[0],&quat4[1],&quat4[2],&quat4[3],
		&vec3[0],&vec3[1],&vec3[2]);
}
static void doublePose2json(double *quat4, double *vec3, char *data, int MAX){
	sprintf(data,"[%lf, %lf, %lf, %lf, %lf, %lf, %lf]",
		quat4[0],quat4[1],quat4[2],quat4[3],
		vec3[0],vec3[1],vec3[2]);
}
static char* getSnapshot(int *len){
	struct stat ss;
	int fd;
	ssize_t blocksz; //, left2read;
	char *filename = "2.jpg";
	if (stat(filename, &ss) < 0) {
		printf("load_file_read: could not stat: %s\n", filename);
		return MHD_NO;
	}
	if (!ss.st_size) {
		printf("load_file_read: file is empty %s\n", filename);
		return MHD_NO;
	}
	if (ss.st_size > SSIZE_MAX) {
		/* file is greater that read's max block size: we must make a loop */
		blocksz = SSIZE_MAX;
	} else {
		blocksz = ss.st_size;
	}
	fd = open(filename, O_RDONLY |  O_BINARY | O_BLOCK);
	if (fd < 0) {
		printf("load_file_read: could not open: %s\n", filename);
		return MHD_NO;
	}
	//response = MHD_create_response_from_fd(ss.st_size,fd); //didn't work in win32, complained ERR_CONTENT_LENGTH_MISMATCH in client
	// will be closed when response is destroyed
	//close(fd);

	char *blob = malloc(blocksz);
	read(fd,blob,blocksz);
	//on win32, it works most reliably -without crashing- if I say for it to deep copy the blob, and free its own copy
	//then free my copy here
	//response = MHD_create_response_from_data(blocksz,(void*) blob,	MHD_NO, MHD_YES);
	//free(blob);
	close(fd);
	*len = blocksz;
	return blob;

}
static int iterate_post (void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
	const char *filename, const char *content_type,
	const char *transfer_encoding, const char *data, 
	uint64_t off, size_t size)
{
	struct connection_info_struct *con_info = coninfo_cls;

	if (0 == strcmp (key, "posepose"))
	{
		if ((size > 0) && (size <= MAXPOSESIZE)) //MAXNAMESIZE
		{
			char *answerstring;
			//double quat4[4], vec3[3];
			SSR_request ssr_req;
			answerstring = malloc (MAXANSWERSIZE);
			if (!answerstring) return MHD_NO;
			ssr_req.type = SSR_POSEPOSE;
			jsonPose2double(ssr_req.quat4,ssr_req.vec3,data);
			dllFreeWRL_SSRserver_enqueue_request_and_wait(fwctx, &ssr_req);
			doublePose2json(ssr_req.quat4,ssr_req.vec3, answerstring, MAXANSWERSIZE);
			//_snprintf (answerstring, MAXANSWERSIZE, greetingpage, data);
			con_info->answerstring = answerstring;  
			con_info->len = strlen(answerstring);    
		} 
		else con_info->answerstring = NULL;

		return MHD_NO;
	}
	if (0 == strcmp (key, "posesnapshot"))
	{
		if ((size > 0) && (size <= MAXPOSESIZE)) //MAXNAMESIZE
		{
   			char *answerstring;
			SSR_request ssr_req;
			ssr_req.type = SSR_POSESNAPSHOT;
			ssr_req.blob = NULL;
			ssr_req.len = 0;
			jsonPose2double(ssr_req.quat4,ssr_req.vec3,data);
			dllFreeWRL_SSRserver_enqueue_request_and_wait(fwctx, &ssr_req);
			con_info->answerstring = ssr_req.blob;
			con_info->len = ssr_req.len;    
		} 
		else con_info->answerstring = NULL;

		return MHD_NO;
	}


	return MHD_YES;
}
void request_completed (void *cls, struct MHD_Connection *connection, 
	void **con_cls,
	enum MHD_RequestTerminationCode toe)
{
	struct connection_info_struct *con_info = *con_cls;

	if (NULL == con_info) return;
	if (con_info->connectiontype == POST)
	{
		MHD_destroy_post_processor (con_info->postprocessor);        
		if (con_info->answerstring) free (con_info->answerstring);
	}
  
	free (con_info);
	*con_cls = NULL;   
}

int haveNasties(char *url){
	// check for nasties 
	//- I don't seem to need any nasty checking:
	//don't want them to fetch ./../../../passowrds.txt or anything above our public folder, 
	// but the .. get cleaned out on the client end, if you trust the clients, 
	// and %20 or %2E (.) get converted to ascii '.', and cleaned out, before this callback
	//on windows, don't want them to fetch C:/passwords.txt, 
	// but it seems impossible anyway, because it comes in as /C:/passwords.txt which is nonsense so file not found
	//putting %08 (backspace, \b) in the url gets in here, and /%08C:/tmp/passwords.html printfs as C:/tmp/passwords.html,
	// but file functions don't back up, so \b stays in filepath and it can't find it
	int urllen, nasties = 0;
	urllen = strlen(url);
	for(int i=0;i<urllen;i++){
		if(i<urllen-1)
			if(url[i]=='.' && url[i+1] == '.') nasties++; 
		if(url[i]==':') nasties++;  
		if(url[i]=='\b') nasties++; //backspace
		//Q. are there other nasties we should check for?
		//Q. have incoming urls converted %20 and %specials to ascii
	}
	//end check for nasties
	return nasties;
}

static int answer_to_connection (void *cls, struct MHD_Connection *connection, 
	const char *url, 
	const char *method, const char *version, 
	const char *upload_data, 
	size_t *upload_data_size, void **con_cls)
{
	if(NULL == *con_cls) 
	{
		struct connection_info_struct *con_info;

		con_info = malloc (sizeof (struct connection_info_struct));
		if (NULL == con_info) return MHD_NO;
		con_info->answerstring = NULL;
	//If the new request is a POST, the postprocessor must be created now. In addition, the type of the request is stored for convenience.

		if (0 == strcmp (method, "POST")) 
		{      
			con_info->postprocessor = MHD_create_post_processor (connection, POSTBUFFERSIZE, 
				iterate_post, (void*) con_info);   
			if (NULL == con_info->postprocessor) 
			{
				free (con_info); 
				return MHD_NO;
			}
			con_info->connectiontype = POST;
		} 
		else 
			con_info->connectiontype = GET;
		//The address of our structure will both serve as the indicator for successive iterations and to remember the particular details about the connection.

		*con_cls = (void*) con_info; 
		return MHD_YES;
	}
	//The rest of the function will not be executed on the first iteration. A GET request is easily satisfied by sending the question form.
	if (0 == strcmp (method, "GET")) 
	{
		//return send_page (connection, askpage);   
		// maybe valid GET request
		struct MHD_Response * response;
		struct stat ss;
		int fd, ret;
		ssize_t blocksz; //, left2read;
		char *filename = &url[1];
		if (stat(filename, &ss) < 0) {
			printf("load_file_read: could not stat: %s\n", filename);
			return MHD_NO;
		}
		if (!ss.st_size) {
			printf("load_file_read: file is empty %s\n", filename);
			return MHD_NO;
		}
		if (ss.st_size > SSIZE_MAX) {
			/* file is greater that read's max block size: we must make a loop */
			blocksz = SSIZE_MAX;
		} else {
			blocksz = ss.st_size;
		}
		fd = open(filename, O_RDONLY |  O_BINARY | O_BLOCK);
		if (fd < 0) {
			printf("load_file_read: could not open: %s\n", filename);
			return MHD_NO;
		}
		//response = MHD_create_response_from_fd(ss.st_size,fd); //didn't work in win32, complained ERR_CONTENT_LENGTH_MISMATCH in client
		// will be closed when response is destroyed
		//close(fd);

		char *blob = malloc(blocksz);
		read(fd,blob,blocksz);
		//on win32, it works most reliably -without crashing- if I say for it to deep copy the blob, and free its own copy
		//then free my copy here
		response = MHD_create_response_from_data(blocksz,(void*) blob,	MHD_NO, MHD_YES);
		free(blob);
		close(fd);
		ret = MHD_queue_response(connection,MHD_HTTP_OK,response);
		MHD_destroy_response(response);
		return ret;

	} 
	//In case of POST, we invoke the post processor for as long as data keeps incoming, setting *upload_data_size to zero in order to indicate that we have processed—or at least have considered—all of it.
	if (0 == strcmp (method, "POST")) 
	{
		struct connection_info_struct *con_info = *con_cls;
		if (*upload_data_size != 0) 
		{
			MHD_post_process (con_info->postprocessor, upload_data,	
				*upload_data_size);
			*upload_data_size = 0;
			return MHD_YES;
		} 
		else 
			if (NULL != con_info->answerstring) 
				return send_page (connection, con_info->answerstring, con_info->len);
	} 
	//Finally, if they are neither GET nor POST requests, the error page is returned.
	return send_page(connection, errorpage, strlen(errorpage)); 
}

// simple handler - no post guts
static int ahc_echo(void * cls,
	struct MHD_Connection * connection,
	const char * url,
	const char * method,
	const char * version,
	const char * upload_data,
	size_t * upload_data_size,
	void ** ptr) 
{
	static int dummy;
	const char * page = cls;
	struct MHD_Response * response;
	int ret;

	if (!strcmp(method, "POST"))
	{
		//POST handler - we'll come in here for our special goodies:
		// come in with viewpoint pose matrix, leave with screen snapshot and updated pose matrix
		printf("url=%s\n",url);
	}

	if(!strcmp(method, "GET"))
	{
		//GET handler - we'll come in here for our ordinary static pages, glMatrix.js etc

		if (&dummy != *ptr)
		{
			/* The first time only the headers are valid,
				do not respond in the first round... */
			*ptr = &dummy;
			return MHD_YES;
		}
		if (0 != *upload_data_size)
			return MHD_NO; /* upload data in a GET!? */
		*ptr = NULL; /* clear context pointer */
		printf(" %p ",connection);
		if(!strcmp(url,"/") || haveNasties(url)){
			// invalid GET request
			response = MHD_create_response_from_data(strlen(page),
				(void*) page,
				MHD_NO,
				MHD_NO);
		}else{
			// maybe valid GET request
			struct stat ss;
			int fd;
			ssize_t blocksz; //, left2read;
			char *filename = &url[1];
			if (stat(filename, &ss) < 0) {
				printf("load_file_read: could not stat: %s\n", filename);
				return MHD_NO;
			}
			if (!ss.st_size) {
				printf("load_file_read: file is empty %s\n", filename);
				return MHD_NO;
			}
			if (ss.st_size > SSIZE_MAX) {
				/* file is greater that read's max block size: we must make a loop */
				blocksz = SSIZE_MAX;
			} else {
				blocksz = ss.st_size;
			}
			fd = open(filename, O_RDONLY |  O_BINARY | O_BLOCK);
			if (fd < 0) {
				printf("load_file_read: could not open: %s\n", filename);
				return MHD_NO;
			}
			//response = MHD_create_response_from_fd(ss.st_size,fd); //didn't work in win32, complained ERR_CONTENT_LENGTH_MISMATCH in client
			// will be closed when response is destroyed
			//close(fd);

			char *blob = malloc(blocksz);
			read(fd,blob,blocksz);
			//on win32, it works most reliably -without crashing- if I say for it to deep copy the blob, and free its own copy
			//then free my copy here
			response = MHD_create_response_from_data(blocksz,(void*) blob,	MHD_NO, MHD_YES);
			free(blob);
			close(fd);
		}
		ret = MHD_queue_response(connection,MHD_HTTP_OK,response);
		MHD_destroy_response(response);

	}else{
		ret = MHD_NO; /* unexpected method, not GET or POST */
	}
	return ret;
}

// see libmicrohttpd docs on this site:
// http://www.gnu.org/software/libmicrohttpd/
// tutorials: http://www.gnu.org/software/libmicrohttpd/tutorial.html
int main0(int argc, char ** argv) {
	struct MHD_Daemon * d;
	char *portstr, *url;
	if (argc < 2) {
		portstr = "8080";
		printf("%s PORT\n",portstr);
		url = scene_path;
		//return 1;
	}else{
		portstr = argv[1];
		if(argc < 3)
			url = scene_path;
		else
			url = argv[2];
	}
	if(0)
	d = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION,
		atoi(portstr),
		NULL,
		NULL,
		&ahc_echo,
		PAGE,
		MHD_OPTION_END);
	if(1)
	d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION, //MHD_USE_SELECT_INTERNALLY
		atoi(portstr), //PORT, 
		NULL, 
		NULL,
		&answer_to_connection, 
		PAGE, 
		MHD_OPTION_NOTIFY_COMPLETED, 
		&request_completed, 
		NULL,
		MHD_OPTION_END);
	if (d == NULL)
		return 1;
	runFW(url);
	printf("Press Enter to stop libmicrohttp deamon and exit:");
	getchar();
	stopFW();
	MHD_stop_daemon(d);
	return 0;
}
#ifdef _MSC_VER
#include <direct.h>
#define chdir _chdir
#endif
int main(int argc, char ** argv) {
	if(strstr(argv[0],"projectfiles")){
		//developer folders, use src/SSR/public for Client.html
		char *pf, *path;
		path = strdup(argv[0]);
		pf = strstr(path,"projectfiles");
		strcpy(pf,"src/SSR/public");
		printf("%s\n",path);
		chdir(path);
	}else{
		//installed, use folder beside ssrserver.exe (PS installer please install public folder with binary distro)
		chdir("./public");
	}
	int iret = main0(argc, argv);
	if(iret){
		printf("Press Enter to exit:");
		getchar();
	}
}