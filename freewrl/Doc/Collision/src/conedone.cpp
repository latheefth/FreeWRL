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

#define DEBUGPTS
#include "Collision.cpp"


struct pt  iv, jv, kv;

enum eslideplane {pxy=0,pyz=1,pzx=2};

eslideplane slide = pxy;
bool perspective = true;



void init(void)
{    

     glClearColor (0.0, 0.0, 0.0, 0.0);
     glShadeModel(GL_FLAT);
     glEnable(GL_DEPTH_TEST);
     glEnable(GL_POINT_SMOOTH);

     glEnable(GL_CULL_FACE);
     glCullFace(GL_BACK);

     glPointSize(4.0);

     pts.push_back(pt(1,-1,1));
     pts.push_back(pt(1,1,1));
     pts.push_back(pt(0,-1,0));
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



void display(void)
{
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
     GLUquadricObj* qo = gluNewQuadric();
     gluQuadricDrawStyle(qo,GLU_SILHOUETTE);

     double r = 1;
     double bradius;

     debugpts.clear();
     debugptsinfo.clear();
     debugsurface = 0;

     
     
     struct pt tmp;
     struct pt i,j;
     struct pt n;
     VECDIFF(pts[1],pts[0],n);

     make_orthogonal_vector_space(&i, &j, n);

     VECDIFF(pts[2],pts[0],tmp);
     bradius = 0.5; //sqrt(vecdot(&tmp,&tmp));


/*     VECDIFF(pts[1],pts[0],i);
     VECDIFF(pts[2],pts[0],n);
     vecscale(&j,&i,- vecdot(&i,&n)/vecdot(&i,&i));
     VECADD(n,j);
     vecnormal(&n,&n);
     pts[3] = pts[0];
     VECADD(pts[3],n);*/

     struct pt disp;
     disp = cone_disp(-1,1,-1,r,pts[0],pts[1],bradius);
     printf("disp = (%f,%f,%f)\n",disp.x,disp.y,disp.z);
    

     /* draw cylinder */
     glPushMatrix();
     glColor3f(0.4,0.4,0.4);
     glRotated(90,-1,0,0);
     glTranslatef(0,0,-1);
     gluCylinder(qo,r,r,2,32,1);
     glPopMatrix();

     /* draw cone */
     GLdouble mat[16];
     GLdouble matinv[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
     glPushMatrix();
     glPushMatrix();
     glColor3f(0.7,0.,0.);
     glLoadIdentity();
     gluLookAt(
	       pts[0].x+disp.x,pts[0].y+disp.y,pts[0].z+disp.z,
	       pts[1].x+disp.x,pts[1].y+disp.y,pts[1].z+disp.z,
	       i.x,i.y,i.z);
     glGetDoublev(GL_MODELVIEW_MATRIX, mat);
     matinverse(matinv,mat);
     glPopMatrix();
     glMultMatrixd(matinv);
     glRotated(180,1,0,0);

     gluCylinder(qo,bradius,0,sqrt(vecdot(&n,&n)),16,1);
     glPopMatrix();
     
	

     displayhandles();
     displayaxis();

/*  draw transparent result 
     glEnable(GL_BLEND);
//     glDisable(GL_DEPTH_TEST);
     glBlendFunc(GL_SRC_ALPHA,GL_ONE);
     

     glDisable(GL_BLEND);
//     glEnable(GL_DEPTH_TEST);

*/
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
