/*******************************************************************
 Copyright (C) 2005 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*****************************************

RenderFuncs.c - do scenegraph rendering.

******************************************/

/*
#define RENDERVERBOSE
*/

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <math.h>

#ifdef AQUA
#include <gl.h>
#include <glu.h>
#include <glext.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#endif

#include "Structs.h"
#include "headers.h"
#include "Polyrep.h"
#include "Collision.h"
#include "Viewer.h"
#include "LinearAlgebra.h"


struct X3D_Virt virt_Transform;

/* Rearrange to take advantage of headlight when off */
int curlight = 0;
int nlightcodes = 7;
int lightcode[7] = {
	GL_LIGHT1,
	GL_LIGHT2,
	GL_LIGHT3,
	GL_LIGHT4,
	GL_LIGHT5,
	GL_LIGHT6,
	GL_LIGHT7,
};
int nextlight() {
	if(curlight == nlightcodes) { return -1; }
	return lightcode[curlight++];
}

/* material node usage depends on texture depth; if rgb (depth1) we blend color field
   and diffusecolor with texture, else, we dont bother with material colors */
int last_texture_depth = 0;
float last_transparency = 0.0;

/* Sounds can come from AudioClip nodes, or from MovieTexture nodes. Different
   structures on these */
int sound_from_audioclip = 0;

/* and, we allow a maximum of so many pixels per texture */
/* if this is zero, first time a texture call is made, this is set to the OpenGL implementations max */
GLint global_texSize = 0;

/* for printing warnings about Sound node problems - only print once per invocation */
int soundWarned = FALSE;

int render_vp; /*set up the inverse viewmatrix of the viewpoint.*/
int render_geom;
int render_light;
int render_sensitive;
int render_blend;
int render_proximity;
int render_collision;

int display_status = 1;  /* display a status bar? */
int be_collision = 0;	/* do collision detection? */

int found_vp; /*true when viewpoint found*/

/* texture stuff - see code. Need array because of MultiTextures */
GLuint bound_textures[MAX_MULTITEXTURE];
int texture_count;

int	have_transparency;	/* did this Shape have transparent material? */
void *	this_textureTransform;  /* do we have some kind of textureTransform? */
int	lightingOn;		/* do we need to restore lighting in Shape? */
int	have_texture;		/* do we have a texture (And thus a push?) */
int	global_lineProperties;	/* line properties -width, etc			*/
int	global_fillProperties;	/* polygon fill properties - hatching, etc	*/
int	cullFace;		/* is GL_CULL_FACE enabled or disabled?		*/
int 	colorMaterialEnabled;	/* state of GL_COLOR_MATERIAL			*/


int smooth_normals = -1; /* -1 means, uninitialized */

int cur_hits=0;

/* Collision detection results */
struct sCollisionInfo CollisionInfo = { {0,0,0} , 0, 0. };

/* Displacement of viewer , used for colision calculation  PROTYPE, CURRENTLY UNUSED*/
struct pt ViewerDelta = {0,0,0};

/* dimentions of viewer, and "up" vector (for collision detection) */
struct sNaviInfo naviinfo = {0.25, 1.6, 0.75};

/* for alignment of collision cylinder, and gravity (later). */
struct pt ViewerUpvector = {0,0,0};

X3D_Viewer Viewer;


/* These two points define a ray in window coordinates */

struct pt r1 = {0,0,-1},r2 = {0,0,0},r3 = {0,1,0};
struct pt t_r1,t_r2,t_r3; /* transformed ray */
void *hypersensitive = 0; int hyperhit = 0;
struct pt hyper_r1,hyper_r2; /* Transformed ray for the hypersensitive node */

GLint viewport[4] = {-1,-1,2,2};

/* These three points define 1. hitpoint 2., 3. two different tangents
 * of the surface at hitpoint (to get transformation correctly */

/* All in window coordinates */

struct pt hp, ht1, ht2;
double hpdist; /* distance in ray: 0 = r1, 1 = r2, 2 = 2*r2-r1... */

/* used to save rayhit and hyperhit for later use by C functions */
struct SFColor hyp_save_posn, hyp_save_norm, ray_save_posn;

/* Any action for the Browser (perl code) to do? */
int BrowserAction = FALSE;
struct X3D_Anchor *AnchorsAnchor;


struct currayhit  rh,rph,rhhyper;
/* used to test new hits */

/* this is used to return the duration of an audioclip to the perl
side of things. SvPV et al. works, but need to figure out all
references, etc. to bypass this fudge JAS */
float AC_LastDuration[50]  = {-1.0,-1.0,-1.0,-1.0,-1.0,
				-1.0,-1.0,-1.0,-1.0,-1.0,
				-1.0,-1.0,-1.0,-1.0,-1.0,
				-1.0,-1.0,-1.0,-1.0,-1.0,
				-1.0,-1.0,-1.0,-1.0,-1.0,
				-1.0,-1.0,-1.0,-1.0,-1.0,
				-1.0,-1.0,-1.0,-1.0,-1.0,
				-1.0,-1.0,-1.0,-1.0,-1.0,
				-1.0,-1.0,-1.0,-1.0,-1.0,
				-1.0,-1.0,-1.0,-1.0,-1.0} ;

/* is the sound engine started yet? */
int SoundEngineStarted = FALSE;

/* stored FreeWRL version, pointers to initialize data */
char *BrowserVersion = NULL;
char *BrowserURL = NULL; 
char *BrowserFullPath = NULL;
char *BrowserName = "FreeWRL VRML/X3D Browser";
char *lastReadFile = NULL;

void *rootNode=0;	/* scene graph root node */

/*******************************************************************************/

/* Sub, rather than big macro... */
void rayhit(float rat, float cx,float cy,float cz, float nx,float ny,float nz,
float tx,float ty, char *descr)  {
	GLdouble modelMatrix[16];
	GLdouble projMatrix[16];

	/* Real rat-testing */
	#ifdef RENDERVERBOSE
		printf("RAY HIT %s! %f (%f %f %f) (%f %f %f)\n\tR: (%f %f %f) (%f %f %f)\n",
		descr, rat,cx,cy,cz,nx,ny,nz,
		t_r1.x, t_r1.y, t_r1.z,
		t_r2.x, t_r2.y, t_r2.z
		);
	#endif

	if(rat<0 || (rat>hpdist && hpdist >= 0)) {
		return;
	}
	fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	fwGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	gluProject(cx,cy,cz, modelMatrix, projMatrix, viewport,
		&hp.x, &hp.y, &hp.z);
	hpdist = rat;
	rh=rph;
	rhhyper=rph;
	#ifdef RENDERVERBOSE 
		printf ("Rayhit, hp.x y z: - %f %f %f rat %f hpdist %f\n",hp.x,hp.y,hp.z, rat, hpdist);
	#endif
}

/* Call this when modelview and projection modified */
void upd_ray() {
	GLdouble modelMatrix[16];
	GLdouble projMatrix[16];
	fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	fwGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	gluUnProject(r1.x,r1.y,r1.z,modelMatrix,projMatrix,viewport,
		&t_r1.x,&t_r1.y,&t_r1.z);
	gluUnProject(r2.x,r2.y,r2.z,modelMatrix,projMatrix,viewport,
		&t_r2.x,&t_r2.y,&t_r2.z);
	gluUnProject(r3.x,r3.y,r3.z,modelMatrix,projMatrix,viewport,
		&t_r3.x,&t_r3.y,&t_r3.z);
/*	printf("Upd_ray: (%f %f %f)->(%f %f %f) == (%f %f %f)->(%f %f %f)\n",
		r1.x,r1.y,r1.z,r2.x,r2.y,r2.z,
		t_r1.x,t_r1.y,t_r1.z,t_r2.x,t_r2.y,t_r2.z);
*/
}


/* if a node changes, void the display lists */
/* Courtesy of Jochen Hoenicke */

void update_node(void *ptr) {
	struct X3D_Box *p;
	int i;

	p = (struct X3D_Box*) ptr;

	/* printf ("update_node for %d %s\n",ptr, stringNodeType(p->_nodeType)); */

	p->_change ++;
	for (i = 0; i < p->_nparents; i++) {
		update_node((void *)p->_parents[i]);
	}
}

/*explicit declaration. Needed for Collision_Child*/
void Group_Child(void *nod_);

/*********************************************************************
 *********************************************************************
 *
 * render_node : call the correct virtual functions to render the node
 * depending on what we are doing right now.
 */

void render_node(void *node) {
	struct X3D_Virt *v;
	struct X3D_Box *p;
	int srg = 0;
	int sch = 0;
	struct currayhit srh;
	#ifdef GLERRORS
	int glerror = GL_NONE;
	char* stage = "";
	#endif

	#ifdef RENDERVERBOSE
		printf("\nRender_node %u\n",(unsigned int) node);
	#endif

	if(!node) {return;}
	v = *(struct X3D_Virt **)node;
	p = (struct X3D_Box *)node;

	#ifdef RENDERVERBOSE 
	    printf("=========================================NODE RENDERED===================================================\n");
	printf ("node %d %d\n",p,v);
	printf ("nodename %s\n",stringNodeType(p->_nodeType));
return;
	    printf("Render_node_v %d (%s) PREP: %d REND: %d CH: %d FIN: %d RAY: %d HYP: %d\n",v,
		   stringNodeType(p->_nodeType),
		   v->prep,
		   v->rend,
		   v->children,
		   v->fin,
		   v->rendray,
		   hypersensitive);
	    printf("Render_state geom %d light %d sens %d\n",
		   render_geom,
		   render_light,
		   render_sensitive);
	    printf ("pchange %d pichange %d vchanged %d\n",p->_change, p->_ichange,v->changed);
	#endif

        /* we found viewpoint on render_vp pass, stop exploring tree.. */
        if(render_vp && found_vp) return;

	if(p->_change != p->_ichange && v->changed)
	  {
	    #ifdef RENDERVERBOSE 
		printf ("rs 1 pch %d pich %d vch %d\n",p->_change,p->_ichange,v->changed);
	    #endif
	    v->changed(node);
	    p->_ichange = p->_change;
	    #ifdef GLERRORS
	    if(glerror == GL_NONE && ((glerror = glGetError()) != GL_NONE) ) stage = "change";
	    #endif
	  }

	if(v->prep)
	  {
	    #ifdef RENDERVERBOSE 
		printf ("rs 2\n");
	    #endif

	    v->prep(node);
	    if(render_sensitive && !hypersensitive)
	      {
		upd_ray();
	      }
	      #ifdef GLERRORS
	    if(glerror == GL_NONE && ((glerror = glGetError()) != GL_NONE) ) stage = "prep";
	    #endif
	  }

	if(render_proximity && v->proximity)
	{
	    #ifdef RENDERVERBOSE 
		printf ("rs 2a\n");
	    #endif
	    v->proximity(node);
	    #ifdef GLERRORS
	    if(glerror == GL_NONE && ((glerror = glGetError()) != GL_NONE) ) stage = "render_proximity";
	    #endif
	}

	if(render_collision && v->collision)
	{
	    #ifdef RENDERVERBOSE 
		printf ("rs 2b\n");
	    #endif

	    v->collision(node);
	    #ifdef GLERRORS
	    if(glerror == GL_NONE && ((glerror = glGetError()) != GL_NONE) ) stage = "render_collision";
	    #endif
	}

	if(render_geom && !render_sensitive && v->rend)
	  {
	    #ifdef RENDERVERBOSE 
		printf ("rs 3\n");
	    #endif

	    v->rend(node);
	    #ifdef GLERRORS
	    if(glerror == GL_NONE && ((glerror = glGetError()) != GL_NONE) ) stage = "render_geom";
	    #endif
	  }
	if(render_light && v->light)
	  {
	    #ifdef RENDERVERBOSE 
		printf ("rs 4\n");
	    #endif

	    v->light(node);
	    #ifdef GLERRORS
	    if(glerror == GL_NONE && ((glerror = glGetError()) != GL_NONE) ) stage = "render_light";
	    #endif
	  }
	/* Future optimization: when doing VP/Lights, do only
	 * that child... further in future: could just calculate
	 * transforms myself..
	 */
	/* if(render_sensitive && (p->_renderFlags & VF_Sensitive)) */
	if(render_sensitive && p->_sens)
	  {
	    #ifdef RENDERVERBOSE 
		printf ("rs 5\n");
	    #endif

	    srg = render_geom;
	    render_geom = 1;
	    #ifdef RENDERVERBOSE 
		printf("CH1 %d: %d\n",node, cur_hits, p->_hit);
	    #endif

	    sch = cur_hits;
	    cur_hits = 0;
	    /* HP */
	      srh = rph;
	    rph.node = node;
	    fwGetDoublev(GL_MODELVIEW_MATRIX, rph.modelMatrix);
	    fwGetDoublev(GL_PROJECTION_MATRIX, rph.projMatrix);
	    #ifdef GLERRORS
	    if(glerror == GL_NONE && ((glerror = glGetError()) != GL_NONE) ) stage = "render_sensitive";
	    #endif

	  }
	if(render_geom && render_sensitive && !hypersensitive && v->rendray)
	  {
	    #ifdef RENDERVERBOSE 
		printf ("rs 6\n");
	    #endif

	    v->rendray(node);
	    #ifdef GLERRORS
	    if(glerror == GL_NONE && ((glerror = glGetError()) != GL_NONE) ) stage = "rs 6";
	    #endif
	  }


        if((render_sensitive) && (hypersensitive == node)) {
            #ifdef RENDERVERBOSE 
		printf ("rs 7\n");
	    #endif

            hyper_r1 = t_r1;
            hyper_r2 = t_r2;
            hyperhit = 1;
        }
        if(v->children) {
            #ifdef RENDERVERBOSE 
		printf ("rs 8\n");
	    #endif

            v->children(node);
	    #ifdef GLERRORS
	    if(glerror == GL_NONE && ((glerror = glGetError()) != GL_NONE) ) stage = "children";
	    #endif
        }

	/* if(render_sensitive && (p->_renderFlags & VF_Sensitive)) */
	if(render_sensitive && p->_sens)
	  {
	    #ifdef RENDERVERBOSE 
		printf ("rs 9\n");
	    #endif

	    render_geom = srg;
	    cur_hits = sch;
	    #ifdef RENDERVERBOSE 
		printf("CH3: %d %d\n",cur_hits, p->_hit);
	    #endif

	    /* HP */
	      rph = srh;
	  }
	if(v->fin)
	  {
	    #ifdef RENDERVERBOSE 
		printf ("rs A\n");
	    #endif

	    v->fin(node);
	    if(render_sensitive && v == &virt_Transform)
	      {
		upd_ray();
	      }
	    #ifdef GLERRORS
	    if(glerror != GL_NONE && ((glerror = glGetError()) != GL_NONE) ) stage = "fin";
	    #endif
	  }
	#ifdef RENDERVERBOSE 
		printf("(end render_node)\n");
	#endif


	#ifdef GLERRORS
	if(glerror != GL_NONE)
	  {
	    printf("============== GLERROR : %s in stage %s =============\n",gluErrorString(glerror),stage);
	    printf("Render_node_v %d (%s) PREP: %d REND: %d CH: %d FIN: %d RAY: %d HYP: %d\n",v,
		   stringNodeType(p->_nodeType),
		   v->prep,
		   v->rend,
		   v->children,
		   v->fin,
		   v->rendray,
		   hypersensitive);
	    printf("Render_state geom %d light %d sens %d\n",
		   render_geom,
		   render_light,
		   render_sensitive);
	    printf ("pchange %d pichange %d vchanged %d\n",p->_change, p->_ichange,v->changed);
	    printf("==============\n");
	  }
	  #endif

}

/*
 * The following code handles keeping track of the parents of a given
 * node. This enables us to traverse the scene on C level for optimizations.
 *
 * We use this array code because VRML nodes usually don't have
 * hundreds of children and don't usually shuffle them too much.
 */

void add_parent(void *node_, void *parent_) {
	struct X3D_Box *node;
	struct X3D_Box *parent;
	if(!node_) return;

	node = (struct X3D_Box *)node_;
	parent = (struct X3D_Box *)parent_;

	#ifdef CHILDVERBOSE
	printf ("adding node %d (%s) to parent %d (%s)\n",node, stringNodeType(node->_nodeType), 
			parent, stringNodeType(parent->_nodeType));
	#endif
 
	parent->_renderFlags = parent->_renderFlags | node->_renderFlags;

	node->_nparents ++;
	if(node->_nparents > node->_nparalloc) {
		node->_nparalloc += 10;
		if (node->_parents == NULL)  {
			node->_parents = (void **)malloc(sizeof(node->_parents[0])* node->_nparalloc) ;
		} else {
		node->_parents =
			(void **)realloc(node->_parents, sizeof(node->_parents[0])*
							node->_nparalloc) ;
		}
	}
	node->_parents[node->_nparents-1] = parent_;
}


void remove_parent(void *node_, void *parent_) {
	struct X3D_Box *node;
	struct X3D_Box *parent;
	int i;
	if(!node_) return;
	node = (struct X3D_Box *)node_;
	parent = (struct X3D_Box *)parent_;
	node->_nparents --;
	for(i=0; i<node->_nparents; i++) {
		if(node->_parents[i] == parent) {
			break;
		}
	}
	for(; i<node->_nparents; i++) {
		node->_parents[i] = node->_parents[i+1];
	}
}

void
render_hier(void *p, int rwhat)
{
	struct pt upvec = {0,1,0};
	GLdouble modelMatrix[16];
	#define XXXrender_pre_profile
	#ifdef render_pre_profile
	/*  profile */
	double xx,yy,zz,aa,bb,cc,dd,ee,ff;
	struct timeval mytime;
	struct timezone tz; /* unused see man gettimeofday */
	#endif


	render_vp = rwhat & VF_Viewpoint;
	found_vp = 0;
	render_geom =  rwhat & VF_Geom;
	render_light = rwhat & VF_Lights;
	render_sensitive = rwhat & VF_Sensitive;
	render_blend = rwhat & VF_Blend;
	render_proximity = rwhat & VF_Proximity;
	render_collision = rwhat & VF_Collision;
	curlight = 0;
	hpdist = -1;


	#ifdef render_pre_profile
	if (render_geom) {
		gettimeofday (&mytime,&tz);
		aa = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;
	}
	#endif

	/*printf ("render_hier vp %d geom %d light %d sens %d blend %d prox %d col %d\n",
	render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision); */

	if (!p) {
		/* we have no geometry yet, sleep for a tiny bit */
		usleep(1000);
		return;
	}

	#ifdef RENDERVERBOSE
  		printf("Render_hier node=%d what=%d\n", p, rwhat);
	#endif

	/* status bar */
	if (render_geom) {
		if (display_status) {
			render_status();
		}
	}


	#ifdef render_pre_profile
	if (render_geom) {
		gettimeofday (&mytime,&tz);
		bb = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;
	}
	#endif

	if (render_sensitive) {
		upd_ray();
	}

	#ifdef render_pre_profile
	if (render_geom) {
		gettimeofday (&mytime,&tz);
		cc = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;
	}
	#endif

	render_node(p);

	#ifdef render_pre_profile
	if (render_geom) {
		gettimeofday (&mytime,&tz);
		dd = (double)mytime.tv_sec+(double)mytime.tv_usec/1000000.0;
		printf ("render_geom status %f ray %f geom %f\n",bb-aa, cc-bb, dd-cc);
	}
	#endif


	/*get viewpoint result, only for upvector*/
	if (render_vp &&
		ViewerUpvector.x == 0 &&
		ViewerUpvector.y == 0 &&
		ViewerUpvector.z == 0) {

		/* store up vector for gravity and collision detection */
		/* naviinfo.reset_upvec is set to 1 after a viewpoint change */
		fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
		matinverse(modelMatrix,modelMatrix);
		transform3x3(&ViewerUpvector,&upvec,modelMatrix);

		#ifdef RENDERVERBOSE 
		printf("ViewerUpvector = (%f,%f,%f)\n", ViewerUpvector);
		#endif

	}
}
