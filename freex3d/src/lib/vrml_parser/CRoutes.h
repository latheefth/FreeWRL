/*


VRML-parsing routines in C.

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


#ifndef __FREEWRL_CROUTES_H__
#define __FREEWRL_CROUTES_H__

/* grab the OpenCL defines here - not many will use it,but the CRStruct needs it */
/* we could do the include in all the referencing files, which would be cleaner... */
#ifdef HAVE_OPENCL
#include "../opencl/OpenCL_Utils.h"
#endif

/* C routes */
typedef struct _CRnodeStruct {
        struct X3D_Node *routeToNode;
        int foffset;
} CRnodeStruct;


struct CRStruct {
        struct X3D_Node*  routeFromNode;
        int fnptr;
        int tonode_count;
        CRnodeStruct *tonodes;
        int     isActive;
        int     len;
        void    (*interpptr)(void *); /* pointer to an interpolator to run */
        int     direction_flag; /* if non-zero indicates script in/out,
                                                   proto in/out */
        int     extra;          /* used to pass a parameter (eg, 1 = addChildren..) */
	int 	intTimeStamp;	/* used for ROUTE loop breaking */
#ifdef HAVE_OPENCL
    cl_kernel CL_Interpolator;
#endif //HAVE_OPENCL

};


#define REINITIALIZE_SORTED_NODES_FIELD(aaa,bbb) \
	/* has this changed size? */ \
	{\
		int nc = aaa.n; \
		if (nc != bbb.n) {\
		\
			FREE_IF_NZ(bbb.p); \
			bbb.p = MALLOC(void *, sizeof (struct X3DNode *) * nc); \
		} \
		\
		/* copy the nodes over; we will sort the sorted list */ \
		memcpy(bbb.p, aaa.p, sizeof (struct X3DNode *) * nc); \
		bbb.n = nc; \
	}


struct CRStruct *getCRoutes();

/* function protos */
void getSpecificRoute (int routeNo, struct X3D_Node **fromNode, int *fromOffset, struct X3D_Node **toNode, int *toOffset);

void do_first(void);
void delete_first(struct X3D_Node *);

unsigned long upper_power_of_two(unsigned long v);

void mark_event (struct X3D_Node *from, int fromoffset);
void mark_event_check (struct X3D_Node *from, int fromoffset,char *fn, int line);
void Multimemcpy (struct X3D_Node *toNode, struct X3D_Node *fromNode, void *tn, void *fn, size_t multitype);

void CRoutes_js_new (int num,int scriptType);

void AddRemoveSFNodeFieldChild(
                struct X3D_Node *parent,
                struct X3D_Node **tn,  //target SFNode field
                struct X3D_Node *node,  //node to set,add or remove from parent
                int ar,  //0=set,1=add,2=remove
                char *file,
                int line);

struct CRscriptStruct *getScriptControlIndex(int actualscript);

#endif /* __FREEWRL_CROUTES_H__ */
