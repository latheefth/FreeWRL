/* VRML-parsing routines in C. */

#ifndef CPARSE_H
#define CPARSE_H

#include "headers.h"

BOOL cParse(void*, unsigned, const char*);

/* Destroy all data associated with the currently parsed world kept. */
void destroyCParserData();

#endif /* Once-check */
