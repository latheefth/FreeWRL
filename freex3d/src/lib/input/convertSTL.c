/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2014 CRC Canada. (http://www.crc.gc.ca)

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
#include <stdio.h>

#include <stdint.h>
#include <float.h>

#include <libFreeWRL.h>

#if defined (INCLUDE_STL_FILES)
 
// the STL_FLOAT_TOLERANCE is chosen to be small enough
// to not trip Polyrep degenerate finding, but big enough
// to actually catch some surfaces.

//#define STL_FLOAT_TOLERANCE_TIGHT 0.000001
#define STL_FLOAT_TOLERANCE_USUAL 0.00001

//#define STL_FLOAT_TOLERANCE_LOOSE 0.001
static double stl_vertex_tolerance = STL_FLOAT_TOLERANCE_USUAL;




#define DO_QUICKSORT

/* JAS STL */
#define STL_ASCII_HEADER "solid "
#define STL_BINARY_HEADER_LEN 84 // header + uint32
#define STL_BINARY_VERTEX_SIZE 50 // 12 * 4 + uint16

#define tmpFile "/vrmlSTLFile.wrl"


struct tbinarySTLvertexIn {
    struct SFVec3f normal;
    struct SFVec3f V1;
    struct SFVec3f V2;
    struct SFVec3f V3;
    unsigned char attrib1;
    unsigned char attrib2;
}binarySTLvertexIn;

struct tstlVertexStruct {
	struct SFVec3f vertex;	// original x,y,z
	int	replacementVertex;	// if this is the same as another vertex
    int condensedVertexNo;  // when finding duplicate vertices, we need a new index for coordIndex
	double	dist;			// distance from origin - used in sorting verticies
}stlVertexStruct;

// stats for us to display, should we wish.
static int degenerateCount=0;
static int triangleInCount=0;
static int finalCoordsWritten=0;
 
static float scaleFactor = 1.0;

// do we generate our own normals, or just use what's given?
// if true, we just use the normals as supplied by the author
static int analyzeSTL = true;

// do we check and display edge errors and 2-manifold errors?
static int checkSTL_for_3Dprinting = false;

static unsigned char *vectorArray =NULL;


/* if looking for bad things in STL rendering for 3D printing */
/* shove 4 vertexes into 1 byte, as we only care about 0, 1, 2, 3 for
   vector uses */
static void recordVector(int size, int a, int b) {
	int ind;
	int box,bits;
    	if (a>b) { int x; x=a; a=b; b=x;}
	//ConsoleMessage ("a %d b %d size %d ind %d\n", a,b,size,a*size+b);
	ind = a*size+b;
	box = ind/4, bits = ind %4;

	//ConsoleMessage ("ori: ind %d box %d bits %d content %x\n",ind, box, bits,vectorArray[box]);
	switch (bits) {
		case 0: { int z = vectorArray[box] & 0x03;
			if (z !=0x03) vectorArray[box]+= 0x01;
			 break; }
		case 1: {
			int z = vectorArray[box] & 0x0C;
			if (z !=0x0C) vectorArray[box]+=0x04;
			 break; }
		case 2: {
			int z= vectorArray[box] & 0x30;
			if (z !=0x30) vectorArray[box]+= 0x10;
			break; }
		case 3:{
			int z= vectorArray[box] & 0xC0;
			if (z !=0xC0) vectorArray[box]+= 0x40;
			break; }
		default: {}// should never get here
	}
	//ConsoleMessage ("now: ind %d box %d bits %d content %x\n\n",ind, box, bits,vectorArray[box]);
}

static char returnIndex (int size, int a, int b) {
	int ind;
	int box,bits;
    	if (a>b) { int x; x=a; a=b; b=x;}
	ind = a*size+b;
	box = ind/4, bits = ind %4;

	switch (bits) {
		case 0: return vectorArray[box] & 0x03; break;
		case 1: return (vectorArray[box] & 0x0C) >> 2; break;
		case 2: return (vectorArray[box] & 0x30) >> 4; break;
		case 3: return (vectorArray[box] & 0xC0) >> 6; break;
		default: {}// should never get here
	}
	return 0;
}


/* if we are running fast, we can throw away vertices, if checking for
   printing, do exact comparisons */
int SAPPROX(double a, double b) {

/*
	if (checkSTL_for_3Dprinting) {
		return (a==b);
	}
*/

	return (fabs(a-b) < stl_vertex_tolerance);
}


int stlDTFT(const unsigned char* buffer, int len)
{
    int32_t *stllen;
    
    unsigned char *tmp = (unsigned char *)buffer;
    while ((tmp != NULL) && (*tmp<=' ')) tmp++;

    // lets see if this is a binary stl file.
    ConsoleMessage ("stldtft, file len %x\n",len);
    if (len > 85) {
    	stllen = offsetPointer_deref(int32_t *, buffer, 80);
    	ConsoleMessage ("triangle count %x\n",*stllen);
    
    	ConsoleMessage ("sizeof struct binarySTLvertexIn %d\n",sizeof(binarySTLvertexIn));
    	ConsoleMessage ("binarySTLvertexIn * triangles + 80 + 1 + 4 = %x\n" , 50 * (*stllen) + 80 + 4);
    	ConsoleMessage ("and, passed in length is %x\n",len);
	ConsoleMessage ("last two bytes %x %x\n",buffer[len-2],buffer[len-1]);
        
        
    	if (len==((*stllen)*50+STL_BINARY_HEADER_LEN)) return IS_TYPE_BINARY_STL;

	// have a file with two nulls on end, and len is one byte more, so
	// try just doing this:
    	if (len==((*stllen)*50+STL_BINARY_HEADER_LEN+1)) return IS_TYPE_BINARY_STL;
    } 
    
    // no, maybe it is an ASCII stl file?
    if (strncmp((const char*)tmp,STL_ASCII_HEADER,strlen(STL_ASCII_HEADER)) == 0) {
        ConsoleMessage ("is ASCII stl");
        return IS_TYPE_ASCII_STL;
    }
    
    return IS_TYPE_UNKNOWN;
}

static void calcExtent_dist(float *_extent, struct tstlVertexStruct *me) {
    if (me->vertex.c[0] > EXTENT_MAX_X) EXTENT_MAX_X = me->vertex.c[0];
    if (me->vertex.c[0] < EXTENT_MIN_X) EXTENT_MIN_X = me->vertex.c[0];
    if (me->vertex.c[1] > EXTENT_MAX_Y) EXTENT_MAX_Y = me->vertex.c[1];
    if (me->vertex.c[1] < EXTENT_MIN_Y) EXTENT_MIN_Y = me->vertex.c[1];
    if (me->vertex.c[2] > EXTENT_MAX_Z) EXTENT_MAX_Z = me->vertex.c[2];
    if (me->vertex.c[2] < EXTENT_MIN_Z) EXTENT_MIN_Z = me->vertex.c[2];
        
    me->dist = me->vertex.c[0]*me->vertex.c[0] +
    me->vertex.c[1]*me->vertex.c[1] +
    me->vertex.c[2]*me->vertex.c[2];
}


#ifdef DO_QUICKSORT
/* quickSort vertices by distance */
static void quickSort(struct Vector* vertices,int *newVertexIndex, int left, int right) {
//void quickSort(int arr[], int left, int right) {
    int i = left, j = right;
    int tmp;
    struct tstlVertexStruct *a, *pivot;
    
    pivot= vector_get(struct tstlVertexStruct*, vertices, newVertexIndex[(left+right)/2]);
    /* partition */
    while (i <= j) {
        a = vector_get(struct tstlVertexStruct*, vertices, newVertexIndex[i]);
        while (a->dist < pivot->dist) {
            i++;
            a = vector_get(struct tstlVertexStruct*, vertices, newVertexIndex[i]);
        }
        a = vector_get(struct tstlVertexStruct*, vertices, newVertexIndex[j]);
        while (a->dist > pivot->dist) {
            j--;
            a = vector_get(struct tstlVertexStruct*, vertices, newVertexIndex[j]);
        }
        if (i <= j) {
            tmp = newVertexIndex[i];
            newVertexIndex[i] = newVertexIndex[j];
            newVertexIndex[j] = tmp;
            i++;
            j--;
        }
    };
    
    /* recursion */
    if (left < j)
        quickSort(vertices,newVertexIndex, left, j);
    if (i < right)
        quickSort(vertices,newVertexIndex, i, right);
}

#else //DO_QUICKSORT

/* bubble sort vertices by distance */
static void bubbleSort(struct Vector* vertices,int *newVertexIndex) {
    int i,j,k;
    int totIn = vectorSize(vertices);
    bool noswitch;
    
    
    // go through, and use this newVertexIndex as a "sorted" index to vertices.
    // we sort based on squared distance from (0,0,0), assuming STL files are
    // positive based; we'll do a final verification for vertex matching later.
    for(i=0; i<totIn; i++) {
        noswitch = TRUE;
        for (j=(totIn-1); j>i; j--) {
            struct tstlVertexStruct *a, *b;
            
            /* printf ("comparing %d %d\n",i,j); */
            a = vector_get(struct tstlVertexStruct*, vertices, newVertexIndex[j-1]);
            b = vector_get(struct tstlVertexStruct*, vertices, newVertexIndex[j]);
            
            /* check to see if a child is NULL - if so, skip it */
            if (a && b) {
                if (a->dist > b->dist) {
                    k = newVertexIndex[j-1];
                    newVertexIndex[j-1] = newVertexIndex[j];
                    newVertexIndex[j] = k;
                    noswitch = FALSE;
                    
                }
            }
        }
        /* did we have a clean run? */
        if (noswitch) {
            break;
        }
    }
}
#endif //DO_QUICKSORT




/* go through, and make this compact and good for displaying */
void analyzeSTLdata(struct Vector* vertices) {
    int i,j;
    int totIn = vectorSize(vertices);

    int replacedVertexCount=0;
    int newVertexNumber=0; // for new coordIndex to point to
    
    int* newVertexIndex = MALLOC(int*, sizeof (int)*totIn+1);
/*
 for (i=0; i<totIn; i++) {
        struct tstlVertexStruct *thisVertex = vector_get(struct tstlVertexStruct *,vertices,i);
        ConsoleMessage ("vertex %d, %f %f %f, dist %lf",i,thisVertex->vertex.c[0],thisVertex->vertex.c[1],thisVertex->vertex.c[2]
                        ,thisVertex->dist);
    }
*/
      
#include <sys/time.h>
        struct timeval tv;
        struct timeval tve;
    
    gettimeofday(&tv, NULL);

    // initialize the newVertexIndex to do a 1:1 match with original index
    for (i=0; i<totIn; i++) newVertexIndex[i] = i;

#ifdef DO_QUICKSORT
    quickSort(vertices,newVertexIndex,0,totIn-1);
#else
    bubbleSort(vertices,newVertexIndex);
#endif
    
       gettimeofday(&tve,NULL);
        //ConsoleMessage("step 1, time %ld usec %d\n",tve.tv_sec-tv.tv_sec, tve.tv_usec-tv.tv_usec);
        gettimeofday(&tv, NULL);
        
    // ok, vertices sorted by distance. How many duplicates might we have?
    for (i=0; i<totIn-1; i++) {
        struct tstlVertexStruct *startVertex = vector_get(struct tstlVertexStruct *,vertices,newVertexIndex[i]);
        
        // if this vertex has not been replaced yet, see if any other vertices are
        // close enough to have THIS one replace THEM
        if (startVertex->replacementVertex == -1) {
            j=i+1;
            struct tstlVertexStruct *runVertex = vector_get(struct tstlVertexStruct *,
                                                        vertices,newVertexIndex[j]);

            while (SAPPROX(startVertex->dist,runVertex->dist) && (j<totIn)) {
                // do the vertexes actually match?
                if ((SAPPROX(startVertex->vertex.c[0],runVertex->vertex.c[0]) && 
                     SAPPROX(startVertex->vertex.c[1],runVertex->vertex.c[1]) &&
                     SAPPROX(startVertex->vertex.c[2],runVertex->vertex.c[2]))) {
                    // distance AND vertex xyz are the same
                    //JAS - runVertex->replacementVertex = i;
                    runVertex->replacementVertex = newVertexIndex[i];
                    
                    replacedVertexCount++;
                }
                j++;
                if (j<totIn)
                    runVertex=vector_get(struct tstlVertexStruct *, vertices, newVertexIndex[j]);
            }
        }
    }
    
    

    for (i=0; i<totIn; i++) {
        //JAS struct tstlVertexStruct *thisVertex = vector_get(struct tstlVertexStruct *,vertices,newVertexIndex[i]);
        struct tstlVertexStruct *thisVertex = vector_get(struct tstlVertexStruct *,vertices,i);
        if (thisVertex->replacementVertex== -1) {
            thisVertex->condensedVertexNo = newVertexNumber;
            newVertexNumber++;
        }
        
        /*
        ConsoleMessage ("vertex %d, rep %d, nv %d, %f %f %f, dist %lf",i,
                        thisVertex->replacementVertex, 
                        thisVertex->condensedVertexNo,
                        thisVertex->vertex.c[0],thisVertex->vertex.c[1],thisVertex->vertex.c[2]
                        ,thisVertex->dist);
         */
    }
    //ConsoleMessage ("ConvertToSTL: totVertexCount %d, replacedVertexCount %d",totIn,replacedVertexCount);
    
    gettimeofday(&tve,NULL);
    //ConsoleMessage ("step 2, time %ld usec %d\n",tve.tv_sec-tv.tv_sec, tve.tv_usec-tv.tv_usec);

    FREE_IF_NZ(newVertexIndex);


}


char *finishThisX3DFile (FILE *fp, int cp, char *tfn, float* _extent, int vertexCount,int coordCount) {
    char *retval = NULL;
    int fread_val = 0;
    scaleFactor = -1000.0f;
    float extentX = EXTENT_MAX_X - EXTENT_MIN_X;
    float extentY = EXTENT_MAX_Y - EXTENT_MIN_Y;
    float extentZ = EXTENT_MAX_Z - EXTENT_MIN_Z;

    
    //ConsoleMessage ("extentX %f extentY %f extentZ %f",extentX,extentY,extentZ);
    
    // move this shape to 0,0,0
    if (vertexCount == 0) {
        int i;
        for (i=0; i<6; i++) _extent[i]=0.0f;
        ConsoleMessage ("No vertices found in STL file");
    } else { 
        float midX, midY, midZ;
        
        //ConsoleMessage ("Extent, %f %f %f\n",extentX, extentY, extentZ);
        if (extentX > scaleFactor) scaleFactor = extentX;
        if (extentY > scaleFactor) scaleFactor = extentY;
        if (extentZ > scaleFactor) scaleFactor = extentZ;
        
        //ConsoleMessage ("scaling is %f",10.0f/scaleFactor);
        
        midX = -EXTENT_MIN_X - (extentX/2.0f);
        midY = -EXTENT_MIN_Y - (extentY/2.0f);
        midZ = -EXTENT_MIN_Z - (extentZ/2.0f);
                
        
        // make the shape fit within a 10x10x10 X3D box.
        cp += fprintf (fp,"}} \n");
  
        if (checkSTL_for_3Dprinting && (coordCount>0) && (vectorArray)) {
            int x;
	    int edgesFound = 0; int manifoldErrorsFound = 0;
	    bool issuesFound = false;

	    /* quick check - any issues? */
            for (x=0; x<(coordCount * coordCount/4);x++) {
		if ((vectorArray[x]& 0x55) != 0x00) {
			issuesFound = TRUE;
			break;
		}
	    }

	    //if (issuesFound) ConsoleMessage ("have issues = go through and get info ");

	    if (issuesFound) {
		int a,b;
	    
            	cp += fprintf (fp,"Shape{appearance Appearance{\n");
            	cp += fprintf (fp,"lineProperties LineProperties {linewidthScaleFactor 4.0}\n");
            	cp += fprintf (fp, "material Material{emissiveColor 1 0 0}}geometry IndexedLineSet {\n");
            	cp += fprintf (fp,"   coord USE STL_COORDS\n");
            	cp += fprintf (fp,"  coordIndex [\n");

	    	/* look for abnormal counts here. For vectors, 
			if count = 0, ok;
			if count = 1, edge, one side is open.
			if count = 2, ok;
			if count = 3, manifold problem. (note, never care if >3)
	    	*/

		for (a=0; a<coordCount; a++) {
			for (b=a; b<coordCount; b++) {
				unsigned char vc = returnIndex(coordCount,a,b);
				//ConsoleMessage ("count for %d,%d is %d\n",a,b,vc);
				if ((vc & 0x01) == 0x01) {
                    			cp += fprintf (fp, "%d, %d, -1,\n",a,b);
					if (vc == 0x01) edgesFound++; else manifoldErrorsFound++;

				}

			}
		}
	
            cp += fprintf (fp,"]\n");
            cp += fprintf (fp,"}}\n");
	    ConsoleMessage("Checking STL file - %d edges %d manifold issues",edgesFound,manifoldErrorsFound);

            }
        }
        
        cp += fprintf (fp,"  ] translation %f %f %f}] scale %f %f %f}\n",midX, midY, midZ,10.0f/scaleFactor,
                       10.0f/scaleFactor, 10.0f/scaleFactor);
                
        midX = EXTENT_MAX_X-EXTENT_MIN_X;
        midY = EXTENT_MAX_Y-EXTENT_MIN_Y;
        //midZ = EXTENT_MAX_Z-EXTENT_MIN_Z;
        //printf ("midX %f midY %f\n",midX,midY);
        if (midX<midY)midX=midY;
    }
    cp += fprintf (fp,"Viewpoint {jump FALSE position 0 0 20} \n");
    cp += fprintf (fp,"Viewpoint {jump FALSE position 0 0 25} \n");
    cp += fprintf (fp,"Viewpoint {jump FALSE position 0 0 40} \n");

    cp += fprintf (fp,"Viewpoint {jump FALSE orientation 0.0 -1.0 0.0 -1.57 position 20.0 0.0 0.0} \n");
     cp += fprintf (fp,"Viewpoint {jump FALSE orientation 0.0 -1.0 0.0 -3.14 position 0.0 0.0 -20.0} \n");
    cp += fprintf (fp,"Viewpoint {jump FALSE orientation 0.0 -1.0 0.0 -4.748 position -20.0 0 0} \n");
    
    cp += fprintf (fp,"Viewpoint {jump FALSE orientation -1.0 0.0 0.0 -1.57 position 0 -20.0 0} \n");
    

    
    
    
    cp += fprintf (fp,"Viewpoint {jump FALSE orientation -0.5888, -0.5688, -0.5743, -2.125 position 20.0 0 0} \n");
    
    cp += fprintf (fp,"Viewpoint {jump FALSE orientation -0.04558, -0.4841, -0.8739, 3.108 position 1.928, 16.93, 11.01} \n");
    
    
    //cp += fprintf(fp,"Shape { appearance Appearance {material Material{}}geometry Sphere{radius 3.0}}\n");

    
	ConsoleMessage ("STL size: (%4.2f,%4.2f), (%4.2f,%4.2f) (%4.2f,%4.2f)",EXTENT_MIN_X,EXTENT_MAX_X,
                    EXTENT_MIN_Y,EXTENT_MAX_Y, EXTENT_MIN_Z,EXTENT_MAX_Z);
    
	fclose (fp);
    
	retval = MALLOC (char *, cp+10);
	fp = fopen(tfn,"r"); 
	fread_val = fread(retval,cp,1,fp);
	ConsoleMessage ("fread is %d\n");
	retval[cp]='\0';
	fclose (fp);
	unlink(tfn);

	FREE_IF_NZ(tfn);
    	FREE_IF_NZ(vectorArray);
    
    //printf ("file is\n%s",retval);
    return retval;
}


static char *makeX3D_orig_STL_File(struct Vector* vertices, 
                   struct Vector* normals,
                   struct Vector* colours,
                   float* _extent) {
    	char *tfn = NULL;
    	FILE *fp;
    	ttglobal tg = gglobal();
    int cp = 0;
    int i;

    tfn=MALLOC(char *,strlen(tg->Mainloop.tmpFileLocation) +strlen (tmpFile) + 10);
	strcpy(tfn,tg->Mainloop.tmpFileLocation);
	strcat(tfn,tmpFile);
    
	// Android 2.2, TEMPNAM does not work, gives back a file that can not be opened.
	// however, OSX, etc, can use TEMPNAM, so we can use it here.
    
 #if !defined (_ANDROID)
	ConsoleMessage ("starting makeX3D_analyzed_STL_File; tmpFileLocation is :%s:\n",tg->Mainloop.tmpFileLocation);
    tfn = TEMPNAM(tg->Mainloop.tmpFileLocation,"/freewrl_tmp");
#endif
    
	fp = fopen(tfn,"w");
	cp += fprintf (fp,"#VRML V2.0 utf8\n");
    cp += fprintf (fp,"Background {skyColor [ 0.7 0.7 1.0 ]}\n");
	cp += fprintf (fp,"Transform { children [Transform { children [Shape{\n");
	cp += fprintf (fp,"appearance Appearance{material TwoSidedMaterial{separateBackColor TRUE diffuseColor 0.8 0.8 0.8 backDiffuseColor 0.8 0 0}}\n");
	cp += fprintf (fp,"geometry TriangleSet {\n");
	cp += fprintf (fp,"normalPerVertex FALSE\n");
    cp += fprintf (fp,"solid FALSE\n");
	cp += fprintf (fp,"coord DEF STL_COORDS Coordinate { point [\n");
    
    for (i=0; i<vectorSize(vertices); i++) {
        struct tstlVertexStruct *thisVertex = vector_get(struct tstlVertexStruct *,vertices,i);
        //calcExtent_dist(_extent,thisVertex);
        cp += fprintf (fp,"%f %f %f,\n",thisVertex->vertex.c[0],thisVertex->vertex.c[1],thisVertex->vertex.c[2]);
    }
	
    cp += fprintf (fp,"]}\n");
 
    //ConsoleMessage ("skipping normals");
    
    if (normals!=NULL) {
        cp += fprintf (fp,"normal Normal { vector [\n");
        for (i=0; i<vectorSize(normals); i++) {
            struct SFVec3f *thisVertex = vector_get(struct SFVec3f *,normals,i);
            cp += fprintf (fp,"%f %f %f,\n",thisVertex->c[0],thisVertex->c[1],thisVertex->c[2]);
        }
        
        cp += fprintf (fp,"]}\n");
    }
    
    
    // 3 vertices makes for 1 triangle, but we keep track of vertices here
    finalCoordsWritten = vectorSize(vertices);
    
    return finishThisX3DFile (fp, cp, tfn, _extent,vectorSize(vertices),0);
}


static char *makeX3D_analyzed_STL_File(struct Vector* vertices, 
                              struct Vector* normals,
                              struct Vector* colours,
                              float* _extent) {
    char *tfn = NULL;
    FILE *fp;
    ttglobal tg = gglobal();
    int cp = 0;
    int i;
    
    
    tfn=MALLOC(char *,strlen(tg->Mainloop.tmpFileLocation) +strlen (tmpFile) + 10);
    strcpy(tfn,tg->Mainloop.tmpFileLocation);
    strcat(tfn,tmpFile);

	// Android 2.2, TEMPNAM does not work, gives back a file that can not be opened.
	// however, OSX, etc, can use TEMPNAM, so we can use it here.
    
 #if !defined (_ANDROID)
	ConsoleMessage ("starting makeX3D_analyzed_STL_File; tmpFileLocation is :%s:\n",tg->Mainloop.tmpFileLocation);
   	 tfn = TEMPNAM(tg->Mainloop.tmpFileLocation,"/freewrl_tmp");
#endif
    
	fp = fopen(tfn,"w");

	cp += fprintf (fp,"#VRML V2.0 utf8\n");
        cp += fprintf (fp,"Background {skyColor [ 0.7 0.7 1.0 ]}\n");
	cp += fprintf (fp,"Transform {children [Transform { children [Shape{\n");
	cp += fprintf (fp,"appearance Appearance{material Material{}}\n");
	cp += fprintf (fp,"geometry IndexedFaceSet {\n");
	cp += fprintf (fp,"normalPerVertex FALSE\n");
    cp += fprintf (fp,"solid FALSE\n");
    cp += fprintf (fp,"creaseAngle 0.75\n");
	cp += fprintf (fp,"coord DEF STL_COORDS Coordinate { point [\n");
    
    for (i=0; i<vectorSize(vertices); i++) {
        struct tstlVertexStruct *thisVertex = vector_get(struct tstlVertexStruct *,vertices,i);
        if (thisVertex->replacementVertex == -1) {
            // this one did NOT get replaced
            //calcExtent_dist(_extent,thisVertex);
            finalCoordsWritten++;
            cp += fprintf (fp,"%f %f %f, #%d\n",thisVertex->vertex.c[0],thisVertex->vertex.c[1],thisVertex->vertex.c[2],
                           thisVertex->condensedVertexNo);
        }
    }
	
    // finish off the Coordinate here
    cp += fprintf (fp,"]}\n");
    
    

    int size=finalCoordsWritten;
    if (checkSTL_for_3Dprinting) {
	/* we need an array to hold vectors to see how often they are used,
	   but we can pack 4 vector indexes into 1 byte, to save memory space
   	   on Android device (at the expense of speed) */

    	vectorArray = MALLOC(unsigned char *,(size*size/4));
    	if (vectorArray) bzero(vectorArray,(size_t)size*size/4);
    }

    // Now do the coordIndex
    { 
        int j = 0;
        int face=0;
        int tv[4];
        int curVertex;
        
        cp += fprintf (fp, "coordIndex [\n");
        for (i=0; i<vectorSize(vertices); i++) {
            j++; // will be 1,2,3
            struct tstlVertexStruct *thisVertex = vector_get(struct tstlVertexStruct *,vertices,i);                
            
            //ConsoleMessage ("coord vector %d...replac %d cond %d",i,thisVertex->replacementVertex,thisVertex->condensedVertexNo);
            if (thisVertex->replacementVertex == -1) {
                curVertex = thisVertex->condensedVertexNo;
            } else {
                struct tstlVertexStruct *rpv = vector_get(struct tstlVertexStruct *,vertices,
                                                          thisVertex->replacementVertex);
                curVertex = rpv->condensedVertexNo;
            }
            cp += fprintf (fp,"%d, ",curVertex);
            tv[j] = curVertex;
            
            if (j==3) {
                j=0;
                cp += fprintf (fp,"-1, #face %d\n",face);
                if (vectorArray) {
                    recordVector(finalCoordsWritten,tv[1],tv[2]);
                    recordVector(finalCoordsWritten,tv[1],tv[3]);
                    recordVector(finalCoordsWritten,tv[2],tv[3]);
                }
                
                face ++;
            }
        }
    // finish the CoordIndex
    cp += fprintf (fp,"]\n");
    }
    
    return finishThisX3DFile (fp, cp, tfn, _extent,vectorSize(vertices),finalCoordsWritten);
}

#define calc_vector_length(pt) veclength(pt)
static float veclength( struct point_XYZ p )
{
    return (float) sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
}

/* Check to see if this triangle is one that we can use for a surface */
static bool degenerate (struct SFVec3f *c1, struct SFVec3f *c2,
                        struct SFVec3f *c3) {
    struct point_XYZ thisfaceNorms;
	float a[3]; float b[3];
    
    
    a[0] = c2->c[0] - c1->c[0];
    a[1] = c2->c[1] - c1->c[1];
    a[2] = c2->c[2] - c1->c[2];
    b[0] = c3->c[0] - c1->c[0];
    b[1] = c3->c[1] - c1->c[1];
    b[2] = c3->c[2] - c1->c[2];
    
    //printf ("a0 %f a1 %f a2 %f b0 %f b1 %f b2 %f\n", a[0],a[1],a[2],b[0],b[1],b[2]); 
    
    thisfaceNorms.x = a[1]*b[2] - b[1]*a[2];
    thisfaceNorms.y = -(a[0]*b[2] - b[0]*a[2]);
    thisfaceNorms.z = a[0]*b[1] - b[0]*a[1];
    
    //ConsoleMessage ("vl is %f",calc_vector_length(thisfaceNorms)); 
    
    return calc_vector_length(thisfaceNorms) > stl_vertex_tolerance;
}


void fwl_stl_set_rendering_type(int nv) {
	switch (nv) {
		case 1:
			// original
			analyzeSTL = FALSE;
			checkSTL_for_3Dprinting = FALSE;
			break;
		case 2:
			// Checked for 2-Manifold and Watertight
			analyzeSTL = TRUE;
			checkSTL_for_3Dprinting = TRUE;
			break;
		case 3:
			// zippy and nice rendering.
			analyzeSTL = TRUE;
			checkSTL_for_3Dprinting = FALSE;
			break;
		default: {}
	}
	//ConsoleMessage("fwl_stl_set_rendering_type is %d",nv);

}

//-------------------------------

static char *analyzeAndGenerate (float *_extent, struct Vector *vertices, struct Vector *normals) {
    char *retval = NULL;
    int i;
    
        //calcExtent_dist(_extent,thisVertex);

    // if we read normals in from the file, we are reading "as-is".
    if (normals == NULL) {
        // analyze the file for duplicate vertices.
        analyzeSTLdata(vertices);
    }
    
    // get the VRML file from this.
    if (analyzeSTL)
        retval = makeX3D_analyzed_STL_File (vertices,normals,NULL,_extent);
    else
        retval = makeX3D_orig_STL_File(vertices,normals,NULL,_extent);

    //ConsoleMessage ("we have a file now of :%s:",retval);
    
    ConsoleMessage ("generating - degenerateTriangleCount %d, Triangles In %d Vertices out %d\n",
                    degenerateCount,triangleInCount,finalCoordsWritten);
    // final coords are vertices; we read in triangles (binary STL has 3 vertices per "record", but we write vertices out and 
    // index them for analyzed IndexedFaceSets, or just write them out for TriangleSets.
    {
        // stats - work in vertex counts
        float fcw = (float) finalCoordsWritten;
        float fcin = (float) triangleInCount * 3;
        if (fcin < 0.5) fcin = 1; // do not want to divide by zero here
        
        ConsoleMessage ("Vertex memory savings %4.1f %% \n", (1-(fcw/fcin))*100.0);
    }
    
    // delete the Vectors
    for (i=0; i<vectorSize(vertices); i++) {
        FREE_IF_NZ(vector_get(struct tstlVertexStruct *,vertices,i));
    }
    deleteVector(struct Vector*, vertices);

    if (normals!=NULL) {
        //ConsoleMessage ("deleting normals here ");
        for (i=0; i<vectorSize(normals); i++) {
            FREE_IF_NZ(vector_get(struct SFVec3f *,normals,i));
        }
        deleteVector(struct Vector*, normals);
    }
  
	//ConsoleMessage (retval);
	return (retval);

}

// read in ascii stl file. 
// ASSUME that all faces consist of 3 vertices, much like a binary
// STL file would.

char *convertAsciiSTL (const char *inp) {

    int i=0;

	struct Vector *vertices = NULL;
	struct Vector *normals = NULL;

	char *normalPtr = NULL;
	char *vertexPtr = NULL;

	int haveNormalHere = false;
    int haveValidNormals = true;
	int haveVertexHere = false;
	float NX,NY,NZ; // last read normal


    float _extent[6];
    struct tstlVertexStruct *thisVertex[3];

#include <sys/time.h>
        struct timeval tv;
        struct timeval tve;
    
    gettimeofday(&tv, NULL);

	char *tptr = (char *)inp;

	int messCount = 0;

	ConsoleMessage ("start reading AsciiSTL - this can take a while");

    //global stats
    degenerateCount=0;
    triangleInCount = 0;
    finalCoordsWritten = 0;
	
    
	// set these up to defaults
	NX=0.0; NY=0.0; NZ=1.0;
    
	// first, read all the vertices.
	vertices = newVector(sizeof (stlVertexStruct),1024);
	EXTENT_MAX_X = -FLT_MAX; EXTENT_MAX_Y = -FLT_MAX; EXTENT_MAX_Z = -FLT_MAX;
    EXTENT_MIN_X = FLT_MAX; EXTENT_MIN_Y = FLT_MAX; EXTENT_MIN_Z = FLT_MAX;
    

    if (!analyzeSTL) {
        // use supplied normals
        normals = newVector(sizeof(struct SFVec3f), 1024);
    }

#define USE_STRING_BUILTINS
#ifdef USE_STRING_BUILTINS
	// skip to either the "vertex" or to the "normal"
    	normalPtr = strcasestr(tptr,"normal ");
	vertexPtr = strcasestr(tptr,"vertex ");
	if ((normalPtr != NULL) &&(normalPtr < vertexPtr)) {
		tptr = normalPtr; 
		haveNormalHere = true;
	} else {
		tptr = vertexPtr;
		haveVertexHere = true;
	}
	if (tptr!=NULL) tptr += strlen("vertex "); // same length as "normal "
#else

	while ((*tptr != '\0') && (*tptr != 'm') && (*tptr != 'x')) tptr++;
	if (*tptr == 'x') {
		haveVertexHere = true;
		tptr++; // skip past the 'x' 
		//if (*tptr != ' ') 
	} else if (*tptr == 'm') {
		tptr++;
		if (*tptr == 'a') {
			tptr++;
			if (*tptr == 'l') {
			tptr ++;
			haveNormalHere = true;
			}
		}
	} else {
		// end of file
		tptr = NULL;
	}	
#endif// USE_STRING_BUILTINS

    //ConsoleMessage ("currently here: %s",tptr);
    
    // we save the vertices only if this is not degenerate.
	while (tptr != NULL) {
		float X,Y,Z;
		if (haveNormalHere) {

			//ConsoleMessage("looking for normals here:%s",tptr);

			#define USE_STRTOF
			#ifdef USE_STRTOF
			NX = strtof(tptr,&tptr);
			NY = strtof(tptr,&tptr);
			NZ = strtof(tptr,&tptr);
			if (1!=1) {
			#else
			if (3!=sscanf (tptr,"%f %f %f", &NX,&NY,&NZ)) {
			#endif
                		if (haveValidNormals) {
                    			char mys[50];
                    			ConsoleMessage ("expected normal, did not get it...");
                    			strncpy(mys,tptr,40); mys[40] = '\0';
                    			ConsoleMessage ("got %s",mys);
                    			NX=0.0; NY=0.0; NZ=1.0;
                    			haveValidNormals = false;
                		}
			}
		} else {
			//ConsoleMessage("Looking for vertexes here:%s",tptr);
			#ifdef USE_STRTOF
			X=strtof(tptr,&tptr);
			Y=strtof(tptr,&tptr);
			Z=strtof(tptr,&tptr);
			{
			#else
			if (3==sscanf (tptr,"%f %f %f", &X,&Y,&Z)) {
			#endif
				thisVertex[i] = MALLOC (struct tstlVertexStruct *, sizeof (stlVertexStruct));
                  
				thisVertex[i]->vertex.c[0] = X;
				thisVertex[i]->vertex.c[1] = Y;
				thisVertex[i]->vertex.c[2] = Z;
                thisVertex[i]->replacementVertex = -1; // no replacement, yet!
                thisVertex[i]->condensedVertexNo = -1; // not done duplicates yet!
                //ConsoleMessage ("read in %f %f %f",X,Y,Z);
                
                		// next vertex, or is the end of a triangle?
                		i++;
                		if (i==3) {
                    			triangleInCount++;
                    
					// valid triangle? if so, push this all, including normal
                    			if (degenerate(&thisVertex[0]->vertex, 
                        	           &thisVertex[1]->vertex, &thisVertex[2]->vertex)) {
						struct SFVec3f *norm;

            					calcExtent_dist(_extent,thisVertex[0]);
            					calcExtent_dist(_extent,thisVertex[1]);
            					calcExtent_dist(_extent,thisVertex[2]);
                        			vector_pushBack(struct tstlVertexStruct *,vertices,thisVertex[0]);
                        			vector_pushBack(struct tstlVertexStruct *,vertices,thisVertex[1]);
                        			vector_pushBack(struct tstlVertexStruct *,vertices,thisVertex[2]);
                                    //ConsoleMessage ("ascii stl, pushed 3 vertices");

                                    if (normals!=NULL) {
                                        norm = MALLOC(struct SFVec3f*, sizeof (struct SFVec3f));
                                        norm->c[0] = NX; norm->c[1]=NY; norm->c[2]=NZ;
                                        vector_pushBack(struct SFVec3f *,normals,norm);
                                        NX = 0.0; NY = 0.0, NZ = 1.0;
                                    }
						
                     			} else {
                        			//ConsoleMessage ("degenerate, skipping %d",i);
                        			degenerateCount++;
                    			}

                    			i=0;
                		}
			}

		}
		// skip to either the "vertex" or to the "normal"
#ifdef USE_STRING_BUILTINS
        normalPtr = strcasestr(tptr,"normal ");
		vertexPtr = strcasestr(tptr,"vertex ");
        if ((normalPtr != NULL) &&(normalPtr < vertexPtr)) {

			tptr = normalPtr; 
			haveNormalHere = true;
			haveVertexHere = false;
		} else {
			tptr = vertexPtr;
			haveVertexHere = true;
			haveNormalHere = false;
            
            messCount ++;
            if (messCount >750) {
                ConsoleMessage("still parsing ASCII STL file... %d triangles, %d degenerates",triangleInCount,degenerateCount);
                messCount = 0;
            }

		}
		if (tptr!=NULL) tptr += strlen("vertex "); // same length as "normal "
        //ConsoleMessage ("currently here: %s",tptr);
#else
	haveVertexHere = false; haveNormalHere = false;
	while ((*tptr != '\0') && (*tptr != 'm') && (*tptr != 'x')) tptr++;
	if (*tptr == 'x') {
		haveVertexHere = true;
            messCount ++;
            if (messCount >750) {
                ConsoleMessage("still parsing ASCII STL file... %d triangles, %d degenerates",triangleInCount,degenerateCount);
                messCount = 0;
            }
		tptr++; // skip past the 'x' 
		//if (*tptr != ' ') 
	} else if (*tptr == 'm') {
		tptr++;
		if (*tptr == 'a') {
			tptr++;
			if (*tptr == 'l') {
			tptr ++;
			haveNormalHere = true;
			}
		}
	} else {
		// end of file
		tptr = NULL;
	}	

#endif

	}

       gettimeofday(&tve,NULL);
       ConsoleMessage ("AsciiSTL - took %ld seconds to parse",tve.tv_sec-tv.tv_sec);
        

    //ConsoleMessage ("asciiSTL, degenerateCount %d",degenerateCount);
    
    return analyzeAndGenerate(_extent,vertices,normals);
}

char *convertBinarySTL (const unsigned char *buffer) {
    int i;
    struct Vector *vertices = NULL;
    struct Vector *normals = NULL;
    float _extent[6];
    int32_t *stllen;
    bool haveAttributeInfo = false;
    
    unsigned char *tmp = (unsigned char *)buffer;
    
    //global stats
    degenerateCount=0;
    triangleInCount = 0;
    finalCoordsWritten = 0;

    
    // create pointers to length and data areas
    stllen = offsetPointer_deref(int32_t *, buffer, 80);
    tmp = offsetPointer_deref(unsigned char*, buffer, STL_BINARY_HEADER_LEN);
    
    //ConsoleMessage ("triangle input count %d\n",*stllen);
      
    //for (i=0; i<80; i++) {
      //  ConsoleMessage("header, i: %d  char %x (%c)",i,buffer[i],buffer[i]);
    //}
    //ConsoleMessage ("Binary STL header :%s:",buffer);
    
    // read all the vertices.
	vertices = newVector(sizeof (stlVertexStruct),(*stllen)*3);
    
    // if we want to use the supplied normals
    if (!analyzeSTL) {
        // use supplied normals
        normals = newVector(sizeof(struct SFVec3f), (*stllen));
        //ConsoleMessage ("binary STL - SFVec3f is %d",sizeof (struct SFVec3f));
        if (sizeof (struct SFVec3f) != 12) {
            ConsoleMessage ("binary reading of STL - SFVec3f wrong size");
        }
    }

    
    // set extents, and do it
    EXTENT_MAX_X = -FLT_MAX; EXTENT_MAX_Y = -FLT_MAX; EXTENT_MAX_Z = -FLT_MAX;
    EXTENT_MIN_X = FLT_MAX; EXTENT_MIN_Y = FLT_MAX; EXTENT_MIN_Z = FLT_MAX;
        
    for (i=0; i<*stllen; i++) {
 
        triangleInCount++;
        
        struct tstlVertexStruct *vertex1 = MALLOC (struct tstlVertexStruct *, sizeof (stlVertexStruct));
        struct tstlVertexStruct *vertex2 = MALLOC (struct tstlVertexStruct *, sizeof (stlVertexStruct));
        struct tstlVertexStruct *vertex3 = MALLOC (struct tstlVertexStruct *, sizeof (stlVertexStruct));
        // binary normal - skip
        
        // vertex 1 
        memcpy (vertex1->vertex.c,&tmp[12],12);
        vertex1->replacementVertex = -1; // no replacement, yet!
        vertex1->condensedVertexNo = -1; // not done duplicates yet!

        // vertex 2
        vertex2 = MALLOC (struct tstlVertexStruct *, sizeof (stlVertexStruct));
        memcpy (vertex2->vertex.c,&(tmp[24]),12);
        vertex2->replacementVertex = -1; // no replacement, yet!
        vertex2->condensedVertexNo = -1; // not done duplicates yet!

        // vertex 3
        vertex3 = MALLOC (struct tstlVertexStruct *, sizeof (stlVertexStruct));
        memcpy (vertex3->vertex.c,&(tmp[36]),12);
        vertex3->replacementVertex = -1; // no replacement, yet!
        vertex3->condensedVertexNo = -1; // not done duplicates yet!

        // check for degenerate triangles
        if (degenerate(&vertex1->vertex, &vertex2->vertex, &vertex3->vertex)) {
            calcExtent_dist(_extent,vertex1);
            calcExtent_dist(_extent,vertex2);
            calcExtent_dist(_extent,vertex3);
            vector_pushBack(struct tstlVertexStruct *,vertices,vertex1);
            vector_pushBack(struct tstlVertexStruct *,vertices,vertex2);
            vector_pushBack(struct tstlVertexStruct *,vertices,vertex3);
        
            // are we using the old normals, not calculating our own?
            if (normals != NULL) {
                struct SFVec3f *norm;
                norm = MALLOC(struct SFVec3f*, sizeof (struct SFVec3f));
                memcpy(norm, tmp, 12);
                vector_pushBack(struct SFVec3f *,normals,norm);
                
            }
            if ((tmp[48] != 0) || (tmp[49]!=0)) {
        
                haveAttributeInfo = true;
            }
        } else {
            //ConsoleMessage ("degenerate, skipping %d",i);
            degenerateCount++;
        }
        
        tmp = offsetPointer_deref(unsigned char*, tmp, STL_BINARY_VERTEX_SIZE);
    }

    if (haveAttributeInfo) {
        //ConsoleMessage ("BINARY STL with Colour info");
    }
    
    //ConsoleMessage ("Triangles in %d, degenerates %d",*stllen,degenerateCount);
    return analyzeAndGenerate(_extent,vertices,normals);
}

/* STL files will get scaled to fit into a good-sized box. Return this for
   FillProperties, etc */
float getLastSTLScale(void) {
	// force it to 10 per meter, not 1 per meter.
	//ConsoleMessage ("getLastSTLScale, in convertSTL.c - sf %f",scaleFactor/10.0);
	if (scaleFactor < 0.0) return 1.0;
	return scaleFactor/10.0;
}

#endif //INCLUDE_STL_FILES
