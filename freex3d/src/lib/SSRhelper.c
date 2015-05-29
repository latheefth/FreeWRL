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
void SSR_set_pose(SSR_request *request)
{
	viewer_setpose(request->quat4, request->vec3);
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
void SSR_reply_pose(SSR_request *request)
{
	viewer_getpose(request->quat4,request->vec3);
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
			case SSR_POSEPOSE:
				SSR_reply_pose(request); break;
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