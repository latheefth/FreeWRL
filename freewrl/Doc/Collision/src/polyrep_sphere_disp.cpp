#define CONSTRUCT

#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <math.h>
#include <string>

#include "LinearAlgebra.h"

#include "Collision.h"




/*false constructor, for personal ease*/
struct pt pt(double ax=0., double ay=0., double az=0.)
{
    struct pt tmp = {ax,ay,az};
    return tmp;
}


using namespace std;

/* modifyable points */
vector<struct pt> pts;
vector<int> activepts;

vector<struct pt> debugpts;
vector<int> debugcolors;
vector<string> debugptsinfo;
int debugpt = 0;
int debugsurface = 0;
int facemask = 323;
int rotcenter = 0;

//#define DEBUGFACEMASK
#define DEBUGPTS
#include "Collision.cpp"

struct VRML_PolyRep polyrep;
int polyrepntri;

struct pt  iv, jv, kv;

enum eslideplane {pxy=0,pyz=1,pzx=2,rot=3};

eslideplane slide = pxy;
bool perspective = true;



void init(void)
{    

    glClearColor (0.0, 0.0, 0.0, 0.0);
    glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POINT_SMOOTH);

//    glEnable(GL_CULL_FACE);
//    glCullFace(GL_BACK);
    glPolygonMode(GL_BACK,GL_LINE);

    glPointSize(4.0);

/*    pts.push_back(pt(1,1,1));
    pts.push_back(pt(1,-1,1));
    pts.push_back(pt(0,0,1));
    pts.push_back(pt(0,-1,1));
    pts.push_back(pt(1,1,2));*/
    pts.push_back(pt(0,0,0));



    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, -3.6);
     
}

void displayhandles(void) {
    glBegin(GL_POINTS);
    glColor3f(0.,1.,0.);
    //active handles
    for(int i=0; i < activepts.size(); i++) {
	glVertex3dv(&pts[activepts[i]].x);
	printf("#%d:(%f,%f,%f)\n",activepts[i],pts[activepts[i]].x,pts[activepts[i]].y,pts[activepts[i]].z);
    }
    //extra points 
    //debugfocus
    if(debugpt > 0 && debugpt < debugpts.size()) {
	glPointSize(6.0);
	glColor3f(1.,1.,1.);
	glVertex3dv(&debugpts[debugpt].x);
	glPointSize(4.0);
    }
    for(int i=0; i < debugpts.size(); i++) {
	if(i < debugcolors.size()) 
	    glColor3f(debugcolors[i]& (1<<0) ? 1.:.4,debugcolors[i]&(1<<1)? 1.:.4,debugcolors[i]&(1<<2)? 1.:.4);
	else 
	    glColor3f(1.,0,0);
	glVertex3dv(&debugpts[i].x);
    }
    //handles.
    glColor3f(0.,.5,.5);
    for(int i=0; i < pts.size(); i++) {
	glVertex3dv(&pts[i].x);
    }
    printf("\n");	
    glEnd();

    /*draw axis oriented pointers*/
    glBegin(GL_LINES);

    for(int i=0; i < activepts.size(); i++) {

	glColor3f(.5,0,0);
	glVertex3dv(&pts[activepts[i]].x);
	glVertex3f(0,pts[activepts[i]].y,pts[activepts[i]].z);

	glColor3f(0,.5,0);
	glVertex3dv(&pts[activepts[i]].x);
	glVertex3f(pts[activepts[i]].x,0,pts[activepts[i]].z);

	glColor3f(0,0,.5);
	glVertex3dv(&pts[activepts[i]].x);
	glVertex3f(pts[activepts[i]].x,pts[activepts[i]].y,0);
     
    }
    glEnd();
     
     
}

void displayaxis(void) {
    glBegin(GL_LINES);

    glColor3f(1,0,0);
    glVertex3f(0,0,0);
    glVertex3f(1,0,0);

    glColor3f(0,1,0);
    glVertex3f(0,0,0);
    glVertex3f(0,1,0);

    glColor3f(0,0,1);
    glVertex3f(0,0,0);
    glVertex3f(0,0,1);
     
    glEnd();

}

VRML_PolyRep makepolyrep(struct pt p1, struct pt p2, struct pt p3, struct pt p4, struct pt p5) {
    static int cindex[18];
    static float coord[15];
    static int colindex[18];
    static float color[18];

    int cindext[18] = {0,1,2, 3,2,1, 3,4,2, 3,1,4, 0,2,4, 0,4,1};
    float coordt[15] = {p1.x,p1.y,p1.z,
			p2.x,p2.y,p2.z,
			p3.x,p3.y,p3.z,
			p4.x,p4.y,p4.z,
			p5.x,p5.y,p5.z };
    int colindext[18] = {0,1,2, 3,4,5, 1,2,3, 4,5,0, 2,3,4, 5,0,1};
    float colort[18] = {.2,.4,.6,
			 .3,.5,.7,
			 .4,.6,.8,
			 .5,.7,.1,
			 .6,.8,.2,
			 .7,.1,.3};
    VRML_PolyRep pr = {0,6,6,cindex,coord,colindex,color,NULL,NULL,NULL,NULL};
    memcpy(cindex,cindext,sizeof(cindex));
    memcpy(coord,coordt,sizeof(coord));
    memcpy(colindex,colindext,sizeof(colindex));
    memcpy(color,colort,sizeof(color));
    return pr;
    
} 

/*
VRML_PolyRep makepolyrep() {
static int cindex[36];
static float coord[108];
 int cindext[36] = {0,1,3,0,3,2,2,3,5,2,5,4,4,5,7,4,7,6,6,7,9,6,9,8,8,9,11,8,11,10,10,11,13,10,13,12};
 float coordt[108] = {0.000000,0.000000,0.000000,4.000000,0.000000,0.000000,0.000000,0.000000,-1.000000,4.000000,0.000000,-1.000000,0.000000,1.000000,-1.000000,4.000000,1.000000,-1.000000,0.000000,1.000000,-2.000000,4.000000,1.000000,-2.000000,0.000000,2.000000,-2.000000,4.000000,2.000000,-2.000000,0.000000,2.000000,-3.000000,4.000000,2.000000,-3.000000,0.000000,3.000000,-3.000000,4.000000,3.000000,-3.000000,0.000000,0.000000,0.000000,0.000000,0.000000,0.000000,0.000000,0.000000,0.000000,0.000000,0.000000,0.000000,0.000000,1.875000,0.000000,0.000000,0.000000,0.000000,0.000000,-1.875000,0.000000,0.000000,0.000000,0.000000,0.000000,2.250000,0.000000,0.000000,0.000000,0.000000,0.000000,1.875000,0.000000,0.000000,0.000000,0.000000,0.000000,-1.875000,0.000000,0.000000,0.000000,0.000000,0.000000,0.000000,0.000000,0.000000,0.000000,0.000000,0.000000,1.875000,0.000000,0.000000,0.000000,0.000000,0.000000,-2.000000,0.000000,0.000000,0.000000,0.000000,0.000000,2.250000,0.000000,0.000000,0.000000,0.000000};
VRML_PolyRep pr = {0,12,0,cindex,coord,NULL,NULL,NULL,NULL,NULL,NULL};
memcpy(cindex,cindext,sizeof(cindex));
memcpy(coord,coordt,sizeof(coord));
return pr; }
*/

VRML_PolyRep makepolyrep() {
 int cindext[1002] = {17,0,18,1,18,0,34,17,35,18,35,17,51,34,35,35,52,51,68,51,52,52,69,68,85,68,69,69,86,85,102,85,86,86,103,102,119,102,103,103,120,119,136,119,120,120,137,136,153,136,154,137,154,136,170,153,171,154,171,153,18,1,2,2,19,18,35,18,19,19,36,35,52,35,53,36,53,35,69,52,70,53,70,52,86,69,87,70,87,69,103,86,104,87,104,86,120,103,121,104,121,103,137,120,138,121,138,120,154,137,138,138,155,154,171,154,155,155,172,171,19,2,20,3,20,2,36,19,37,20,37,19,53,36,37,37,54,53,70,53,54,54,71,70,87,70,71,71,88,87,104,87,88,88,105,104,121,104,105,105,122,121,138,121,122,122,139,138,155,138,156,139,156,138,172,155,173,156,173,155,20,3,4,4,21,20,37,20,21,21,38,37,54,37,55,38,55,37,71,54,72,55,72,54,88,71,89,72,89,71,105,88,106,89,106,88,122,105,123,106,123,105,139,122,140,123,140,122,156,139,140,140,157,156,173,156,157,157,174,173,21,4,22,5,22,4,38,21,39,22,39,21,55,38,39,39,56,55,72,55,56,56,73,72,89,72,73,73,90,89,106,89,90,90,107,106,123,106,107,107,124,123,140,123,124,124,141,140,157,140,158,141,158,140,174,157,175,158,175,157,22,5,6,6,23,22,39,22,23,23,40,39,56,39,57,40,57,39,73,56,74,57,74,56,90,73,91,74,91,73,107,90,108,91,108,90,124,107,125,108,125,107,141,124,142,125,142,124,158,141,142,142,159,158,175,158,159,159,176,175,23,6,24,7,24,6,40,23,41,24,41,23,57,40,41,41,58,57,74,57,58,58,75,74,91,74,75,75,92,91,108,91,92,92,109,108,125,108,109,109,126,125,142,125,126,126,143,142,159,142,160,143,160,142,176,159,177,160,177,159,24,7,8,8,25,24,41,24,25,25,42,41,58,41,59,42,59,41,75,58,76,59,76,58,92,75,93,76,93,75,109,92,110,93,110,92,126,109,127,110,127,109,143,126,144,127,144,126,160,143,144,144,161,160,177,160,161,161,178,177,25,8,26,9,26,8,42,25,43,26,43,25,59,42,43,43,60,59,76,59,60,60,77,76,93,76,77,77,94,93,110,93,94,94,111,110,127,110,111,111,128,127,144,127,128,128,145,144,161,144,162,145,162,144,178,161,179,162,179,161,26,9,10,10,27,26,43,26,27,27,44,43,60,43,61,44,61,43,77,60,78,61,78,60,94,77,95,78,95,77,111,94,112,95,112,94,128,111,129,112,129,111,145,128,146,129,146,128,162,145,146,146,163,162,179,162,163,163,180,179,27,10,28,11,28,10,44,27,45,28,45,27,61,44,45,45,62,61,78,61,62,62,79,78,95,78,79,79,96,95,112,95,96,96,113,112,129,112,113,113,130,129,146,129,130,130,147,146,163,146,164,147,164,146,180,163,181,164,181,163,28,11,12,12,29,28,45,28,29,29,46,45,62,45,63,46,63,45,79,62,80,63,80,62,96,79,97,80,97,79,113,96,114,97,114,96,130,113,131,114,131,113,147,130,148,131,148,130,164,147,148,148,165,164,181,164,165,165,182,181,29,12,30,13,30,12,46,29,47,30,47,29,63,46,47,47,64,63,80,63,64,64,81,80,97,80,81,81,98,97,114,97,98,98,115,114,131,114,115,115,132,131,148,131,132,132,149,148,165,148,166,149,166,148,182,165,183,166,183,165,30,13,14,14,31,30,47,30,31,31,48,47,64,47,65,48,65,47,81,64,82,65,82,64,98,81,99,82,99,81,115,98,116,99,116,98,132,115,133,116,133,115,149,132,150,133,150,132,166,149,150,150,167,166,183,166,167,167,184,183,31,14,32,15,32,14,48,31,49,32,49,31,65,48,49,49,66,65,82,65,66,66,83,82,99,82,83,83,100,99,116,99,100,100,117,116,133,116,117,117,134,133,150,133,134,134,151,150,167,150,168,151,168,150,184,167,185,168,185,167,32,15,0,0,17,32,49,32,17,17,34,49,66,49,51,34,51,49,83,66,68,51,68,66,100,83,85,68,85,83,117,100,102,85,102,100,134,117,119,102,119,117,151,134,136,119,136,134,168,151,136,136,153,168,185,168,153,153,170,185,0,2,1,0,3,2,0,4,3,0,5,4,0,6,5,0,7,6,0,8,7,0,9,8,0,10,9,0,11,10,0,12,11,0,13,12,0,14,13,0,15,14};
 float coordt[555] = {1.800000,0.000000,0.000000,1.656000,0.000000,-0.684000,1.278000,0.000000,-1.278000,0.684000,0.000000,-1.656000,0.000000,0.000000,-1.800000,-0.684000,0.000000,-1.656000,-1.278000,0.000000,-1.278000,-1.656000,0.000000,-0.684000,-1.800000,0.000000,0.000000,-1.656000,0.000000,0.684000,-1.278000,0.000000,1.278000,-0.684000,0.000000,1.656000,0.000000,0.000000,1.800000,0.684000,0.000000,1.656000,1.278000,0.000000,1.278000,1.656000,0.000000,0.684000,1.800000,0.000000,0.000000,1.950000,0.400000,0.000000,1.794000,0.400000,-0.741000,1.384500,0.400000,-1.384500,0.741000,0.400000,-1.794000,0.000000,0.400000,-1.950000,-0.741000,0.400000,-1.794000,-1.384500,0.400000,-1.384500,-1.794000,0.400000,-0.741000,-1.950000,0.400000,0.000000,-1.794000,0.400000,0.741000,-1.384500,0.400000,1.384500,-0.741000,0.400000,1.794000,0.000000,0.400000,1.950000,0.741000,0.400000,1.794000,1.384500,0.400000,1.384500,1.794000,0.400000,0.741000,1.950000,0.400000,0.000000,2.000000,0.800000,0.000000,1.840000,0.800000,-0.760000,1.420000,0.800000,-1.420000,0.760000,0.800000,-1.840000,0.000000,0.800000,-2.000000,-0.760000,0.800000,-1.840000,-1.420000,0.800000,-1.420000,-1.840000,0.800000,-0.760000,-2.000000,0.800000,0.000000,-1.840000,0.800000,0.760000,-1.420000,0.800000,1.420000,-0.760000,0.800000,1.840000,0.000000,0.800000,2.000000,0.760000,0.800000,1.840000,1.420000,0.800000,1.420000,1.840000,0.800000,0.760000,2.000000,0.800000,0.000000,1.950000,1.200000,0.000000,1.794000,1.200000,-0.741000,1.384500,1.200000,-1.384500,0.741000,1.200000,-1.794000,0.000000,1.200000,-1.950000,-0.741000,1.200000,-1.794000,-1.384500,1.200000,-1.384500,-1.794000,1.200000,-0.741000,-1.950000,1.200000,0.000000,-1.794000,1.200000,0.741000,-1.384500,1.200000,1.384500,-0.741000,1.200000,1.794000,0.000000,1.200000,1.950000,0.741000,1.200000,1.794000,1.384500,1.200000,1.384500,1.794000,1.200000,0.741000,1.950000,1.200000,0.000000,1.800000,1.600000,0.000000,1.656000,1.600000,-0.684000,1.278000,1.600000,-1.278000,0.684000,1.600000,-1.656000,0.000000,1.600000,-1.800000,-0.684000,1.600000,-1.656000,-1.278000,1.600000,-1.278000,-1.656000,1.600000,-0.684000,-1.800000,1.600000,0.000000,-1.656000,1.600000,0.684000,-1.278000,1.600000,1.278000,-0.684000,1.600000,1.656000,0.000000,1.600000,1.800000,0.684000,1.600000,1.656000,1.278000,1.600000,1.278000,1.656000,1.600000,0.684000,1.800000,1.600000,0.000000,1.500000,2.000000,0.000000,1.380000,2.000000,-0.570000,1.065000,2.000000,-1.065000,0.570000,2.000000,-1.380000,0.000000,2.000000,-1.500000,-0.570000,2.000000,-1.380000,-1.065000,2.000000,-1.065000,-1.380000,2.000000,-0.570000,-1.500000,2.000000,0.000000,-1.380000,2.000000,0.570000,-1.065000,2.000000,1.065000,-0.570000,2.000000,1.380000,0.000000,2.000000,1.500000,0.570000,2.000000,1.380000,1.065000,2.000000,1.065000,1.380000,2.000000,0.570000,1.500000,2.000000,0.000000,1.200000,2.400000,0.000000,1.104000,2.400000,-0.456000,0.852000,2.400000,-0.852000,0.456000,2.400000,-1.104000,0.000000,2.400000,-1.200000,-0.456000,2.400000,-1.104000,-0.852000,2.400000,-0.852000,-1.104000,2.400000,-0.456000,-1.200000,2.400000,0.000000,-1.104000,2.400000,0.456000,-0.852000,2.400000,0.852000,-0.456000,2.400000,1.104000,0.000000,2.400000,1.200000,0.456000,2.400000,1.104000,0.852000,2.400000,0.852000,1.104000,2.400000,0.456000,1.200000,2.400000,0.000000,1.050000,2.800000,0.000000,0.966000,2.800000,-0.399000,0.745500,2.800000,-0.745500,0.399000,2.800000,-0.966000,0.000000,2.800000,-1.050000,-0.399000,2.800000,-0.966000,-0.745500,2.800000,-0.745500,-0.966000,2.800000,-0.399000,-1.050000,2.800000,0.000000,-0.966000,2.800000,0.399000,-0.745500,2.800000,0.745500,-0.399000,2.800000,0.966000,0.000000,2.800000,1.050000,0.399000,2.800000,0.966000,0.745500,2.800000,0.745500,0.966000,2.800000,0.399000,1.050000,2.800000,0.000000,1.000000,3.200000,0.000000,0.920000,3.200000,-0.380000,0.710000,3.200000,-0.710000,0.380000,3.200000,-0.920000,0.000000,3.200000,-1.000000,-0.380000,3.200000,-0.920000,-0.710000,3.200000,-0.710000,-0.920000,3.200000,-0.380000,-1.000000,3.200000,0.000000,-0.920000,3.200000,0.380000,-0.710000,3.200000,0.710000,-0.380000,3.200000,0.920000,0.000000,3.200000,1.000000,0.380000,3.200000,0.920000,0.710000,3.200000,0.710000,0.920000,3.200000,0.380000,1.000000,3.200000,0.000000,1.050000,3.600000,0.000000,0.966000,3.600000,-0.399000,0.745500,3.600000,-0.745500,0.399000,3.600000,-0.966000,0.000000,3.600000,-1.050000,-0.399000,3.600000,-0.966000,-0.745500,3.600000,-0.745500,-0.966000,3.600000,-0.399000,-1.050000,3.600000,0.000000,-0.966000,3.600000,0.399000,-0.745500,3.600000,0.745500,-0.399000,3.600000,0.966000,0.000000,3.600000,1.050000,0.399000,3.600000,0.966000,0.745500,3.600000,0.745500,0.966000,3.600000,0.399000,1.050000,3.600000,0.000000,1.150000,4.000000,0.000000,1.058000,4.000000,-0.437000,0.816500,4.000000,-0.816500,0.437000,4.000000,-1.058000,0.000000,4.000000,-1.150000,-0.437000,4.000000,-1.058000,-0.816500,4.000000,-0.816500,-1.058000,4.000000,-0.437000,-1.150000,4.000000,0.000000,-1.058000,4.000000,0.437000,-0.816500,4.000000,0.816500,-0.437000,4.000000,1.058000,0.000000,4.000000,1.150000,0.437000,4.000000,1.058000,0.816500,4.000000,0.816500};
 int *cindex;
 float *coord;
VRML_PolyRep pr = {0,334,0,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

cindex = (int*)malloc(sizeof(cindext));
coord = (float*)malloc(sizeof(coordt));
pr.cindex = cindex;
pr.coord = coord;

memcpy(cindex,cindext,sizeof(cindext));
memcpy(coord,coordt,sizeof(coordt));
return pr; }

float clamp(float f) {
    f = f/M_PI;
    f = f - (int)f;
    if(f < 0) f = -f;
    return f;
}

void display_polyrep(VRML_PolyRep pr) {
    glBegin(GL_TRIANGLES);
    for(int i = 0; i < pr.ntri*3; i++) {
	if(pr.colindex && pr.color) 
	    glColor4f(pr.color[pr.colindex[i]*3],pr.color[pr.colindex[i]*3+1],pr.color[pr.colindex[i]*3+2],0.5);
	else
	    glColor4f(clamp(pr.coord[pr.cindex[i]*3]),
		      clamp(pr.coord[pr.cindex[i]*3+1]),
		      clamp(pr.coord[pr.cindex[i]*3+2]),0.5);
	glVertex3fv(pr.coord+ pr.cindex[i]*3);
    }
    glEnd();
}
/*void getmatrix(GLdouble* mat) {
mat[0] = -0.731221;
mat[1] = 0.148913;
mat[2] = -0.665685;
mat[3] = 0.000000;
mat[4] = 0.664668;
mat[5] = -0.063910;
mat[6] = -0.744390;
mat[7] = 0.000000;
mat[8] = -0.153389;
mat[9] = -0.986775;
mat[10] = -0.052261;
mat[11] = 0.000000;
mat[12] = -0.017169;
mat[13] = 0.002585;
mat[14] = -0.225434;
mat[15] = 1.000000;
}

//COLLISION_IFS: ref1 (-0.017169 0.002585 -0.225434) (0.048021 -0.043701 0.015917)
*/
/*
void getmatrix(GLdouble* mat,struct pt trans) {
mat[0] = 0.680112;
mat[1] = 0.269340;
mat[2] = -0.681836;
mat[3] = 0.000000;
mat[4] = -0.702454;
mat[5] = -0.026701;
mat[6] = -0.711226;
mat[7] = 0.000000;
mat[8] = -0.209767;
mat[9] = 0.962674;
mat[10] = 0.171039;
mat[11] = 0.000000;
mat[12] = 0.691816+trans.x;
mat[13] = 0.179929+trans.y;
mat[14] = 1.286911+trans.z;
mat[15] = 1.000000;
}*/
/*
COLLISION_IFS: ref173 (0.686824 0.178589 1.267539) (-0.004988 -0.001324 0.001336)
COLLISION_IFS: ref174 (0.691816 0.179929 1.286911) (0.139200 -0.415191 -0.018517)
COLLISION_IFS: ref175 (0.552450 0.595075 1.305162) (0.000014 -0.000039 -0.000002)
*/

/*
//ref405:
void getmatrix(GLdouble* mat,struct pt trans) {
mat[0] = 0.133220;
mat[1] = -0.032780;
mat[2] = -0.990541;
mat[3] = 0.000000;
mat[4] = -0.991071;
mat[5] = -0.009264;
mat[6] = -0.132988;
mat[7] = 0.000000;
mat[8] = -0.004814;
mat[9] = 0.999417;
mat[10] = -0.033724;
mat[11] = 0.000000;
mat[12] = 4.029228+trans.x;
mat[13] = -0.046689+trans.y;
mat[14] = 2.675783+trans.z;
mat[15] = 1.000000;
}
//COLLISION_IFS: ref405 (4.029228 -0.046689 2.675783) (-0.000919 0.000239 0.007211)
*/

/*void getmatrix(GLdouble* mat, struct pt disp) {
mat[0] = 1.000000;
mat[1] = 0.000000;
mat[2] = 0.000000;
mat[3] = 0.000000;
mat[4] = 0.000000;
mat[5] = 1.000000;
mat[6] = 0.000000;
mat[7] = 0.000000;
mat[8] = 0.000000;
mat[9] = 0.000000;
mat[10] = 1.000000;
mat[11] = 0.000000;
mat[12] = -2.000000 +disp.x;
mat[13] = -1.500000 +disp.y;
mat[14] = 0.762920 +disp.z;
mat[15] = 1.000000;
}
//COLLISION_IFS: ref0 (-2.000000 -1.500000 0.762920) (-0.000000 0.566667 0.012920)
*/
/*
void getmatrix(GLdouble* mat, struct pt disp) {
mat[0] = 1.000000;
mat[1] = 0.000000;
mat[2] = 0.000000;
mat[3] = 0.000000;
mat[4] = 0.000000;
mat[5] = 1.000000;
mat[6] = 0.000000;
mat[7] = 0.000000;
mat[8] = 0.000000;
mat[9] = 0.000000;
mat[10] = 1.000000;
mat[11] = 0.000000;
mat[12] = 0.000000 +disp.x;
mat[13] = 0.094218 +disp.y;
mat[14] = -5.110169 +disp.z;
mat[15] = 1.000000;
}
//COLLISION_EXT: ref149 (0.000000 0.094218 -5.110169) (-0.000000 -0.155782 -0.000000)
void getmatrix(GLdouble* mat, struct pt disp) {
mat[0] = 1.000000;
mat[1] = 0.000000;
mat[2] = 0.000000;
mat[3] = 0.000000;
mat[4] = 0.000000;
mat[5] = 1.000000;
mat[6] = 0.000000;
mat[7] = 0.000000;
mat[8] = 0.000000;
mat[9] = 0.000000;
mat[10] = 1.000000;
mat[11] = 0.000000;
mat[12] = 0.000188 +disp.x;
mat[13] = -0.228682 +disp.y;
mat[14] = -0.225170 +disp.z;
mat[15] = 1.000000;
}
COLLISION_EXT: ref491 (0.000188 -0.228682 -0.225170) (0.000000 0.021318 0.000000)
*/
void getmatrix(GLdouble* mat, struct pt disp) {
mat[0] = 1.000000;
mat[1] = 0.000000;
mat[2] = 0.000000;
mat[3] = 0.000000;
mat[4] = 0.000000;
mat[5] = 1.000000;
mat[6] = 0.000000;
mat[7] = 0.000000;
mat[8] = 0.000000;
mat[9] = 0.000000;
mat[10] = 1.000000;
mat[11] = 0.000000;
mat[12] = 0.000188 +disp.x;
mat[13] = 0.016396 +disp.y;
mat[14] = -0.224827 +disp.z;
mat[15] = 1.000000;
}
//COLLISION_EXT: ref492 (0.000188 0.016396 -0.224827) (-0.000000 -0.233604 -0.000000)

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GLUquadricObj* qo = gluNewQuadric();
    gluQuadricDrawStyle(qo,GLU_SILHOUETTE);

    double r = 0.25;
    double y1 = -2*1.6/3;
    double y2 = 1.6/3;
    double bradius;
    struct pt disp;

    debugpts.clear();
    debugptsinfo.clear();
    debugsurface = 1;

    GLdouble mat[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    getmatrix(mat,pts[0]);



//    polyrep.ntri = min(polyrepntri,facemask);



    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    glPushMatrix();
    glMultMatrixd(mat);
    display_polyrep(polyrep);
    glPopMatrix();
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glPolygonMode(GL_BACK,GL_LINE);



    disp = polyrep_disp(y1,y2,y2,r,polyrep,mat,PR_DOUBLESIDED);
    printf("disp = (%f,%f,%f)\n",disp.x,disp.y,disp.z);





    /* draw cylinder */
/*    glPushMatrix();
    glColor3f(0.4,0.4,0.4);
    glRotated(90,-1,0,0);
    glTranslatef(0,0,y1);
    gluCylinder(qo,r,r,y2-y1,32,1);
    glPopMatrix();*/
    /* draw sphere */
    glColor3f(0.4,0.4,0.4);
    gluSphere(qo,r,16,16);


    displayhandles();
    displayaxis();

/*  draw transparent result */
//    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
//     glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    glPushMatrix();
    glTranslatef(disp.x,disp.y,disp.z);
    glMultMatrixd(mat);
    display_polyrep(polyrep);
    glPopMatrix();

    glDisable(GL_BLEND);

    glFlush();
    gluDeleteQuadric(qo);


}















void reshape(int nw=0, int nh=0)
{
    static int w,h;
    if(nw) w = nw;
    if(nh) h = nh;
    glViewport(0, 0, (GLsizei) w, (GLsizei) h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if(perspective) {
	gluPerspective(60.0, (GLfloat) w/(GLfloat) h, 1.0, 30.0);
    } else
	glOrtho(-1,1,-1,1,-1000,1000);

    glMatrixMode(GL_MODELVIEW);
}

void keyboard (unsigned char key, int x, int y)
{
    GLdouble premat[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    GLdouble postmat[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    const GLdouble ident[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    static double speed = 0.2;
    static double rspeed = 5;
    static bool examine = true;

    if(examine) {
	memcpy(postmat,ident,sizeof(ident));
	glGetDoublev(GL_MODELVIEW_MATRIX,premat);
    }
    else {
	glGetDoublev(GL_MODELVIEW_MATRIX,postmat);
	memcpy(premat,ident,sizeof(ident));
    }
     

    switch (key) {
    case '=':
    case '+':
	activepts.resize(1);
	activepts[0] = (activepts[0]+1) % pts.size();
	break;
    case '.':
	activepts.push_back((activepts.back()+1) % pts.size());
	break;
    case '-':
	activepts.resize(1);
	activepts[0] = (activepts[0]-1) % pts.size();
	break;
    case '[':
	debugpt = (debugpt-1)%debugpts.size();
	cout << "debugpt=" << debugpt << endl;
	break;
    case ']':
	debugpt = (debugpt+1)%debugpts.size();
	cout << "debugpt=" << debugpt << endl;
	break;
    case '<':
	facemask = facemask-1;
	cout << "facemask=" << facemask << endl;
	break;
    case '>':
	facemask = facemask+1;
	cout << "facemask=" << facemask << endl;
	break;
    case 'x':
	slide = eslideplane(((int)(slide) +1) % 4);
	switch( slide ) {
	case pxy:
	    printf("sliding on xy axis\n");
	    break;
	case pyz:
	    printf("sliding on yz axis\n");
	    break;
	case pzx:
	    printf("sliding on zx axis\n");
	    break;
	case rot:
	    printf("rotating around pt#%d ('r' to change)\n",rotcenter);
	    break;
	}
	break;
    case 'r':
	if(activepts.size() > 0 )
	    rotcenter = activepts[0];
	printf("rotation center is now %d\n",rotcenter);
	break;
    case 'l':
	examine = !examine;
	printf(examine ? "switching to examine mode\n" : "switching to freelook mode\n");
	break;
    case 'p':
	perspective = !perspective;
	printf(perspective ? "perspecive view\n" : "orthogonal view ('/','*' for zooming)\n");
	reshape();
	break;
    case 'q':
    case 27:
	exit(0);
	break;
	  
    case '*':
	glLoadMatrixd(premat);
	glScalef(sqrt(2.),sqrt(2.),sqrt(2.));
	glMultMatrixd(postmat);
	break;
    case '/':
	glLoadMatrixd(premat);
	glScalef(sqrt(2.)/2,sqrt(2.)/2,sqrt(2.)/2);
	glMultMatrixd(postmat);
	break;
    case 'd':
	glLoadMatrixd(premat);
	glTranslatef(0,0,speed);
	glMultMatrixd(postmat);
	break;
    case 'c':
	glLoadMatrixd(premat);
	glTranslatef(0,0,-speed);
	glMultMatrixd(postmat);
	break;
    case 'a':
	glLoadMatrixd(premat);
	glTranslatef(speed,0,0);
	glMultMatrixd(postmat);
	break;
    case 's':
	glLoadMatrixd(premat);
	glTranslatef(-speed,0,0);
	glMultMatrixd(postmat);
	break;
    case 'f':
	glLoadMatrixd(premat);
	glTranslatef(0,-speed,0);
	glMultMatrixd(postmat);
	break;
    case 'v':
	glLoadMatrixd(premat);
	glTranslatef(0,speed,0);
	glMultMatrixd(postmat);
	break;
    case '4':
	glLoadMatrixd(premat);
	glRotatef(rspeed,0,-1,0);
	glMultMatrixd(postmat);
	break;
    case '6':
	glLoadMatrixd(premat);
	glRotatef(rspeed,0,1,0);
	glMultMatrixd(postmat);
	break;
    case '8':
	glLoadMatrixd(premat);
	glRotatef(rspeed,1,0,0);
	glMultMatrixd(postmat);
	break;
    case '5':
    case '2':
	glLoadMatrixd(premat);
	glRotatef(rspeed,-1,0,0);
	glMultMatrixd(postmat);
	break;
    case '7':
	glLoadMatrixd(premat);
	glRotatef(rspeed,0,0,-1);
	glMultMatrixd(postmat);
	break;
    case '9':
	glLoadMatrixd(premat);
	glRotatef(rspeed,0,0,1);
	glMultMatrixd(postmat);
	break;
	  
	  


	  

    default:
	break;
    }
    glutPostRedisplay();
}


void mouse (int button, int state, int x, int y){
	
    static int ox,oy;
    static double scale = 0.01;	
    static double rscale = 0.01;

    y = -y;

    printf("button=%d, state=%d, (%d,%d)\n",button,state,x,y);

    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
	ox = x;
	oy = y;	
    }
    if(button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
	GLdouble mat[16];
	  
	if(rotcenter < pts.size() && slide == rot) {
	    GLdouble matr[16];
	    GLdouble matt1[16] = {1,0,0,0,0,1,0,0,0,0,1,0,pts[rotcenter].x,pts[rotcenter].y,pts[rotcenter].z };
	    GLdouble matt2[16] = {1,0,0,0,0,1,0,0,0,0,1,0,-pts[rotcenter].x,-pts[rotcenter].y,-pts[rotcenter].z };
	    matrotate(matr,(y-oy) *rscale, 1,0,0);
	    matmultiply(mat,matt1,matr);
	    matrotate(matr,(x-ox) *rscale, 0,1,0);
	    matmultiply(mat,mat,matr);
	    matmultiply(mat,mat,matt2);
	}
	for(vector<int>::iterator i = activepts.begin(); i != activepts.end(); i++) {
	    switch( slide ) {
	    case pxy:
		pts[*i].x += (x-ox) *scale;
		pts[*i].y += (y-oy) *scale;
		break;
	    case pyz:
		pts[*i].y += (x-ox) *scale;
		pts[*i].z += (y-oy) *scale;
		break;
	    case pzx:
		pts[*i].z += (x-ox) *scale;
		pts[*i].x += (y-oy) *scale;
		break;
	    case rot:
		transform(&pts[*i],&pts[*i],mat);
		break;
	    }
	       
	}
	glutPostRedisplay();
    }
}


int main(int argc, char** argv)
{
    polyrep = makepolyrep();//(pts[0],pts[1],pts[2],pts[3],pts[4]);
    polyrepntri = polyrep.ntri;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(250, 250);
    glutInitWindowPosition(100, 100);
    glutCreateWindow(argv[0]);
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMainLoop();
    return 0; 
}
