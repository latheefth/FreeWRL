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
int facemask = 0;
//#define DEBUGFACEMASK
#define DEBUGPTS

#include "Collision.cpp"

struct pt  iv, jv, kv;

enum eslideplane {pxy=0,pyz=1,pzx=2};

eslideplane slide = pxy;
bool perspective = true;




void drawbox(struct pt p0, struct pt i, struct pt j, struct pt k) {
     struct pt p[8] = {p0,p0,p0,p0,p0,p0,p0,p0};
     struct pt n[6];
     struct pt mindispv = pt(0,0,0);
     double mindisp = 1E99;
     struct pt middle;
     static const int faces[6][4] = {
	  {0,2,7,1},
	  {0,3,6,2},
	  {0,1,5,3},
	  {4,6,3,5},
	  {4,5,1,7},
	  {4,7,2,6}
     };

     VECADD(p[1],i);
     VECADD(p[2],j);
     VECADD(p[3],k);
     VECADD(p[4],i); VECADD(p[4],j); VECADD(p[4],k); //p[4]= i+j+k
     VECADD(p[5],k); VECADD(p[5],i); //p[6]= k+i
     VECADD(p[6],j); VECADD(p[6],k); //p[5]= j+k
     VECADD(p[7],i); VECADD(p[7],j); //p[7]= i+j

     veccross(&n[0],j,i);
     veccross(&n[1],k,j);
     veccross(&n[2],i,k);
     vecscale(&n[3],&n[0],-1.);
     vecscale(&n[4],&n[1],-1.);
     vecscale(&n[5],&n[2],-1.);

     glBegin(GL_QUADS);
     for(int i = 0; i < 6; i++) {
	  glColor4f((i+5)/10.0,0,0,0.5);
	  glVertex3dv(&p[faces[i][0]].x);
	  glVertex3dv(&p[faces[i][1]].x);
	  glVertex3dv(&p[faces[i][2]].x);
	  glVertex3dv(&p[faces[i][3]].x);
     }
     glEnd();
}



void init(void)
{

     glClearColor (0.0, 0.0, 0.0, 0.0);
     glShadeModel(GL_FLAT);
     glEnable(GL_DEPTH_TEST);
     glEnable(GL_POINT_SMOOTH);

     glEnable(GL_CULL_FACE);
     glCullFace(GL_BACK);

     glPointSize(4.0);

     pts.push_back(pt(-1,0,1));
     pts.push_back(pt(1,1,1));
     pts.push_back(pt(1,0,1));
     pts.push_back(pt(0,0,0));
     pts.push_back(pt(0,0,0));


/* closest to y axis proj regression test * *****

//COLLISION_BOX: (-1.560430 -1.552213 -3.122532) (-0.024862 0.180540 0.070708)
//iv=(1.926152 0.404462 -0.355458) jv=(-0.254369 1.847146 0.723427) kv=(0.474591 -0.651507 1.830383)
     pts[0] = pt(-1.560430, -1.552213, -3.122532);
     i = pt(1.926152, 0.404462, -0.355458);
     j = pt(-0.254369, 1.847146, 0.723427);
     k = pt(0.474591, -0.651507, 1.830383);
//COLLISION_BOX: (0.342769 2.546072 -1.749308) (0.003578 -0.012258 0.001437)
//iv=(-0.834273 -0.449940 -1.761109) jv=(0.556846 -1.907853 0.223643) kv=(-1.730281 -0.397049 0.921077)
     pts[0] = pt(0.342769, 2.546072, -1.749308);
     iv = pt(-0.834273, -0.449940, -1.761109);
     jv = pt(0.556846, -1.907853, 0.223643);
     kv = pt(-1.730281, -0.397049, 0.921077);
//COLLISION_BOX: (-0.471929 -1.895799 -0.544501) (0.000000 -0.000000 0.000000)
//iv=(0.358616 -0.926360 -1.735859) jv=(-1.348217 1.169415 -0.902616) kv=(1.433059 1.332015 -0.414786)
     pts[0] = pt(-0.471929, -1.895799, -0.544501);
     iv = pt(0.358616, -0.926360, -1.735859);
     jv = pt(-1.348217, 1.169415, -0.902616);
     kv = pt(1.433059, 1.332015, -0.414786);

//COLLISION_BOX: (-0.918421 -1.829499 -3.330159) (-0.000000 1.503833 -0.000000)
//iv=(0.048967 0.000000 -1.999400) jv=(0.000000 1.999999 0.000000) kv=(1.999400 0.000000 0.048967)
     pts[0] = pt(-0.918421, -1.829499, -3.330159);
     iv = pt(0.048967, 0.000000, -1.999400);
     jv = pt(0.000000, 1.999999, 0.000000);
     kv = pt(1.999400, 0.000000, 0.048967);
******/

/*     pts[0] = pt(0.9, -2, -2);
     iv = pt(4,0,0);
     jv = pt(0,4,0);
     kv = pt(0,0,4);*/


//COLLISION_BOX: (-0.929302 -1.407500 -0.989273) (-0.000000 1.925833 0.000000)
//iv=(-0.006418 0.000000 -1.999990) jv=(0.000000 2.000000 0.000000) kv=(1.999990 0.000000 -0.006418)
     pts[0] = pt(-0.929302, -1.407500, -0.989273);
     iv = pt(-0.006418, 0.000000, -1.999990);
     jv = pt(0.000000, 2.000000, 0.000000);
     kv = pt(1.999990, 0.000000, -0.006418);

     pts[1] = pts[2] = pts[3] = pts[0];
     VECADD(pts[1],iv);
     VECADD(pts[2],jv);
     VECADD(pts[3],kv);

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





void display(void)
{
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
     GLUquadricObj* qo = gluNewQuadric();
     gluQuadricDrawStyle(qo,GLU_SILHOUETTE);

    double r = 1;
    double y1 = -2*2./3;
    double y2 = 2./3;

/*     glBegin(GL_TRIANGLES);
     glColor3f(1,1,1);
     for(int i=0; i < pts.size(); i++) {
	  glVertex3dv(&pts[i].x);
     }

     glEnd();*/


     debugpts.clear();
     debugptsinfo.clear();
     debugsurface = 0;




     VECDIFF(pts[1],pts[0],iv);
     VECDIFF(pts[2],pts[0],jv);
     VECDIFF(pts[3],pts[0],kv);
     double len = sqrt(vecdot(&kv,&kv));
     veccross(&kv,iv,jv);
     vecnormal(&kv,&kv);
     vecscale(&kv,&kv,len);

     debugsurface = 0;
     pts[4] = box_disp(y1,y2,y1,r,pts[0],iv,jv,kv);

     printf("box_disp() = (%f,%f,%f)\n",pts[4].x,pts[4].y,pts[4].z);
     printf("debugptsnum = %d\n",debugpts.size());
     if(debugpt < debugpts.size()) printf("debugpt=%d : (%f,%f,%f)\n",debugpt,debugpts[debugpt].x,debugpts[debugpt].y,debugpts[debugpt].z);
/*     for(int i = 0; i < debugptsinfo.size(); i++) {
	  printf("%d : %s\n",i, debugptsinfo[i].c_str());
	  }*/

     //descendant vector
/*     glBegin(GL_LINES);
     glColor3f(.0,.7,.1);
     glVertex3d(0,0,0);
     glVertex3dv(&pts[5].x);
     glEnd();
*/

     //descending line
     glBegin(GL_LINES);
     glColor3f(.5,.5,.5);
     glVertex3dv(&pts[pts.size()-1].x);
     glVertex3dv(&pts[pts.size()-2].x);
     glEnd();

     /*   //min/maxsect
     glPushMatrix();
     glColor3f(0.4,0.4,0.);
     glRotated(90,-1,0,0);
     glTranslatef(0,0,pts[pts.size()-2].y);
     gluCylinder(qo,r,r,pts[pts.size()-1].y - pts[pts.size()-2].y,16,1);
     glPopMatrix();



     glBegin(GL_LINES);
     glColor3f(.0,.7,.7);
     glVertex3d(0,0,0);
     glVertex3dv(&pts[3].x);
     glEnd();
*/




     /*draw box*/
     glPolygonMode(GL_FRONT,GL_LINE);
     drawbox(pts[0],iv,jv,kv);
     glPolygonMode(GL_FRONT,GL_FILL);

     /*draw translated box*/
     struct pt tp = pts[0];
     VECADD(tp,pts[4]);



     /* draw translated tri */
/*     glBegin(GL_LINE_STRIP);
     glColor3f(.8,0,.8);
     glVertex3d(pts[1].x+pts[4].x,pts[1].y+pts[4].y,pts[1].z+pts[4].z);
     glVertex3d(pts[0].x+pts[4].x,pts[0].y+pts[4].y,pts[0].z+pts[4].z);
     glVertex3d(pts[2].x+pts[4].x,pts[2].y+pts[4].y,pts[2].z+pts[4].z);
//     glColor3f(.4,.4,.4);
     glVertex3d(pts[1].x+pts[4].x,pts[1].y+pts[4].y,pts[1].z+pts[4].z);
     glEnd();
*/

     /* draw cylinder */
     glPushMatrix();
     glColor3f(0.4,0.4,0.4);
     glRotated(90,-1,0,0);
     glTranslatef(0,0,y1);
     gluCylinder(qo,r,r,y2-y1,32,1);
     glPopMatrix();

     /* draw tri */
/*     glBegin(GL_LINE_STRIP);
     glColor3f(.6,.6,.6);
     glVertex3dv(&pts[1].x);
     glVertex3dv(&pts[0].x);
     glVertex3dv(&pts[2].x);
//     glColor3f(.4,.4,.4);
     glVertex3dv(&pts[1].x);
     glEnd();

*/
     displayhandles();
     displayaxis();

     glEnable(GL_BLEND);
//     glDisable(GL_DEPTH_TEST);
     glBlendFunc(GL_SRC_ALPHA,GL_ONE);
     drawbox(tp,iv,jv,kv);
     glDisable(GL_BLEND);
//     glEnable(GL_DEPTH_TEST);


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
	  break;
     case ']':
	  debugpt = (debugpt+1)%debugpts.size();
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
	  slide = eslideplane(((int)(slide) +1) % 3);
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
	  }
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

     y = -y;

     printf("button=%d, state=%d, (%d,%d)\n",button,state,x,y);

     if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
	  ox = x;
	  oy = y;
     }
     if(button == GLUT_LEFT_BUTTON && state == GLUT_UP) {

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
