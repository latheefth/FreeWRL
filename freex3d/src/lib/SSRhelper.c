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
#include <string.h>
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
#include <math.h>
static int reverse_sense_init = 0; //0 conceptually correct
static int reverse_sense_quat4 = 0; //0 conceptually correct
static int reverse_sense_vec3 = 0;
static int reverse_order_quat4 = 1; //0 conceptually correct
void vp2world_initialize()
{
	/*  
		Computes the View' and inverseView' transforms where View' is View except without the .Quat, .Pos effects
			View' = inv(.Quat) x inv(.Pos) x View
		- then SSR_reply_pose() and SSR_set_pose() can transform client-side quat4,vec3 to/from viewer.Quat,.Pos:
			(vec3,quat4) = (.Pos,.Quat) x View'
			(.Pos,.Quat) = (vec3,quat4) x invView'
		For SSR, assume View doesn't change through life of scene:
			- no viewpoint animation 
			- no switching bound viewpionts
			-> then we only call this once on init ie ssr's init_pose (it's a bit heavy with inverses)
		and assume this call is being made from outside render_hier()
			- after seeking the view matrix
			- so just the view matrix is in opengl's modelview matrix

		Colum-major vs Row-Major notation - it's a notion in the mind, and either way is correct
			https://www.opengl.org/archives/resources/faq/technical/transformations.htm
			opengl matrices elements 12,13,14 are the translations; 
			3,7,11 are perspectives (zero for modelview, nonzero for projection); 15 == 1
			OpenGL manuals show column major which is a bit counter-intuitive for C programmers:
			[x]    [ 0  4  8 12]   [x]
			[y]  = [ 1  5  9 13] x [y]    Column major notation, as OGL manuals show
			[z]    [ 2  6 10 14]   [z]    4x1 = 4x4 x 4x1
			[w]    [ 3  7 11 15]   [1]
			Equivalent C row major order:
			[x y z w] = [x y z 1] x [ 0  1  2  3]
			.                       [ 4  5  6  7]   Row major notation
			.                       [ 8  9 10 11]    1x4 = 1x4 x 4x4
			.                       [12 13 14 15]
		Note the notational order on right hand side is opposite. Below we will use the opengl column major ordering 
			transform(r,a,M): r =  M x [a,1] //note vector in column form on right of matrix
			matmultiply(r,a,b): r = a x b
		(client gl-matrix.js is in opengl matrix order, same transform, but reverses order in matrix multiply)

		The sense/direction of the view is the opengl sense of modelview: transform world2vp. 

		vp2world sense:
			world - View - Viewpoint - .position - .orientation - vp
		world2vp sense (what is stored in opengl modelview matrix):
			[vp xyz] =  world2vpMatrix x [world xyz] =  ViewMatrix x [world]
			[vp xyz] =   viewMatrix x modelMatrix x [shape xyz]
		ViewMatrix in world2vp sense (and opengl row-major order):
		ViewMatrix =  viewer.Quat x -viewer.Pos  x viewer.AntiPos  x viewer.AntiQuat x -vp.orientation x -vp.position x -transforms 
		View (world2vp sense, row major order) =  (mobile device screen orientation) x  world2vp(.Quat, .Pos) x vp2world(.AntiPos, .AntiQuat) x world2vp(.orientation, .position) x Transforms(in vp2world direction) 
		assuming no screen orientation, and (.AntiPos,.AntiQuat) == (cancels) .orientation,.position, this simiplfies to:
		View (world2vp sense, row major order) =  x (.Quat in world2vp, -(.Pos in vp2world)) x Transforms(in world2vp sense) 

		opengl sense:
		(a glTranslate(0,0,-5) will	translate an object in world coordinates by -5 along camera/viewpoint Z axis. 
		Viewpoint/camera Z axis +backward, so -5 on object shifts the object 5 further away from camera 
		assuming it starts in front of the camera. In this interpretation, object at world 0,0,0 gets transformed to 
		camera/viewpoint 0,0,-5, and modelview matrix is in the sense of world to camera, or world2vp)
		a glRotate(+angle,1,0,0) will rotate counter clockwise about the axis as seen looking down the axis toward the origin
			-or what is normally called 'right hand rule': align the thumb on your right hand with the axis, and the
			 way the fingers curl is the direction of rotation
			-rotates world2vp
		
		x3d sense:
		X3D's transform stack between root/world and viewpoint -the View matrix equivalent- 
		is declared in X3D in the sense of (leaf object) to world or in our case (viewpoint) to world, or vp2world
		X3D.s viewpoint .position, .orientation is also declared in the vp2world sense
		freewrl reverses the x3d sense when building the View part of the Modelview, to put it in the opengl world2vp sense
			- see INITIATE_POSITION_ANTIPOSITION macro, as used in bind_viewpoint() 
			- see viewpoint_togl() as called from setup_viewpoint()
			- see fin_transform() for render_vp/VF_Viewpoint pass

		For example a translation of 10,0,0 in a Transform around bound vp will translate vp to the right in the world, 
		   that's a vp2world sense
		A vp.position=10,0,0 has the same sense and magnitude, vp2world
		To get this effect in an opengl modelview matrix transforming world2vp, freewrl subtracts 10 / uses -10 when
			creating the opengl matrix.
		And when converting .orientation to Quat, it inverts the direction. 
		And when using .Pos = .position, it negates ie glTranslate(-position.x,-position.y,-position.z) and -.Pos.x, -Pos.y, -Pos.z

		Note on quaternion commutivity: qa * qb != qb * qa -in general 3D rotations are non-commutative.
		If you want to reverse the order of matrices C = A*B to get equivalent C = B'*A, you compute:
			B'*A = A*B
			multiply each side by inv(A)
			inv(A)*B'A = inv(A)*A*B = Identity*B = B
			rearranging:
			B = inv(A)*B'*A
			post-multiply each side by inv(A)
			B*inv(A) = inv(A)*B'*A*inv(A) = inv(A)*B'*Identity = inv(A)*B'
			multiply each side by A
			A*B*inv(A) = A*inv(A)*B' = Identity*B' = B'
			changing sides
			B' = A*B*inv(A)
			check by substituting in C=B'*A
			C = A*B*inv(A)*A = A*B*Identity = A*B
		If you want to reverse the order of quaternions qc = qa * qb = qb' * qa,
			qb' * qa = qa * qb
			qb' * qa * conj(qa) = qa * qb * conj(qa)
			qb' = Identity = qa * qb * conj(qa)
			qb' = qa * qb * conj(qa)   //method A
			Numerically you can test, comparing to textbook formulas
			qa*qb = conj(conj(qb)*conj(qa))   //method B
				== qb'*qa ?
			qb'*qa = conj(conj(qb)*conj(qa))
			qb'*qa*conj(qa) = conj(conj(qb)*conj(qa))*conj(qa)
			qb' = conj(conj(qb)*conj(qa))*conj(qa) ?


		conceptually correct transforms, in row-major-notation order:
		client (html javascript using gl-matrix.js):
			viewpoint < cumQuat < cumPos < worldGrid
			viewpoint =  cumQuat x cumPos x  [worldGrid]
		server (freewrl):
			viewpoint  < .Quat  <   -.Pos  <  View'  <  world
			viewpoint = .Quat x -.Pos x View' x [world]
		All the above transforms cumQuat,cumPos, .Quat, .Pos, View conceptually should have the world2vp sense '<'
		To reverse the sense of the equation from vp2world to world2vp we multiply each side by the inverse of the 
		 last-applied/first-notational transform on the right side, then inverse-something x something = Identity/I/1.0
			invQuat x viewpoint = invQuat x Quat x -Pos x View' x [world] 
							= Identity x -Pos x View' x [world] 
							= -Pos x View' x [world]
			inv(-Pos) x invQuat x viewpoint = inv(-Pos) x -Pos x View' x [world] 
										= View' x [world]
			invView' x inv(-Pos) x invQuat x viewpoint = invView' x View' x [world] = [world]
			changing sides:
			[world] = invView' x inv(-Pos) x invQuat x viewpoint 
		
		Details:
		server_side
		A.vp2world_initialize
			1) view = modelview at top of glmatrix stack, at scene root, in setup_viewpoint()
				after viewer_togl() and render_heir() VF_Viewpoint pass
					- includes .Quat, -.Pos
			2) a) view' = f(view .Quat, .Pos)
			   b) view' = inv(-Pos) x inv(.Quat) x View 
						= inv(Quat x (-Pos)) x View
			3) viewQuat' = matrix_to_quaternion(view')
			4) invView = inv(view'), invQuat = inv(viewQuat') //converts from world2vp sense to vp2world sense
		B.reply_pose
			1) cumPos  = f(view',.Quat,.Pos) =  invView x -.Pos //we want .pos to be more 'worldly'
			2) cumQuat = f(view',.Quat,.Pos) =  ViewQuat x .Quat //we just want to concatonate 2 quats, keeping the same sense
		C.set_pose
			1) .Pos = f(view',cumQuat,cumPos)  
				mulitplying each side of B1 by view':
				view' x cumPos = view' x invView' x -.Pos
							=  Identity x -.Pos = -.Pos
				.Pos =  view' x -cumPos
			2) .Quat = f(view',cumQuat,cumPos)
				mulitplying each side of B2 by inv(viewQuat):
				inv(viewQuat) x cumQuat = inv(viewQuat) x viewQuat x .Quat
								= Identity x .Quat = .Quat
				.Quat = inv(viewQuat) x cumQuat

	*/
	if(!vp2world_initialized){
		//world2vp sense of view
		double mat[16], mat2[16], mat3[16], mat4[16], quat4[4], vec3[3];
		viewer_getview(view);  //gets modelview matrix and assumes it is ViewMatrix, and in opengl sense world2vp
		printmatrix2(view,"view with .pos .ori");
		//problem: view matrix also has world2vp(.position, .orientation) in it. We'd like to keep them separate.
		//solution: construct a few matrices with vec3, quat4, invert, and multiply view by them
		//View' =  View x inverse(.Quat,-.Pos)  //world2vp
		loadIdentityMatrix(mat);
		viewer_getpose(quat4,vec3); //viewer.position,.orientation, in sense of world2vp (vec3 = -.Pos, .Pos = .position, quat4 = .Quat = inv(.orientation)
		printf("fixing view, current pose vec3=%lf %lf %lf\n",vec3[0],vec3[1],vec3[2]);

		if(1){
			Quaternion qq;
			double2quat(&qq,quat4);
			loadIdentityMatrix(mat2);
			quaternion_to_matrix(mat2, &qq);
			printmatrix2(mat2,"matqq using quaternion_to_matrix");
		}
		else
		{
			loadIdentityMatrix(mat2);
			printf("quat xyzw [%lf %lf %lf %lf]",quat4[0],quat4[1],quat4[2],quat4[3]);
			matrotate(mat2,2.0*acos(quat4[3]),quat4[0],quat4[1],quat4[2]); 
			printmatrix2(mat2,"mat2 using matrotate 2*cos(quat4[3])");
		}
		loadIdentityMatrix(mat3);
		mattranslate(mat3,vec3[0],vec3[1],vec3[2]);
		printmatrix2(mat3,"mat3 vec3 translation");
		if(0){
			if(1)matmultiply(mat4,mat2,mat3); //conceptually correct
			else matmultiplyAFFINE(mat4,mat2,mat3);
			printmatrix2(mat4,"combined quat x vec3");
			matinverseAFFINE(mat,mat4);
			printmatrix2(mat,"inv(quat x vec3) = inv(vec3)xinv(quat)");
		}else{
			double matinvpos[16],matinvquat[16];
			matinverseAFFINE(matinvquat,mat2);
			matinverseAFFINE(matinvpos,mat3);
			if(1) matmultiply(mat,matinvpos,matinvquat); //conceptually correct
			else matmultiply(mat,matinvquat,matinvpos);
			printmatrix2(mat,"inv(vec3) x inv(quat)");
		}

		if(1)
			matmultiply(view,mat,view); //conceptually correct - take off effect of .Pos, .Quat from View)
		else
			matmultiply(view,view,mat);
		printmatrix2(view,"view - no .pos .ori");
		//now view should not have the .position, .orientation in it - just the transform stack from world to vp
		matinverseAFFINE(inv_view, view); //we'll prepare inverse so we can transform position both ways
		if(reverse_sense_init){
			matrix_to_quaternion(&viewQuat, inv_view); 
			quaternion_inverse(&inv_viewQuat,&viewQuat); //prepare inverse in a way we can also transform .orientation in the form of a quaternion
		}else{
			//Quaternion q2;
			//double ppii;
			matrix_to_quaternion(&viewQuat, view); //conceptually correct
			quaternion_normalize(&viewQuat);
			//quaternion_print(&viewQuat,"viewQuat");
			//ppii = acos(-1.0);
			//printf("pi=%lf\n",ppii);
			//vrmlrot_to_quaternion(&q2, 1.0, 0.0, 0.0, ppii * .5);
			//quaternion_print(&q2,"rotateX(pi/2)");
			if(0){
				quaternion_inverse(&inv_viewQuat,&viewQuat); //prepare inverse in a way we can also transform .orientation in the form of a quaternion
			}else{
				matrix_to_quaternion(&inv_viewQuat,inv_view); //s.b. equavalent
				quaternion_normalize(&inv_viewQuat);
			}
		}
		vp2world_initialized = TRUE;
	}
}
struct point_XYZ {GLDOUBLE x,y,z;};
void SSR_reply_pose(SSR_request *request, int initialpose)
{
	/* client's pose(vec3,quat4) - world - View - (Viewpoint node) - .Pos - ..Quat  - vp
		Thinking of vec3,quat4 as things to transform:
			transform server's scene bound-viewpoint-relative pose(Viewer.Pos,.Quat) to client's world coords pose(vec3,quat4)
		Or thinking of vec3,quat4 as part of a transform chain:
			add on View part of (quat4,vec3) transform to get from world2vp
			(except we don't need any View scale on the client - it has its own scale for moving around, 
				so the 'vp as a point to be transformed' way of thinking above makes more sense)
		Assume View doesn't change through life of scene:
			- no viewpoint animation 
			- no switching bound viewpionts
		initialpose - TRUE: for init_pose() reutrn the bind-time .pos,.quat
			- FALSE: for posepose, return adjusted pose sent with posepose request
	*/
	if(use_vp2world){
		//convert boundviewpoint2world
		// viewpoint-local .pos, .quat to world (scene root) coordinate system by applying inv(ViewMatrix) 
		// the .position/.Pos is on the world side of the .orientation/quat
		// view matrix can be thought of as (Rot) x (translation)
		// if you want to change the order to translation' x Rot, 
		// remember rotations and translations are not commutative. Then:
		// translation' = invrse(Rot) x translation
		// then View = (translation') x (Rot)
		// vp = Quat x Pos x ViewTrans x ViewRot x world
		// to gather the translations together, so we have
		// vp = TotalQuat x TotalVec x world
		// TotalQuat = Quat x ViewRot  //3D rotations are not commutative, so their order needs to be maintained
		// TotalVec = inverse(ViewRot) x (Pos + ViewTrans)
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

		//am I missing the translation part of view here? 
		//Should I be using just the rotation inverse here?, 
		//or does inv_view contain the translation part I need to add to vec3?
		printf("in reply_pose\n");
		printmatrix2(inv_view,"inv_view");
		printf("vec3 before=%lf %lf %lf\n",vec3[0],vec3[1],vec3[2]);
		if(reverse_sense_vec3){
			transformAFFINEd(request->vec3, vec3, inv_view); 
		}else{
			transformAFFINEd(request->vec3, vec3, view); 
		}
		printf("vec3 after=%lf %lf %lf\n",request->vec3[0],request->vec3[1],request->vec3[2]);

		double2quaternion(&q4,quat4);
		if(reverse_sense_quat4){
			if(reverse_order_quat4)
				quaternion_multiply(&qtmp,&q4, &inv_viewQuat);
			else
				quaternion_multiply(&qtmp,&inv_viewQuat,&q4);
		}else{
			if(reverse_order_quat4)
				quaternion_multiply(&qtmp,&viewQuat,&q4);
			else
				quaternion_multiply(&qtmp,&q4, &viewQuat); //conceptually correct
		}
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
static ssr_test_initialized = FALSE;
static run_ssr_test = FALSE;
char *get_key_val(char *key);
static double incYaw;
static double incTrans[3];
static int haveInc = FALSE;
void ssr_test_key_val(char *key, char *val){
	double dval;
	int ok;
	incTrans[0] = incTrans[1] = incTrans[2] = incYaw = 0.0;

	ok = sscanf(val,"%lf",&dval);
	if(!strcmp(key,"yaw")){
		incYaw = dval;
	} else
	if(!strcmp(key,"z")){
		incTrans[2] = dval;
	} else
	if(!strcmp(key,"x")){
		incTrans[0] = dval;
	} else
	if(!strcmp(key,"y")){
		incTrans[1] = dval;
	}
	haveInc = TRUE;
}
//#include <stdio.h>
//#include <string.h>


int ssr_test(char *keyval){
	//save arbitrary char* keyval = "key,val" pairs, 
	// for later retrieval with print_keyval or get_key_val
	int i, iret;
	char kv[100];
	i = strlen(keyval);
	iret = 0;
	if(i > 100) 
		iret = -1;
	else
	{
		char *sep;
		strcpy(kv,keyval);
		sep = strchr(kv,',');
		if(!sep) sep = strchr(kv,' ');
		if(sep){
			char *key, *val;
			val = &sep[1];
			(*sep) = '\0';
			key = kv;
			ssr_test_key_val(key,val);
			iret = 1;
		}
	}
	return iret;
}
void SSR_set_pose(SSR_request *request);
void SSR_test_cumulative_pose(){
	SSR_request r;
	static int test_replace_view = 1; // replaces the view matrix with one from client-side-equivalent cumQuat,cumTrans
	static int test_replace_quatpos = 1; //allows spacebar command ssrtest,yaw,.707 increments to cumQuat, cumTrans, then replaces viewer.quat,.pos
	//we don't want to run this test when doing SSR, just when running normal freewrl. Its a kind of SSR client emulator test.
	if(!ssr_test_initialized)
	{
		char *running_ssr = get_key_val("SSR");
		run_ssr_test = TRUE;
		if(running_ssr)
			if(!strcmp(running_ssr,"true"))
				run_ssr_test = FALSE;
		ssr_test_initialized = TRUE;
	}
	if(!run_ssr_test) return;

	vp2world_initialized = FALSE;
	vp2world_initialize();
	SSR_reply_pose(&r,FALSE);
	//create M = f(cumQuat, cumTrans) like the SSRClient.html does, and set it as View matrix.
	//mat4.identity(tMatrix);
	//mat4.translate(tMatrix,tMatrix,gridTrans);
	//mat4.fromQuat(qMatrix,cumQuat);
	////world2vp = mvMatrix = cumQuat x f(cumTrans) x mGrid
	//mat4.multiply(qtMatrix,qMatrix,tMatrix); 
	//mat4.multiply(mvMatrix,qtMatrix,mMatrix);
	{
		double tMatrix[16], qMatrix[16], mvMatrix[16], rawView[16];
		Quaternion cumQuat;
		double cumTrans[3];

		if(0){
			//test freewrl/linearalgebra matmultiply order
			double C[16],A[16],B[16], ppii;
			ppii = acos(-1.0);
			loadIdentityMatrix(A);
			loadIdentityMatrix(B);
			matrotate(A,ppii/2.0,1.0,0.0,0.0); //rotate about x by pi/2
			matrotate(B,ppii/2.0,0.0,1.0,0.0); //rotate about y by pi/2
			matmultiply(C,A,B);
			printmatrix2(A,"A");
			printmatrix2(B,"B");
			printmatrix2(C,"C=AxB");
		}

		veccopyd(cumTrans,r.vec3);
		viewer_getview(rawView);
		printmatrix2(rawView,"rawView");
		double2quat(&cumQuat,r.quat4);
		if(haveInc){
			Quaternion incQuat,invQuat,aQuat,bQuat;
			vrmlrot_to_quaternion(&incQuat, 0.0,1.0,0.0,incYaw);
			switch(1){
			case 0:
				//normally the incYaw is in vp space, and so we just keep multiplying cumQuat by incQuat.
				// - like we do during navigation in Viewer.c
				//but do we multiply on the left or the right? Or does it matter?
				//Left - as with opengl columnwise notation, vp = vpspace x world2vp x [world], and ours is in vp space
				quaternion_multiply(&cumQuat,&incQuat,&cumQuat);
				break;
			case 1:
				//Right
				quaternion_multiply(&cumQuat,&cumQuat,&incQuat);
				break;
			case 2:
				//I can't think of a good reason why, but maybe we start on the 
				// right, and want to get incQuat on the left of cumQuat before we multiply 
				//qb' * qa = qa * qb
				//qb' = qa * qb * conj(qa)   //method A
				quaternion_multiply(&aQuat,&cumQuat,&incQuat); //qa * qb
				quaternion_multiply(&cumQuat,&aQuat,&invQuat); //*conj(qa)
				break;
			case 3:
				//another way to get incQuat from the right to the left side of cumQuat
				//before multiplying
				//qb' * qa = qa * qb
				//qa*qb = conj(conj(qb)*conj(qa))   //method B
				quaternion_inverse(&aQuat,&incQuat);  //conj(qb)
				quaternion_multiply(&bQuat,&aQuat,&invQuat); //qb'*qa'
				quaternion_inverse(&cumQuat,&bQuat); //conj(qb'*qa')
				break;
			default:
				break;
			}
				
			quaternion_normalize(&cumQuat);
			quaternion_inverse(&invQuat,&cumQuat);
			quaternion_rotationd(incTrans,&invQuat,incTrans);
			vecaddd(cumTrans,incTrans,cumTrans);
		}
		loadIdentityMatrix(tMatrix);
		mattranslate(tMatrix,cumTrans[0],cumTrans[1],cumTrans[2]);

		loadIdentityMatrix(qMatrix);
		quaternion_to_matrix(qMatrix, &cumQuat);
		matmultiply(mvMatrix,qMatrix,tMatrix);
		printmatrix2(mvMatrix,"mvMatrix");
		if(test_replace_view)
			viewer_setview(mvMatrix);
		if(test_replace_quatpos && haveInc){
			SSR_request r;
			veccopyd(r.vec3,cumTrans);
			quaternion2double(r.quat4,&cumQuat);
			SSR_set_pose(&r);
			haveInc = FALSE;
		}
	}
}

void SSR_set_pose(SSR_request *request)
{
	/* request->quat4,vec3 - world - View - (Viewpoint node) - .Pos - .Quat - vp
		tranform client's pose from its world coordinates to scene's bound-viewpoint-relative vp coords
		Assume View doesn't change through life of scene:
			- no viewpoint animation 
			- no switching bound viewpionts
	*/
	if(use_vp2world){
		//convert world2boundviewpoint / world2vp
		// .pos, .quat world (scene root) coordinate system to bound viewpoint relative
		// by applying ViewMatrix
		double quat4[4], vec3[3];
		Quaternion qtmp, q4;
		vp2world_initialize();
		if(reverse_sense_vec3){
			transformAFFINEd(vec3,request->vec3,view);
		}else{
			transformAFFINEd(vec3,request->vec3,inv_view); //conceptually correct
		}
		double2quaternion(&q4,request->quat4);
		if(0){
			if(reverse_sense_quat4){
				if(reverse_order_quat4)
					quaternion_multiply(&qtmp,&q4,&viewQuat);
				else
					quaternion_multiply(&qtmp,&viewQuat,&q4);
			}else{
				if(reverse_order_quat4)
					quaternion_multiply(&qtmp,&inv_viewQuat,&q4);
				else
					quaternion_multiply(&qtmp,&q4,&inv_viewQuat); //conceptually correct
			}
		}
		switch(2){
			//we want to know quat such that cumQuat = ViewQuat x quat.
			//we have cumQuat, viewQuat.
			//  viewquat' x cumQuat = viewquat' x viewquat x quat = Identity x quat = quat
			// quat = viewquat' x cumQuat

		case 0:
			//best guess
			quaternion_multiply(&qtmp,&inv_viewQuat,&q4);
			break;
		case 1:
			//reverse order
			quaternion_multiply(&qtmp,&q4,&inv_viewQuat);
			break;
		case 2:
			//another way to get incQuat from the right to the left side of cumQuat
			//before multiplying
			//qb' * qa = qa * qb
			//qa*qb = conj(conj(qb)*conj(qa))   //method B
			quaternion_inverse(&q4,&q4);  //conj(qb)
			quaternion_multiply(&q4,&q4,&inv_viewQuat); //qb'*qa'
			quaternion_inverse(&qtmp,&q4); //conj(qb'*qa')
			break;
		default:
			break;
		}

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