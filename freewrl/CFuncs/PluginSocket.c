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


static int PluginSocketVerbose = 0; /*  CHECK DIRECTORY IN PLUGINPRINT*/
static FILE * tty = NULL;
fd_set rfds;
struct timeval tv;

char return_url[FILENAME_MAX]; /* used to be local, but was returned as a pointer */

/* added M. Ward Dec8/04*/
extern void abort();

/* prints to a log file if we are running as a plugin */
void pluginprint (const char *m, const char *p) {
	if (!PluginSocketVerbose) return;
	if (tty == NULL) {
		tty = fopen("/home/luigi/logPluginSocket", "w");
		if (tty == NULL)
			abort();
		fprintf (tty, "\nplugin restarted\n");
	}

	fprintf(tty, m,p);
	fflush(tty);
}

/* loop about waiting for the Browser to send us some stuff. */
int waitForData(int sock) {

	int retval;
	int count;
	int totalcount;

	pluginprint ("waitForData, BN %s\n",NetscapeName);
	retval = FALSE;
	count = 0;
	totalcount = 1000000;

	if (strncmp (NetscapeName,"Mozilla",strlen("Mozilla")) == 0) {
		/* Mozilla,  lets give it 10 seconds */
		pluginprint ("have Mozilla, reducing timeout to 10 secs","");
		totalcount = 1000;
	}

	do {
		tv.tv_sec = 0;
		tv.tv_usec = 100;
		FD_ZERO(&rfds);
		FD_SET((sock), &rfds);

		retval = select((sock)+1, &rfds, NULL, NULL, &tv);


		if (retval) {
			pluginprint ("waitForData returns TRUE\n","");
			return (TRUE);
		} else {
			count ++;
			if (count > totalcount) {
				pluginprint ("waitForData, timing out\n","");
				return (FALSE);
			}
		}
	} while (!retval);
}

char *
requestUrlfromPlugin(int sockDesc,
		   unsigned int plugin_instance,
		   const char *url)
{
	size_t len = 0, ulen = 0, bytes = 0;
	urlRequest request;
	FILE  *infile;
	int linecount;
	int linelen;
	char buf[2004];

	pluginprint ("requestURL fromPlugin, getting %s\n",url);

	request.instance = (void *) plugin_instance;
	request.notifyCode = 0; /* get a file  */

	len = FILENAME_MAX * sizeof(char);
	memset(request.url, 0, len);
	memset(return_url, 0, len);

	ulen = strlen(url) + 1;
	memmove(request.url, url, ulen);
	bytes = sizeof(urlRequest);

	pluginprint ("requestURL fromPlugin, step 1\n","");

	if (write(sockDesc, (urlRequest *) &request, bytes) < 0) {
		pluginprint ("write failed in requestUrlfromPlugin","");
		return NULL;
	}

	pluginprint ("requestURL fromPlugin, step 2\n","");

	/* wait around for a bit to see if this is going to pass or fail */
	if (!waitForData(sockDesc)) return NULL;

	if (read(sockDesc, (char *) return_url, len) < 0) {
		pluginprint("read failed in requestUrlfromPlugin","");
		pluginprint("Testing: error from read -- returned url is %s.\n", return_url);
		return NULL;
	}

	pluginprint ("requestURL fromPlugin, returning %s\n",return_url);

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
			pluginprint ("found a 404 in :%s:\n",buf);
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
		   unsigned int plugin_instance,
		   const char *url)
{
	size_t len = 0, ulen = 0, bytes = 0;
	urlRequest request;
	FILE  *infile;
	int linecount;
	int linelen;
	char buf[2004];

	pluginprint ("requestNewWindow fromPlugin, getting %s\n",url);

	request.instance = (void *) plugin_instance;
	request.notifyCode = 1; /* tell plugin that we want a new window */

	len = FILENAME_MAX * sizeof(char);
	memset(request.url, 0, len);
	memset(return_url, 0, len);

	ulen = strlen(url) + 1;
	memmove(request.url, url, ulen);
	bytes = sizeof(urlRequest);

	pluginprint ("requestNewWindow fromPlugin, step 1\n","");

	if (write(sockDesc, (urlRequest *) &request, bytes) < 0) {
		pluginprint ("write failed in requestUrlfromPlugin","");
		return;
	}
}
