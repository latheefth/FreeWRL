/*

  FreeWRL support library.
  IO with HTTP protocol.

*/

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



#include <config.h>
#include <system.h>
#include <resources.h>
//#include <display.h>
#include <internal.h>

//#include <libFreeWRL.h>
//#include <list.h>
//
//#include <io_files.h>
//#include <io_http.h>
//#include <threads.h>
//#include "scenegraph/Vector.h"
//#include "main/ProdCon.h"




#ifdef _MSC_VER
#define strncasecmp _strnicmp
#endif

bool is_url(const char *url)
{
#define	MAX_PROTOS 4
	static const char *protos[MAX_PROTOS] = { "ftp", "http", "https", "urn" };

	int i;
	char *pat;
	unsigned long delta = 0;

	pat = strstr(url, "://");
	if (!pat) {
		return FALSE;
	}

	delta = (long)pat - (long)url;
	if (delta > 5) {
		return FALSE;
	}

	for (i = 0; i < MAX_PROTOS ; i++) {
		if (strncasecmp(protos[i], url, strlen(protos[i])) == 0) {
			return TRUE;
		}
	}
	return FALSE;
}


/*
  New FreeWRL internal API for HTTP[S] downloads

  - enqueue an URL for download
  - test if the download is over
  - open the downloaded file
  - remove the downloaded file when neede
    - {same as temp file}

  * structure:
    - download structure with URL, temp filename, status
*/

#if defined(HAVE_LIBCURL)

/*
  To be effectively used, libcurl has to be enabled
  during configure, then it has to be enabled on 
  the command line (see main program: options.c).

  Instead of downloading files at once with wget,
  I propose to try libCurl which provides some nice
  mechanism to reuse open connections and to multi-
  thread downloads.

  At the moment I've only hacked a basic mise en oeuvre
  of this library.
*/

#include <curl/curl.h>

static CURL *curl_h = NULL;

int with_libcurl = TRUE;

/*
  libCurl needs to be initialized once.
  We've choosen the very simple method of curl_easy_init
  but, in the future we'll use the full features.
*/
void init_curl()
{
    CURLcode c;

    if ( (c=curl_global_init(CURL_GLOBAL_ALL)) != 0 ) {
	ERROR_MSG("Curl init failed: %d\n", (int)c);
	exit(1);
    }

    ASSERT(curl_h == NULL);

    curl_h = curl_easy_init( );

    if (!curl_h) {
	ERROR_MSG("Curl easy_init failed\n");
	exit(1);
    }
}

/* return the temp file where we got the contents of the URL requested */
/* old char* download_url_curl(const char *url, const char *tmp) */
char* download_url_curl(resource_item_t *res)
{
    CURLcode success;
    char *temp;
    FILE *file;

    if (res->temp_dir) {
	    temp = STRDUP(res->temp_dir);
    } else {
	    temp = tempnam(gglobal()->Mainloop.tmpFileLocation, "freewrl_download_curl_XXXXXXXX");
	    if (!temp) {
		    PERROR_MSG("download_url_curl: can't create temporary name.\n");
		    return NULL;	
	    }
    }

    file = fopen(temp, "w");
    if (!file) {
	free(temp);
	ERROR_MSG("Cannot create temp file (fopen)\n");
	return NULL;	
    }

    if (!curl_h) {
	init_curl();
    }

    /*
      Ask libCurl to download one url at once,
      and to write it to the specified file.
    */
    curl_easy_setopt(curl_h, CURLOPT_URL, res->parsed_request);

    curl_easy_setopt(curl_h, CURLOPT_WRITEDATA, file);

    success = curl_easy_perform(curl_h); 

    if (success == 0) {
#ifdef TRACE_DOWNLOADS
	TRACE_MSG("Download sucessfull [curl] for url %s\n", res->parsed_request);
#endif
	fclose(file);
	return temp;
    } else {
	ERROR_MSG("Download failed for url %s (%d)\n", res->parsed_request, (int) success);
	fclose(file);
	unlink(temp);
	FREE(temp);
	return NULL;
    }
}

#endif /* HAVE_LIBCURL */


#if defined(HAVE_WININET)

/*
ms windows native methods WinInet - for clients (like freewrl) 
http://msdn.microsoft.com/en-us/library/aa385331(VS.85).aspx   C - what I used below
http://msdn.microsoft.com/en-us/library/sb35xf67.aspx sample browser in C++
*/
#include <WinInet.h>

static HINTERNET hWinInet = NULL; //static for an application, although multiple inits OK
HINTERNET winInetInit()
{
//winInet_h = InternetOpen(
//  __in  LPCTSTR lpszAgent,
//  __in  DWORD dwAccessType,
//  __in  LPCTSTR lpszProxyName,
//  __in  LPCTSTR lpszProxyBypass,
//  __in  DWORD dwFlags
//);


	if(hWinInet == NULL)
		hWinInet = InternetOpen("freewrl",INTERNET_OPEN_TYPE_DIRECT,NULL,NULL,0); //INTERNET_OPEN_TYPE_PRECONFIG_WITH_NO_AUTOPROXY/*/INTERNET_OPEN_TYPE_PRECONFIG*/,NULL,NULL,0);//INTERNET_FLAG_ASYNC - for callback);
	return hWinInet;
}
void closeWinInetHandle()
{
	InternetCloseHandle(hWinInet);
}

//#define ERROR_MSG ConsoleMessage
//#define PERROR_MSG ConsoleMessage
   /* return the temp file where we got the contents of the URL requested */
/* char* download_url_WinInet(const char *url, const char *tmp) */
char* download_url_WinInet(resource_item_t *res)
{
	if(!hWinInet)
	{
		hWinInet = winInetInit();
	}
	if(!hWinInet) 
		return NULL;
	else
	{
		DWORD dataLength, len;
		HINTERNET hOpenUrl;
		if(0){
			static FILE* fp = NULL;
			if (!fp) fp = fopen("http_log.txt", "w+");
			fprintf(fp,"[%s]\n", res->parsed_request);
		}
		hOpenUrl=InternetOpenUrl(hWinInet,res->parsed_request,NULL,0,0,0); //INTERNET_FLAG_NO_UI|INTERNET_FLAG_RELOAD/*|INTERNET_FLAG_IGNORE_CERT_CN_INVALID install the cert instead*/,0);
		DWORD buflen = 1023;
		DWORD dwError;
		char buffer[1024];
		//if(InternetGetLastResponseInfo(&dwError,buffer,&buflen)){
		//	printf("error=%s\n",buffer);
		//}else{
		//	printf("no error\n");
		//}
// BOOL HttpQueryInfo(
//  _In_     HINTERNET hRequest,
//  _In_     DWORD dwInfoLevel,
//  _Inout_  LPVOID lpvBuffer,
//  _Inout_  LPDWORD lpdwBufferLength,
//  _Inout_  LPDWORD lpdwIndex
//);
		//http://msdn.microsoft.com/en-us/library/aa384238(v=vs.85).aspx
		DWORD InfoLevel, Index;
		buflen = 1023;
		Index = 0;
		InfoLevel = HTTP_QUERY_RAW_HEADERS_CRLF;
		if(HttpQueryInfo(hOpenUrl,InfoLevel,buffer,&buflen,NULL)){
			//printf("query buffer=%s\n",buffer);
			if(strstr(buffer,"404 Not Found")){
				//HTTP/1.1 404 Not Found
				ERROR_MSG("Download failed for url %s\n", res->parsed_request);
				return NULL;
			}
			//else 200 OK
		}
		//else{
		//	printf("no query buffer\n");
		//}


		//DWORD err = GetLastError();
		if (!(hOpenUrl))
		{
			ERROR_MSG("Download failed for url %s\n", res->parsed_request);
			return NULL;
		}
		else
		{
			char *temp;
			FILE *file;

			if (res->temp_dir) {
				temp = STRDUP(res->temp_dir);
			} else {
				//temp = _tempnam(gglobal()->Mainloop.tmpFileLocation, "freewrl_download_XXXXXXXX");
				temp = _tempnam(NULL, "freewrl_download_XXXXXXXX");
				if (!temp) {
					PERROR_MSG("download_url: can't create temporary name.\n");
					return NULL;	
				}
			}

			file = fopen(temp, "wb");
			if (!file) {
				free(temp);
				ERROR_MSG("Cannot create temp file (fopen)\n");
				return NULL;	
			}

			dataLength=0;
			len=0;

			while((InternetQueryDataAvailable(hOpenUrl,&dataLength,0,0))&&(dataLength>0))
			{
				void *block = malloc(dataLength);
				if ((InternetReadFile(hOpenUrl,(void*)block,dataLength,&len))&&(len>0))
				{
					fwrite(block,dataLength,1,file);
				}
				free(block);
			}
			InternetCloseHandle(hOpenUrl); 
			hOpenUrl=NULL;
			fclose(file);
			return temp;
		}
    } 
}

#endif

/* lets try this...  this should be in the config files */
#ifdef WGET
#define HAVE_WGET
#endif

#ifdef HAVE_WGET

/**
 *   launch wget to download requested url
 *   if tmp is not NULL then use that tempnam
 *   return the temp file where we got the contents of the URL requested
 */
char* download_url_wget(resource_item_t *res)
{
    char *temp, *wgetcmd;
    int ret;

#if defined (TARGET_AQUA)
	#define WGET_OPTIONS ""
	#define WGET_OUTPUT_DIRECT "-o"
#else
	// move this to another place (where we check wget options)
	#define WGET_OPTIONS "--no-check-certificate"
	#define WGET_OUTPUT_DIRECT "-O"
#endif

    // create temp filename
    if (res->temp_dir) {
	    temp = STRDUP(res->temp_dir);
    } else {
	    temp = tempnam(gglobal()->Mainloop.tmpFileLocation, "freewrl_download_wget_XXXXXXXX");
	    if (!temp) {
		    PERROR_MSG("download_url_wget: can't create temporary name.\n");
		    return NULL;
	    }
    }

    // create wget command line
    wgetcmd = malloc( strlen(WGET) +
	                    strlen(WGET_OPTIONS) + 
	                    strlen(res->parsed_request) +
                            strlen(temp) + 6 +1+1);

#if defined (TARGET_AQUA)
    /* AQUA - we DO NOT have the options, but we can not have the space - it screws the freewrlSystem up */
    sprintf(wgetcmd, "%s %s %s %s", WGET, res->parsed_request, WGET_OUTPUT_DIRECT, temp);
#else
    sprintf(wgetcmd, "%s %s %s %s %s", WGET, WGET_OPTIONS, res->parsed_request, WGET_OUTPUT_DIRECT, temp);
#endif

    /* printf ("wgetcmd is %s\n",wgetcmd); */

    // call wget
    ret = freewrlSystem(wgetcmd);
    if (ret < 0) {
	ERROR_MSG("Error in wget (%s)\n", wgetcmd);
	FREE(temp);
	FREE(wgetcmd);
	return NULL;
    }
    FREE(wgetcmd);
    return temp;
}
#endif /* HAVE_WGET */


/* get the file from the network, UNLESS the front end gets files. Old way - the freewrl 
   library tries to get files; new way, the front end gets the files from the network. This
   allows, for instance, the browser plugin to use proxy servers and cache for getting files.

   So, if the FRONTEND_GETS_FILES is set, we simply pass the http:// filename along....
*/
void close_internetHandles()
{
#ifdef HAVE_WININET
	closeWinInetHandle();
#endif
}
void download_url(resource_item_t *res)
{

#if defined(FRONTEND_GETS_FILES)
	res->actual_file = STRDUP(res->parsed_request);

#elif defined(HAVE_LIBCURL)
	if (with_libcurl) {
		res->actual_file = download_url_curl(res);
	} else {
		res->actual_file = download_url_wget(res);
	}

#elif defined (HAVE_WGET) 
	res->actual_file = download_url_wget(res);


#elif defined (HAVE_WININET)
	res->actual_file = download_url_WinInet(res);
#endif 

	/* status indication now */
	if (res->actual_file) {
		/* download succeeded */
		res->status = ress_downloaded;
		if(strcmp(res->actual_file,res->parsed_request)){
			//it's a temp file 
			s_list_t *item;
			item = ml_new(res->actual_file);
			if (!res->cached_files)
				res->cached_files = (void *)item;
			else
				res->cached_files = ml_append(res->cached_files,item);
		}
	} else {
		/* download failed */
		res->status = ress_failed;
		ERROR_MSG("resource_fetch: download failed for url: %s\n", res->parsed_request);
	}
}
