/*

  General utility functions.

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

#include "../vrml_parser/Structs.h"
#include "headers.h"


char *BrowserFullPath = NULL;
char *BrowserName = "FreeWRL VRML/X3D Browser";

const char* freewrl_get_browser_program()
{
    char *tmp;

    /*
      1. Check BROWSER environment variable
      2. Use configuration value BROWSER
    */

    tmp = getenv("BROWSER");
    if (!tmp) {
	tmp = BROWSER;
    }
    return tmp;
}

/**
 * This code get compiled only when debugging is enabled
 */

#ifdef DEBUG_MALLOC

#define FREETABLE(a,file,line) mcount=0; \
	while ((mcount<(MAXMALLOCSTOKEEP-1)) && (mcheck[mcount]!=a)) mcount++; \
		if (mcheck[mcount]!=a) printf ("freewrlFree - did not find %x at %s:%d\n",a,file,line); \
		else { \
			/* printf ("found %d in mcheck table\n"); */ \
			mcheck[mcount] = NULL; \
			mlineno[mcount] = 0; \
			if (mplace[mcount]!=NULL) free(mplace[mcount]); \
			mplace[mcount]=NULL; \
		} 

#define RESERVETABLE(a,file,line,size) mcount=0; \
	while ((mcount<(MAXMALLOCSTOKEEP-1)) && (mcheck[mcount]!=NULL)) mcount++; \
		if (mcheck[mcount]!=NULL) printf ("freewrlMalloc - out of malloc check store\n");\
		else {\
			mcheck[mcount] = a;\
			mlineno[mcount] = line;\
			mplace[mcount] = strdup(file);\
			msize[mcount] = size;\
		}

#define MAXMALLOCSTOKEEP 100000
static int mcheckinit = FALSE;
static void* mcheck[MAXMALLOCSTOKEEP];
static char* mplace[MAXMALLOCSTOKEEP];
static int mlineno[MAXMALLOCSTOKEEP];
static size_t msize[MAXMALLOCSTOKEEP];
static int mcount;

void freewrlFree(int line, char *file, void *a)
{
    printf ("freewrlFree %x xfree at %s:%d\n",a,file,line); 
    FREETABLE(a,file,line);
    free(a);
}

void scanMallocTableOnQuit_old()
{
    for (mcount=0; mcount<MAXMALLOCSTOKEEP;mcount++) {
	if (mcheck[mcount]!=NULL) {
	    printf ("unfreed memory %x created at %s:%d \n",mcheck[mcount], mplace[mcount],mlineno[mcount]);
	}
    }
}
typedef struct malloc_location {
	int count;
	int line;
	size_t size;
	char *fname;
} malloc_location;
#include <memory.h>
#ifdef _MSC_VER
#define alloca _alloca
#endif
void scanMallocTableOnQuit()
{
	//this version will sum up the lines were the mallocs are occuring that aren't freed
	int nlocs,j,iloc;
	size_t total;
	malloc_location *mlocs = malloc(sizeof(malloc_location)*MAXMALLOCSTOKEEP);
	memset(mlocs,0,sizeof(malloc_location)*MAXMALLOCSTOKEEP);
	nlocs = 0;
    for (mcount=0; mcount<MAXMALLOCSTOKEEP;mcount++) {
		if (mcheck[mcount]!=NULL) {
			//printf ("unfreed memory %x created at %s:%d \n",mcheck[mcount], mplace[mcount],mlineno[mcount]);
			iloc = -1;
			for(j=0;j<nlocs;j++){
				if(!strcmp(mplace[mcount],mlocs[j].fname) && (mlineno[mcount] == mlocs[j].line) ){
					mlocs[j].count ++;
					mlocs[j].size += msize[mcount];
					iloc = j;
					break;
				}
			}
			if(iloc == -1){
				mlocs[nlocs].count = 1;
				mlocs[nlocs].fname = mplace[mcount];
				mlocs[nlocs].line = mlineno[mcount];
				mlocs[nlocs].size = msize[mcount];
				nlocs++;
			}
		}
    }
	printf("unfreed:\n");
	printf("%5s %8s %4s %55s\n","count","size","line","file");
	total = 0;
	for(j=0;j<nlocs;j++){
		printf("%5d %8d %4d %55s\n",mlocs[j].count,mlocs[j].size, mlocs[j].line,mlocs[j].fname);
		total += mlocs[j].size;
	}
	printf("total bytes not freed %d\n",total);
	free(mlocs);
	getchar();
}


/**
 * Check all mallocs
 */
void *freewrlMalloc(int line, char *file, size_t sz, int zeroData)
{
    void *rv;
    char myline[400];

    if (!mcheckinit) {
	for (mcount=0; mcount < MAXMALLOCSTOKEEP; mcount++) {
	    mcheck[mcount] = NULL;
	    mplace[mcount] = NULL;
	    mlineno[mcount] = 0;
	}
	mcheckinit = TRUE;
    }

    rv = malloc(sz);
    if (rv==NULL) {
	sprintf (myline, "MALLOC PROBLEM - out of memory at %s:%d for %d",file,line,sz);
	outOfMemory (myline);
    }
    printf ("%x malloc %d at %s:%d\n",rv,sz,file,line); 
    RESERVETABLE(rv,file,line,sz);

    if (zeroData) bzero (rv, sz);
    return rv;
}

void *freewrlRealloc (int line, char *file, void *ptr, size_t size)
{
    void *rv;
    char myline[400];

    printf ("%x xfree (from realloc) at %s:%d\n",ptr,file,line);
    rv = realloc (ptr,size);
    if (rv==NULL) {
	if (size != 0) {
	    sprintf (myline, "REALLOC PROBLEM - out of memory at %s:%d size %d",file,line,size);
	    outOfMemory (myline);
	}
    }
    
    /* printf ("%x malloc (from realloc) %d at %s:%d\n",rv,size,file,line); */
    FREETABLE(ptr,file,line);
    RESERVETABLE(rv,file,line,size);
	
    return rv;
}


void *freewrlStrdup (int line, char *file, char *str)
{
    void *rv;
    char myline[400];

    printf("freewrlStrdup, at line %d file %s\n",line,file);
    rv = strdup (str);
    if (rv==NULL) {
	sprintf (myline, "STRDUP PROBLEM - out of memory at %s:%d ",file,line);
	outOfMemory (myline);
    }
printf ("freewrlStrdup, before reservetable\n");

    RESERVETABLE(rv,file,line,strlen(str)+1);
    return rv;
}

#endif /* defined(DEBUG_MALLOC) */

/**
 * function to debug multi strings
 * we need to find where to put it....
 */
void Multi_String_print(struct Multi_String *url)
{
	if (url) {
		if (!url->p) {
			PRINTF("multi url: <empty>");
		} else {
			int i;

			PRINTF("multi url: ");
			for (i = 0; i < url->n; i++) {
				struct Uni_String *s = url->p[i];
				PRINTF("[%d] %s", i, s->strptr);
			}
		}
		PRINTF("\n");
	}
}
