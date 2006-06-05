/* A general purpose vector data structure. */

#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>

/* This is the vector structure. */
struct Vector
{
 size_t	n;
 size_t	allocn;
 void*	data;
};

/* Constructor/destructor */
struct Vector* newVector_(size_t elSize, size_t initSize);
#define newVector(type, initSize) \
 newVector_(sizeof(type), initSize)
void deleteVector_(size_t elSize, struct Vector*);
#define deleteVector(type, me) \
 deleteVector_(sizeof(type), me)

/* Ensures there's at least one space free. */
void vector_ensureSpace_(size_t, struct Vector*);

/* Element retrieval. */
#define vector_get(type, me, ind) \
 ((type*)(me)->data)[ind]

/* Size of vector */
#define vector_size(me) \
 ((me)->n)

/* Shrink the vector to minimum required space. */
void vector_shrink_(size_t, struct Vector*);
#define vector_shrink(type, me) \
 vector_shrink_(sizeof(type), me)

/* Push back operation. */
#define vector_pushBack(type, me, el) \
 { \
  vector_ensureSpace_(sizeof(type), me); \
  assert((me)->n<(me)->allocn); \
  vector_get(type, me, (me)->n)=el; \
  ++(me)->n; \
 }

/* Release and get vector data. */
void* vector_releaseData_(size_t, struct Vector*);
#define vector_releaseData(type, me) \
 vector_releaseData_(sizeof(type), me)

#endif /* Once-check */
