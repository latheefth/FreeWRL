#include <config.h>
#ifdef SSR_SERVER
/*	
	D. SSR: server-side rendering
	SSRhelper: supplies the libfreewrl parts:
		a) a queue for SSR client requests: 
			i) pose-pose - given a viewpoint pose, run a draw() including collision, 
				animation, to compute a new pose, and return the new pose
			ii) pose-snapshot - given a viewpoint pose, run a draw() and render
				a frame, do a snapshot, convert snapshot to .jpg and return .jpg as blob (char* binary large object)
		b) a function for _DisplayThread to run once per frame
	What's not in here: 
		A. the html client part - talks to the web server part
		B. the web server part - talks to the html client part, and to this SSRhelper part in libfreewrl
		C. libfreewrl (dllfreewrl) export interfaces on the enqueue function
*/
#include <stdio.h>
#include <stdlib.h>
#include "list.h"
#include "SSRhelper.h"
//from Prodcon.c L.1100
void threadsafe_enqueue_item(s_list_t *item, s_list_t** queue, pthread_mutex_t* queue_lock);
s_list_t* threadsafe_dequeue_item(s_list_t** queue, pthread_mutex_t *queue_lock );
void threadsafe_enqueue_item_signal(s_list_t *item, s_list_t** queue, pthread_mutex_t* queue_lock, pthread_cond_t *queue_nonzero);
s_list_t* threadsafe_dequeue_item_wait(s_list_t** queue, pthread_mutex_t *queue_lock, pthread_cond_t *queue_nonzero, int *waiting );
//from io_files.c L.310
int load_file_blob(const char *filename, char **blob, int *len);
//from Viewer.c L.1978
void viewer_setpose(double *quat4, double *vec3);
void viewer_getpose(double *quat4, double *vec3);
void viewer_getbindpose(double *quat4, double *vec3);

#define FALSE 0
#define TRUE 1
typedef struct iiglobal *ttglobal;
static s_list_t *ssr_queue = NULL;
static pthread_mutex_t ssr_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ssr_queue_condition = PTHREAD_COND_INITIALIZER;
static int ssr_server_waiting = FALSE;

void SSRserver_enqueue_request_and_wait(void *fwctx, SSR_request *request){
	//called by A -> B -> C
	//in B -the web server- a new thread is created for each A request.
	//this function is called from those many different temporary threads
	//the backend _displayThread will own our queue -so it's a thread funnel
	s_list_t *item = ml_new(request);
	pthread_mutex_init(&request->requester_mutex,NULL);
	pthread_cond_init(&request->requester_condition,NULL);
	pthread_mutex_lock(&request->requester_mutex);
	request->answered = 0;
	request->blob = NULL;
	request->len = 0;
	if(1)threadsafe_enqueue_item(item,&ssr_queue, &ssr_queue_mutex);
	if(0)threadsafe_enqueue_item_signal(item,&ssr_queue, &ssr_queue_mutex,&ssr_queue_condition);
	while (request->answered == 0){
		pthread_cond_wait(&request->requester_condition, &request->requester_mutex);
	}
	pthread_cond_destroy(&request->requester_condition);
	pthread_mutex_destroy(&request->requester_mutex);

	return;
}
#define BOOL	int
#define GLDOUBLE double
#include "../lib/scenegraph/quaternion.h"
#include "../lib/scenegraph/LinearAlgebra.h"
static double view[16], inv_view[16];
static Quaternion viewQuat, inv_viewQuat;
static int vp2world_initialized = FALSE;
static int use_vp2world = TRUE;
void double2quaternion(Quaternion *quat, double *q4){
	//freewrl puts w as first element, client-side gl-matrix.js puts it last
	quat->w = q4[3];
	quat->x = q4[0];
	quat->y = q4[1];
	quat->z = q4[2];
}
void quaternion2double(double *q4, Quaternion *quat){
	//freewrl puts w as first element, client-side gl-matrix.js puts it last
	q4[3] = quat->w;
	q4[0] = quat->x;
	q4[1] = quat->y;
	q4[2] = quat->z;
}
void vp2world_initialize()
{
	/* world - View - Viewpoint - .position - .orientation
		get ViewMatrix for worldcoordinates to bound-viewpoint-relative transformations
		Assume View doesn't change through life of scene:
			- no viewpoint animation 
			- no switching bound viewpionts
			-> then we only call this once on init ie ssr's init_pose (it's a bit heavy with inverses)
		and assume this call is being made from outside render_hier()
			- after seeking the view matrix
			- so just the view matrix is in opengl's modelview matrix
			- (do I have that correct?)
		The sense/direction of the view is to transform from vp to world (or eventually to shape in freewrl)
		For example a translation of 10,0,0 in a Transform around bound vp will translate vp to the right in the world
			 (or world to the left wrt vp). a vp.position=10,0,0 has the same sense and magnitude.
		To get this effect in an opengl modelview matrix transforming vp2world, freewrl subtracts 10 / uses -10 when
			creating the opengl matrix.
		View (vp2world sense) = (mobile device screen orientation) x (.Quat, .Pos) x (.AntiPos, .AntiQuat) x (.orientation, .position) x Transforms(in vp2world direction)
		assuming no screen orientation, and (.AntiPos,.AntiQuat) == (cancels) .orientation,.position, this simiplfies to:
		View (vp2world sense) = .Quat, .Pos x Transforms(in vp2world sense)
		View' = inverse(.pos,.quat) x View (vp2world sense)
		dug9: after theory came up short, I worked out the formulas below by trial and error, 
			combinations and permutations, until:
				1. client, server were cycling (single click on client would refresh from server to same pose in model)
				2. synced (client grid and server model going same way for yaw,x,y,z)
	*/
	if(!vp2world_initialized){
		//vp2world sense of view
		double mat[16], mat2[16], mat3[16], mat4[16], quat4[4], vec3[3];
		viewer_getview(view);  //gets modelview matrix and assumes it is ViewMatrix
		printmatrix2(view,"view with .pos .ori");
		//problem: view matrix also has .position, .orientation in it. We'd like to keep them separate.
		//solution: construct a few matrices with vec3, quat4, invert, and multiply view by them
		loadIdentityMatrix(mat);
		viewer_getpose(quat4,vec3);
		printf("fixing view, current pose vec3=%lf %lf %lf\n",vec3[0],vec3[1],vec3[2]);
		matrotate(mat2,2.0*acos(quat4[3]),quat4[0],quat4[1],quat4[2]);
		mattranslate(mat3,vec3[0],vec3[1],vec3[2]);
		matmultiply(mat4,mat2,mat3);
		matinverseAFFINE(mat,mat4);
		matmultiply(view,mat,view);
		printmatrix2(view,"view - no .pos .ori");
		//now view should not have the .position, .orientation in it - just the transform stack from vp to world
		matinverseAFFINE(inv_view, view); //we'll prepare inverse so we can transform position both ways
		matrix_to_quaternion(&viewQuat, inv_view); 
		quaternion_inverse(&inv_viewQuat,&viewQuat); //prepare inverse in a way we can also transform .orientation in the form of a quaternion
		vp2world_initialized = TRUE;
	}
}
struct point_XYZ {GLDOUBLE x,y,z;};
void SSR_set_pose(SSR_request *request)
{
	/* request->quat4,vec3 - world - View - (Viewpoint node) - .Pos - .Quat - vp
		tranform client's pose from its world coordinates to scene's bound-viewpoint-relative vp coords
		Assume View doesn't change through life of scene:
			- no viewpoint animation 
			- no switching bound viewpionts
	*/
	if(use_vp2world){
		//convert world2boundviewpoint
		// .pos, .quat world (scene root) coordinate system to bound viewpoint relative
		// by applying ViewMatrix
		double quat4[4], vec3[3];
		Quaternion qtmp, q4;
		vp2world_initialize();
		transformAFFINEd(vec3,request->vec3,view);
		double2quaternion(&q4,request->quat4);
		quaternion_multiply(&qtmp,&q4,&viewQuat);
		quaternion2double(quat4,&qtmp);
		printf("setting server pose quat4=[%lf %lf %lf %lf] vec3=[%lf %lf %lf]\n",quat4[0],quat4[1],quat4[2],quat4[3],vec3[0],vec3[1],vec3[2]);
		viewer_setpose(quat4,vec3);
	}else{
		//assume View is identity / at scene root / no bound viewpoint
		viewer_setpose(request->quat4, request->vec3);
	}
}
static SSR_request *ssr_current_request = NULL; //held for one draw loop
int isSceneLoaded();
void dequeue_SSR_request(ttglobal tg)
{
	//called by D: _DisplayThread, should be in backend thread, gglobal() should work
	s_list_t *item;
	SSR_request *request = NULL;
	item = NULL;
	if(isSceneLoaded()){
		//item = threadsafe_dequeue_item_wait(&ssr_queue, &ssr_queue_mutex,&ssr_queue_condition,&ssr_server_waiting);
		//item = threadsafe_dequeue_item_timed_wait(&ssr_queue, &ssr_queue_mutex,&ssr_queue_condition,&ssr_server_waiting);
		//pthread timed wait seems complicated, so I'll use usleep in a loop - laziness
		//Goal: allow a bit of server work to continue (not frozen)
		// while freeing CPU cores and GPU from senseless drawing,
		// allowing many/100s of SSR process instances to be running on the same server. 
		// And when a/the client requests something the server is quick to respond.
		int slept;
		int sleepincus = 1000; //1ms maximum to sleep in one shot - small enough server remains responsive to client
		int maxsleepus = 1000000; //1s - max to sleep when no SSRClient requets, between fwl_draw()s
		slept = 0;
		while(!item && slept < maxsleepus) {
			item = threadsafe_dequeue_item(&ssr_queue, &ssr_queue_mutex );
			if(!item){
				usleep(sleepincus);
				slept += sleepincus;
			}
		}
	}else if(ssr_queue){
		item = threadsafe_dequeue_item(&ssr_queue, &ssr_queue_mutex );
	}
	if(item){
		request = item->elem;
		//free(item);
		switch(request->type)
		{
			case SSR_INITPOSE:
				break;
			case SSR_POSEPOSE:
			case SSR_POSESNAPSHOT:
				SSR_set_pose(request);
				break;
			default:
				break;
		}
	}
	ssr_current_request = request;

}

void SSR_reply_pose(SSR_request *request, int initialpose)
{
	/* client's pose(vec3,quat4) - world - View - (Viewpoint node) - .Pos - ..Quat  - vp
		tranform server's scene bound-viewpoint-relative pose(Viewer.Pos,.Quat) to client's world coords pose(vec3,quat4)
		Assume View doesn't change through life of scene:
			- no viewpoint animation 
			- no switching bound viewpionts
		initialpose - TRUE: for init_pose() reutrn the bind-time .pos,.quat
			- FALSE: for posepose, return adjusted pose sent with posepose request
	*/
	if(use_vp2world){
		//convert boundviewpoint2world
		// .pos, .quat to world (scene root) coordinate system by applying inv(ViewMatrix) 
		double quat4[4], vec3[3];
		Quaternion qtmp, q4;
		vp2world_initialize();
		if(initialpose){
			viewer_getbindpose(quat4,vec3);
			printf("getting initial viewpoint pose:\n");
		}else{
			viewer_getpose(quat4,vec3);
			printf("getting current viewpoint pose:\n");
		}
		printf(" quat4=[%lf %lf %lf %lf] vec3=[%lf %lf %lf]\n",
		quat4[0],quat4[1],quat4[2],quat4[3],vec3[0],vec3[1],vec3[2]);
		double2quaternion(&q4,quat4);
		transformAFFINEd(request->vec3, vec3, inv_view);
		quaternion_multiply(&qtmp,&q4, &inv_viewQuat);
		quaternion2double(request->quat4,&qtmp);
		memcpy(vec3,request->vec3,3*sizeof(double));
		memcpy(quat4,request->quat4,4*sizeof(double));
		printf("getting server pose quat4=[%lf %lf %lf %lf] vec3=[%lf %lf %lf]\n",
		quat4[0],quat4[1],quat4[2],quat4[3],vec3[0],vec3[1],vec3[2]);
	}else{
		//assume View is identity/at scene root/no bound viewpoint
		// send bound viewpoint relative .position,  .orientaiton
		viewer_getpose(request->quat4,request->vec3);
	}
}

#ifdef _MSC_VER
static char *snapshot_filename = "snapshot.bmp"; //option: get this from the snapshot.c module after saving
#else
static char *snapshot_filename = "snapshot.png";
#endif
void SSR_reply_snapshot(SSR_request *request)
{
	int iret;
	Snapshot1(snapshot_filename); //win32 has Snapshot1 with filename, need to add to linux
	iret = load_file_blob(snapshot_filename,&request->blob,&request->len);
	if(!iret) 
		printf("snapshot file not found %s\n",snapshot_filename);
	else
		unlink(snapshot_filename);
	
}
void SSR_reply(){
	//called by D: _DisplayThread at end of draw loop (or just before SSR_dequeue_request at start of next loop)
	//only one ssr_current_request is processed per frame/draw loop
	if(ssr_current_request){
		SSR_request *request = ssr_current_request;
		switch(request->type){
			case SSR_INITPOSE:
				SSR_reply_pose(request,TRUE); break;
			case SSR_POSEPOSE:
				SSR_reply_pose(request,FALSE); break;
			case SSR_POSESNAPSHOT:
				SSR_reply_snapshot(request); break;
			default:
				break;
		}
		//tell waiting B server thread to wake up and reply to A html client
		request->answered = 1;
		pthread_cond_signal(&request->requester_condition);
		ssr_current_request = NULL;
	}
}
#endif //SSR_SERVER