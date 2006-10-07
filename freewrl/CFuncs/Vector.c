/* Sourcecode for Vector.h */

#include <stdlib.h>
#include <assert.h>

#include "Vector.h"

/* ************************************************************************** */
/* ******************************** Vector ********************************** */
/* ************************************************************************** */

/* Constructor/destructor */

struct Vector* newVector_(size_t elSize, size_t initSize)
{
 struct Vector* ret=malloc(sizeof(struct Vector));
 assert(ret);
 ret->n=0;
 ret->allocn=initSize;
 ret->data=malloc(elSize*ret->allocn);
 assert(ret->data);
 return ret;
}

void deleteVector_(size_t elSize, struct Vector* me)
{
 assert(me);
 if(me->data)
  free(me->data);
 free(me);
}

/* Ensures there's at least one space free. */
void vector_ensureSpace_(size_t elSize, struct Vector* me)
{
 assert(me);
 if(me->n==me->allocn)
 {
  if(me->allocn)
   me->allocn*=2;
  else
   me->allocn=1;
  me->data=realloc(me->data, elSize*me->allocn);
  assert(me->data);
 }
 assert(me->n<me->allocn);
}

/* Shrinks the vector to allocn==n. */
void vector_shrink_(size_t elSize, struct Vector* me)
{
 assert(me);
 assert(me->allocn>=me->n);
 if(me->n==me->allocn) return;

 me->allocn=me->n;
 me->data=realloc(me->data, elSize*me->allocn);
 assert(!me->allocn || me->data);
}

void* vector_releaseData_(size_t elSize, struct Vector* me)
{
 void* ret;

 vector_shrink_(elSize, me);
 ret=me->data;
 me->data=NULL;

 return ret;
}
