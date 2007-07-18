/* Class to wrap a java script for CParser */

#ifndef CSCRIPTS_H
#define CSCRIPTS_H

#include "headers.h"
#include "Vector.h"

#include "CFieldDecls.h"
#include "CParseGeneral.h"


/* ************************************************************************** */
/* ************************ Methods used by X3D Parser  ********************* */
/* ************************************************************************** */
uintptr_t nextScriptHandle (void);
void zeroScriptHandles (void);
struct X3D_Script * protoScript_copy (struct X3D_Script *me);


/* ************************************************************************** */
/* ****************************** ScriptFieldDecl *************************** */
/* ************************************************************************** */

/* Struct */
/* ****** */

struct ScriptFieldDecl
{
 /* subclass of FieldDecl */
 struct FieldDecl* fieldDecl;

 /* Stringified */
 const char* name;
 const char* type;
 const char* ISname;

 /* For fields */
 union anyVrml value;
 BOOL valueSet;	/* Has the value been set? */
};

/* Constructor and destructor */
/* ************************** */

struct ScriptFieldDecl* newScriptFieldDecl(indexT, indexT, indexT);
void deleteScriptFieldDecl(struct ScriptFieldDecl*);

/* Other members */
/* ************* */

/* Get "offset" data for routing */
int scriptFieldDecl_getRoutingOffset(struct ScriptFieldDecl*);

/* Set field value */
void scriptFieldDecl_setFieldValue(struct ScriptFieldDecl*, union anyVrml);

/* Do JS-init for given script handle */
void scriptFieldDecl_jsFieldInit(struct ScriptFieldDecl*, uintptr_t);

/* Forwards to inherited methods */
#define scriptFieldDecl_getLength(me) \
 fieldDecl_getLength((me)->fieldDecl)
#define scriptFieldDecl_isField(me, nam, mod) \
 fieldDecl_isField((me)->fieldDecl, nam, mod)

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
void script_addField(struct Script*, struct ScriptFieldDecl*);

/* Get a field by name */
struct ScriptFieldDecl* script_getField(struct Script*, indexT ind, indexT mod);

#endif /* Once-check */
