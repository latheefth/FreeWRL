/* Class to wrap a java script for CParser */

#ifndef CSCRIPTS_H
#define CSCRIPTS_H

#include "headers.h"

/* Struct */
/* ****** */

struct Script
{
 uintptr_t num;	/* The script handle */
 BOOL loaded;	/* Has the code been loaded into this script? */
};

/* Constructor and destructor */
/* ************************** */

struct Script* newScript();
void deleteScript();

/* Other members */
/* ************* */

/* Initializes the script with its code */
BOOL script_initCode(struct Script*, const char*);
BOOL script_initCodeFromUri(struct Script*, const char*);
BOOL script_initCodeFromMFUri(struct Script*, const struct Multi_String*);

#endif /* Once-check */
