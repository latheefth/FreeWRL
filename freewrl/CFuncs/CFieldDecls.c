/* CFieldDecls.c - Sourcecode for CFieldDecls.h */

#include <stdlib.h>
#include <assert.h>

#include "CFieldDecls.h"

/* ************************************************************************** */
/* ********************************** FieldDecl ***************************** */
/* ************************************************************************** */

/* Constructor and destructor */
/* ************************** */

struct FieldDecl* newFieldDecl(indexT mode, indexT type, indexT name)
{
 struct FieldDecl* ret=malloc(sizeof(struct FieldDecl));
 ret->mode=mode;
 ret->type=type;
 ret->name=name;

 return ret;
}

/* Other members */
/* ************* */

size_t fieldDecl_getLength(struct FieldDecl* me)
{
 #define SF_TYPE(ttype, type, stype) \
  case FIELDTYPE_##ttype: \
   return sizeof(vrml##stype##T);
 #define MF_TYPE(ttype, type, stype) \
  case FIELDTYPE_##ttype: \
   return 0;

 switch(me->type)
 {
  #include "VrmlTypeList.h"
#ifndef NDEBUG
  default:
   assert(FALSE);
#endif
 }

 #undef SF_TYPE
 #undef MF_TYPE

 assert(FALSE);
 return 0;
}
