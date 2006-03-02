/*
 * $Id$
 *
 * Common functions used by Mozilla and Netscape plugins...(maybe
 * PluginGlue too?)
 */

#include "PluginSocket.h"
#ifdef F_SETSIG
#define FSIGOK
#endif

/* what Browser are we running under? Mozilla? Opera?*/
char NetscapeName[MAXNETSCAPENAMELEN];

/*  CHECK DIRECTORY IN PLUGINPRINT*/
#undef PLUGINSOCKETVERBOSE

fd_set rfds;
struct timeval tv;

char return_url[FILENAME_MAX]; /* used to be local, but was returned as a pointer */


#ifdef PLUGINSOCKETVERBOSE
static FILE * tty = NULL;
extern void abort();

/* prints to a log file if we are running as a plugin */
void pluginprint (const char *m, const char *p) {
	if (tty == NULL) {
		tty = fopen("/home/luigi/logPluginSocket", "w");
		if (tty == NULL)
			abort();
		fprintf (tty, "\nplugin restarted\n");
	}

	fprintf(tty, m,p);
	fflush(tty);
}
#endif

/* loop about waiting for the Browser to send us some stuff. */
int waitForData(int sock) {

	int retval;
	int count;
	int totalcount;

	#ifdef PLUGINSOCKETVERBOSE
	pluginprint ("waitForData, BN %s\n",NetscapeName);
	#endif

	retval = FALSE;
	count = 0;
	totalcount = 1000000;

	if (strncmp (NetscapeName,"Mozilla",strlen("Mozilla")) == 0) {
		/* Mozilla,  lets give it 10 seconds */
		#ifdef PLUGINSOCKETVERBOSE
		pluginprint ("have Mozilla, reducing timeout to 10 secs","");
		#endif
		totalcount = 1000;
	}

	do {
		tv.tv_sec = 0;
		tv.tv_usec = 100;
		FD_ZERO(&rfds);
		FD_SET((sock), &rfds);

		retval = select((sock)+1, &rfds, NULL, NULL, &tv);


		if (retval) {
			#ifdef PLUGINSOCKETVERBOSE
			pluginprint ("waitForData returns TRUE\n","");
			#endif

			return (TRUE);
		} else {
			count ++;
			if (count > totalcount) {
				#ifdef PLUGINSOCKETVERBOSE
				pluginprint ("waitForData, timing out\n","");
				#endif

				return (FALSE);
			}
		}
	} while (!retval);
}

char *
requestUrlfromPlugin(int sockDesc,
		   uintptr_t plugin_instance,
		   const char *url)
{
	size_t len = 0, ulen = 0, bytes = 0;
	urlRequest request;
	FILE  *infile;
	int linecount;
	int linelen;
	char buf[2004];
	char encodedUrl[2000];

	#ifdef PLUGINSOCKETVERBOSE
	pluginprint ("requestURL fromPlugin, getting %s\n",url);
	#endif

	URLencod(encodedUrl,url,2000);
	#ifdef PLUGINSOCKETVERBOSE
	pluginprint ("requestURL fromPlugin, NOW getting %s\n",encodedUrl);
	#endif

	request.instance = (void *) plugin_instance;
	request.notifyCode = 0; /* get a file  */

	len = FILENAME_MAX * sizeof(char);
	memset(request.url, 0, len);
	memset(return_url, 0, len);

	ulen = strlen(encodedUrl) + 1;
	memmove(request.url, encodedUrl, ulen);
	bytes = sizeof(urlRequest);

	#ifdef PLUGINSOCKETVERBOSE
	pluginprint ("requestURL fromPlugin, step 1\n","");
	#endif

	if (write(sockDesc, (urlRequest *) &request, bytes) < 0) {
		#ifdef PLUGINSOCKETVERBOSE
		pluginprint ("write failed in requestUrlfromPlugin","");
		#endif
		return NULL;
	}

	#ifdef PLUGINSOCKETVERBOSE
	pluginprint ("requestURL fromPlugin, step 2\n","");
	#endif


	/* wait around for a bit to see if this is going to pass or fail */
	if (!waitForData(sockDesc)) return NULL;

	if (read(sockDesc, (char *) return_url, len) < 0) {
		#ifdef PLUGINSOCKETVERBOSE
		pluginprint("read failed in requestUrlfromPlugin","");
		pluginprint("Testing: error from read -- returned url is %s.\n", return_url);
		#endif
		return NULL;
	}

	#ifdef PLUGINSOCKETVERBOSE
	pluginprint ("requestURL fromPlugin, returning %s\n",return_url);
	#endif

	/* now, did this request return a text file with a html page indicating 404- not found? */
	infile = fopen (return_url,"r");
	if (infile < 0) return NULL;

	linecount = 0;
	linelen = fread (buf,1,2000,infile);
	/* pluginprint ("verify read, read in %d characters\n",linelen);*/
	while ((linelen > 0) && (linecount < 5)){
	/* 	pluginprint ("verify read, read in %d characters\n",linelen);*/

		/* did we find a "404 file not found" message? */
		/* some, all??? will eventually return a 404 html text in
		   place of whatever you requested */
		if (strstr(buf,"<TITLE>404 Not Found</TITLE>") != NULL) {
			#ifdef PLUGINSOCKETVERBOSE
			pluginprint ("found a 404 in :%s:\n",buf);
			#endif
			fclose (infile);
			return NULL;
		}
		linecount ++;
		linelen = fread (buf,1,2000,infile);
	}
	fclose (infile);


	/* we must be returning something here */
	return return_url;
}


/* tell Netscape that a new window is required (eg, Anchor
 * clicked and it is an HTML page */

void requestNewWindowfromPlugin(int sockDesc,
		   uintptr_t plugin_instance,
		   const char *url)
{
	size_t len = 0, ulen = 0, bytes = 0;
	urlRequest request;
	FILE  *infile;
	int linecount;
	int linelen;
	char buf[2004];

	#ifdef PLUGINSOCKETVERBOSE
	pluginprint ("requestNewWindow fromPlugin, getting %s\n",url);
	#endif

	request.instance = (void *) plugin_instance;
	request.notifyCode = 1; /* tell plugin that we want a new window */

	len = FILENAME_MAX * sizeof(char);
	memset(request.url, 0, len);
	memset(return_url, 0, len);

	ulen = strlen(url) + 1;
	memmove(request.url, url, ulen);
	bytes = sizeof(urlRequest);

	#ifdef PLUGINSOCKETVERBOSE
	pluginprint ("requestNewWindow fromPlugin, step 1\n","");
	#endif

	if (write(sockDesc, (urlRequest *) &request, bytes) < 0) {
		#ifdef PLUGINSOCKETVERBOSE
		pluginprint ("write failed in requestUrlfromPlugin","");
		#endif
		return;
	}
}
