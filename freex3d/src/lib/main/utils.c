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
#ifdef DISABLER
// ================ nice new code by disabler. use while tg still alive===============
#ifdef _ANDROID
#define WRAP_MALLOC 1

#ifdef DEBUG
int DROIDDEBUG( const char*pFmtStr, ...);
#define printf DROIDDEBUG
#endif

#endif

/**
 * This code get compiled only when debugging is enabled
 */

#if defined(WRAP_MALLOC) || defined(DEBUG_MALLOC)

#define FWL_NOT_FOUND_IN_MEM_TABLE -1

void __free_memtable_elem(void *ptr);
void __free_memtable_elem_with_data(void *ptr);
int __match_memtable_elem(void *elem1, void *elem2);

typedef struct fwl_memtable_elem
{
    void *ptr;
    int lineNubmer;
    char *fileName;
} fwl_memtable_elem;


static ttglobal sLastSeenGlobal = NULL;

void __freeWholeMemTable(ttglobal gg)
{
    if (gg->__memTable != NULL)
    {
        gg->__memTable->free = &__free_memtable_elem_with_data;
        dbl_list_destroy(gg->__memTable);
        gg->__memTable = NULL;
    }
}

int __removeFromMemTable(ttglobal gg, void *ptr, char *file, int line)
{
    int retVal = FWL_NOT_FOUND_IN_MEM_TABLE;
	if (gg->__memTable != NULL)
    {
        fwl_memtable_elem searchedElem;
        searchedElem.ptr = ptr;
        dbl_list_node_t *node = dbl_list_find(gg->__memTable, &searchedElem);
        if (node)
        {
            retVal = 0;
            dbl_list_remove(gg->__memTable, node);
        }
        else
        {
            printf ("freewrlFree - did not find 0x%016llx at %s:%d\n", (unsigned long long)ptr,file,line);
        }
    }
    
    return retVal;
}

void __reserveInMemTable(ttglobal gg, void *ptr, char *file, int line)
{
	if (gg->__memTable != NULL)
    {
        fwl_memtable_elem searchedElem;
        searchedElem.ptr = ptr;
        dbl_list_node_t *node = dbl_list_find(gg->__memTable, &searchedElem);
        if (node)
        {
            fwl_memtable_elem *foundElem = (fwl_memtable_elem *)node->val;
            
            printf ("freewrl__ReserveInMemTable - ptr already in the table 0x%016llx at %s:%d, added at %s:%d\n", (unsigned long long)ptr, file, line, foundElem->fileName, foundElem->lineNubmer);

        }
        else
        {
            fwl_memtable_elem *newElem = malloc(sizeof(fwl_memtable_elem));
            newElem->fileName = file;
            newElem->lineNubmer = line;
            newElem->ptr = ptr;
            dbl_list_rpush(gg->__memTable, dbl_list_node_new(newElem));
        }
    }
}

#define LOCK_GLOBAL_MEMORYTABLE 		if (tg) pthread_mutex_lock(&tg->__memTableGlobalLock);
#define UNLOCK_GLOBAL_MEMORYTABLE		if (tg) pthread_mutex_unlock(&tg->__memTableGlobalLock);

ttglobal freewrlGetActualGlobal()
{
    ttglobal tg = gglobal0();
    if (tg)
    {
        sLastSeenGlobal = tg;
    }
    else
    {
        tg = sLastSeenGlobal;
    }
    
    return tg;
}

void freewrlInitMemTable()
{
    ttglobal tg = freewrlGetActualGlobal();
    if (tg)
    {
        pthread_mutex_init(&(tg->__memTableGlobalLock), NULL);
    }
    LOCK_GLOBAL_MEMORYTABLE
    
    if (tg && !tg->__memTable_CheckInit) {
        
        tg->__memTable = dbl_list_new();
        tg->__memTable->free = &__free_memtable_elem;
        tg->__memTable->match = &__match_memtable_elem;
        tg->__memTable_CheckInit = TRUE;
    }
    
    UNLOCK_GLOBAL_MEMORYTABLE
}

void __free_memtable_elem_with_data(void *ptr)
{
    fwl_memtable_elem *elem = (fwl_memtable_elem *)ptr;
    #ifdef DEBUG_MALLOC
        printf ("freewrl MemTable disposing ptr 0x%016llx\n", (unsigned long long)elem->ptr);
    #endif
    free(elem->ptr);
    free(elem);
}

void __free_memtable_elem(void *ptr)
{
    free(ptr);
}

int __match_memtable_elem(void *elem1, void *elem2)
{
    return ((fwl_memtable_elem *)elem1)->ptr == ((fwl_memtable_elem *)elem2)->ptr;
}

void freewrlDisposeMemTable()
{
    ttglobal tg = freewrlGetActualGlobal();
    if (tg)
    {
        pthread_mutex_destroy(&(tg->__memTableGlobalLock));
        if (sLastSeenGlobal == tg)
        {
            sLastSeenGlobal = NULL;
        }
    }
}

void freewrlFree(int line, char *file, void *a)
{
    ttglobal tg = freewrlGetActualGlobal();
    LOCK_GLOBAL_MEMORYTABLE
    if (tg)
    {
        #ifdef DEBUG_MALLOC
            printf ("freewrlFree 0x%016llx xfree at %s:%d\n", (unsigned long long)a, file,line);
        #endif
        
        __removeFromMemTable(tg, a,file,line);
    }
    
    UNLOCK_GLOBAL_MEMORYTABLE
    free(a);
}

void scanMallocTableOnQuit()
{
    ttglobal tg = freewrlGetActualGlobal();
    LOCK_GLOBAL_MEMORYTABLE
    if (tg)
    {
        dbl_list_iterator_t *it = dbl_list_iterator_new(tg->__memTable, LIST_HEAD);
        dbl_list_node_t *node;
        while ((node = dbl_list_iterator_next(it))) {
            fwl_memtable_elem *elem = ((fwl_memtable_elem *)node->val);
            printf ("unfreed memory %016llx created at %s:%d \n", (unsigned long long)elem->ptr, elem->fileName, elem->lineNubmer);
        }
        dbl_list_iterator_destroy(it);
    }
    UNLOCK_GLOBAL_MEMORYTABLE
}

void freewrlSetShouldRegisterAllocation(bool shouldRegisterAllocation)
{
    ttglobal tg = freewrlGetActualGlobal();
    LOCK_GLOBAL_MEMORYTABLE
    if (tg)
    {
        tg->__memTable_ShouldRegisterAllocation = shouldRegisterAllocation;
    }
    UNLOCK_GLOBAL_MEMORYTABLE
}

bool freewrlIsRegisteringAllocation()
{
    ttglobal tg = freewrlGetActualGlobal();
    
    if (tg)
    {
        return tg->__memTable_ShouldRegisterAllocation;
    }
    
    return FALSE;
}

void freewrlFreeAllRegisteredAllocations()
{
    ttglobal tg = freewrlGetActualGlobal();
    LOCK_GLOBAL_MEMORYTABLE
    if (tg)
    {
        __freeWholeMemTable(tg);
    }
    UNLOCK_GLOBAL_MEMORYTABLE
}

/**
 * Check all mallocs
 */
void *freewrlMalloc(int line, char *file, size_t sz, int zeroData)
{
    void *rv;
    
    ttglobal tg = freewrlGetActualGlobal();
    LOCK_GLOBAL_MEMORYTABLE
    
    rv = malloc(sz);
    if (zeroData) bzero (rv, sz);
    
    #ifdef DEBUG_MALLOC
        if (rv == NULL)
        {
            char myline[400];
            sprintf (myline, "MALLOC PROBLEM - out of memory at %s:%d for %zu",file,line,sz);
            outOfMemory (myline);
        }

        printf ("freewrlMalloc 0x%016llx size %zu at %s:%d\n", (unsigned long long)rv,sz,file,line);
    #endif

    if (tg && tg->__memTable_ShouldRegisterAllocation)
    {
        __reserveInMemTable(tg, rv,file,line);
    }
    
    UNLOCK_GLOBAL_MEMORYTABLE
    
    return rv;
}

void *freewrlRealloc(int line, char *file, void *ptr, size_t size)
{
    void *rv;
    
    ttglobal tg = freewrlGetActualGlobal();
    LOCK_GLOBAL_MEMORYTABLE

//    printf ("%016llx xfree (from realloc) at %s:%d\n",ptr,file,line);
    rv = realloc (ptr,size);
    
    #ifdef DEBUG_MALLOC
        if (rv == NULL)
        {
            if (size != 0)
            {
                char myline[400];
                sprintf (myline, "REALLOC PROBLEM - out of memory at %s:%d size %zu",file,line,size);
                outOfMemory (myline);
            }
        }
    
        printf ("freewrlRealloc 0x%016llx to 0x%016llx size %zu at %s:%d\n", (unsigned long long)ptr, (unsigned long long)rv, size, file, line);
    #endif
    
    if (tg)
    {
        int result = 0;
        if (NULL != ptr)
        {
            result = __removeFromMemTable(tg, ptr,file,line);
        }
        if (result != FWL_NOT_FOUND_IN_MEM_TABLE) // If we were tracking this ptr previously
        {
            if (tg->__memTable_ShouldRegisterAllocation)
            {
                __reserveInMemTable(tg, rv,file,line);
            }
        }
        else
        {
            printf ("0x%016llx FOR REALLOC NOT FOUND for size %zu at %s:%d\n", (unsigned long long)ptr,size,file,line);
        }
    }
    
	UNLOCK_GLOBAL_MEMORYTABLE
    return rv;
}


void *freewrlStrdup (int line, char *file, const char *str)
{
    void *rv;

    ttglobal tg = freewrlGetActualGlobal();
    LOCK_GLOBAL_MEMORYTABLE
    rv = strdup (str);
    
    #ifdef DEBUG_MALLOC
        printf("freewrlStrdup 0x%016llx, at line %d file %s\n",(unsigned long long)rv, line,file);
        if (rv == NULL)
        {
            char myline[400];
            sprintf (myline, "STRDUP PROBLEM - out of memory at %s:%d ",file,line);
            outOfMemory (myline);
        }
    #endif
    
    if (tg && tg->__memTable_ShouldRegisterAllocation)
    {
        __reserveInMemTable(tg, rv,file,line);
    }
    UNLOCK_GLOBAL_MEMORYTABLE
    return rv;
}

void *freewrlStrndup (int line, char *file, const char *str, size_t n)
{
    void *rv;
    ttglobal tg = freewrlGetActualGlobal();
    LOCK_GLOBAL_MEMORYTABLE

    rv = strndup (str, n);
    
    #ifdef DEBUG_MALLOC
        printf("freewrlStrndup 0x%016llx count at line %d file %s\n", (unsigned long long)rv, line,file);
        if (rv == NULL)
        {
            char myline[400];
            sprintf (myline, "STRNDUP PROBLEM - out of memory at %s:%d ",file,line);
            outOfMemory (myline);
        }
    #endif
    
    if (tg && tg->__memTable_ShouldRegisterAllocation)
    {
        __reserveInMemTable(tg, rv,file,line);
    }
    UNLOCK_GLOBAL_MEMORYTABLE
    return rv;
}

#endif /* defined(WRAP_MALLOC) || defined(DEBUG_MALLOC) */

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
#endif

//================ older code hacked by dug9, can work after tg disposed================
#ifndef DISABLER

/**
 * This code get compiled only when debugging is enabled
 */
pthread_mutex_t __memTableGlobalLock = PTHREAD_MUTEX_INITIALIZER;
#define LOCK_GLOBAL_MEMORYTABLE 		pthread_mutex_lock(&__memTableGlobalLock);
#define UNLOCK_GLOBAL_MEMORYTABLE		pthread_mutex_unlock(&__memTableGlobalLock);

#ifdef DEBUG_MALLOC
static int _noisy = 0; //=1 if more printfs during malloc and free, 0 if just summary on exit

#define MAXMALLOCSTOKEEP 100000
static int mcheckinit = FALSE;
static void* mcheck[MAXMALLOCSTOKEEP];
static char* mplace[MAXMALLOCSTOKEEP];
static int mlineno[MAXMALLOCSTOKEEP];
static size_t msize[MAXMALLOCSTOKEEP];
static int mcount;

static mcheck_init(){
    if (!mcheckinit) {
		for (mcount=0; mcount < MAXMALLOCSTOKEEP; mcount++) {
			mcheck[mcount] = NULL;
			mplace[mcount] = NULL;
			mlineno[mcount] = 0;
		}
		mcheckinit = TRUE;
    }

}

void FREETABLE(void *a,char *file,int line) {
	LOCK_GLOBAL_MEMORYTABLE
	mcount=0;
	while ((mcount<(MAXMALLOCSTOKEEP-1)) && (mcheck[mcount]!=a)) mcount++;
		if (mcheck[mcount]!=a) {
			printf ("freewrlFree - did not find %p at %s:%d\n",a,file,line);
			printf("mcount = %d\n",mcount);
		} else {
			/* printf ("found %d in mcheck table\n"); */
			mcheck[mcount] = NULL;
			mlineno[mcount] = 0;
			if (mplace[mcount]!=NULL) free(mplace[mcount]);
			mplace[mcount]=NULL;
		}
	UNLOCK_GLOBAL_MEMORYTABLE 
}

void RESERVETABLE(void *a, char *file,int line,int size) {
	LOCK_GLOBAL_MEMORYTABLE
    mcount=0;
	while ((mcount<(MAXMALLOCSTOKEEP-1)) && (mcheck[mcount]!=NULL)) mcount++;
		if (mcheck[mcount]!=NULL) {
		 printf ("freewrlMalloc - out of malloc check store\n");
		 printf("mcount=%d\n",mcount);
		 printf("a=%p\n",a);
		} else {
			mcheck[mcount] = a;
			mlineno[mcount] = line;
			mplace[mcount] = strdup(file);
			msize[mcount] = size;
		}
	UNLOCK_GLOBAL_MEMORYTABLE 
}

void freewrlFree(int line, char *file, void *a)
{
	mcheck_init();
    if(_noisy) printf ("freewrlFree %p xfree at %s:%d\n",a,file,line); 
	if(a)
    FREETABLE(a,file,line);
    free(a);
}

void scanMallocTableOnQuit_old()
{
    for (mcount=0; mcount<MAXMALLOCSTOKEEP;mcount++) {
	if (mcheck[mcount]!=NULL) {
	    printf ("unfreed memory %p created at %s:%d \n",mcheck[mcount], mplace[mcount],mlineno[mcount]);
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
int comp_count (const void * elem1, const void * elem2) 
{
    malloc_location *e1, *e2;
	e1 = (malloc_location*)elem1;
	e2 = (malloc_location*)elem2;
    return e1->count < e2->count ?  -1 : e1->count > e2->count ? 1 : 0;
}
int comp_size (const void * elem1, const void * elem2) 
{
    malloc_location *e1, *e2;
	e1 = (malloc_location*)elem1;
	e2 = (malloc_location*)elem2;
    return e1->size < e2->size ?  -1 : e1->size > e2->size ? 1 : 0;
}
int comp_file (const void * elem1, const void * elem2) 
{
    malloc_location *e1, *e2;
	e1 = (malloc_location*)elem1;
	e2 = (malloc_location*)elem2;
    return strcmp(e1->fname,e2->fname);
}
int comp_line (const void * elem1, const void * elem2) 
{
    malloc_location *e1, *e2;
	e1 = (malloc_location*)elem1;
	e2 = (malloc_location*)elem2;
    return e1->line < e2->line ?  -1 : e1->line > e2->line ? 1 : 0;
}
int comp_fileline (const void * elem1, const void * elem2) 
{
	int iret;
    iret = comp_file(elem1,elem2);
	if(iret == 0)
		iret = comp_line(elem1,elem2);
	return iret;
}

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
				char *file, *mfile;
				int line, mline;
				file = mplace[mcount];
				line = mlineno[mcount];
				mfile = mlocs[j].fname;
				mline = mlocs[j].line;
				if(!file){
					printf("line %d",line);
					if(mfile) printf("mfile=%s",mfile);
				}
				if(!mfile){
					printf("line %d",line);
					if(file) printf("file=%s",file);
				}
				if(mline && mfile)
				if(!strcmp(file,mfile) && (line == mline) ){
					mlocs[j].count ++;
					mlocs[j].size += msize[mcount];
					iloc = j;
					break;
				}
			}
			if(iloc == -1){
				mlocs[nlocs].count = 1;
				mlocs[nlocs].fname = mplace[mcount];
				if(!mplace[mcount])
					printf("adding null place\n");
				mlocs[nlocs].line = mlineno[mcount];
				mlocs[nlocs].size = msize[mcount];
				nlocs++;
			}
		}
    }
	//sort by file, count or size
	if(1) qsort(mlocs,nlocs,sizeof(malloc_location),comp_fileline);
	if(0) qsort(mlocs,nlocs,sizeof(malloc_location),comp_line);
	if(0) qsort(mlocs,nlocs,sizeof(malloc_location),comp_file);
	if(0) qsort(mlocs,nlocs,sizeof(malloc_location),comp_count);
	if(0) qsort(mlocs,nlocs,sizeof(malloc_location),comp_size);
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

	mcheck_init();

    rv = malloc(sz);
    if (rv==NULL) {
		sprintf (myline, "MALLOC PROBLEM - out of memory at %s:%d for %d",file,line,sz);
		outOfMemory (myline);
    }
    if(_noisy)printf ("%p malloc %d at %s:%d\n",rv,sz,file,line); 
	if(!file)
		printf("");
    RESERVETABLE(rv,file,line,sz);

    if (zeroData) bzero (rv, sz);
    return rv;
}

void *freewrlRealloc (int line, char *file, void *ptr, size_t size)
{
    void *rv;
    char myline[400];

	mcheck_init();

    if(_noisy) printf ("%p xfree (from realloc) at %s:%d\n",ptr,file,line);
    rv = realloc (ptr,size);
    if (rv==NULL) {
		if (size != 0) {
			sprintf (myline, "REALLOC PROBLEM - out of memory at %s:%d size %d",file,line,size);
			outOfMemory (myline);
		}
    }
    
    /* printf ("%x malloc (from realloc) %d at %s:%d\n",rv,size,file,line); */
	if(!file)
		printf("");
    FREETABLE(ptr,file,line);
    RESERVETABLE(rv,file,line,size);
	
    return rv;
}


void *freewrlStrdup (int line, char *file, char *str)
{
    void *rv;
    char myline[400];
	mcheck_init();
    if(_noisy) printf("freewrlStrdup, at line %d file %s\n",line,file);
    rv = strdup (str);
    if (rv==NULL) {
		sprintf (myline, "STRDUP PROBLEM - out of memory at %s:%d ",file,line);
		outOfMemory (myline);
    }
	if(_noisy) printf ("freewrlStrdup, before reservetable\n");
	if(!file)
		printf("");
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

#endif