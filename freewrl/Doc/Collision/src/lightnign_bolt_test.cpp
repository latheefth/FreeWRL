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
int facemask = 7;
int rotcenter = 0;

#define DEBUGPTS
#include "Collision.cpp"

struct VRML_PolyRep polyrep;

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

VRML_PolyRep makepolyrep() {
static int cindex[66];
static float coord[39];
 int cindext[66] = {0,1,2,2,3,4,2,4,5,2,5,6,0,2,6,12,11,10,12,10,9,12,9,8,0,12,8,0,8,7,0,7,1,1,7,8,1,8,2,2,8,9,2,9,3,3,9,10,3,10,4,4,10,11,4,11,5,5,11,12,5,12,6,6,12,0};
 float coordt[39] = {0.000000,0.000000,0.000000,5.500000,5.000000,0.880000,4.000000,5.500000,0.968000,7.000000,8.000000,1.408000,4.000000,9.000000,1.584000,1.000000,5.000000,0.880000,2.500000,4.500000,0.792000,5.500000,5.000000,-0.880000,4.000000,5.500000,-0.968000,7.000000,8.000000,-1.408000,4.000000,9.000000,-1.584000,1.000000,5.000000,-0.880000,2.500000,4.500000,-0.792000};
VRML_PolyRep pr = {0,22,16,cindex,coord,NULL,NULL,NULL,NULL,NULL,NULL};
memcpy(cindex,cindext,sizeof(cindex));
memcpy(coord,coordt,sizeof(coord));
return pr; }

float clamp(float f) {
    f = f - (int)f;
    if(f < 0) f = -f;
//    if(f < 0.3) f += 0.5;
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
void getmatrix(GLdouble* mat,struct pt trans) {
mat[0] = 0.819553;
mat[1] = 0.053174;
mat[2] = -0.570529;
mat[3] = 0.000000;
mat[4] = -0.572030;
mat[5] = 0.133874;
mat[6] = -0.809233;
mat[7] = 0.000000;
mat[8] = 0.033350;
mat[9] = 0.989570;
mat[10] = 0.140134;
mat[11] = 0.000000;
mat[12] = 0.500680+trans.x;
mat[13] = -0.650880+trans.y;
mat[14] = 1.586645+trans.z;
mat[15] = 1.000000;
}
//COLLISION_IFS: ref0 (0.500680 -0.650880 1.586645) (0.117216 0.844841 0.247117)

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
void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GLUquadricObj* qo = gluNewQuadric();
    gluQuadricDrawStyle(qo,GLU_SILHOUETTE);

    double r = 0.25;
    double y1 = -2*1.6/3;
    double y2 = 1.6/3;
    double bradius;

    debugpts.clear();
    debugptsinfo.clear();
    debugsurface = 0;

    GLdouble mat[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    getmatrix(mat,pts[0]);

     
    polyrep = makepolyrep();//(pts[0],pts[1],pts[2],pts[3],pts[4]);
//    polyrep.ntri = min(polyrep.ntri,facemask);

    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    glPushMatrix();
    glMultMatrixd(mat);
    display_polyrep(polyrep);
    glPopMatrix();
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glPolygonMode(GL_BACK,GL_LINE);

    struct pt disp = polyrep_disp(y1,y2,y1,r,polyrep,mat,0);
    printf("disp = (%f,%f,%f)\n",disp.x,disp.y,disp.z);



    /* draw cylinder */
    glPushMatrix();
    glColor3f(0.4,0.4,0.4);
    glRotated(90,-1,0,0);
    glTranslatef(0,0,y1);
    gluCylinder(qo,r,r,y2-y1,32,1);
    glPopMatrix();


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
	facemask = (facemask-1)%8;
	cout << "facemask=" << facemask << endl;
	break;
    case '>':
	facemask = (facemask+1)%8;
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
