//[s release];
/*

  FreeWRL support library.
  IO with files.

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
#include <display.h>
#include <internal.h>
#include <libFreeWRL.h>
#include <errno.h>

#include <list.h> /* internal use only */
#include <io_files.h>
#include <io_http.h>

#include <sys/stat.h>

#include <threads.h> /* for freewrlSystem */

#if HAVE_DIRENT_H
# include <dirent.h>
#endif

#ifndef _MSC_VER
#include <sys/mman.h> /* mmap */
#else
#include <direct.h> //for getcwd
#define getcwd _getcwd
#define mkdir _mkdir
#endif
#include <limits.h>   /* SSIZE_MAX */

#include "input/InputFunctions.h"
#include "plugin/pluginUtils.h"
#include "plugin/PluginSocket.h"

#if defined (INCLUDE_STL_FILES)
#include "input/convertSTL.h"
#endif //INCLUDE_STL_FILES

#define UNUSED(v) ((void) v)

/* Internal function prototypes */
void append_openned_file(s_list_t *list, const char *filename, int fd, char *text);

int inputFileType = IS_TYPE_UNKNOWN;
int inputFileVersion[3] = {0,0,0};


int fw_mkdir(const char* path){
#ifdef _MSC_VER
	return mkdir(path);
#else
	return mkdir(path,0755);
#endif
}

/**
 *   concat_path: concat two string with a / in between
 */
char* concat_path(const char *a, const char *b)
{
	size_t la, lb;
	char *tmp;

	if (!a) {
		if (!b) return NULL;
		/* returns "/b" */
		lb = strlen(b);
		tmp = MALLOC(char *, 2+lb); /* why 2? room for the slash and the trailing NULL */
		sprintf(tmp, "/%s", b);
		return tmp;
	} else {
		if (!b) {
			/* returns "a/" */
			la = strlen(a);
			tmp = MALLOC(char *, la+2); /* why 2? room for the slash and the trailing NULL */
			sprintf(tmp, "%s/", a);
			return tmp;
		}
	}

	la = strlen(a);
	lb = strlen(b);

	if (a[la-1] == '/') {
		tmp = MALLOC(char *, la + lb + 1); /* why 1? room for the trailing NULL */
		sprintf(tmp, "%s%s", a, b);
	} else {
		tmp = MALLOC(char *, la + lb + 2); /* why 2? room for the slash and the trailing NULL */
		sprintf(tmp, "%s/%s", a, b);
	}

	return tmp;
}

/**
 *   remove_filename_from_path: this works also with url.
 */
char* remove_filename_from_path(const char *path)
{
	char *rv = NULL;
	char *slash;

	slash = strrchr(path, '/');
	if (slash) {
#ifdef DEBUG_MALLOC
printf ("remove_filename_from_path going to copy %d\n", ((int)slash-(int)path)+1);
		rv = strndup(path, ((int)slash-(int)path)+1);
		rv = STRDUP(path);
		slash = strrchr(rv,'/');
		*slash = '\0';
printf ("remove_filename_from_path, returning :%s:\n",rv);
#else
		rv = strndup(path, (size_t)slash - (size_t)path + 1);
#endif

	}
	return rv;
}
char *strBackslash2fore(char *str)
{
#ifdef _MSC_VER
	int jj;
	for( jj=0;jj<strlen(str);jj++)
		if(str[jj] == '\\' ) str[jj] = '/';
#endif
	return str;
}

char *get_current_dir()
{
	char *cwd , *retvar;
	cwd = MALLOC(char *, PATH_MAX);
	retvar = getcwd(cwd, PATH_MAX);
	if (NULL != retvar) {
			size_t ll;
			ll = strlen(cwd);
			cwd = strBackslash2fore(cwd);
			cwd[ll] = '/';  /* put / ending to match posix version which puts local file name on end*/
			cwd[ll+1] = '\0';
	} else {
		printf("Unable to establish current working directory in %s,%d errno=%d",__FILE__,__LINE__,errno) ;
		cwd = strdup("./"); // "/tmp/";
	}
	return cwd;
}

/*
  NOTES: temp dir

  tmp_dir=/tmp/freewrl-YYYY-MM-DD-$PID/<main_world>/ must then 
  add <relative path> at the end.

  input request: url "tex.jpg" => $tmp_dir/tex.jpg
  url "images/tex.jpg" => create images subdir, => $tmp_dir/images/tex.jpg
*/


#if !defined(FRONTEND_GETS_FILES)
/**
 *   do_file_exists: asserts that the given file exists.
 */
bool do_file_exists(const char *filename)
{
	struct stat ss;
	if (stat(filename, &ss) == 0) {
		return TRUE;
	}
	return FALSE;
}

/**
 *   do_file_readable: asserts that the given file is readable.
 */
bool do_file_readable(const char *filename)
{
	if (access(filename, R_OK) == 0) {
		return TRUE;
	}
	return FALSE;
}

#endif //FRONTEND_GETS_FILES

/**
 *   do_dir_exists: asserts that the given directory exists.
 */
bool do_dir_exists(const char *dir)
{
	struct stat ss;

#if defined(_MSC_VER)
	/* TODO: Remove any trailing backslash from *dir */
#endif

	if (stat(dir, &ss) == 0) {
		if (access(dir,X_OK) == 0) {
			return TRUE;
		} else {
			WARN_MSG("directory '%s' exists but is not accessible\n", dir);
		}
	}
	return FALSE;
}


/**
 *   of_dump: print the structure.
 */
void of_dump(openned_file_t *of)
{
	static char first_ten[11];
	if (of->fileData) {
        int len = of->fileDataSize;
        if (len>10)len=10;
		memcpy(first_ten, of->fileData, len);
	}
	printf("{%s, %d, %d, %s%s}\n", of->fileFileName, of->fileDescriptor, of->fileDataSize, (of->fileData ? first_ten : "(null)"), (of->fileData ? "..." : ""));
}

/**
 *   create_openned_file: store the triplet {filename, file descriptor,
 *                        and data buffer} into an openned file object.
 *                        Purpose: to be able to close and free all that stuff.
 */
static openned_file_t* create_openned_file(const char *filename, int fd, int dataSize, unsigned char *data, int imageHeight, int imageWidth, bool imageAlpha)
{
	openned_file_t *of;

	of = XALLOC(openned_file_t);
	of->fileFileName = filename;
	of->fileDescriptor = fd;
	of->fileData = data;            // XXXX FREE_IF_NZ this after use.
	of->fileDataSize = dataSize;
	of->imageHeight = imageHeight;
	of->imageWidth = imageWidth;
	of->imageAlpha = imageAlpha;
    //printf ("create_openned_file, datasize %d file %s\n",dataSize,filename);
    //if (dataSize <4000) printf ("create_openned_file, stringlen of data %ld\n",strlen(data));
	return of;
}

#if !defined(FRONTEND_GETS_FILES)

/**
 * (internal)   load_file_mmap: implement load_file with mmap.
 */
#if defined(FW_USE_MMAP)
static void* load_file_mmap(const char *filename)
{
	struct stat ss;
	char *text;
	int fd;

	if (stat(filename, &ss) < 0) {
		PERROR_MSG("load_file_mmap: could not stat: %s\n", filename);
		return NULL;
	}
	fd = open(filename, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		PERROR_MSG("load_file_mmap: could not open: %s\n", filename);
		return NULL;
	}
	if (!ss.st_size) {
		ERROR_MSG("load_file_mmap: file is empty %s\n", filename);
		close(fd);
		return NULL;
	}
	text = mmap(NULL, ss.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if ((text == MAP_FAILED) || (!text)) {
		PERROR_MSG("load_file_mmap: could not mmap: %s\n", filename);
		close(fd);
		return NULL;
	}
	return create_openned_file(filename, fd, text,0,0,FALSE);
}
#endif


/**
 * (internal)   load_file_read: implement load_file with read.
 */
static openned_file_t* load_file_read(const char *filename)
{
	struct stat ss;
	int fd;
	unsigned char *text, *current;

#ifdef _MSC_VER
	size_t blocksz, readsz, left2read;
#else
	ssize_t blocksz, readsz, left2read;
#endif

	if (stat(filename, &ss) < 0) {
		PERROR_MSG("load_file_read: could not stat: %s\n", filename);
		return NULL;
	}
#ifdef _MSC_VER
	fd = open(filename, O_RDONLY | O_BINARY);
#else
	fd = open(filename, O_RDONLY | O_NONBLOCK);
#endif
	if (fd < 0) {
		PERROR_MSG("load_file_read: could not open: %s\n", filename);
		return NULL;
	}
	if (!ss.st_size) {
		ERROR_MSG("load_file_read: file is empty %s\n", filename);
		close(fd);
		return NULL;
	}

	text = current = MALLOC(unsigned char *, ss.st_size +1); /* include space for a null terminating character */
	if (!text) {
		ERROR_MSG("load_file_read: cannot allocate memory to read file %s\n", filename);
		close(fd);
		return NULL;
	}

	if (ss.st_size > SSIZE_MAX) {
		/* file is greater that read's max block size: we must make a loop */
		blocksz = SSIZE_MAX;
	} else {
		blocksz = ss.st_size+1;
	}

	left2read = ss.st_size; //+1;
	readsz = 0;

	while (left2read > 0) {
		readsz = read(fd, current, blocksz);
		if (readsz > 0) {
			/* ok, we have read a block, continue */
			current += blocksz;
			left2read -= blocksz;
		} else {
			/* is this the end of the file ? */
			if (readsz == 0) {
				/* yes */
				break;
			} else {
				/* error */
				PERROR_MSG("load_file_read: error reading file %s\n", filename);
				/* cleanup */
				FREE(text);
				close(fd);
				return NULL;
			}
		}
	}
	/* null terminate this string */
	text[ss.st_size] = '\0';

	return create_openned_file(filename, fd, ss.st_size+1, text,0,0,FALSE);
}
#endif //FRONTEND_GETS_FILES

#ifdef FRONTEND_GETS_FILES
/* these variables are used on return of data from front end, and are passed on to create_openned_file */
static unsigned char *fileText = NULL;
static char *fileToGet = NULL;
static int frontend_return_status = 0;
static int fileSize = 0;
static int imageWidth;
static int imageHeight;
static bool imageAlpha;

static pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
#define LOCK_LOAD_FILE_FUNCTION pthread_mutex_lock( &mutex1 );
#define UNLOCK_LOAD_FILE_FUNCTION pthread_mutex_unlock(&mutex1);

static pthread_mutex_t  getAFileLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t waitingForFile = PTHREAD_COND_INITIALIZER;
#define MUTEX_LOCK_FILE_RETRIEVAL               pthread_mutex_lock(&getAFileLock);
#define MUTEX_FREE_LOCK_FILE_RETRIEVAL       pthread_mutex_unlock(&getAFileLock);
#define WAIT_FOR_FILE_SIGNAL		pthread_cond_wait(&waitingForFile,&getAFileLock);
#define SEND_FILE_SIGNAL		pthread_cond_signal(&waitingForFile);

/* accessor functions */
/* return the filename of the file we want. */

char *fwg_frontEndWantsFileName() {
	//if (fileToGet != NULL) printf ("fwg_frontEndWantsFileName called - fileName currently %s\n",fileToGet);
	return fileToGet;
}

void fwg_frontEndReturningData(unsigned char* fileData,int len,int width,int height,bool hasAlpha) {

	MUTEX_LOCK_FILE_RETRIEVAL
    
    //ConsoleMessage ("fwg_frontEndReturningData, len %d",len);
	/* did we get data? is "len" not zero?? */
	if (len == 0) {
		// printf ("fwg_frontEndReturningData, returning error\n");
		//frontend_return_status = -1;
		fileText = NULL;
		fileSize = 0;

	} else {
		// printf ("fwg_frontEndReturningData, returning ok\n");
    		/* note the "+1" ....*/
		fileText = MALLOC (unsigned char *, len+1);
		memcpy (fileText, fileData, len);
		fileSize = len;
		imageWidth = width;
		imageHeight = height;
		imageAlpha = hasAlpha;
    
	    /* ok - we do not know if this is a binary or a text file,
	       but because we added 1 to it, we can put a null terminator
	       on the end - that will terminate a text string, but will
	       not affect a binary file, because we have the binary data
	       and binary length recorded. */

	    fileText[len] = '\0';  /* the string terminator */
         //printf ("fwg_frontEndReturningData: returning data, but fileToGet setting to NULL, was %s\n",fileToGet);
        
        
		fileToGet = NULL; /* not freed as only passed by pointer */

		frontend_return_status = 0;
		/* got the file, send along a message */
	}

    
	SEND_FILE_SIGNAL

	MUTEX_FREE_LOCK_FILE_RETRIEVAL
}

#else
char *fwg_frontEndWantsFileName() {return NULL;}
void fwg_frontEndReturningData(unsigned char* fileData,int length,int width,int height,bool hasAlpha) {}
#endif




/**
 *   load_file: read file into memory, return the buffer.
 */
openned_file_t* load_file(const char *filename)
{

    openned_file_t *of = NULL;


    
    
    
	DEBUG_RES("loading file: %s pthread %p\n", filename,pthread_self());
    //printf ("load_file, fileToGet %s, load_file %s thread %ld\n",fileToGet,filename,pthread_self());
    
#ifdef FRONTEND_GETS_FILES

 
    
    LOCK_LOAD_FILE_FUNCTION


    MUTEX_LOCK_FILE_RETRIEVAL
    
	//JAS - we keep this around until done with resource FREE_IF_NZ(fileText);

	fileToGet = (char *)filename;
    //printf ("load_file, fileToGet set to :%s:\n",filename);
    
    
    WAIT_FOR_FILE_SIGNAL

	MUTEX_FREE_LOCK_FILE_RETRIEVAL

 
    
	if(frontend_return_status == -1) of = NULL;
    else of = create_openned_file(filename, -1, fileSize, fileText, imageHeight, imageWidth, imageAlpha);
    
    UNLOCK_LOAD_FILE_FUNCTION  
    return of;
    
#else //FRONTEND_GETS_FILES 



#if defined(FW_USE_MMAP)
#if !defined(_MSC_VER)
	/* UNIX mmap */
	of = load_file_mmap(filename);
#else
	/* Windows CreateFileMapping / MapViewOfFile */
	of = load_file_win32_mmap(filename);
#endif
#else
	/* Standard read */
	of = load_file_read(filename);
#endif
	DEBUG_RES("%s loading status: %s\n", filename, BOOL_STR((of!=NULL)));
	return of;
#endif //FRONTEND_GETS_FILES
}


/**
 *   check the first few lines to see if this is an XMLified file
 */
int determineFileType(const unsigned char *buffer, const int len)
{
	const unsigned char *rv;
	int count;
	int foundStart = FALSE;
    
	for (count = 0; count < 3; count ++) inputFileVersion[count] = 0;

	/* is this an XML file? see also further down for < detection*/
	if (strncmp((const char*)buffer,"<?xml version",12) == 0){
		rv = buffer;	

		/* skip past the header; we will look for lines like: 
		   <?xml version="1.0" encoding="UTF-8"?>
		   <!DOCTYPE X3D PUBLIC "ISO//Web3D//DTD X3D 3.0//EN"   "http://www.web3d.org/specifications/x3d-3.0.dtd">
		   <X3D
		*/
		rv++;
		while (!foundStart) {
			while ((*rv != '<') && (*rv != '\0')) rv++;
			if (*rv == '<') {
				rv++;
				if (*rv != '!') foundStart = TRUE;
			} else if (*rv == '\0') foundStart = TRUE;	
		}
		if (strncmp((const char*)rv,"X3D",3) == 0) {
			/* the full version number will be found by the parser */
			inputFileVersion[0] = 3;
			return IS_TYPE_XML_X3D;
        }
        
#if defined (INCLUDE_NON_WEB3D_FORMATS)
		if (strncmp((const char*)rv,"COLLADA",7) == 0) {
			return IS_TYPE_COLLADA;
		}
		if (strncmp((const char*)rv,"kml",3) == 0) {
			return IS_TYPE_KML;
		}
#endif //INCLUDE_NON_WEB3D_FORMATS

	} else {
		if (strncmp((const char*)buffer,"#VRML V2.0 utf8",15) == 0) {
			inputFileVersion[0] = 2;
			return IS_TYPE_VRML;
		}

		if (strncmp ((const char*)buffer, "#X3D",4) == 0) {
			inputFileVersion[0] = 3;
			/* ok, have X3D here, what version? */

			if (strncmp ((const char*)buffer,"#X3D V3.0 utf8",14) == 0) {
				return IS_TYPE_VRML;
			}
			if (strncmp ((const char*)buffer,"#X3D V3.1 utf8",14) == 0) {
				inputFileVersion[1] = 1;
				return IS_TYPE_VRML;
			}
			if (strncmp ((const char*)buffer,"#X3D V3.2 utf8",14) == 0) {
				inputFileVersion[1] = 2;
				return IS_TYPE_VRML;
			}
			if (strncmp ((const char*)buffer,"#X3D V3.3 utf8",14) == 0) {
				inputFileVersion[1] = 3;
				return IS_TYPE_VRML;
			}
			if (strncmp ((const char*)buffer,"#X3D V3.4 utf8",14) == 0) {
				inputFileVersion[1] = 4;
				return IS_TYPE_VRML;
			}
			/* if we fall off the end, we just assume X3D 3.0 */
		}
		
		/* VRML V1? */
		if (strncmp((const char*)buffer,"#VRML V1.0 asc",10) == 0) {
			return IS_TYPE_VRML1;
		}


	}
	/* try simple x3d ie when its a partial string from createX3DfromString */
	rv = buffer;	
	while(rv && *rv != '\0'){
		if(*rv == '<') return IS_TYPE_XML_X3D;
		if (*rv == '{') return IS_TYPE_VRML;
		rv++;
	}

    #if defined (INCLUDE_STL_FILES)
	return stlDTFT(buffer,len);
    #endif //INCLUDE_STL_FILES
    
	return IS_TYPE_UNKNOWN;
}

/*
 * FIXME: what are the possible return codes for this function ???
 *
 * FIXME: refactor this function, too :)
 *
 */
#ifndef _MSC_VER
int freewrlSystem (const char *sysline)
{

//#ifdef _MSC_VER
//	return system(sysline);
//#else
#define MAXEXECPARAMS 10
#define EXECBUFSIZE	2000
	char *paramline[MAXEXECPARAMS];
	char buf[EXECBUFSIZE];
	char *internbuf;
	int count;
	/* pid_t childProcess[lastchildProcess]; */
	pid_t child;
	int pidStatus;
	int waitForChild;
	int haveXmessage;


	/* initialize the paramline... */
	memset(paramline, 0, sizeof(paramline));
		
	waitForChild = TRUE;
	haveXmessage = !strncmp(sysline, FREEWRL_MESSAGE_WRAPPER, strlen(FREEWRL_MESSAGE_WRAPPER));

	internbuf = buf;

	/* bounds check */
	if (strlen(sysline)>=EXECBUFSIZE) return FALSE;
	strcpy (buf,sysline);

	/* printf ("freewrlSystem, have %s here\n",internbuf); */
	count = 0;

	/* do we have a console message - (which is text with spaces) */
	if (haveXmessage) {
		paramline[0] = FREEWRL_MESSAGE_WRAPPER;
		paramline[1] = strchr(internbuf,' ');
		count = 2;
	} else {
		/* split the command off of internbuf, for execing. */
		while (internbuf != NULL) {
			/* printf ("freewrlSystem: looping, count is %d\n",count);  */
			paramline[count] = internbuf;
			internbuf = strchr(internbuf,' ');
			if (internbuf != NULL) {
				/* printf ("freewrlSystem: more strings here! :%s:\n",internbuf); */
				*internbuf = '\0';
				/* printf ("param %d is :%s:\n",count,paramline[count]); */
				internbuf++;
				count ++;
				if (count >= MAXEXECPARAMS) return -1; /*  never...*/
			}
		}
	}
	
	/* printf ("freewrlSystem: finished while loop, count %d\n",count); 
	
	   { int xx;
	   for (xx=0; xx<MAXEXECPARAMS;xx++) {
	   printf ("item %d is :%s:\n",xx,paramline[xx]);
	   }} */
	
	if (haveXmessage) {
		waitForChild = FALSE;
	} else {
		/* is the last string "&"? if so, we don't need to wait around */
		if (strncmp(paramline[count],"&",strlen(paramline[count])) == 0) {
			waitForChild=FALSE;
			paramline[count] = '\0'; /*  remove the ampersand.*/
		}
	}

	if (count > 0) {
/* 		switch (childProcess[lastchildProcess]=fork()) { */
		child = fork();
		switch (child) {
		case -1:
			perror ("fork");
			exit(1);
			break;

		case 0: 
		{
			int Xrv;
				
			/* child process */
			/* printf ("freewrlSystem: child execing, pid %d %d\n",childProcess[lastchildProcess], getpid());  */
			Xrv = execl((const char *)paramline[0],
				    (const char *)paramline[0],paramline[1], paramline[2],
				    paramline[3],paramline[4],paramline[5],
				    paramline[6],paramline[7], NULL);
			printf ("FreeWRL: Fatal problem execing %s\n",paramline[0]);
			perror("FreeWRL: "); 
			exit (Xrv);
		}
		break;

		default: 
		{
			/* parent process */
			/* printf ("freewrlSystem: parent waiting for child %d\n",childProcess[lastchildProcess]); */

			/* do we have to wait around? */
			if (!waitForChild) {
				/* printf ("freewrlSystem - do not have to wait around\n"); */
				return TRUE;
			}
/* 			waitpid (childProcess[lastchildProcess],&pidStatus,0); */
			waitpid(child, &pidStatus, 0);

			/* printf ("freewrlSystem: parent - child finished - pidStatus %d \n",
			   pidStatus);  */
				
			/* printf ("freewrlSystem: WIFEXITED is %d\n",WIFEXITED(pidStatus)); */
				
			/* if (WIFEXITED(pidStatus) == TRUE) printf ("returned ok\n"); else printf ("problem with return\n"); */
		}
		}
		return (WIFEXITED(pidStatus) == TRUE);
	} else {
		printf ("System call failed :%s:\n",sysline);
	}
	return -1; /* should we return FALSE or -1 ??? */
//#endif
}
#endif
//goal: remove a directory and its contents - used for removing the temp unzip folder for .z3z / .zip file processing
#ifdef _MSC_VER
//http://msdn.microsoft.com/en-us/windows/desktop/aa365488
//#undef _UNICODE
//#undef UNICODE
//#include <windows.h>
//#undef _UNICODE
////#undef _MBCS
//#undef UNICODE

#include <TCHAR.H>
#ifdef UNICODE
static TCHAR *singleDot = L".";
static TCHAR *doubleDot = L"..";
static TCHAR *backslash = L"\\";
static TCHAR *star = L"*";

#else
static TCHAR *singleDot = ".";
static TCHAR *doubleDot = "..";
static TCHAR *backslash = "\\";
static TCHAR *star = "*";
#endif

// http://www.codeproject.com/Articles/9089/Deleting-a-directory-along-with-sub-folders
BOOL IsDots(const TCHAR* str) {
	if(_tcscmp(str,singleDot) && _tcscmp(str,doubleDot))
		return FALSE;
	return TRUE;
}
BOOL DeleteDirectory0(const TCHAR* sPath) {
	HANDLE hFind;  // file handle
	WIN32_FIND_DATA FindFileData;
	TCHAR DirPath[MAX_PATH];
	TCHAR FileName[MAX_PATH];
	BOOL bSearch;

	_tcscpy(DirPath,sPath);
	_tcscat(DirPath,backslash);    // searching all files
	_tcscat(DirPath,star);
	_tcscpy(FileName,sPath);
	_tcscat(FileName,backslash);

#if _MSC_VER > 1500
	hFind = FindFirstFileEx(DirPath, FindExInfoStandard, &FindFileData, FindExSearchNameMatch, NULL, 0); // find the first file - requires windows XP or later
#else
	//
	hFind = FindFirstFile(DirPath,&FindFileData); // find the first file
#endif
	if(hFind == INVALID_HANDLE_VALUE) 
		return FALSE;
	_tcscpy(DirPath,FileName);
        
	bSearch = TRUE;
	while(bSearch) { // until we finds an entry
		if(FindNextFile(hFind,&FindFileData)) {
			if(IsDots(FindFileData.cFileName)) continue;
			_tcscat(FileName,FindFileData.cFileName);
			if((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				// we have found a directory, recurse
				if(!DeleteDirectory0(FileName)) { 
					FindClose(hFind); 
					return FALSE; // directory couldn't be deleted
				}
				RemoveDirectory(FileName); // remove the empty directory
				_tcscpy(FileName,DirPath);
			}
			else {
				if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
					_tchmod(FileName, 777); //_S_IWRITE); // change read-only file mode
				if(!DeleteFile(FileName)) {  // delete the file
					/*
					DWORD err = GetLastError();
					if (err == ERROR_FILE_NOT_FOUND)
						printf("file not found\n");
					else if (err == ERROR_ACCESS_DENIED)
						printf("access denied\n");
					else if (err == ERROR_SHARING_VIOLATION)
						printf("sharing violation\n");
					else
						printf("other erro\n");
					*/
					FindClose(hFind); 
					return FALSE; 
				}                 
				_tcscpy(FileName,DirPath);
			}
		}
		else {
			if(GetLastError() == ERROR_NO_MORE_FILES) // no more files there
				bSearch = FALSE;
			else {
				// some error occured, close the handle and return FALSE
				FindClose(hFind); 
				return FALSE;
			}
		}
	}
	FindClose(hFind);  // closing file handle
	return RemoveDirectory(sPath); // remove the empty directory
}
BOOL directory_remove_all(const char* sPath) {
	int jj;
    size_t convertedChars = 0;
    TCHAR wcstring[MAX_PATH];
	char fname2[MAX_PATH];
	size_t origsize; //= strlen(fname) + 1;
	BOOL retval;
	origsize = strlen(sPath) + 1;
	strcpy(fname2,sPath);
	for(jj=0;jj<strlen(fname2);jj++)
		if(fname2[jj] == '/' ) fname2[jj] = '\\';

#ifdef _UNICODE
#if _MSC_VER >= 1500
	mbstowcs_s(&convertedChars, wcstring, origsize, fname2, _TRUNCATE);
#else
	mbstowcs(wcstring, fname2, MB_CUR_MAX);
#endif
#else
	_tcscpy(wcstring,fname2);
#endif
	retval = DeleteDirectory0(wcstring);
	return retval;
}
BOOL tdirectory_remove_all(TCHAR *sPath){
	BOOL retval;
	retval = DeleteDirectory0(sPath);
	return retval;
}
void tremove_file_or_folder(TCHAR *path){
	int iret, isDir;
	DWORD finfo, err;
#if _MSC_VER > 1500
	// http://msdn.microsoft.com/en-us/library/windows/desktop/aa364946(v=vs.85).aspx
	WIN32_FILE_ATTRIBUTE_DATA fad;
	finfo = GetFileAttributesEx(path, GetFileExInfoStandard, &fad);
	if (!finfo){
		err = GetLastError();
		//FormatMessage()
		ConsoleMessage("GetFileAttribuesEx err=%d maxpath%d pathlen%d", (int)err,MAX_PATH,_tcslen(path)); //http://msdn.microsoft.com/en-us/library/windows/desktop/ms681381(v=vs.85).aspx
		isDir = ! _tcsstr(path, singleDot);
	}else
	isDir = finfo && (fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
#else
	// http://msdn.microsoft.com/en-us/library/windows/desktop/aa364944%28v=vs.85%29.aspx
	// http://msdn.microsoft.com/en-us/library/windows/desktop/gg258117%28v=vs.85%29.aspx
	finfo = GetFileAttributes(path);
	isDir = FILE_ATTRIBUTE_DIRECTORY & finfo;
#endif
	if(isDir)
		tdirectory_remove_all(path);
	else
		DeleteFile(path);
}
void remove_file_or_folder(const char *path){
	//libfreewrl uses ascii or multibyte string functions, like strcpy, that look for a '\0' as end of string
	//when sending something into freewrl thats 2-byte wide string, first convert it to multibyte
	//when coming out, if you want to go back to wide-string then you need to convert to wide string
	//tchar functions are supposed to be agnostic -they compile either way
	int jj;
    size_t convertedChars = 0;
    TCHAR wcstring[MAX_PATH];
	char fname2[MAX_PATH];
	size_t origsize; //= strlen(fname) + 1;
	BOOL retval;
	origsize = strlen(path) + 1;
	strcpy(fname2,path);
	for(jj=0;jj<strlen(fname2);jj++)
		if(fname2[jj] == '/' ) fname2[jj] = '\\';

#ifdef _UNICODE
#if _MSC_VER >= 1500
	mbstowcs_s(&convertedChars, wcstring, origsize, fname2, _TRUNCATE);
#else
	mbstowcs(wcstring, fname2, MB_CUR_MAX);
#endif
#else
	_tcscpy(wcstring,fname2);
#endif
	tremove_file_or_folder(wcstring);
}
#else // POSIX and OSX - WARNING UNTESTED as of Sept 7, 2013
//according to boost, unlike posix OSX must do separate rmdir for directory and unlink for file
//goal: remove a directory and its contents - used for removing the temp unzip folder for .z3z / .zip file processing
int directory_remove_all(const char *path)
{
	DIR *d = opendir(path);
	size_t path_len = strlen(path);
	int r = -1;

	if (d)
	{
		struct dirent *p;
		r = 0;

		while (!r && (p=readdir(d)))
		{
			int r2 = -1;
			char *buf;
			size_t len;

			/* Skip the names "." and ".." as we don't want to recurse on them. */
			if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
			{
				continue;
			}
			len = path_len + strlen(p->d_name) + 2; 
			buf = malloc(len);
			if (buf)
			{
				struct stat statbuf;
				snprintf(buf, len, "%s/%s", path, p->d_name);
				if (!stat(buf, &statbuf))
				{
					if (S_ISDIR(statbuf.st_mode))
					{
						r2 = directory_remove_all(buf);
					}
					else
					{
						r2 = unlink(buf);
					}
				}
				free(buf);
			}
			r = r2;
		}
		closedir(d);
	}
	if (!r)
	{
		r = rmdir(path);
	}
	return r;
}
void remove_file_or_folder(const char * path){
	struct stat statbuf;
	if (!stat(path, &statbuf))
	{
        int r2;
	UNUSED (r2);

		if (S_ISDIR(statbuf.st_mode))
		{
			r2 = directory_remove_all(path);
		}
		else
		{
			r2 = unlink(path);
		}
	}
}
#endif



//could maybe be in a separate C file?
#ifdef HAVE_UNZIP_H
#include <unzip.h>
#define WRITEBUFFERSIZE (8192)


int unzip_archive_to_temp_folder(const char *zipfilename, const char* tempfolderpath)
{

    const char *filename_to_extract=NULL;
    int ret_value=0;
    const char *dirname=NULL;
	char temppath[256];
	char *fullpath = NULL;
    unzFile uf=NULL;

    uf = unzOpen(zipfilename);
    if (uf==NULL)
    {
        printf("Cannot open %s \n",zipfilename);
        return 1;
    }
    printf("%s opened\n",zipfilename);
	temppath[0] = '\0';
	if(tempfolderpath){
		fw_mkdir(tempfolderpath);
	}

	{
		uLong i;
		unz_global_info gi;
		int err;
		FILE* fout=NULL;

		err = unzGetGlobalInfo(uf,&gi);
		if (err!=UNZ_OK){
			printf("error %d with zipfile in unzGetGlobalInfo \n",err);
			return err;
		}

		for (i=0;i<gi.number_entry;i++)
		{
			{
				char filename_inzip[256];
				char* filename_withoutpath;
				char* p;
				int err=UNZ_OK;
				FILE *fout=NULL;
				void* buf;
				uInt size_buf;

				unz_file_info file_info;
				uLong ratio=0;
				err = unzGetCurrentFileInfo(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);

				if (err!=UNZ_OK)
				{
					printf("error %d with zipfile in unzGetCurrentFileInfo\n",err);
					return err;
				}

				size_buf = WRITEBUFFERSIZE;
				buf = (void*)malloc(size_buf);
				if (buf==NULL)
				{
					printf("Error allocating memory\n");
					return UNZ_INTERNALERROR;
				}

				p = filename_withoutpath = filename_inzip;
				while ((*p) != '\0')
				{
					if (((*p)=='/') || ((*p)=='\\'))
						filename_withoutpath = p+1;
					p++;
				}

				if ((*filename_withoutpath)=='\0')
				{
					printf("creating directory: %s\n",filename_inzip);
					strcpy(temppath,tempfolderpath);
					strcat(temppath,"/");
					strcat(temppath,filename_inzip);
					//fw_mkdir(filename_inzip);
					fw_mkdir(temppath);
				}
				else
				{
					const char* write_filename;
					int skip=0;

					write_filename = filename_inzip;

					err = unzOpenCurrentFile(uf);
					if (err!=UNZ_OK)
					{
						printf("error %d with zipfile in unzOpenCurrentFile\n",err);
					}

					if (err==UNZ_OK)
					{
						strcpy(temppath,tempfolderpath);
						strcat(temppath,"/");
						strcat(temppath,write_filename);
						//fout=fopen(write_filename,"wb");
						fout=fopen(temppath,"wb");
					}

					if (fout!=NULL)
					{
						printf(" extracting: %s\n",write_filename);

						do
						{
							err = unzReadCurrentFile(uf,buf,size_buf);
							if (err<0)
							{
								printf("error %d with zipfile in unzReadCurrentFile\n",err);
								break;
							}
							if (err>0)
								if (fwrite(buf,err,1,fout)!=1)
								{
									printf("error in writing extracted file\n");
									err=UNZ_ERRNO;
									break;
								}
						}
						while (err>0);
						if (fout)
								fclose(fout);

					}

					if (err==UNZ_OK)
					{
						err = unzCloseCurrentFile (uf);
						if (err!=UNZ_OK)
						{
							printf("error %d with zipfile in unzCloseCurrentFile\n",err);
						}
					}
					else
						unzCloseCurrentFile(uf); /* don't lose the error */
				}

				free(buf);
			}
			if(err) break;

			if ((i+1)<gi.number_entry)
			{
				err = unzGoToNextFile(uf);
				if (err!=UNZ_OK)
				{
					printf("error %d with zipfile in unzGoToNextFile\n",err);
					break;
				}
			}
		}

	}

    unzClose(uf);
    return ret_value;
}

char* remove_filename_from_path(const char *path);
char *strBackslash2fore(char *str);
void resitem_enqueue(s_list_t *item);
void process_x3z(resource_item_t *res){
	int err;
	char request[256];
	char* tempfolderpath;
	if (0){
		tempfolderpath = tempnam(gglobal()->Mainloop.tmpFileLocation, "freewrl_download_XXXXXXXX");
	}else{
		tempfolderpath = STRDUP(res->URLrequest);
		tempfolderpath = strBackslash2fore(tempfolderpath);
		tempfolderpath = remove_filename_from_path(tempfolderpath);
		tempfolderpath = tempnam(tempfolderpath, "freewrl_download_XXXXXXXX");
	}
	err = unzip_archive_to_temp_folder(res->actual_file, tempfolderpath);
	if(!err){
		resource_item_t *docx3d;
		//I need a resource just for cleaning up the temp folder in one shot
		strcpy(request,tempfolderpath);
		strcat(request,"/doc.x3d");
		docx3d = resource_create_single(request);
		docx3d->parent = NULL; //divorce so it doesn't inherit rest_url
		docx3d->type = rest_file;
		docx3d->media_type = resm_x3d;
		docx3d->treat_as_root = 1;
		//docx3d->temp_dir = tempfolderpath;
		resitem_enqueue(ml_new(docx3d));
		// clean up temp folder via resource with opennedfile entry
		res->cached_files = ml_append(res->cached_files,ml_new(tempfolderpath));
		ConsoleMessage("unzip folder:%s\n", tempfolderpath);
	}
	else{
		ConsoleMessage("unzip failed to folder:%s\n", tempfolderpath);
	}
}

#else
void process_x3z(resource_item_t *res){
}
#endif
