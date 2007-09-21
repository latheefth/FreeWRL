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
 struct FieldDecl* ret=MALLOC(sizeof(struct FieldDecl));
 ret->mode=mode;
 ret->type=type;
 ret->name=name;

 return ret;
}
