
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
	Apr 10, 2015:
		* ZoneBalancer:
			- will be adding a geographic zoneserver/loadbalancer/reverseproxy/gateway
			- ZoneBalancer acts as a gateway between LAN and internet
			- gets request from an internet client
			- sniffs the request content for geographic coords
			- looks up in a table of {polygon,IP:PORT} using point-in-polygon algorithm 
				to determine which zone polygon the client's viewer is in, then uses 
				the associated IP and port number to call regular SSRserver,
				wait for response, then relay response back to internet client
			- benefit: more CPUs, GPUs and memory slots can be used for very large 3D datasets
				allowing the solution to scale up using ordinary desktop computers which these days 
				have 32GB RAM limit (4x8GB). So if your data has a 96GB footprint in memory, you could use
				3 SSRservers and 1 ZoneBalancer to distribute the load
			- for programmer convenience the ZoneBalancer will be just a runtime configuration of SSRserver
		In SSRserver.c response handler pseudocode:
		if( static html) {
			serve directly (ie SSRClient.html)
		} else if working as SSRserver {
			render snapshot and return response - the original SSRserver configuration
		} else if working as zone balancer {
			1. sniff request for geographic coords of client's ViewPose/avatar
			2. do point-in-polygon on a list of polygons to determine which zone the avatar is in
				and which IP:Port / SSRServer instance to relay to
			3. open a TCP connection using sockets, and forward the request
			4. wait for response from SSR on the ZoneBalancer's request handler thread
			5. when response arrives, copy it to the zoneBalancer's response and return response to client
		}
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

//=========ZONE BALANCER==>>>>>>>>>>>>
static int running_as_zonebalancer = 0;
#ifdef _MSC_VER
#include <direct.h>
#define chdir _chdir
#define strcasecmp _stricmp
WSADATA wsaData;
void initialize_sockets(){
	int iResult;
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
	}
}
#else
void initialize_sockets(){}
#endif

//POINT IN POLY
typedef struct Point {
	double x;
	double y;
} Point;

typedef struct zone {
	char *name;
	Point center;
	Point *poly;
	int n;
	char * ssrname;
	void * ssr;
	void * next;
} zone;
typedef struct ssr {
	char *name;
	char *ip;
	char *port;
	void *next;
} ssr;
static zone *zones = NULL;
static ssr *ssrs = NULL;
void ssr_list_add(ssr *s){
	if(!ssrs){
		ssrs = s;
		s->next = NULL;
	}else{
		ssr *cur = ssrs;
		while(cur->next)
			cur = cur->next;
		cur->next = s;
		s->next = NULL;
	}
}
void zone_list_add(zone *z){
	if(!zones){
		zones = z;
		z->next = NULL;
	}else{
		zone *cur = zones;
		while(cur->next)
			cur = cur->next;
		cur->next = z;
		z->next = NULL;
	}
}


#include <libxml/parser.h>
#include <libxml/tree.h>
static void print_element_names(xmlNode * a_node)
{
    xmlNode *cur_node = NULL;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
			struct _xmlAttr *cp;
            printf("node type: Element, name: %s\n", cur_node->name);
			for(cp = cur_node->properties; cp; cp = cp->next) {
				// http://www.xmlsoft.org/html/libxml-tree.html
				printf("\tname=%s value=%s\n",cp->name,xmlGetNoNsProp(cur_node,cp->name));
			}
        }

        print_element_names(cur_node->children);
    }
}

static void load_zone_elements(xmlNode * a_node)
{
    xmlNode *cur_node = NULL;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
			struct _xmlAttr *cp;
			char *value;
            printf("node type: Element, name: %s\n", cur_node->name);
			if(!strcasecmp(cur_node->name,"zone")){
				//add to zone list
				zone *z = malloc(sizeof(zone));
				for(cp = cur_node->properties; cp; cp = cp->next) {
					// http://www.xmlsoft.org/html/libxml-tree.html
					value = xmlGetNoNsProp(cur_node,cp->name);
					printf("\tname=%s value=%s\n",cp->name,value);
					if(!strcasecmp(cp->name,"ssr")){
						z->ssrname = strdup(value);
					}else if(!strcmp(cp->name,"name")){
						z->name = strdup(value);
					}else if(!strcasecmp(cp->name,"center")){
						double x,y,zz;
						sscanf(value,"%lf %lf %lf",&x,&y,&zz);
						z->center.x = x;
						z->center.y = y; 
					}else if(!strcasecmp(cp->name,"polygon")){
						double x,y;
						Point points[1000];
						int n, m;
						char *p, *q;
						q = value;
						n = 0;
						while(q){
							m = sscanf(q,"%lf %lf",&x,&y);
							if(m==2){
								points[n].x = x;
								points[n].y = y;
								n++;
							}else{
								break;
							}
							q = strchr(q,',');
							if(!q) break;
							q++;
						}
						z->n = n;
						z->poly = malloc(n*sizeof(Point));
						memcpy(z->poly,points,n*sizeof(Point));
					}
				}

				zone_list_add(z);
			}else if(!strcasecmp(cur_node->name,"ssrserver")){
				//add to ssr list
				ssr *s = malloc(sizeof(ssr));
				for(cp = cur_node->properties; cp; cp = cp->next) {
					value = xmlGetNoNsProp(cur_node,cp->name);
					printf("\tname=%s value=%s\n",cp->name,value);
					if(!strcasecmp(cp->name,"name")){
						s->name = strdup(value);
					}else if(!strcasecmp(cp->name,"ip")){
						s->ip = strdup(value);
					}else if(!strcasecmp(cp->name,"port")){
						s->port = strdup(value);
					}
				}
				ssr_list_add(s);
			}
        }
		//recurse
        load_zone_elements(cur_node->children);
    }
}
zone *find_zone_by_name(char *name){
	zone *z = zones;
	while(z){
		if(!strcmp(z->name,name))
			break;
		z=z->next;
	}
	return z;
}
int cn_PnPoly( Point P, Point *V, int n );
zone *find_zone_by_point(Point P){
	int inside;
	zone *z;
	z = zones;
	while(z){
		inside = cn_PnPoly( P, z->poly, z->n -1 );
		if(inside)
			break;
		z = z->next;
	}
	return z;
}
void test_pointinpoly(){
	zone *z, *zn;
	int inside;
	Point p;
	zn = find_zone_by_name("StrandEnDuin"); //("Afrikaanderwuk");
	z = NULL;
	if(zn){
		p.x = zn->center.x;
		p.y = zn->center.y;
		z = find_zone_by_point(p);
		if(z){
			printf("inside polygon %s\n",z->name);
		}else{
			printf("polygon not found\n");
		}
	}

}
void assign_ssr_by_name(){
	zone *z;
	ssr *s;
	z = zones;
	while(z){
		z->ssr = NULL;
		s = ssrs;
		while(s){
			if(!strcasecmp(z->ssrname,s->name)){
				z->ssr = s;
				break;
			}
		}
		z = z->next;
	}
}
static int testing_pointinpoly = 1;
void load_polys(char *filename){
    xmlDocPtr doc; /* the resulting document tree */
    xmlNodePtr root_node = NULL, node = NULL, node1 = NULL;/* node pointers */
    doc = xmlReadFile(filename, NULL, 0);
    if (doc == NULL) {
        fprintf(stderr, "Failed to parse %s\n", filename);
	return;
    }
    root_node = xmlDocGetRootElement(doc);
	//print_element_names(root_node);
	load_zone_elements(root_node);
	assign_ssr_by_name();
    xmlFreeDoc(doc);	
	if(testing_pointinpoly)
		test_pointinpoly();
	printf("done test_pointinpoly\n");
}
//<<<<<<<<===ZONE BALANCER===========



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
#ifdef WIN32
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#define strdup _strdup
	#include <winsock2.h>	
	#include <ws2tcpip.h> /* for TCPIP - are we using tcp? */
	#define SHUT_RDWR SD_BOTH
	#include <windows.h>
	#define snprintf _snprintf
	//#define sscanf sscanf_s
	#define STRTOK_S strtok_s
int sockwrite(SOCKET s, const char *buf, int len){
	return send(s,buf,len,0);
}
int sockread(SOCKET s, const char *buf, int len){
	return recv(s,buf,len,0);
}

#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#define STRTOK_S strtok_r
int sockwrite(SOCKET s, const char *buf, int len){
	return write(s,buf,len);
}
int sockread(SOCKET s, const char *buf, int len){
	return recv(s,buf,len,0);
}
#endif

int socket_connect(char *host, int port){
	struct hostent *hp;
	struct sockaddr_in addr;
	int on = 1, sock;     
	unsigned short port16;
	struct addrinfo *result = NULL,
                *ptr = NULL,
                hints;
	int iResult;

ZeroMemory( &hints, sizeof(hints) );
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
hints.ai_protocol = IPPROTO_TCP;

iResult = getaddrinfo("localhost","8081", &hints, &result);
if (iResult != 0) {
    printf("getaddrinfo failed: %d\n", iResult);
    //WSACleanup();
    return 0;
} 
	//if((hp = gethostbyname(host)) == NULL){
	if((hp = gethostbyname("localhost")) == NULL){
		perror("gethostbyname");
		return 0;
	}
	//memcpy(hp->h_addr, &addr.sin_addr, hp->h_length);
	memcpy(&addr.sin_addr,hp->h_addr, hp->h_length);
	port16 = port; //long to unsigned short
	addr.sin_port = htons(port); //unsigned short to network byte order
	addr.sin_family = AF_INET;
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));
 
	if(sock == -1){
		perror("setsockopt");
		return 0;
	}
	
	if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1){
		perror("connect");
		return 0;
 
	}
	//write(sock,"hi",2); // write(fd, char[]*, len);
	//sockwrite(sock,"hi",2);
	return sock;
}

static char * fakepostpose = "POST /pose HTTP/1.1\r\nHost: localhost:8080\r\nConnection: keep-alive\r\nContent-Length: 100\r\nOrigin: http://localhost:8080\r\nUser-Agent: Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2272.118 Safari/537.36\r\nContent-type: application/x-www-form-urlencoded\r\nAccept: */*\r\nReferer: http://localhost:8080/SSRClient.html\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: en-US,en;q=0.8\r\n\r\nposepose=[0, -0.9977955222129822, 0, -0.06639080494642258, -161.7969512939453, 0,284.27691650390625]";
static char * fakepostsnap = "POST /pose HTTP/1.1\r\nHost: localhost:8080\r\nConnection: keep-alive\r\nContent-Length: 100\r\nOrigin: http://localhost:8080\r\nUser-Agent: Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2272.118 Safari/537.36\r\nContent-type: application/x-www-form-urlencoded\r\nAccept: */*\r\nReferer: http://localhost:8080/SSRClient.html\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: en-US,en;q=0.8\r\n\r\nposesnapshot=[0, -0.9977955222129822, 0, -0.06639080494642258, -161.7969512939453, 0,284.27691650390625]";
int build_http_post(char *post, char *host, int port, int lencontent, char *useragent, char *origin, char *referer)
{
	int len;
	char *post_fmt;
	if(0){
		post_fmt = "POST /pose HTTP/1.1\r\nHost: %s:%d\r\nConnection: keep-alive\r\nContent-Length: %d\r\nUser-Agent: %s\r\nOrigin: %s\r\nContent-type: application/x-www-form-urlencoded\r\nAccept: */*\r\nReferer: %s\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: en-US,en;q=0.8\r\n\r\n";
		sprintf(post, post_fmt, host, port, lencontent, useragent, origin, referer);
	}
	if(0){
		post_fmt = "POST /pose HTTP/1.1\r\nHost: %s:%d\r\nConnection: keep-alive\r\nContent-Length: %d\r\nContent-type: application/x-www-form-urlencoded\r\nAccept: */*\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: en-US,en;q=0.8\r\n\r\n";
		sprintf(post, post_fmt, host, port, lencontent);
	}
	if(1){
		post_fmt = "POST /pose HTTP/1.1\r\nHost: %s:%d\r\nConnection: keep-alive\r\nContent-Length: %d\r\nContent-type: application/x-www-form-urlencoded\r\nAccept: */*\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: en-US,en;q=0.8\r\n\r\n";
		sprintf(post, post_fmt, host, port,lencontent);
	}
	len = strlen(post);
	return len;
}


#define BUFFER_SIZE 32768

static char *request_line_format = "POST /%s HTTP/1.1\n";
static char *request_header_format = "Host: %s\nConnection: Keep-Alive\nAccept: */*\nAccept-Language: us-en\nContent-Length: %d\n\n";
static char *request_line = NULL;
static char *request_header = NULL;
char * content_type = "Content_type: application/x-www-form-urlencoded\n";
char * content_length_format = "Content-Length: %d\n";
int reverse_proxy(char *host, char *port, char *ssr_command, char *key, char *request, int size, char **response) 
{
	int fd, li,lr;
	char *r;
	char buffer[BUFFER_SIZE];
	char content_length[100];
	if(!request_line)
		request_line = malloc(500);
	sprintf(request_line,request_line_format,ssr_command); //port);

	if(!request_header)
		request_header = malloc(500);
	sprintf(request_header,request_header_format,"localhost",strlen(request));
    *response = NULL;
	fd = socket_connect(host, atoi(port)); 
if (fd == INVALID_SOCKET) {
    printf("Error at socket(): %ld\n", WSAGetLastError());
    //freeaddrinfo(result);
   // WSACleanup();
    //return 1;
	return 0;
}

	if(fd){
		int lh;
		char *answer, *temp;
		int answermax = BUFFER_SIZE;
		answer = malloc(answermax);

		//POST request-URI HTTP-version
		//Content-Type: mime-type
		//Content-Length: number-of-bytes
		//(other optional request headers)
		//  
		//(URL-encoded query string)

		//temp = "POST /\r\n";
		//lh = strlen(temp);
		//sockwrite(fd,temp,lh); // write(fd, char[]*, len);  
		if(1){
			char post[8192];
			int lenpost;
			int lencontent = size + strlen(key) + 1;
			lenpost = build_http_post(post, host, atoi(port), lencontent, NULL, NULL, NULL);
			sprintf(&post[lenpost],"%s=",key);
			lenpost += strlen(key)+1;
			memcpy(&post[lenpost],request,size);
			lenpost += size;
			memcpy(&post[lenpost],"\r\n",3);
			printf("%s",post);
			sockwrite(fd,post,lenpost);
		}
		if(0){
			char * fakepostpose = "POST /pose HTTP/1.1\r\nHost: localhost:8080\r\nConnection: keep-alive\r\nContent-Length: 100\r\nOrigin: http://localhost:8080\r\nUser-Agent: Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2272.118 Safari/537.36\r\nContent-type: application/x-www-form-urlencoded\r\nAccept: */*\r\nReferer: http://localhost:8080/SSRClient.html\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: en-US,en;q=0.8\r\n\r\nposepose=[0, -0.9977955222129822, 0, -0.06639080494642258, -161.7969512939453, 0,284.27691650390625]";

			char * fakepostsnap = "POST /pose HTTP/1.1\r\nHost: localhost:8080\r\nConnection: keep-alive\r\nContent-Length: 100\r\nOrigin: http://localhost:8080\r\nUser-Agent: Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2272.118 Safari/537.36\r\nContent-type: application/x-www-form-urlencoded\r\nAccept: */*\r\nReferer: http://localhost:8080/SSRClient.html\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: en-US,en;q=0.8\r\n\r\nposesnapshot=[0, -0.9977955222129822, 0, -0.06639080494642258, -161.7969512939453, 0,284.27691650390625]";
			char *fakepost;
			if(!strcmp(key,"posepose"))
				fakepost = fakepostpose;
			else 
				fakepost = fakepostsnap;

			sockwrite(fd,fakepost,strlen(fakepost));
		}
		if(0){
			sockwrite(fd,request_line,strlen(request_line));
			printf("wrote request\n");
			//lh = strlen(request_header);
			lh = strlen(content_type); // = "application/x-www-form-urlencoded"
			sockwrite(fd,content_type,lh);
			char key_str[100];
			sprintf(key_str,"%s=",key);
			int lks = strlen(key_str);
			char *wholestring = "posepose=[1.0 0.0 0.0 0.0, 1.0, 1.0, 0.0]\n";
			int lkwhole = strlen(wholestring);

			sprintf(content_length,content_length_format,lkwhole); //size+lks);
			lh = strlen(content_length);
			sockwrite(fd,content_length,lh);
			printf(content_length);
			sockwrite(fd,"\n",1); //blank line
			sockwrite(fd,wholestring,lkwhole);
			//sockwrite(fd,key_str,lks);
			//sockwrite(fd,request,size);
			//sockwrite(fd,"\n",1);
		}
		//shutdown(fd, SD_SEND); //shutdown the client's side of the connection (SSR server side can still send)
		memset(buffer,0, BUFFER_SIZE); //we need \0 on the end of each read, so we can safely do strstr below
		r = answer;
		lr = 0;
		int icount = 0;
		//char *content = NULL;
		int lencontent = 0;
		int offcontent = 0;
		while( li = sockread(fd, buffer, BUFFER_SIZE - 1) ){  //subtract one because we need \0 on the end for strstr below
			if(lr + li > answermax ){
				answermax *= 2;
				answer = realloc(answer,answermax);
				r = &answer[lr];
			}
			memcpy(r,buffer,li);
			lr += li;
			r += li;
			//if(1){
			//	//H: ssr server / libmicrohttpd isn't shutting down connection, so we hang
			//	memset(buffer,0, BUFFER_SIZE);
			//	li = sockread(fd, buffer, BUFFER_SIZE - 1);
			//}else{
			//	break; //so we'll exit after first chunk
			//}
			memset(buffer,0, BUFFER_SIZE);
			if(!offcontent){
				//first time reading, lets get the Content-Length and \n\n linebreak pointer to data
				if(!lencontent){
					char *cl = strstr(answer,"Content-Length:");
					if(cl){
						cl += strlen("Content-Length:");
						int lc = atoi(cl);
						lencontent = lc;
					}
				}
				if(lencontent && !offcontent){
					char *cd = strstr(answer,"\r\n\r\n"); //linebreak
					if(cd) cd +=4;
					if(!cd){
						cd = strstr(answer,"\n\n");
						if(cd) cd +=2;
					}
					if(cd) offcontent = cd - answer;
				}
			}
			if(lencontent && offcontent)
				if(lr >= offcontent + lencontent) break;
			//if(li < BUFFER_SIZE -1) break;
			icount++;
		}
 		shutdown(fd, SHUT_RDWR); 
		//close(fd); 	
		if(lencontent && offcontent){
			char *finalanswer = malloc(lencontent+1);
			memcpy(finalanswer,&answer[offcontent],lencontent);
			finalanswer[lencontent] = '\0';
			lr = lencontent;
			free(answer);
			answer = finalanswer;
		}
		*response = answer;
	}
	return lr;
}
int sniff_and_foreward(char *ssr_command, char *key, char *data, int size, char **answerstring)
{
	int len;
	double quat4[4];
	double vec3[3];
	Point posexy;
	zone *z;

	//sniff request for map coordinates
	jsonPose2double(quat4,vec3,data);
	posexy.x = vec3[0];
	posexy.y = vec3[1];
	//lookup zone 
	z = find_zone_by_point(posexy);
	if(!z) z = zones;
	if(z){
		ssr *ssri;
		//get the host:port address of the running ssr server to forward to
		ssri = (ssr*)z->ssr;
		//forward request, wait for response
		len = reverse_proxy(ssri->ip, ssri->port, ssr_command, key, data, size, answerstring);
		//if we got a response, return it
	}
	return len;
}

/*zonebalancer version of iterate_post, acts like a load balancer and gateway
1. sniffs request for geographic coords
2. looks up SSR in zone table
3. acts as reverse proxy and forwards request, waits for response
4  copies response to zonebalancer response
*/
static int iterate_post_zb (void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
	const char *filename, const char *content_type,
	const char *transfer_encoding, const char *data, 
	uint64_t off, size_t size)
{
	struct connection_info_struct *con_info = coninfo_cls;
	printf("Key=%s\n",key);
	printf("filename=%s\n",filename);
	printf("content_type=%s\n",content_type);
	printf("transfer_encoding=%s\n",transfer_encoding);
	printf("data=%s\n",data);
	if (0 == strcmp (key, "posepose"))
	{
		if ((size > 0) && (size <= MAXPOSESIZE)) //MAXNAMESIZE
		{
			char *answerstring;
			int len;
			SSR_request ssr_req;

			//answerstring = malloc (MAXANSWERSIZE);
			answerstring = NULL;
			ssr_req.type = SSR_POSEPOSE;
			len = sniff_and_foreward("pose",key,data,size,&answerstring);
			con_info->answerstring = answerstring;  
			con_info->len = len;    
		} 
		else con_info->answerstring = NULL;

		return MHD_NO;
	}
	if (0 == strcmp (key, "posesnapshot"))
	{
		if ((size > 0) && (size <= MAXPOSESIZE)) //MAXNAMESIZE
		{
   			char *answerstring;
			int len;
			SSR_request ssr_req;
			ssr_req.type = SSR_POSESNAPSHOT;
			answerstring = NULL;
			len = sniff_and_foreward("pose",key,data,size,&answerstring);
			con_info->answerstring = answerstring;  
			con_info->len = len;    
		} 
		else con_info->answerstring = NULL;

		return MHD_NO;
	}

	return MHD_YES;
}


static int iterate_post (void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
	const char *filename, const char *content_type,
	const char *transfer_encoding, const char *data, 
	uint64_t off, size_t size)
{
	struct connection_info_struct *con_info = coninfo_cls;
	printf("Key=%s\n",key);
	printf("filename=%s\n",filename);
	printf("content_type=%s\n",content_type);
	printf("transfer_encoding=%s\n",transfer_encoding);
	printf("data=%s\n",data);

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
	printf("\nurl=%s\n",url);
	printf("method=%s\n",method);
	printf("version=%s\n",version);
	printf("upload_data=%s\n",upload_data);
	if(NULL == *con_cls) 
	{
		struct connection_info_struct *con_info;

		con_info = malloc (sizeof (struct connection_info_struct));
		if (NULL == con_info) return MHD_NO;
		con_info->answerstring = NULL;
	//If the new request is a POST, the postprocessor must be created now. In addition, the type of the request is stored for convenience.

		if (0 == strcmp (method, "POST")) 
		{      
			if(running_as_zonebalancer)
				con_info->postprocessor = MHD_create_post_processor (connection, POSTBUFFERSIZE, 
					iterate_post_zb, (void*) con_info);   
			else
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
		printf("method=%s\n",method);
		printf("version=%s\n",version);
		printf("upload_data=%s\n",upload_data);
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
/* 
How to run SSR Server
SSRServer.exe 8081 "C:/myscenes/sceneA.x3d"

How to run ZoneBalancer:
SSRServer.exe 8080 --zonebalancer
	- it attempts to read zonebalancer.xml from the folder above the current working directory
*/
	struct MHD_Daemon * d;
	char *portstr, *url;
	int iaction;
	if (argc < 2) {
		portstr = "8080";
		printf("%s PORT\n",portstr);
		url = scene_path;
		//return 1;
	}else{
		portstr = argv[1];
		if(argc < 3)
			url = scene_path;
		else{
			url = argv[2];
			if(!strcmp(url,"--zonebalancer")){
				load_polys("..\\zonebalancer.xml");
				running_as_zonebalancer = 1;
				initialize_sockets();
			}
		}
	}
	iaction = 2;
	if(running_as_zonebalancer)
		iaction = 2;
	if(iaction == 1) {
		//simple echo of incoming request
		d = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION,
			atoi(portstr),
			NULL,
			NULL,
			&ahc_echo,
			PAGE,
			MHD_OPTION_END);
	}
	if(iaction == 2){
		//our special SSRServer request handler
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
	}
	if (d == NULL)
		return 1;
	if(!running_as_zonebalancer)
		runFW(url);
	printf("Press Enter to stop libmicrohttp deamon and exit:");
	getchar();
	stopFW();
	MHD_stop_daemon(d);
	return 0;
}


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