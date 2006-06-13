/*******************************************************************
 Copyright (C) 2005 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*****************************************

RenderFuncs.c - do scenegraph rendering.

******************************************/

#include <math.h>

#include "headers.h"
#include "Polyrep.h"
#include "Collision.h"
#include "Viewer.h"
#include "LinearAlgebra.h"
#include "SensInterps.h"


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
int textures_take_priority = TRUE;
int useShapeThreadIfPossible = TRUE;

/* for printing warnings about Sound node problems - only print once per invocation */
int soundWarned = FALSE;

int render_vp; /*set up the inverse viewmatrix of the viewpoint.*/
int render_geom;
int render_light;
int render_sensitive;
int render_blend;
int render_proximity;
int render_collision;

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

int     shutterGlasses = 0; 	/* stereo shutter glasses */


GLint smooth_normals = TRUE; /* do normal generation? */

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
void *empty_group=0;

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

	/* printf ("update_node for %d %s nparents %d\n",ptr, stringNodeType(p->_nodeType),p->_nparents); */

	p->_change ++;
	for (i = 0; i < p->_nparents; i++) {
		void *n = (void *)p->_parents[i];
		if(n == p) {
		    fprintf(stderr, "Error: self-referential node structure! (node:'%s')\n", stringNodeType(p->_nodeType));
		    p->_parents[i] = empty_group;
		} else if( n != 0 ) {
		    update_node(n);
		}
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
	GLint glerror = GL_NONE;
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

	/* call the "changed_" function */
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
	int oldparcount;

	if(!node_) return;

	node = (struct X3D_Box *)node_;
	parent = (struct X3D_Box *)parent_;

	#ifdef CHILDVERBOSE
	printf ("adding node %d (%s) to parent %d (%s)\n",node, stringNodeType(node->_nodeType), 
			parent, stringNodeType(parent->_nodeType));
	#endif
 
	parent->_renderFlags = parent->_renderFlags | node->_renderFlags;

	oldparcount = node->_nparents;
	if((oldparcount+1) > node->_nparalloc) {
		node->_nparents = 0; /* for possible threading issues */
		node->_nparalloc += 10;
		if (node->_parents == NULL)  {
			node->_parents = (void **)malloc(sizeof(node->_parents[0])* node->_nparalloc) ;
		} else {
		node->_parents =
			(void **)realloc(node->_parents, sizeof(node->_parents[0])*
							node->_nparalloc) ;
		}
	}
	node->_parents[oldparcount] = parent_;
	node->_nparents = oldparcount+1;
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

/* handle setting shutter from parameters */
void setShutter (void) {
        shutterGlasses = 1;
}

/******************************************************************************
 *
 * shape compiler "thread"
 *
 ******************************************************************************/
#ifdef DO_MULTI_OPENGL_THREADS

/* threading variables for loading shapes in threads */
static pthread_t shapeThread;
static pthread_mutex_t shapeMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t shapeCond   = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t shapeGenMutex = PTHREAD_MUTEX_INITIALIZER;
#define SLOCK           pthread_mutex_lock(&shapeMutex);
#define SUNLOCK         pthread_mutex_unlock(&shapeMutex);
#define S_LOCK_SIGNAL   pthread_cond_signal(&shapeCond);
#define S_LOCK_WAIT     pthread_cond_wait(&shapeCond,&shapeMutex);
/* lock the reallocs of data structures */
#define REGENLOCK       pthread_mutex_lock(&shapeGenMutex);
#define REGENUNLOCK     pthread_mutex_unlock(&shapeGenMutex);

/* are we currently active? */
int shapeCompiling = FALSE;

int CompileThreadInitialized = FALSE;
static void (*shapemethodptr)(void *, void *, void *, void *, void *); 	/* method used to compile this node 	*/
static void *shapenodeptr;		/* node pointer of node data		*/
static void *shapecoord;		/* Polrep shape coord node		*/
static void *shapecolor;		/* Polyrep shape color node		*/
static void *shapenormal;		/* Polyrep shape normal node		*/
static void *shapetexCoord;		/* Polyrep shape tex coord node		*/

void _shapeCompileThread () {
	/* we wait forever for the data signal to be sent */
	for (;;) {
		SLOCK
		CompileThreadInitialized = TRUE;
		S_LOCK_WAIT
		shapeCompiling = TRUE;
		/* printf ("shapethread compiling\n"); */

		/* so, lets do the compile */
		shapemethodptr(shapenodeptr, shapecoord, shapecolor, shapenormal, shapetexCoord);

		shapeCompiling = FALSE;
		SUNLOCK
	}
}


void initializeShapeCompileThread() {
	int iret;
        iret = pthread_create(&shapeThread, NULL, (void *(*)(void *))&_shapeCompileThread, NULL);
}

#endif

int isShapeCompilerParsing() {
	#ifdef DO_MULTI_OPENGL_THREADS
	return shapeCompiling;
	#else
	return FALSE;
	#endif
}

void compileNode (void (*nodefn)(void *, void *, void *, void *, void *), void *node, void *coord, void *color, void *normal, void *texCoord) {
	/* check to see if textures are being parsed right now */

	/* give textures priority over node compiling */
	if (textures_take_priority) {
		if (isTextureParsing()==TRUE) {
			/* printf ("compileNode, textures parsing, returning\n"); */
			return;
		}
	}

	#ifdef DO_MULTI_OPENGL_THREADS

	/* do we want to use a seperate thread for compiling shapes, or THIS thread? */
	if (useShapeThreadIfPossible) {
		if (!shapeCompiling) {
			if (!CompileThreadInitialized) return; /* still starting up */
	
	
			/* lock for exclusive thread access */
	        	SLOCK
	
			/* copy our params over */
			shapemethodptr = nodefn;
			shapenodeptr = node;
			shapecoord = coord;
			shapecolor = color;
			shapenormal = normal;
			shapetexCoord = texCoord;
			/* signal to the shape compiler thread that there is data here */
			S_LOCK_SIGNAL
	        	SUNLOCK
			
		}
		sched_yield();
	} else {
		/* ok, we do not want to use the shape compile thread, just do it */
		nodefn(node, coord, color, normal, texCoord);
	}

	#else
	/* ok, we cant do a shape compile thread, just do it */
	nodefn(node, coord, color, normal, texCoord);
	#endif
}

/* go through the generated table FIELDNAMES, and find the int of this string, returning it, or -1 on error 
	or if it is an "internal" field */
int findFieldInFIELDNAMES(char *field) {
	int x;
	int mystrlen;
	
	if (field[0] == '_') {
		printf ("findFieldInFIELDNAMES - internal field %s\n",field);
	}

	mystrlen = strlen(field);
	/* printf ("findFieldInFIELDNAMES, string :%s: is %d long\n",field,mystrlen);  */
	for (x=0; x<FIELDNAMES_COUNT; x++) {
		if (strlen(FIELDNAMES[x]) == mystrlen) {
			if (strcmp(field,FIELDNAMES[x])==0) return x;
		} 
	}
	return -1;
}

/* lets see if this node has a routed field  fromTo  = 0 = from node, anything else = to node */
int findRoutedFieldInFIELDNAMES (char *field, int fromTo) {
	int retval;
	char mychar[200];
	int tmp;

	retval = findFieldInFIELDNAMES(field);
	if (retval == -1) {
		/* try removing the "set_" or "_changed" */
		strncpy (mychar, field, 100);
		if (fromTo != 0) {
			retval = findFieldInFIELDNAMES(&mychar[4]);
		} else {
			if (strlen(field) > strlen("_changed")) {
				mychar[strlen(field) - strlen("_changed")] = '\0';
				retval = findFieldInFIELDNAMES(mychar);
			}
		}


	}

	return retval;
}

/* go through the generated table NODENAMES, and find the int of this string, returning it, or -1 on error */
int findNodeInNODES(char *node) {
	int x;
	int mystrlen;
	
	mystrlen = strlen(node);
	/* printf ("findNodeInNODENAMES, string :%s: is %d long\n",node,mystrlen); */
	for (x=0; x<NODES_COUNT; x++) {
		if (strlen(NODES[x]) == mystrlen) {
			if (strcmp(node,NODES[x])==0) return x;
		} 
	}
	return -1;
}
/* go through the generated table FIELDNAMES, and find the int of this string, returning it, or -1 on error */
int findFieldInALLFIELDNAMES(char *field) {
	int x;
	int mystrlen;
	
	mystrlen = strlen(field);
	/* printf ("findFieldInFIELDNAMES, string :%s: is %d long\n",field,mystrlen); */
	for (x=0; x<FIELDNAMES_COUNT; x++) {
		if (strlen(FIELDNAMES[x]) == mystrlen) {
			if (strcmp(field,FIELDNAMES[x])==0) return x;
		} 
	}
	return -1;
}

/* go through the OFFSETS for this node, looking for field, and return offset, type, and kind */
void findFieldInOFFSETS(int *nodeOffsetPtr, int field, int *coffset, int *ctype, int *ckind) {
	int *x;

	x = nodeOffsetPtr;
	/* printf ("findFieldInOffsets, comparing %d to %d\n",*x, field); */
	while ((*x != field) && (*x != -1)) {
		x += 4;
	}
	if (*x == field) {
		/* printf ("found field!\n"); */
		x++; *coffset = *x; x++; *ctype = *x; x++; *ckind = *x; 
		return;
	}
	if (*x == -1) {
		printf ("did not find field %d in OFFSETS\n",field);
		*coffset = -1; *ctype = -1, *ckind = -1;
		return;
	}
}

int countCommas (char *instr) {
	int retval = 0;
	while (*instr != '\0') {
		if (*instr == ',') retval++;
		instr++;
	}
	return retval;
}

/* called effectively by VRMLCU.pm */
void Perl_scanStringValueToMem(void *ptr, int coffset, int ctype, char *value) {
	int datasize;
	int commaCount;

	char *nst;                      /* used for pointer maths */
	void *mdata;
	int *iptr;
	float *fptr;
	SV **svptr;
	SV *mysv;
	int tmp;
	int myStrLen;
	

	/* temporary for sscanfing */
	float fl[4];
	int in[4];
	uintptr_t inNode[4];
	double dv;
	char mytmpstr[20000];

	/* printf ("PST, for %s we have %s strlen %d\n",FIELD_TYPE_STRING(ctype), value, strlen(value)); */
	nst = (char *) ptr; /* should be 64 bit compatible */
	nst += coffset;

	datasize = returnElementLength(ctype);
	commaCount = countCommas(value);
	
	switch (ctype) {
		case SFFLOAT: {sscanf (value,"%f",fl); memcpy (nst,fl,datasize); break;} 

		case SFBOOL:
		case SFINT32:
			{ sscanf (value,"%d",in); 
				memcpy(nst,in,datasize); 
				/* SFNODES need to have the parent field linked in */
				if (ctype == SFNODE) {
					add_parent((void *)in[0], ptr); 
				}
				
			break;}
		case FREEWRLPTR:
		case SFNODE: { 
				sscanf (value,"%d",inNode); 
				/* if (inNode[0] != 0) {
					printf (" andof type %s\n",stringNodeType(((struct X3D_Box *)inNode[0])->_nodeType));
				} */
				memcpy(nst,inNode,datasize); 
				/* SFNODES need to have the parent field linked in */
				if (ctype == SFNODE) {
					add_parent((void *)inNode[0], ptr); 
				}
				
			break;}
		case SFVEC2F:
			{sscanf (value,"%f,%f",&fl[0],&fl[1]); memcpy (nst,fl,datasize*2); break;}
		case SFROTATION:
		case SFCOLORRGBA:
			{sscanf (value,"%f,%f,%f,%f",&fl[0],&fl[1],&fl[2],&fl[3]); memcpy (nst,fl,datasize*4); break;}
		case SFVEC3F:
		case SFCOLOR:
			{ sscanf (value,"%f,%f,%f",&fl[0],&fl[1],&fl[2]); memcpy (nst,fl,datasize*3); break;}
		case MFBOOL:
		case MFINT32: {
			mdata = malloc ((commaCount+1) * datasize);
			iptr = (int *)mdata;
			for (tmp = 0; tmp < (commaCount+1); tmp++) {
				sscanf(value, "%d",iptr);
				iptr ++;
				/* skip past the number and trailing comma, if there is one */
				if (*value == '-') value++;
				while (*value>='0') value++;
				if ((*value == ' ') || (*value == ',')) value++;
			}
			((struct Multi_Node *)nst)->p=mdata;
			((struct Multi_Node *)nst)->n = commaCount+1;
			break;
			}

		case MFNODE: {
			for (tmp = 0; tmp < (commaCount+1); tmp++) {
				sscanf(value, "%d",inNode);
				addToNode(ptr,coffset,(void *)inNode[0]); 
				/* printf ("MFNODE, have to add child %d to parent %d\n",inNode[0],ptr);  */
				add_parent((void *)inNode[0], ptr); 
				/* skip past the number and trailing comma, if there is one */
				if (*value == '-') value++;
				while (*value>='0') value++;
				if ((*value == ' ') || (*value == ',')) value++;
			}
			break;
			}
		case SFTIME: { sscanf (value, "%lf", &dv); 
				/* printf ("SFtime, for value %s has %lf datasize %d\n",value,dv,datasize); */
				memcpy (nst,&dv,datasize);
			break; }

		case MFROTATION:
		case MFCOLOR:
		case MFFLOAT:
		case MFTIME:
		case MFVEC2F:
		case MFVEC3F:
		case MFCOLORRGBA: {
			/* printf ("data size is %d elerow %d commaCount %d\n",datasize, returnElementRowSize(ctype),commaCount+1); */
			mdata = malloc ((commaCount+1) * datasize);
			fptr = (float *)mdata;
			for (tmp = 0; tmp < (commaCount+1); tmp++) {
				sscanf(value, "%f",fptr);
				fptr ++;
				/* skip past the number; checking to see if it has a decimal point or not */
				while ((*value != '\0') && (*value != ' ') && (*value != ',')) value++;

				/* skip to the beginning of the next number */
				if ((*value == ' ') || (*value == ',')) value++;
			}
			((struct Multi_Node *)nst)->p=mdata;
			((struct Multi_Node *)nst)->n = (commaCount+1)/returnElementRowSize(ctype);
			break;
			}

		case SFSTRING: 
		case SFIMAGE: {
			mysv  = EAI_newSVpv(value); 
			memcpy (nst,&mysv,sizeof(SV *));
			break; }
			
		case MFSTRING: {
			/* string comes in from VRMLCU.pm as:
				1:8:MODULATE,3:ADD 
			   where 1: is the last element (ie, 2 elements)
				 8: is the length of the first string,
				 3: is the length of the next string */

			/* get a new count of elements */
			sscanf (value,"%d",&commaCount);
			while ((*value >=0) && (*value<='9')) value++;
			if (*value == ':') value++;
		
			mdata = malloc ((commaCount+1) * datasize);
			svptr = (SV **)mdata;

			for (tmp = 0; tmp < (commaCount+1); tmp++) {
				sscanf(value, "%d",&myStrLen);

				if (myStrLen>20000) myStrLen = 19000;

				while (*value<='9') value++;
				if (*value == ':') value++;

				strncpy (mytmpstr,value,myStrLen);
				mytmpstr[myStrLen] = '\0';
				value += myStrLen;
				*svptr = EAI_newSVpv(mytmpstr);

				svptr ++;
				if (*value == ',') value++;
			}
			((struct Multi_Node *)nst)->p=mdata;
			((struct Multi_Node *)nst)->n = commaCount+1;
			break;
			}

		default: {
printf ("Unhandled PST, %s: value %s, ptrnode %s nst %d offset %d numelements %d\n",
	FIELD_TYPE_STRING(ctype),value,stringNodeType(((struct X3D_Box *)ptr)->_nodeType),nst,coffset,commaCount+1);
			break;
			};
	}
}


/* for CRoutes, we need to have a function pointer to an interpolator to run, if we
route TO an interpolator */
void *returnInterpolatorPointer (char *x) {
	if (strncmp("OrientationInterpolator",x,strlen("OrientationInterpolator"))==0) {
		return (void *)do_Oint4;
	} else if (strncmp("CoordinateInterpolator2D",x,strlen("CoordinateInterpolator2D"))==0) {
		return (void *)do_OintCoord2D;
	} else if (strncmp("PositionInterpolator2D",x,strlen("PositionInterpolator2D"))==0) {
		return (void *)do_OintPos2D;
	} else if (strncmp("ScalarInterpolator",x,strlen("ScalarInterpolator"))==0) {
		return (void *)do_OintScalar;
	} else if (strncmp("ColorInterpolator",x,strlen("ColorInterpolator"))==0) {
		return (void *)do_Oint3;
	} else if (strncmp("PositionInterpolator",x,strlen("PositionInterpolator"))==0) {
		return (void *)do_Oint3;
	} else if (strncmp("CoordinateInterpolator",x,strlen("CoordinateInterpolator"))==0) {
		return (void *)do_OintCoord;
	} else if (strncmp("NormalInterpolator",x,strlen("NormalInterpolator"))==0) {
		return (void *)do_OintCoord;
	} else if (strncmp("GeoPositionInterpolator",x,strlen("GeoPositionInterpolator"))==0) {
		return (void *)do_GeoOint;
	} else if (strncmp("BooleanFilter",x,strlen("BooleanFilter"))==0) {
		return (void *)do_BooleanFilter;
	} else if (strncmp("BooleanSequencer",x,strlen("BooleanSequencer"))==0) {
		return (void *)do_BooleanSequencer;
	} else if (strncmp("BooleanToggle",x,strlen("BooleanToggle"))==0) {
		return (void *)do_BooleanToggle;
	} else if (strncmp("BooleanTrigger",x,strlen("BooleanTrigger"))==0) {
		return (void *)do_BooleanTrigger;
	} else if (strncmp("IntegerTrigger",x,strlen("IntegerTrigger"))==0) {
		return (void *)do_IntegerTrigger;
	} else if (strncmp("TimeTrigger",x,strlen("TimeTrigger"))==0) {
		return (void *)do_TimeTrigger;
	} else {
		return 0;
	}
}
