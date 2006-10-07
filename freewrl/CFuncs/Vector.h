/* 
Vector.h
General purpose containers - vector and stack (implemented on top of it)
*/

#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>

/* ************************************************************************** */
/* ******************************** Vector ********************************** */
/* ************************************************************************** */

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
 ((type*)((struct Vector*)me)->data)[ind]

/* Size of vector */
#define vector_size(me) \
 (((struct Vector*)me)->n)

/* Back of a vector */
#define vector_back(type, me) \
 vector_get(type, me, vector_size(me)-1)

/* Is the vector empty? */
#define vector_empty(me) \
 (!vector_size(me))

/* Shrink the vector to minimum required space. */
void vector_shrink_(size_t, struct Vector*);
#define vector_shrink(type, me) \
 vector_shrink_(sizeof(type), me)

/* Push back operation. */
#define vector_pushBack(type, me, el) \
 { \
  vector_ensureSpace_(sizeof(type), me); \
  assert(((struct Vector*)me)->n<((struct Vector*)me)->allocn); \
  vector_get(type, me, ((struct Vector*)me)->n)=el; \
  ++((struct Vector*)me)->n; \
 }

/* Pop back operation */
#define vector_popBack(type, me) \
 { \
  assert(!vector_empty(me)); \
  --((struct Vector*)me)->n; \
 }
#define vector_popBackN(type, me, popn) \
 { \
  assert(popn<=vector_size(me)); \
  ((struct Vector*)me)->n-=popn; \
 }

/* Release and get vector data. */
void* vector_releaseData_(size_t, struct Vector*);
#define vector_releaseData(type, me) \
 vector_releaseData_(sizeof(type), me)

/* ************************************************************************** */
/* ************************************ Stack ******************************* */
/* ************************************************************************** */

/* A stack is essentially a vector */
typedef struct Vector Stack;

/* Constructor and destructor */
#define newStack(type) \
 newVector(sizeof(type), 4)
#define deleteStack(type, me) \
 deleteVector(sizeof(type), me)

/* Push and pop */
#define stack_push(type, me, el) \
 vector_pushBack(type, me, el)
#define stack_pop(type, me) \
 vector_popBack(type, me)

/* Top of stack */
#define stack_top(type, me) \
 vector_get(type, me, vector_size(me)-1)

/* Is the stack empty? */
#define stack_empty(me) \
 vector_empty(me)

#endif /* Once-check */
