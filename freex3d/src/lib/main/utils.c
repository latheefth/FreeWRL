/*

  General utility functions.

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



#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "headers.h"


char *BrowserFullPath = NULL;
char *BrowserName = "FreeWRL VRML/X3D Browser";

const char* freewrl_get_browser_program()
{
    char *tmp;

    /*
      1. Check BROWSER environment variable
      2. Use configuration value BROWSER
    */

    tmp = getenv("BROWSER");
    if (!tmp) {
	tmp = BROWSER;
    }
    return tmp;
}

#define Boolean int

/* Return DEFed name from its node, or NULL if not found */
int isNodeDEFedYet(struct X3D_Node *node, Stack *DEFedNodes)
{
	int ind;
	if(DEFedNodes == NULL) return 0;
	for (ind=0; ind < DEFedNodes->n; ind++) {
		/* did we find this index? */
		if (vector_get(struct X3D_Node*, DEFedNodes, ind) == node) {
			return 1;
		}
	}
	return 0;
}

char * dontRecurseList [] = {
	"_sortedChildren",
	NULL,
};
int doRecurse(const char *fieldname){
	int dont, j;
	dont = 0;
	j=0;
	while(dontRecurseList[j] != NULL)
	{
		dont = dont || !strcmp(dontRecurseList[j],fieldname);
		j++;
	}
	return dont == 0 ? 1 : 0;
}
void print_field_value(FILE *fp, int typeIndex, union anyVrml* value)
{
	int i;
	switch(typeIndex)
	{
		case FIELDTYPE_FreeWRLPTR:
		{
			fprintf(fp," %p \n",(void *)value);
			break;
		}
		case FIELDTYPE_SFNode:
		{
			fprintf(fp," %p \n",(void *)value);
			break;
		}
		case FIELDTYPE_MFNode:
		{
			int j;
			struct Multi_Node* mfnode;
			mfnode = (struct Multi_Node*)value;
			fprintf(fp,"{ ");
			for(j=0;j<mfnode->n;j++)
				fprintf(fp," %p, ",mfnode->p[j]);
			break;
		}
		case FIELDTYPE_SFString:
		{
			struct Uni_String** sfstring = (struct Uni_String**)value;
			fprintf (fp," %s ",(*sfstring)->strptr);
			break;
		}
		case FIELDTYPE_MFString:
		{
			struct Multi_String* mfstring = (struct Multi_String*)value;
			fprintf (fp," { ");
			for (i=0; i<mfstring->n; i++) { fprintf (fp,"%s, ",mfstring->p[i]->strptr); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFFloat:
		{
			float *flt = (float*)value;
			fprintf(fp," %4.3f ",*flt);
			break;
		}
		case FIELDTYPE_MFFloat:
		{
			struct Multi_Float *mffloat = (struct Multi_Float*)value;
			fprintf (fp,"{ ");
			for (i=0; i<mffloat->n; i++) { fprintf (fp," %4.3f,",mffloat->p[i]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFTime:
		case FIELDTYPE_SFDouble:
		{
			double *sftime = (double*)value;
			fprintf (fp,"%4.3f",*sftime);
			break;
		}
		case FIELDTYPE_MFTime:
		case FIELDTYPE_MFDouble:
		{
			struct Multi_Double *mfdouble = (struct Multi_Double*)value;
			fprintf (fp,"{");
			for (i=0; i<mfdouble->n; i++) { fprintf (fp," %4.3f,",mfdouble->p[i]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFInt32:
		case FIELDTYPE_SFBool:
		{
			int *sfint32 = (int*)(value);
			fprintf (fp," \t%d\n",*sfint32);
			break;
		}
		case FIELDTYPE_MFInt32:
		case FIELDTYPE_MFBool:
		{
			struct Multi_Int32 *mfint32 = (struct Multi_Int32*)value;
			fprintf (fp,"{");
			for (i=0; i<mfint32->n; i++) { fprintf (fp," %d,",mfint32->p[i]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFVec2f:
		{
			struct SFVec2f * sfvec2f = (struct SFVec2f *)value;
            for (i=0; i<2; i++) { fprintf (fp,"%4.3f  ",sfvec2f->c[i]); }
			break;
		}
		case FIELDTYPE_MFVec2f:
		{
			struct Multi_Vec2f *mfvec2f = (struct Multi_Vec2f*)value;
			fprintf (fp,"{");
			for (i=0; i<mfvec2f->n; i++)
				{ fprintf (fp,"[%4.3f, %4.3f],",mfvec2f->p[i].c[0], mfvec2f->p[i].c[1]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFVec2d:
		{
			struct SFVec2d * sfvec2d = (struct SFVec2d *)value;
			for (i=0; i<2; i++) { fprintf (fp,"%4.3f,  ",sfvec2d->c[i]); }
			break;
		}
		case FIELDTYPE_MFVec2d:
		{
			struct Multi_Vec2d *mfvec2d = (struct Multi_Vec2d*)value;
			fprintf (fp,"{");
			for (i=0; i<mfvec2d->n; i++)
				{ fprintf (fp,"[%4.3f, %4.3f], ",mfvec2d->p[i].c[0], mfvec2d->p[i].c[1]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFVec3f:
		case FIELDTYPE_SFColor:
		{
			struct SFVec3f * sfvec3f = (struct SFVec3f *)value;
			for (i=0; i<3; i++) { fprintf (fp,"%4.3f  ",sfvec3f->c[i]); }
			break;
		}
		case FIELDTYPE_MFVec3f:
		case FIELDTYPE_MFColor:
		{
			struct Multi_Vec3f *mfvec3f = (struct Multi_Vec3f*)value;
			fprintf (fp,"{");
			for (i=0; i<mfvec3f->n; i++)
				{ fprintf (fp,"[%4.3f, %4.3f, %4.3f],",mfvec3f->p[i].c[0], mfvec3f->p[i].c[1],mfvec3f->p[i].c[2]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFVec3d:
		{
			struct SFVec3d * sfvec3d = (struct SFVec3d *)value;
			for (i=0; i<3; i++) { fprintf (fp,"%4.3f  ",sfvec3d->c[i]); }
			break;
		}
		case FIELDTYPE_MFVec3d:
		{
			struct Multi_Vec3d *mfvec3d = (struct Multi_Vec3d*)value;
			fprintf (fp,"{");
			for (i=0; i<mfvec3d->n; i++)
				{ fprintf (fp,"[%4.3f, %4.3f, %4.3f],",mfvec3d->p[i].c[0], mfvec3d->p[i].c[1],mfvec3d->p[i].c[2]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFVec4f:
		case FIELDTYPE_SFColorRGBA:
		case FIELDTYPE_SFRotation:
		{
			struct SFRotation * sfrot = (struct SFRotation *)value;
			for (i=0; i<4; i++) { fprintf (fp,"%4.3f  ",sfrot->c[i]); }
			break;
		}
		case FIELDTYPE_MFVec4f:
		case FIELDTYPE_MFColorRGBA:
		case FIELDTYPE_MFRotation:
		{
			struct Multi_ColorRGBA *mfrgba = (struct Multi_ColorRGBA*)value;
			fprintf (fp,"{");
			for (i=0; i<mfrgba->n; i++)
				{ fprintf (fp,"[%4.3f, %4.3f, %4.3f, %4.3f]\n",mfrgba->p[i].c[0], mfrgba->p[i].c[1],mfrgba->p[i].c[2],mfrgba->p[i].c[3]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFVec4d:
		{
			struct SFVec4d * sfvec4d = (struct SFVec4d *)value;
			for (i=0; i<4; i++) { fprintf (fp,"%4.3f  ",sfvec4d->c[i]); }
			break;
		}
		case FIELDTYPE_MFVec4d:
		{
			struct Multi_Vec4d *mfvec4d = (struct Multi_Vec4d*)value;
			fprintf (fp,"{");
			for (i=0; i<mfvec4d->n; i++)
				{ fprintf (fp,"[%4.3f, %4.3f, %4.3f, %4.3f],",mfvec4d->p[i].c[0], mfvec4d->p[i].c[1],mfvec4d->p[i].c[2],mfvec4d->p[i].c[3]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFMatrix3f:
		{
			struct SFMatrix3f *sfmat3f = (struct SFMatrix3f*)value;
			fprintf (fp," [%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f ]\n",
			sfmat3f->c[0],sfmat3f->c[1],sfmat3f->c[2],
			sfmat3f->c[3],sfmat3f->c[4],sfmat3f->c[5],
			sfmat3f->c[6],sfmat3f->c[7],sfmat3f->c[8]);
			break;
		}
		case FIELDTYPE_MFMatrix3f:
		{
			struct Multi_Matrix3f *mfmat3f = (struct Multi_Matrix3f*)value;
			fprintf (fp,"{");
			for (i=0; i<mfmat3f->n; i++) {
				fprintf (fp,"[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f ],",
				mfmat3f->p[i].c[0],mfmat3f->p[i].c[1],mfmat3f->p[i].c[2],
				mfmat3f->p[i].c[3],mfmat3f->p[i].c[4],mfmat3f->p[i].c[5],
				mfmat3f->p[i].c[6],mfmat3f->p[i].c[7],mfmat3f->p[i].c[8]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFMatrix3d:
		{
			struct SFMatrix3d *sfmat3d = (struct SFMatrix3d*)value;
			fprintf (fp," [%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f ]",
			sfmat3d->c[0],sfmat3d->c[1],sfmat3d->c[2],
			sfmat3d->c[3],sfmat3d->c[4],sfmat3d->c[5],
			sfmat3d->c[6],sfmat3d->c[7],sfmat3d->c[8]);
			break;
		}
		case FIELDTYPE_MFMatrix3d:
		{
			struct Multi_Matrix3d *mfmat3d = (struct Multi_Matrix3d*)value;
			fprintf (fp,"{");
			for (i=0; i<mfmat3d->n; i++) {
				fprintf (fp,"			%d: \t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f ]\n",i,
				mfmat3d->p[i].c[0],mfmat3d->p[i].c[1],mfmat3d->p[i].c[2],
				mfmat3d->p[i].c[3],mfmat3d->p[i].c[4],mfmat3d->p[i].c[5],
				mfmat3d->p[i].c[6],mfmat3d->p[i].c[7],mfmat3d->p[i].c[8]); }
			break;
		}
		case FIELDTYPE_SFMatrix4f:
		{
			struct SFMatrix4f *sfmat4f = (struct SFMatrix4f*)value;
			fprintf (fp," \t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f, %4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f ]\n",
			sfmat4f->c[0],sfmat4f->c[1],sfmat4f->c[2],sfmat4f->c[3],
			sfmat4f->c[4],sfmat4f->c[5],sfmat4f->c[6],sfmat4f->c[7],
			sfmat4f->c[8],sfmat4f->c[9],sfmat4f->c[10],sfmat4f->c[11],
			sfmat4f->c[12],sfmat4f->c[13],sfmat4f->c[14],sfmat4f->c[15]);
			break;
		}
		case FIELDTYPE_MFMatrix4f:
		{
			struct Multi_Matrix4f *mfmat4f = (struct Multi_Matrix4f*)value;
			fprintf (fp,"{");
			for (i=0; i<mfmat4f->n; i++) {
				fprintf (fp,"[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f, %4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f ],",
				mfmat4f->p[i].c[0],mfmat4f->p[i].c[1],mfmat4f->p[i].c[2],mfmat4f->p[i].c[3],
				mfmat4f->p[i].c[4],mfmat4f->p[i].c[5],mfmat4f->p[i].c[6],mfmat4f->p[i].c[7],
				mfmat4f->p[i].c[8],mfmat4f->p[i].c[9],mfmat4f->p[i].c[10],mfmat4f->p[i].c[11],
				mfmat4f->p[i].c[12],mfmat4f->p[i].c[13],mfmat4f->p[i].c[14],mfmat4f->p[i].c[15]); }
			fprintf(fp,"}");
			break;
		}
		case FIELDTYPE_SFMatrix4d:
		{
			struct SFMatrix4d *sfmat4d = (struct SFMatrix4d*)value;
			fprintf (fp," [%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f, %4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f ]",
			sfmat4d->c[0],sfmat4d->c[1],sfmat4d->c[2],sfmat4d->c[3],
			sfmat4d->c[4],sfmat4d->c[5],sfmat4d->c[6],sfmat4d->c[7],
			sfmat4d->c[8],sfmat4d->c[9],sfmat4d->c[10],sfmat4d->c[11],
			sfmat4d->c[12],sfmat4d->c[13],sfmat4d->c[14],sfmat4d->c[15]);
			break;
		}
		case FIELDTYPE_MFMatrix4d:	
		{
			struct Multi_Matrix4d *mfmat4d = (struct Multi_Matrix4d*)value;
			fprintf (fp,"{");
			for (i=0; i<mfmat4d->n; i++) {
				fprintf (fp,"[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f, %4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f ],",
				mfmat4d->p[i].c[0],mfmat4d->p[i].c[1],mfmat4d->p[i].c[2],mfmat4d->p[i].c[3],
				mfmat4d->p[i].c[4],mfmat4d->p[i].c[5],mfmat4d->p[i].c[6],mfmat4d->p[i].c[7],
				mfmat4d->p[i].c[8],mfmat4d->p[i].c[9],mfmat4d->p[i].c[10],mfmat4d->p[i].c[11],
				mfmat4d->p[i].c[12],mfmat4d->p[i].c[13],mfmat4d->p[i].c[14],mfmat4d->p[i].c[15]); }
			fprintf(fp,"}");
			break;
		}

		case FIELDTYPE_SFImage:
		{
			fprintf(fp," %p ",(void *)value); //no SFImage struct defined
			break;
		}
	}
} //return print_field
void dump_scene(FILE *fp, int level, struct X3D_Node* node);
void dump_scene2(FILE *fp, int level, struct X3D_Node* node, int recurse, Stack *DEFedNodes);
// print_field is used by dump_scene2() to pretty-print a single field.
// recurses into dump_scene2 for SFNode and MFNodes to print them in detail.
void print_field(FILE *fp,int level, int typeIndex, const char* fieldName, union anyVrml* value, Stack* DEFedNodes)
{
	int lc, i;
	#define spacer	for (lc=0; lc<level; lc++) fprintf (fp," ");

	switch(typeIndex)
	{
		case FIELDTYPE_FreeWRLPTR:
		{
			fprintf(fp," %p \n",(void *)value);
			break;
		}
		case FIELDTYPE_SFNode:
		{
			int dore;
			struct X3D_Node** sfnode = (struct X3D_Node**)value;
			dore = doRecurse(fieldName);
			fprintf (fp,":\n"); dump_scene2(fp,level+1,*sfnode,dore,DEFedNodes);
			break;
		}
		case FIELDTYPE_MFNode:
		{
			int j, dore;
			struct Multi_Node* mfnode;
			dore = doRecurse(fieldName);
			mfnode = (struct Multi_Node*)value;
			fprintf(fp,":\n");
			for(j=0;j<mfnode->n;j++)
				dump_scene2(fp,level+1,mfnode->p[j],dore,DEFedNodes);
			break;
		}
		case FIELDTYPE_SFString:
		{
			struct Uni_String** sfstring = (struct Uni_String**)value;
			fprintf (fp," \t%s\n",(*sfstring)->strptr);
			break;
		}
		case FIELDTYPE_MFString:
		{
			struct Multi_String* mfstring = (struct Multi_String*)value;
			fprintf (fp," : \n");
			for (i=0; i<mfstring->n; i++) { spacer fprintf (fp,"			%d: \t%s\n",i,mfstring->p[i]->strptr); }
			break;
		}
		case FIELDTYPE_SFFloat:
		{
			float *flt = (float*)value;
			fprintf (fp," \t%4.3f\n",*flt);
			break;
		}
		case FIELDTYPE_MFFloat:
		{
			struct Multi_Float *mffloat = (struct Multi_Float*)value;
			fprintf (fp," :\n");
			for (i=0; i<mffloat->n; i++) { spacer fprintf (fp,"			%d: \t%4.3f\n",i,mffloat->p[i]); }
			break;
		}
		case FIELDTYPE_SFTime:
		case FIELDTYPE_SFDouble:
		{
			double *sftime = (double*)value;
			fprintf (fp," \t%4.3f\n",*sftime);
			break;
		}
		case FIELDTYPE_MFTime:
		case FIELDTYPE_MFDouble:
		{
			struct Multi_Double *mfdouble = (struct Multi_Double*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfdouble->n; i++) { spacer fprintf (fp,"			%d: \t%4.3f\n",i,mfdouble->p[i]); }
			break;
		}
		case FIELDTYPE_SFInt32:
		case FIELDTYPE_SFBool:
		{
			int *sfint32 = (int*)(value);
			fprintf (fp," \t%d\n",*sfint32);
			break;
		}
		case FIELDTYPE_MFInt32:
		case FIELDTYPE_MFBool:
		{
			struct Multi_Int32 *mfint32 = (struct Multi_Int32*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfint32->n; i++) { spacer fprintf (fp,"			%d: \t%d\n",i,mfint32->p[i]); }
			break;
		}
		case FIELDTYPE_SFVec2f:
		{
			struct SFVec2f * sfvec2f = (struct SFVec2f *)value;
			fprintf (fp,": \t");
			for (i=0; i<2; i++) { fprintf (fp,"%4.3f  ",sfvec2f->c[i]); }
			fprintf (fp,"\n");
			break;
		}
		case FIELDTYPE_MFVec2f:
		{
			struct Multi_Vec2f *mfvec2f = (struct Multi_Vec2f*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfvec2f->n; i++)
				{ spacer fprintf (fp,"			%d: \t[%4.3f, %4.3f]\n",i,mfvec2f->p[i].c[0], mfvec2f->p[i].c[1]); }
			break;
		}
		case FIELDTYPE_SFVec2d:
		{
			struct SFVec2d * sfvec2d = (struct SFVec2d *)value;
			fprintf (fp,": \t");
			for (i=0; i<2; i++) { fprintf (fp,"%4.3f  ",sfvec2d->c[i]); }
			fprintf (fp,"\n");
			break;
		}
		case FIELDTYPE_MFVec2d:
		{
			struct Multi_Vec2d *mfvec2d = (struct Multi_Vec2d*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfvec2d->n; i++)
				{ spacer fprintf (fp,"			%d: \t[%4.3f, %4.3f]\n",i,mfvec2d->p[i].c[0], mfvec2d->p[i].c[1]); }
			break;
		}
		case FIELDTYPE_SFVec3f:
		case FIELDTYPE_SFColor:
		{
			struct SFVec3f * sfvec3f = (struct SFVec3f *)value;
			fprintf (fp,": \t");
			for (i=0; i<3; i++) { fprintf (fp,"%4.3f  ",sfvec3f->c[i]); }
			fprintf (fp,"\n");
			break;
		}
		case FIELDTYPE_MFVec3f:
		case FIELDTYPE_MFColor:
		{
			struct Multi_Vec3f *mfvec3f = (struct Multi_Vec3f*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfvec3f->n; i++)
				{ spacer fprintf (fp,"			%d: \t[%4.3f, %4.3f, %4.3f]\n",i,mfvec3f->p[i].c[0], mfvec3f->p[i].c[1],mfvec3f->p[i].c[2]); }
			break;
		}
		case FIELDTYPE_SFVec3d:
		{
			struct SFVec3d * sfvec3d = (struct SFVec3d *)value;
			fprintf (fp,": \t");
			for (i=0; i<3; i++) { fprintf (fp,"%4.3f  ",sfvec3d->c[i]); }
			fprintf (fp,"\n");
			break;
		}
		case FIELDTYPE_MFVec3d:
		{
			struct Multi_Vec3d *mfvec3d = (struct Multi_Vec3d*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfvec3d->n; i++)
				{ spacer fprintf (fp,"			%d: \t[%4.3f, %4.3f, %4.3f]\n",i,mfvec3d->p[i].c[0], mfvec3d->p[i].c[1],mfvec3d->p[i].c[2]); }
			break;
		}
		case FIELDTYPE_SFVec4f:
		case FIELDTYPE_SFColorRGBA:
		case FIELDTYPE_SFRotation:
		{
			struct SFRotation * sfrot = (struct SFRotation *)value;
			fprintf (fp,": \t");
			for (i=0; i<4; i++) { fprintf (fp,"%4.3f  ",sfrot->c[i]); }
			fprintf (fp,"\n");
			break;
		}
		case FIELDTYPE_MFVec4f:
		case FIELDTYPE_MFColorRGBA:
		case FIELDTYPE_MFRotation:
		{
			struct Multi_ColorRGBA *mfrgba = (struct Multi_ColorRGBA*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfrgba->n; i++)
				{ spacer fprintf (fp,"			%d: \t[%4.3f, %4.3f, %4.3f, %4.3f]\n",i,mfrgba->p[i].c[0], mfrgba->p[i].c[1],mfrgba->p[i].c[2],mfrgba->p[i].c[3]); }
			break;
		}
		case FIELDTYPE_SFVec4d:
		{
			struct SFVec4d * sfvec4d = (struct SFVec4d *)value;
			fprintf (fp,": \t");
			for (i=0; i<4; i++) { fprintf (fp,"%4.3f  ",sfvec4d->c[i]); }
			fprintf (fp,"\n");
			break;
		}
		case FIELDTYPE_MFVec4d:
		{
			struct Multi_Vec4d *mfvec4d = (struct Multi_Vec4d*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfvec4d->n; i++)
				{ spacer fprintf (fp,"			%d: \t[%4.3f, %4.3f, %4.3f, %4.3f]\n",i,mfvec4d->p[i].c[0], mfvec4d->p[i].c[1],mfvec4d->p[i].c[2],mfvec4d->p[i].c[3]); }
			break;
		}
		case FIELDTYPE_SFMatrix3f:
		{
			struct SFMatrix3f *sfmat3f = (struct SFMatrix3f*)value;
			spacer fprintf (fp," \t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f ]\n",
			sfmat3f->c[0],sfmat3f->c[1],sfmat3f->c[2],
			sfmat3f->c[3],sfmat3f->c[4],sfmat3f->c[5],
			sfmat3f->c[6],sfmat3f->c[7],sfmat3f->c[8]);
			break;
		}
		case FIELDTYPE_MFMatrix3f:
		{
			struct Multi_Matrix3f *mfmat3f = (struct Multi_Matrix3f*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfmat3f->n; i++) {
				spacer fprintf (fp,"			%d: \t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f ]\n",i,
				mfmat3f->p[i].c[0],mfmat3f->p[i].c[1],mfmat3f->p[i].c[2],
				mfmat3f->p[i].c[3],mfmat3f->p[i].c[4],mfmat3f->p[i].c[5],
				mfmat3f->p[i].c[6],mfmat3f->p[i].c[7],mfmat3f->p[i].c[8]); }
			break;
		}
		case FIELDTYPE_SFMatrix3d:
		{
			struct SFMatrix3d *sfmat3d = (struct SFMatrix3d*)value;
			spacer fprintf (fp," \t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f ]\n",
			sfmat3d->c[0],sfmat3d->c[1],sfmat3d->c[2],
			sfmat3d->c[3],sfmat3d->c[4],sfmat3d->c[5],
			sfmat3d->c[6],sfmat3d->c[7],sfmat3d->c[8]);
			break;
		}
		case FIELDTYPE_MFMatrix3d:
		{
			struct Multi_Matrix3d *mfmat3d = (struct Multi_Matrix3d*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfmat3d->n; i++) {
				spacer fprintf (fp,"			%d: \t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f ]\n",i,
				mfmat3d->p[i].c[0],mfmat3d->p[i].c[1],mfmat3d->p[i].c[2],
				mfmat3d->p[i].c[3],mfmat3d->p[i].c[4],mfmat3d->p[i].c[5],
				mfmat3d->p[i].c[6],mfmat3d->p[i].c[7],mfmat3d->p[i].c[8]); }
			break;
		}
		case FIELDTYPE_SFMatrix4f:
		{
			struct SFMatrix4f *sfmat4f = (struct SFMatrix4f*)value;
			fprintf (fp," \t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f, %4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f ]\n",
			sfmat4f->c[0],sfmat4f->c[1],sfmat4f->c[2],sfmat4f->c[3],
			sfmat4f->c[4],sfmat4f->c[5],sfmat4f->c[6],sfmat4f->c[7],
			sfmat4f->c[8],sfmat4f->c[9],sfmat4f->c[10],sfmat4f->c[11],
			sfmat4f->c[12],sfmat4f->c[13],sfmat4f->c[14],sfmat4f->c[15]);
			break;
		}
		case FIELDTYPE_MFMatrix4f:
		{
			struct Multi_Matrix4f *mfmat4f = (struct Multi_Matrix4f*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfmat4f->n; i++) {
				spacer
				fprintf (fp,"			%d: \t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f, %4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f ]\n",i,
				mfmat4f->p[i].c[0],mfmat4f->p[i].c[1],mfmat4f->p[i].c[2],mfmat4f->p[i].c[3],
				mfmat4f->p[i].c[4],mfmat4f->p[i].c[5],mfmat4f->p[i].c[6],mfmat4f->p[i].c[7],
				mfmat4f->p[i].c[8],mfmat4f->p[i].c[9],mfmat4f->p[i].c[10],mfmat4f->p[i].c[11],
				mfmat4f->p[i].c[12],mfmat4f->p[i].c[13],mfmat4f->p[i].c[14],mfmat4f->p[i].c[15]); }
			break;
		}
		case FIELDTYPE_SFMatrix4d:
		{
			struct SFMatrix4d *sfmat4d = (struct SFMatrix4d*)value;
			fprintf (fp," \t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f, %4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f ]\n",
			sfmat4d->c[0],sfmat4d->c[1],sfmat4d->c[2],sfmat4d->c[3],
			sfmat4d->c[4],sfmat4d->c[5],sfmat4d->c[6],sfmat4d->c[7],
			sfmat4d->c[8],sfmat4d->c[9],sfmat4d->c[10],sfmat4d->c[11],
			sfmat4d->c[12],sfmat4d->c[13],sfmat4d->c[14],sfmat4d->c[15]);
			break;
		}
		case FIELDTYPE_MFMatrix4d:	
		{
			struct Multi_Matrix4d *mfmat4d = (struct Multi_Matrix4d*)value;
			fprintf (fp," :\n");
			for (i=0; i<mfmat4d->n; i++) {
				spacer
				fprintf (fp,"			%d: \t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f, %4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f ]\n",i,
				mfmat4d->p[i].c[0],mfmat4d->p[i].c[1],mfmat4d->p[i].c[2],mfmat4d->p[i].c[3],
				mfmat4d->p[i].c[4],mfmat4d->p[i].c[5],mfmat4d->p[i].c[6],mfmat4d->p[i].c[7],
				mfmat4d->p[i].c[8],mfmat4d->p[i].c[9],mfmat4d->p[i].c[10],mfmat4d->p[i].c[11],
				mfmat4d->p[i].c[12],mfmat4d->p[i].c[13],mfmat4d->p[i].c[14],mfmat4d->p[i].c[15]); }
			break;
		}

		case FIELDTYPE_SFImage:
		{
			fprintf(fp," %p \n",(void *)value); //no SFImage struct defined
			break;
		}
	}
} //return print_field

/*
dump_scene2() is like dump_scene() - a way to printf all the nodes and their fields,
when you hit a key on the keyboard ie '|'
and recurse if a field is an SFNode or MFNode, tabbing in and out to show the recursion level
- except dump_scene2 iterates over fields in a generic way to get all fields
- could be used as an example for deep copying binary nodes
- shows script/user fields and built-in fields
*/
void dump_scene2(FILE *fp, int level, struct X3D_Node* node, int recurse, Stack *DEFedNodes) {
	#define spacer	for (lc=0; lc<level; lc++) fprintf (fp," ");
	int lc;
	int isDefed;
	char *nodeName;
	//(int) FIELDNAMES_children, (int) offsetof (struct X3D_Group, children),  (int) FIELDTYPE_MFNode, (int) KW_inputOutput, (int) (SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33),
	typedef struct field_info{
		int nameIndex;
		int offset;
		int typeIndex;
		int ioType;
		int version;
	} *finfo;
	finfo offsets;
	finfo field;
	int ifield;

	#ifdef FW_DEBUG
		Boolean allFields;
		if (fileno(fp) == fileno(stdout)) { allFields = TRUE; } else { allFields = FALSE; }
	#else
		Boolean allFields = TRUE; //FALSE;
	#endif
	/* See vi +/double_conditional codegen/VRMLC.pm */
	if (node==NULL) return;

	fflush(fp);
	if (level == 0) fprintf (fp,"starting dump_scene2\n");
	nodeName = parser_getNameFromNode(node) ;
	isDefed = isNodeDEFedYet(node,DEFedNodes);
	spacer fprintf (fp,"L%d: node (%p) (",level,node);
	if(nodeName != NULL) {
		if(isDefed)
			fprintf(fp,"USE %s",nodeName);
		else
			fprintf(fp,"DEF %s",nodeName);
	}
	fprintf(fp,") type %s\n",stringNodeType(node->_nodeType));
	//fprintf(fp,"recurse=%d ",recurse);
	if(recurse && !isDefed)
	{
		vector_pushBack(struct X3D_Node*, DEFedNodes, node);
		offsets = (finfo)NODE_OFFSETS[node->_nodeType];
		ifield = 0;
		field = &offsets[ifield];
		while( field->nameIndex > -1) //<< generalized for scripts and builtins?
		{
			int privat;
			privat = FIELDNAMES[field->nameIndex][0] == '_';
			privat = privat && strcmp(FIELDNAMES[field->nameIndex],"__scriptObj");
			privat = privat && strcmp(FIELDNAMES[field->nameIndex],"__protoDef");
			if(allFields || !privat)
			{
				spacer
				fprintf(fp," %s",FIELDNAMES[field->nameIndex]); //[0]]);
				fprintf(fp," (%s)",FIELDTYPES[field->typeIndex]); //field[2]]);
				if(node->_nodeType == NODE_Script && !strcmp(FIELDNAMES[field->nameIndex],"__scriptObj") )
				{
					int k;
					struct Vector *sfields;
					struct ScriptFieldDecl *sfield;
					struct FieldDecl *fdecl;
					struct Shader_Script *sp;
					struct CRjsnameStruct *JSparamnames = getJSparamnames();

					sp = *(struct Shader_Script **)&((char*)node)[field->offset];
					fprintf(fp,"loaded = %d\n",sp->loaded);
					sfields = sp->fields;
					//fprintf(fp,"sp->fields->n = %d\n",sp->fields->n);
					for(k=0;k<sfields->n;k++)
					{
						char *fieldName;
						sfield = vector_get(struct ScriptFieldDecl *,sfields,k);
						//if(sfield->ASCIIvalue) printf("Ascii value=%s\n",sfield->ASCIIvalue);
						fdecl = sfield->fieldDecl;
						fieldName = fieldDecl_getShaderScriptName(fdecl);
						fprintf(fp,"  %s",fieldName);
						//fprintf(fp," (%s)",FIELDTYPES[field->typeIndex]); //field[2]]);
						fprintf(fp," (%s)", stringFieldtypeType(fdecl->fieldType)); //fdecl->fieldType)
						fprintf(fp," %s ",stringPROTOKeywordType(fdecl->PKWmode));

						if(fdecl->PKWmode == PKW_initializeOnly)
							print_field(fp,level,fdecl->fieldType,fieldName,&(sfield->value),DEFedNodes);
						else
							fprintf(fp,"\n");
					}
					level--;
				}
				else if(node->_nodeType == NODE_Proto && !strcmp(FIELDNAMES[field->nameIndex],"__protoDef") )
				{
					int k; //, mode;
					struct ProtoFieldDecl* pfield;
					struct X3D_Proto* pnode = (struct X3D_Proto*)node;
					struct ProtoDefinition* pstruct = (struct ProtoDefinition*) pnode->__protoDef;
					if(pstruct){
						fprintf(fp," user fields:\n");
						level++;
						if(pstruct->iface)
						for(k=0; k!=vectorSize(pstruct->iface); ++k)
						{
							const char *fieldName;
							pfield= vector_get(struct ProtoFieldDecl*, pstruct->iface, k);
							//mode = pfield->mode;
							fieldName = pfield->cname;
							spacer
							fprintf(fp," %p ",(void*)pfield);
							fprintf(fp,"  %s",fieldName);
							fprintf(fp," (%s)", stringFieldtypeType(pfield->type)); //fdecl->fieldType)
							fprintf(fp," %s ",stringPROTOKeywordType(pfield->mode));

							if(pfield->mode == PKW_initializeOnly || pfield->mode == PKW_inputOutput)
								print_field(fp,level,pfield->type,fieldName,&(pfield->defaultVal),DEFedNodes);
							else
								fprintf(fp,"\n");
						}
						level--;
					}
				}else{
					union anyVrml* any_except_PTR = (union anyVrml*)&((char*)node)[field->offset];
					print_field(fp,level,field->typeIndex,FIELDNAMES[field->nameIndex],any_except_PTR,DEFedNodes);
				}
			}
			ifield++;
			field = &offsets[ifield];
		}
	}
	fflush(fp) ;
	spacer fprintf (fp,"L%d end\n",level);
	if (level == 0) fprintf (fp,"ending dump_scene2\n");
}

/* deep_copy2() - experimental keyboard reachable deepcopy function */
void deep_copy2(int iopt, char* defname)
{
	struct X3D_Node* node;
	char *name2;
	node = NULL;
	ConsoleMessage("in deep_copy2 - for copying a node and its fields\n");
	ConsoleMessage("got iopt=%d defname=%s\n",iopt,defname);
	if(iopt == 0) return;
	if(iopt == 1)
	{
		node = parser_getNodeFromName(defname);
	}
	if(iopt == 2)
	{
		node = (struct X3D_Node*)rootNode();
	}
	if(iopt == 3)
	{
		sscanf(defname,"%p",&node);
	}
	if( checkNode(node, NULL, 0) )
	{
		name2 = parser_getNameFromNode(node);
		if(name2 != NULL)
			ConsoleMessage("You entered %s\n",name2);
		else
			ConsoleMessage("Node exists!\n");
	}else{
		ConsoleMessage("Node does not exist.\n");
	}
}

void print_DEFed_node_names_and_pointers(FILE* fp)
{
	int ind,j,jj,nstack,nvector;
	char * name;
	struct X3D_Node * node;
	struct Vector *curNameStackTop;
	struct Vector *curNodeStackTop;
	struct VRMLParser *globalParser = (struct VRMLParser *)gglobal()->CParse.globalParser;

	fprintf(fp,"DEFedNodes ");
	if(!globalParser) return;
	if(globalParser->DEFedNodes == NULL)
	{
		fprintf(fp," NULL\n");
		return;
	}
	nstack = globalParser->lexer->userNodeNames->n;
	fprintf(fp," lexer namespace vectors = %d\n",nstack);
	for(j=0;j<nstack;j++)
	{
		curNameStackTop = vector_get(struct Vector *, globalParser->lexer->userNodeNames,j);
		curNodeStackTop = vector_get(struct Vector *, globalParser->DEFedNodes,j);
		if(curNameStackTop && curNodeStackTop)
		{
			nvector = vectorSize(curNodeStackTop);
			for(jj=0;jj<j;jj++) fprintf(fp,"  ");
			fprintf(fp,"vector %d name count = %d\n",j,nvector);
			for (ind=0; ind < nvector; ind++)
			{
				for(jj=0;jj<j;jj++) fprintf(fp,"  ");
				node = vector_get(struct X3D_Node*,curNodeStackTop, ind);
				name = vector_get(char *,curNameStackTop, ind);
				fprintf (fp,"L%d: node (%p) name (%s) \n",jj,node,name);
			}
		}
	}
}
char *findFIELDNAMESfromNodeOffset0(struct X3D_Node *node, int offset)
{
	if( node->_nodeType != NODE_Script)
	{
		if( node->_nodeType == NODE_Proto )
		{
			//int mode;
			struct ProtoFieldDecl* pfield;
			struct X3D_Proto* pnode = (struct X3D_Proto*)node;
			struct ProtoDefinition* pstruct = (struct ProtoDefinition*) pnode->__protoDef;
			if(pstruct){
				if(pstruct->iface) {
				    if(offset < vectorSize(pstruct->iface))
				    {
					//JAS const char *fieldName;
					pfield= vector_get(struct ProtoFieldDecl*, pstruct->iface, offset);
					//mode = pfield->mode;
					return pfield->cname;
				    } else return NULL;
				}
			}else return NULL;
		}
		else
		  //return (char *)FIELDNAMES[NODE_OFFSETS[node->_nodeType][offset*5]];
		  return (char *)findFIELDNAMESfromNodeOffset(node,offset);
	}
  #ifdef HAVE_JAVASCRIPT
	{
		struct Vector* fields;
		struct ScriptFieldDecl* curField;

		struct Shader_Script *myObj = X3D_SCRIPT(node)->__scriptObj;
		struct CRjsnameStruct *JSparamnames = getJSparamnames();

		fields = myObj->fields;
		curField = vector_get(struct ScriptFieldDecl*, fields, offset);
		return fieldDecl_getShaderScriptName(curField->fieldDecl);
	}
  #else
	return "script";
  #endif

}
char *findFIELDNAMES0(struct X3D_Node *node, int offset)
{
	return findFIELDNAMESfromNodeOffset0(node,offset);
}
#include "../vrml_parser/CRoutes.h"
void print_routes_ready_to_register(FILE* fp);
void print_routes(FILE* fp)
{
	int numRoutes;
	int count;
	struct X3D_Node *fromNode;
	struct X3D_Node *toNode;
	int fromOffset;
	int toOffset;
	char *fromName;
	char *toName;

	print_routes_ready_to_register(fp);
	numRoutes = getRoutesCount();
	fprintf(fp,"Number of Routes %d\n",numRoutes-2);
	if (numRoutes < 2) {
		return;
	}

	/* remember, in the routing table, the first and last entres are invalid, so skip them */
	for (count = 1; count < (numRoutes-1); count++) {
		getSpecificRoute (count,&fromNode, &fromOffset, &toNode, &toOffset);
		fromName = parser_getNameFromNode(fromNode);
		toName   = parser_getNameFromNode(toNode);

		fprintf (fp, " %p %s.%s TO %p %s.%s \n",fromNode,fromName,
			findFIELDNAMESfromNodeOffset0(fromNode,fromOffset),
			toNode,toName,
			findFIELDNAMESfromNodeOffset0(toNode,toOffset)
			);
	}
}
static struct consoleMenuState
{
	int active;
	void (*f)(void*,char*);
	char buf[100];
	int len;
	char *dfault;
	void *yourData;
} ConsoleMenuState;
int consoleMenuActive()
{
	return ConsoleMenuState.active;
}
void setConsoleMenu(void *yourData, char *prompt, void (*callback), char* dfault)
{
	ConsoleMenuState.f = callback;
	ConsoleMenuState.len = 0;
	ConsoleMenuState.buf[0] = '\0';
	ConsoleMenuState.active = TRUE;
	ConsoleMenuState.dfault = dfault;
	ConsoleMenuState.yourData = yourData;
	ConsoleMessage(prompt);
	ConsoleMessage("[%s]:",dfault);
}
void deep_copy_defname(void *myData, char *defname)
{
	int iopt;
	ConsoleMessage("you entered defname: %s\n",defname);
	memcpy(&iopt,myData,4);
	deep_copy2(iopt,defname);
	FREE(myData);
}
void deep_copy_option(void* yourData, char *opt)
{
	int iopt;
	ConsoleMessage("you chose option %s\n",opt);
	sscanf(opt,"%d",&iopt);
	if(iopt == 0) return;
	if(iopt == 1 || iopt == 3)
	{
		void* myData = MALLOC(void *, 4); //could store in gglobal->mainloop or wherever, then don't free in deep_copy_defname
		memcpy(myData,&iopt,4);
		setConsoleMenu(myData,"Enter DEFname or node address:", deep_copy_defname, "");
	}
	if(iopt == 2)
		deep_copy2(iopt, NULL);
}
void dump_scenegraph(int method)
{
//#ifdef FW_DEBUG
	if(method == 1) // '\\'
		dump_scene(stdout, 0, (struct X3D_Node*) rootNode());
	else if(method == 2) // '|'
	{
		Stack * DEFedNodes = newVector(struct X3D_Node*, 2);
		dump_scene2(stdout, 0, (struct X3D_Node*) rootNode(),1,DEFedNodes);
		deleteVector(struct X3D_Node*,DEFedNodes);
	}
	else if(method == 3) // '='
	{
		print_DEFed_node_names_and_pointers(stdout);
	}
	else if(method == 4) // '+'
	{
		print_routes(stdout);
	}
	else if(method == 5) // '-'
	{
		//ConsoleMenuState.active = 1; //deep_copy2();
		setConsoleMenu(NULL,"0. Exit 1.DEFname 2.ROOTNODE 3.node address", deep_copy_option, "0");
	}
//#endif
}







#ifdef DISABLER
// ================ nice new code by disabler. use while tg still alive===============
#ifdef _ANDROID
#define WRAP_MALLOC 1

#ifdef DEBUG
int DROIDDEBUG( const char*pFmtStr, ...);
#define printf DROIDDEBUG
#endif

#endif

/**
 * This code get compiled only when debugging is enabled
 */

#if defined(WRAP_MALLOC) || defined(DEBUG_MALLOC)

#define FWL_NOT_FOUND_IN_MEM_TABLE -1

void __free_memtable_elem(void *ptr);
void __free_memtable_elem_with_data(void *ptr);
int __match_memtable_elem(void *elem1, void *elem2);

typedef struct fwl_memtable_elem
{
    void *ptr;
    int lineNubmer;
    char *fileName;
} fwl_memtable_elem;


static ttglobal sLastSeenGlobal = NULL;

void __freeWholeMemTable(ttglobal gg)
{
    if (gg->__memTable != NULL)
    {
        gg->__memTable->free = &__free_memtable_elem_with_data;
        dbl_list_destroy(gg->__memTable);
        gg->__memTable = NULL;
    }
}

int __removeFromMemTable(ttglobal gg, void *ptr, char *file, int line)
{
    int retVal = FWL_NOT_FOUND_IN_MEM_TABLE;
	if (gg->__memTable != NULL)
    {
        fwl_memtable_elem searchedElem;
        searchedElem.ptr = ptr;
        dbl_list_node_t *node = dbl_list_find(gg->__memTable, &searchedElem);
        if (node)
        {
            retVal = 0;
            dbl_list_remove(gg->__memTable, node);
        }
        else
        {
            printf ("freewrlFree - did not find 0x%016llx at %s:%d\n", (unsigned long long)ptr,file,line);
        }
    }
    
    return retVal;
}

void __reserveInMemTable(ttglobal gg, void *ptr, char *file, int line)
{
	if (gg->__memTable != NULL)
    {
        fwl_memtable_elem searchedElem;
        searchedElem.ptr = ptr;
        dbl_list_node_t *node = dbl_list_find(gg->__memTable, &searchedElem);
        if (node)
        {
            fwl_memtable_elem *foundElem = (fwl_memtable_elem *)node->val;
            
            printf ("freewrl__ReserveInMemTable - ptr already in the table 0x%016llx at %s:%d, added at %s:%d\n", (unsigned long long)ptr, file, line, foundElem->fileName, foundElem->lineNubmer);

        }
        else
        {
            fwl_memtable_elem *newElem = malloc(sizeof(fwl_memtable_elem));
            newElem->fileName = file;
            newElem->lineNubmer = line;
            newElem->ptr = ptr;
            dbl_list_rpush(gg->__memTable, dbl_list_node_new(newElem));
        }
    }
}

#define LOCK_GLOBAL_MEMORYTABLE 		if (tg) pthread_mutex_lock(&tg->__memTableGlobalLock);
#define UNLOCK_GLOBAL_MEMORYTABLE		if (tg) pthread_mutex_unlock(&tg->__memTableGlobalLock);

ttglobal freewrlGetActualGlobal()
{
    ttglobal tg = gglobal0();
    if (tg)
    {
        sLastSeenGlobal = tg;
    }
    else
    {
        tg = sLastSeenGlobal;
    }
    
    return tg;
}

void freewrlInitMemTable()
{
    ttglobal tg = freewrlGetActualGlobal();
    if (tg)
    {
        pthread_mutex_init(&(tg->__memTableGlobalLock), NULL);
    }
    LOCK_GLOBAL_MEMORYTABLE
    
    if (tg && !tg->__memTable_CheckInit) {
        
        tg->__memTable = dbl_list_new();
        tg->__memTable->free = &__free_memtable_elem;
        tg->__memTable->match = &__match_memtable_elem;
        tg->__memTable_CheckInit = TRUE;
    }
    
    UNLOCK_GLOBAL_MEMORYTABLE
}

void __free_memtable_elem_with_data(void *ptr)
{
    fwl_memtable_elem *elem = (fwl_memtable_elem *)ptr;
    #ifdef DEBUG_MALLOC
        printf ("freewrl MemTable disposing ptr 0x%016llx\n", (unsigned long long)elem->ptr);
    #endif
    free(elem->ptr);
    free(elem);
}

void __free_memtable_elem(void *ptr)
{
    free(ptr);
}

int __match_memtable_elem(void *elem1, void *elem2)
{
    return ((fwl_memtable_elem *)elem1)->ptr == ((fwl_memtable_elem *)elem2)->ptr;
}

void freewrlDisposeMemTable()
{
    ttglobal tg = freewrlGetActualGlobal();
    if (tg)
    {
        pthread_mutex_destroy(&(tg->__memTableGlobalLock));
        if (sLastSeenGlobal == tg)
        {
            sLastSeenGlobal = NULL;
        }
    }
}

void freewrlFree(int line, char *file, void *a)
{
    ttglobal tg = freewrlGetActualGlobal();
    LOCK_GLOBAL_MEMORYTABLE
    if (tg)
    {
        #ifdef DEBUG_MALLOC
            printf ("freewrlFree 0x%016llx xfree at %s:%d\n", (unsigned long long)a, file,line);
        #endif
        
        __removeFromMemTable(tg, a,file,line);
    }
    
    UNLOCK_GLOBAL_MEMORYTABLE
    free(a);
}

void scanMallocTableOnQuit()
{
    ttglobal tg = freewrlGetActualGlobal();
    LOCK_GLOBAL_MEMORYTABLE
    if (tg)
    {
        dbl_list_iterator_t *it = dbl_list_iterator_new(tg->__memTable, LIST_HEAD);
        dbl_list_node_t *node;
        while ((node = dbl_list_iterator_next(it))) {
            fwl_memtable_elem *elem = ((fwl_memtable_elem *)node->val);
            printf ("unfreed memory %016llx created at %s:%d \n", (unsigned long long)elem->ptr, elem->fileName, elem->lineNubmer);
        }
        dbl_list_iterator_destroy(it);
    }
    UNLOCK_GLOBAL_MEMORYTABLE
}

void freewrlSetShouldRegisterAllocation(bool shouldRegisterAllocation)
{
    ttglobal tg = freewrlGetActualGlobal();
    LOCK_GLOBAL_MEMORYTABLE
    if (tg)
    {
        tg->__memTable_ShouldRegisterAllocation = shouldRegisterAllocation;
    }
    UNLOCK_GLOBAL_MEMORYTABLE
}

bool freewrlIsRegisteringAllocation()
{
    ttglobal tg = freewrlGetActualGlobal();
    
    if (tg)
    {
        return tg->__memTable_ShouldRegisterAllocation;
    }
    
    return FALSE;
}

void freewrlFreeAllRegisteredAllocations()
{
    ttglobal tg = freewrlGetActualGlobal();
    LOCK_GLOBAL_MEMORYTABLE
    if (tg)
    {
        __freeWholeMemTable(tg);
    }
    UNLOCK_GLOBAL_MEMORYTABLE
}

/**
 * Check all mallocs
 */
void *freewrlMalloc(int line, char *file, size_t sz, int zeroData)
{
    void *rv;
    
    ttglobal tg = freewrlGetActualGlobal();
    LOCK_GLOBAL_MEMORYTABLE
    
    rv = malloc(sz);
    if (zeroData) bzero (rv, sz);
    
    #ifdef DEBUG_MALLOC
        if (rv == NULL)
        {
            char myline[400];
            sprintf (myline, "MALLOC PROBLEM - out of memory at %s:%d for %zu",file,line,sz);
            outOfMemory (myline);
        }

        printf ("freewrlMalloc 0x%016llx size %zu at %s:%d\n", (unsigned long long)rv,sz,file,line);
    #endif

    if (tg && tg->__memTable_ShouldRegisterAllocation)
    {
        __reserveInMemTable(tg, rv,file,line);
    }
    
    UNLOCK_GLOBAL_MEMORYTABLE
    
    return rv;
}

void *freewrlRealloc(int line, char *file, void *ptr, size_t size)
{
    void *rv;
    
    ttglobal tg = freewrlGetActualGlobal();
    LOCK_GLOBAL_MEMORYTABLE

//    printf ("%016llx xfree (from realloc) at %s:%d\n",ptr,file,line);
    rv = realloc (ptr,size);
    
    #ifdef DEBUG_MALLOC
        if (rv == NULL)
        {
            if (size != 0)
            {
                char myline[400];
                sprintf (myline, "REALLOC PROBLEM - out of memory at %s:%d size %zu",file,line,size);
                outOfMemory (myline);
            }
        }
    
        printf ("freewrlRealloc 0x%016llx to 0x%016llx size %zu at %s:%d\n", (unsigned long long)ptr, (unsigned long long)rv, size, file, line);
    #endif
    
    if (tg)
    {
        int result = 0;
        if (NULL != ptr)
        {
            result = __removeFromMemTable(tg, ptr,file,line);
        }
        if (result != FWL_NOT_FOUND_IN_MEM_TABLE) // If we were tracking this ptr previously
        {
            if (tg->__memTable_ShouldRegisterAllocation)
            {
                __reserveInMemTable(tg, rv,file,line);
            }
        }
        else
        {
            printf ("0x%016llx FOR REALLOC NOT FOUND for size %zu at %s:%d\n", (unsigned long long)ptr,size,file,line);
        }
    }
    
	UNLOCK_GLOBAL_MEMORYTABLE
    return rv;
}


void *freewrlStrdup (int line, char *file, const char *str)
{
    void *rv;

    ttglobal tg = freewrlGetActualGlobal();
    LOCK_GLOBAL_MEMORYTABLE
    rv = strdup (str);
    
    #ifdef DEBUG_MALLOC
        printf("freewrlStrdup 0x%016llx, at line %d file %s\n",(unsigned long long)rv, line,file);
        if (rv == NULL)
        {
            char myline[400];
            sprintf (myline, "STRDUP PROBLEM - out of memory at %s:%d ",file,line);
            outOfMemory (myline);
        }
    #endif
    
    if (tg && tg->__memTable_ShouldRegisterAllocation)
    {
        __reserveInMemTable(tg, rv,file,line);
    }
    UNLOCK_GLOBAL_MEMORYTABLE
    return rv;
}

void *freewrlStrndup (int line, char *file, const char *str, size_t n)
{
    void *rv;
    ttglobal tg = freewrlGetActualGlobal();
    LOCK_GLOBAL_MEMORYTABLE

    rv = strndup (str, n);
    
    #ifdef DEBUG_MALLOC
        printf("freewrlStrndup 0x%016llx count at line %d file %s\n", (unsigned long long)rv, line,file);
        if (rv == NULL)
        {
            char myline[400];
            sprintf (myline, "STRNDUP PROBLEM - out of memory at %s:%d ",file,line);
            outOfMemory (myline);
        }
    #endif
    
    if (tg && tg->__memTable_ShouldRegisterAllocation)
    {
        __reserveInMemTable(tg, rv,file,line);
    }
    UNLOCK_GLOBAL_MEMORYTABLE
    return rv;
}

#endif /* defined(WRAP_MALLOC) || defined(DEBUG_MALLOC) */

/**
 * function to debug multi strings
 * we need to find where to put it....
 */
void Multi_String_print(struct Multi_String *url)
{
	if (url) {
		if (!url->p) {
			PRINTF("multi url: <empty>");
		} else {
			int i;

			PRINTF("multi url: ");
			for (i = 0; i < url->n; i++) {
				struct Uni_String *s = url->p[i];
				PRINTF("[%d] %s", i, s->strptr);
			}
		}
		PRINTF("\n");
	}
}
#endif

//================ older code hacked by dug9, can work after tg disposed================
#ifndef DISABLER

/**
 * This code get compiled only when debugging is enabled
 */
pthread_mutex_t __memTableGlobalLock = PTHREAD_MUTEX_INITIALIZER;
#define LOCK_GLOBAL_MEMORYTABLE 		pthread_mutex_lock(&__memTableGlobalLock);
#define UNLOCK_GLOBAL_MEMORYTABLE		pthread_mutex_unlock(&__memTableGlobalLock);

#ifdef DEBUG_MALLOC
static int _noisy = 0; //=1 if more printfs during malloc and free, 0 if just summary on exit

#define MAXMALLOCSTOKEEP 100000
static int mcheckinit = FALSE;
static void* mcheck[MAXMALLOCSTOKEEP];
static char* mplace[MAXMALLOCSTOKEEP];
static int mlineno[MAXMALLOCSTOKEEP];
static size_t msize[MAXMALLOCSTOKEEP];
static int mcount;

static mcheck_init(){
    if (!mcheckinit) {
		for (mcount=0; mcount < MAXMALLOCSTOKEEP; mcount++) {
			mcheck[mcount] = NULL;
			mplace[mcount] = NULL;
			mlineno[mcount] = 0;
		}
		mcheckinit = TRUE;
    }

}

void FREETABLE(void *a,char *file,int line) {
	LOCK_GLOBAL_MEMORYTABLE
	mcount=0;
	while ((mcount<(MAXMALLOCSTOKEEP-1)) && (mcheck[mcount]!=a)) mcount++;
		if (mcheck[mcount]!=a) {
			printf ("freewrlFree - did not find %p at %s:%d\n",a,file,line);
			printf("mcount = %d\n",mcount);
		} else {
			/* printf ("found %d in mcheck table\n"); */
			mcheck[mcount] = NULL;
			mlineno[mcount] = 0;
			if (mplace[mcount]!=NULL) free(mplace[mcount]);
			mplace[mcount]=NULL;
		}
	UNLOCK_GLOBAL_MEMORYTABLE 
}

void RESERVETABLE(void *a, char *file,int line,int size) {
	LOCK_GLOBAL_MEMORYTABLE
    mcount=0;
	while ((mcount<(MAXMALLOCSTOKEEP-1)) && (mcheck[mcount]!=NULL)) mcount++;
		if (mcheck[mcount]!=NULL) {
		 printf ("freewrlMalloc - out of malloc check store\n");
		 printf("mcount=%d\n",mcount);
		 printf("a=%p\n",a);
		} else {
			mcheck[mcount] = a;
			mlineno[mcount] = line;
			mplace[mcount] = strdup(file);
			msize[mcount] = size;
		}
	UNLOCK_GLOBAL_MEMORYTABLE 
}

void freewrlFree(int line, char *file, void *a)
{
	mcheck_init();
    if(_noisy) printf ("freewrlFree %p xfree at %s:%d\n",a,file,line); 
	if(a)
    FREETABLE(a,file,line);
    free(a);
}

void scanMallocTableOnQuit_old()
{
    for (mcount=0; mcount<MAXMALLOCSTOKEEP;mcount++) {
	if (mcheck[mcount]!=NULL) {
	    printf ("unfreed memory %p created at %s:%d \n",mcheck[mcount], mplace[mcount],mlineno[mcount]);
	}
    }
}
typedef struct malloc_location {
	int count;
	int line;
	size_t size;
	char *fname;
} malloc_location;
#include <memory.h>
#ifdef _MSC_VER
#define alloca _alloca
#endif
int comp_count (const void * elem1, const void * elem2) 
{
    malloc_location *e1, *e2;
	e1 = (malloc_location*)elem1;
	e2 = (malloc_location*)elem2;
    return e1->count < e2->count ?  -1 : e1->count > e2->count ? 1 : 0;
}
int comp_size (const void * elem1, const void * elem2) 
{
    malloc_location *e1, *e2;
	e1 = (malloc_location*)elem1;
	e2 = (malloc_location*)elem2;
    return e1->size < e2->size ?  -1 : e1->size > e2->size ? 1 : 0;
}
int comp_file (const void * elem1, const void * elem2) 
{
    malloc_location *e1, *e2;
	e1 = (malloc_location*)elem1;
	e2 = (malloc_location*)elem2;
    return strcmp(e1->fname,e2->fname);
}
int comp_line (const void * elem1, const void * elem2) 
{
    malloc_location *e1, *e2;
	e1 = (malloc_location*)elem1;
	e2 = (malloc_location*)elem2;
    return e1->line < e2->line ?  -1 : e1->line > e2->line ? 1 : 0;
}
int comp_fileline (const void * elem1, const void * elem2) 
{
	int iret;
    iret = comp_file(elem1,elem2);
	if(iret == 0)
		iret = comp_line(elem1,elem2);
	return iret;
}
void scanForVectorTypes(){
    for (mcount=0; mcount<MAXMALLOCSTOKEEP;mcount++) {
		if (mcheck[mcount]!=NULL) {
			if(mlineno[mcount]==5873){ //strstr("GeneratedCode.c",mplace[mcount]) && 
				//pexky _parentVector
				struct Vector * v = (struct Vector*)mcheck[mcount];
				printf("!%d!",v->n);
			}
		}
	}
}
void scanForFieldTypes(){
    for (mcount=0; mcount<MAXMALLOCSTOKEEP;mcount++) {
		if (mcheck[mcount]!=NULL) {
			if(mlineno[mcount]==105){ //strstr("GeneratedCode.c",mplace[mcount]) && 
				//pexky _parentVector
				union anyVrml u;
				struct Uni_String *us;
				//u.mfstring.p = mcheck[mcount];
				us = (struct Uni_String*)mcheck[mcount];
				printf("#%s#",us->strptr);
				//printf("!%p!",u.mfnode.p);
			}
		}
	}
}
void scanForCstringTypes(){
    for (mcount=0; mcount<MAXMALLOCSTOKEEP;mcount++) {
		if (mcheck[mcount]!=NULL) {
			if(mlineno[mcount]==456 || mlineno[mcount]==117){ 
				char *s = (char*)mcheck[mcount];
				printf(">%s<",s);
			}
		}
	}
}
void scanMallocTableOnQuit()
{
	//this version will sum up the lines were the mallocs are occuring that aren't freed
	int nlocs,j,iloc;
	size_t total;
	//scanForVectorTypes();
	scanForFieldTypes();
	//scanForCstringTypes();
	malloc_location *mlocs = malloc(sizeof(malloc_location)*MAXMALLOCSTOKEEP);
	memset(mlocs,0,sizeof(malloc_location)*MAXMALLOCSTOKEEP);
	nlocs = 0;
    for (mcount=0; mcount<MAXMALLOCSTOKEEP;mcount++) {
		if (mcheck[mcount]!=NULL) {
			//printf ("unfreed memory %x created at %s:%d \n",mcheck[mcount], mplace[mcount],mlineno[mcount]);
			iloc = -1;
			for(j=0;j<nlocs;j++){
				char *file, *mfile;
				int line, mline;
				file = mplace[mcount];
				line = mlineno[mcount];
				mfile = mlocs[j].fname;
				mline = mlocs[j].line;
				if(!file){
					printf("line %d",line);
					if(mfile) printf("mfile=%s",mfile);
				}
				if(!mfile){
					printf("line %d",line);
					if(file) printf("file=%s",file);
				}
				if(mline && mfile)
				if(!strcmp(file,mfile) && (line == mline) ){
					mlocs[j].count ++;
					mlocs[j].size += msize[mcount];
					iloc = j;
					break;
				}
			}
			if(iloc == -1){
				mlocs[nlocs].count = 1;
				mlocs[nlocs].fname = mplace[mcount];
				if(!mplace[mcount])
					printf("adding null place\n");
				mlocs[nlocs].line = mlineno[mcount];
				mlocs[nlocs].size = msize[mcount];
				nlocs++;
			}
		}
    }
	//sort by file, count or size
	if(1) qsort(mlocs,nlocs,sizeof(malloc_location),comp_fileline);
	if(0) qsort(mlocs,nlocs,sizeof(malloc_location),comp_line);
	if(0) qsort(mlocs,nlocs,sizeof(malloc_location),comp_file);
	if(0) qsort(mlocs,nlocs,sizeof(malloc_location),comp_count);
	if(0) qsort(mlocs,nlocs,sizeof(malloc_location),comp_size);
	printf("unfreed:\n");
	printf("%5s %8s %4s %55s\n","count","size","line","file");
	total = 0;
	for(j=0;j<nlocs;j++){
		printf("%5d %8d %4d %55s\n",mlocs[j].count,mlocs[j].size, mlocs[j].line,mlocs[j].fname);
		total += mlocs[j].size;
	}
	printf("total bytes not freed %d\n",total);
	free(mlocs);
	getchar();
}


/**
 * Check all mallocs
 */
void *freewrlMalloc(int line, char *file, size_t sz, int zeroData)
{
    void *rv;
    char myline[400];

	mcheck_init();

    rv = malloc(sz);
    if (rv==NULL) {
		sprintf (myline, "MALLOC PROBLEM - out of memory at %s:%d for %d",file,line,sz);
		outOfMemory (myline);
    }
    if(_noisy)printf ("%p malloc %d at %s:%d\n",rv,sz,file,line); 
	if(!file)
		printf("");
    RESERVETABLE(rv,file,line,sz);

    if (zeroData) bzero (rv, sz);
    return rv;
}

void *freewrlRealloc (int line, char *file, void *ptr, size_t size)
{
    void *rv;
    char myline[400];

	mcheck_init();

    if(_noisy) printf ("%p xfree (from realloc) at %s:%d\n",ptr,file,line);
    rv = realloc (ptr,size);
    if (rv==NULL) {
		if (size != 0) {
			sprintf (myline, "REALLOC PROBLEM - out of memory at %s:%d size %d",file,line,size);
			outOfMemory (myline);
		}
    }
    
    /* printf ("%x malloc (from realloc) %d at %s:%d\n",rv,size,file,line); */
	if(!file)
		printf("");
    FREETABLE(ptr,file,line);
    RESERVETABLE(rv,file,line,size);
	
    return rv;
}


void *freewrlStrdup (int line, char *file, char *str)
{
    void *rv;
    char myline[400];
	mcheck_init();
    if(_noisy) printf("freewrlStrdup, at line %d file %s\n",line,file);
    rv = strdup (str);
    if (rv==NULL) {
		sprintf (myline, "STRDUP PROBLEM - out of memory at %s:%d ",file,line);
		outOfMemory (myline);
    }
	if(_noisy) printf ("freewrlStrdup, before reservetable\n");
	if(!file)
		printf("");
    RESERVETABLE(rv,file,line,strlen(str)+1);
    return rv;
}

#endif /* defined(DEBUG_MALLOC) */

/**
 * function to debug multi strings
 * we need to find where to put it....
 */
void Multi_String_print(struct Multi_String *url)
{
	if (url) {
		if (!url->p) {
			PRINTF("multi url: <empty>");
		} else {
			int i;

			PRINTF("multi url: ");
			for (i = 0; i < url->n; i++) {
				struct Uni_String *s = url->p[i];
				PRINTF("[%d] %s", i, s->strptr);
			}
		}
		PRINTF("\n");
	}
}

#endif