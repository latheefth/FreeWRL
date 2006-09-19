/* Class to wrap a java script for CParser */

#ifndef CSCRIPTS_H
#define CSCRIPTS_H

#include "headers.h"
#include "Vector.h"

/* ************************************************************************** */
/* ****************************** ScriptFieldDecl *************************** */
/* ************************************************************************** */

/* Struct */
/* ****** */

struct ScriptFieldDecl
{
 const char* kind;
 const char* type;
 indexT name;
 /* For fields */
 char* value;
};

/* Constructor and destructor */
/* ************************** */

struct ScriptFieldDecl* newScriptFieldDecl(indexT, indexT, indexT);
void deleteScriptFieldDecl(struct ScriptFieldDecl*);

/* Other members */
/* ************* */

/* Get "offset" data for routing */
int script_getRoutingOffset(struct ScriptFieldDecl*);

/* ************************************************************************** */
/* ********************************** Script ******************************** */
/* ************************************************************************** */

/* Struct */
/* ****** */

struct Script
{
 uintptr_t num;	/* The script handle */
 BOOL loaded;	/* Has the code been loaded into this script? */
 struct Vector* fields;
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

/* Add a new field */
void scrip_addField(struct Script*, struct ScriptFieldDecl*);

/* Get a field by name */
struct ScriptFieldDecl* script_getField(struct Script*, indexT ind, indexT mod);

#endif /* Once-check */
