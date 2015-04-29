/*


???

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
#include "../main/headers.h"

#include "Vector.h"

static int _noisy = 0;
/* ************************************************************************** */
/* ******************************** Vector ********************************** */
/* ************************************************************************** */

/* Constructor/destructor */

struct Vector* newVector_(int elSize, int initSize,char *fi, int line) {
 	struct Vector* ret;
#ifdef DEBUG_MALLOC
	//inherit __line__ and __file__ from particular spot in code that does newVector
	ret=(struct Vector *)freewrlMalloc(line,fi,sizeof(struct Vector), FALSE);
#else
	ret=MALLOC(struct Vector *, sizeof(struct Vector));
#endif
 	ASSERT(ret);
 	ret->n=0;
 	ret->allocn=initSize;
#ifdef DEBUG_MALLOC
	//inherit __line__ and __file__ from particular spot in code that does newVector
	ret->data=(void *)freewrlMalloc(line, fi,elSize*ret->allocn, FALSE);
#else
 	ret->data=MALLOC(void *, elSize*ret->allocn);
#endif
 	ASSERT(ret->data);
	#ifdef DEBUG_MALLOC2
		ConsoleMessage ("vector, new  %x, data %x, size %d at %s:%d",ret, ret->data, initSize,fi,line);
	#endif
	
	return ret;
}

#if defined(WRAP_MALLOC) || defined(DEBUG_MALLOC)
void deleteVector_(char *file, int line, int elSize, struct Vector** myp) {
#else
void deleteVector_(int elSize, struct Vector** myp) {
#endif

	struct Vector *me = *myp;

	if (!me) {
		//ConsoleMessage ("Vector - already empty");
		return;
	}

	ASSERT(me);
	#ifdef DEBUG_MALLOC
		if(_noisy) printf("vector, deleting me %p data %p at %s:%d\n",me,me->data,file,line);
	#endif
	if(me->data) {FREE_IF_NZ(me->data);}
	FREE_IF_NZ(me);
	*myp = NULL;
}

/* Ensures there's at least one space free. */
void vector_ensureSpace_(int elSize, struct Vector* me, char *fi, int line) {
	ASSERT(me);
    if (me->n>me->allocn)
    {
        ASSERT(FALSE);
    }
	if(me->n==me->allocn) {
		if(me->allocn)
        {
            me->allocn*=2;
        }
		else
        {
            me->allocn=1;
            me->n = 0;
        }

#ifdef DEBUG_MALLOC
		me->data=freewrlRealloc(line, fi,me->data, elSize*me->allocn);
#else
		me->data=REALLOC(me->data, elSize*me->allocn);
#endif
		#ifdef DEBUG_MALLOC
			if(_noisy) printf ("vector, ensureSpace, me %p, data %p\n",me, me->data);
		#endif
		ASSERT(me->data);
	}
	ASSERT(me->n<me->allocn);
}

void vector_popBack_(struct Vector* me, size_t count)
{
    ASSERT(!vector_empty(me));
    me->n -= count;

    #ifdef DEBUG_MALLOC
        if(_noisy) printf ("vector, popping back, me 0x%016llx, data 0x%016llx n %zu\n", (unsigned long long)me, (unsigned long long)me->data, me->n);
    #endif
}

/* Shrinks the vector to allocn==n. */
void vector_shrink_(int elSize, struct Vector* me) {
	ASSERT(me);
	ASSERT(me->allocn>=me->n);
	if(me->n==me->allocn) return;

	me->allocn=me->n;
    void *oldData = me->data;
	me->data=REALLOC(oldData, elSize*me->allocn);
    
    #ifdef DEBUG_MALLOC
        if(_noisy) printf ("vector, shrink, me 0x%016llx, data 0x%016llx\n size %zu allocatedSize %zu", (unsigned long long)me, (unsigned long long)me->data, me->n, me->allocn);
    #endif
    
    //if (!me->data)
    //{
    //    FREE_IF_NZ(oldData); //bombs in win32 due to REALLOC above doing the equivalent of freeing the memory, so oldData is pointing to invalid memory
    //}
	ASSERT(!me->allocn || me->data);
}

void* vector_releaseData_(int elSize, struct Vector* me) {
	void* ret;

	vector_shrink_(elSize, me);
	ret=me->data;
	#ifdef DEBUG_MALLOC
		if(_noisy) printf ("vector, me %p data %p\n",me, me->data);
	#endif
	
    me->data=NULL;
    me->n = 0;
    me->allocn = 0;
	return ret;
}

void vector_removeElement(int elSize,struct Vector* myp, int element)
{
	struct Vector *me = myp;
	if(me){
		if(me->data && me->n > 0 && element < me->n && element > -1) {
			char *el0,*el1;
			int i;
			for(i=element;i<me->n;i++){
				el0 = (char *)(me->data) + i*elSize;
				el1 = el0 + elSize;
				memcpy(el0,el1,elSize);
			}
			me->n--;
            
            #ifdef DEBUG_MALLOC
                if(_noisy) printf ("vector, removing element me 0x%016llx data 0x%016llx\n", (unsigned long long)me, (unsigned long long)me->data);
            #endif
			
            //me->n--; two of these
		}
	}
}