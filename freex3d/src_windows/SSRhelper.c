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
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "list.h"
//from Prodcon.c L.1100
void threadsafe_enqueue_item(s_list_t *item, s_list_t** queue, pthread_mutex_t* queue_lock);
s_list_t* threadsafe_dequeue_item(s_list_t** queue, pthread_mutex_t *queue_lock );
//from io_files.c L.310
int load_file_blob(const char *filename, char **blob, int *len);

typedef struct iiglobal *ttglobal;
static s_list_t *ssr_queue = NULL;
static pthread_mutex_t ssr_queue_mutex;

enum {
	POSEPOSE = 0,
	POSESNAPSHOT = 1
};
typedef struct SSR_request {
	int type;
	pthread_mutex_t requester_mutex;
	pthread_cond_t requester_condition;
	double pose[16];
	char *blob;
	int len;
	int answered;
} SSR_request;

void enqueue_SSR_request_and_wait(void *fwctx, SSR_request *request){
	//called by A -> B -> C
	//in B -the web server- a new thread is created for each A request.
	//this function is called from those many different temporary threads
	//the backend _displayThread will own our queue -so it's a thread funnel
	s_list_t *item = ml_new(request);
	pthread_mutex_lock(&request->requester_mutex);
	request->answered = 0;
	request->blob = NULL;
	request->len = 0;
	threadsafe_enqueue_item(item,&ssr_queue, &ssr_queue_mutex);
	while (request->answered == 0){
		pthread_cond_wait(&request->requester_condition, &request->requester_mutex);
	}
	return;
}
void SSR_set_pose(ttglobal tg, double *pose)
{
	viewer_setpose(pose);
}
static SSR_request *ssr_current_request = NULL; //held for one draw loop
void dequeue_SSR_request(ttglobal tg)
{
	//called by D: _DisplayThread, should be in backend thread, gglobal() should work
	SSR_request *request = NULL;
	s_list_t *item = threadsafe_dequeue_item(&ssr_queue, &ssr_queue_mutex );
	if(item){
		request = item->elem;
		free(item);
		switch(request->type)
		{
			case POSEPOSE:
			case POSESNAPSHOT:
				SSR_set_pose(tg, request->pose);
				break;
			default:
				break;
		}
	}
	ssr_current_request = request;
}
void SSR_reply_pose(SSR_request *request)
{
	double *pose;
	viewer_getpose(&pose);
	memcpy(request->pose,pose);
}
//#ifdef _MSC_VER
//static char * convert_command = "convert snapshot.bmp -png"; 
//#else
//static char * convert_command = "convert snapshot.bmp -png"; 
//#endif
//#include <process.h>
#ifdef _MSC_VER
static char *snapshot_filename = "snapshot.bmp";
#else
static char *snapshot_filename = "snapshot.png";
#endif
void SSR_reply_snapshot(SSR_request *request)
{
	int iret;
	char *blob;
	Snapshot();
	iret = load_file_blob(snapshot_filename,request->blob,request->len);
	if(!iret) 
		printf("snapshot file not found %s\n",snapshot_filename);
	
}
void SSR_reply(ttglobal tg){
	//called by D: _DisplayThread at end of draw loop (or just before SSR_dequeue_request at start of next loop)
	//only one ssr_current_request is processed per frame/draw loop
	if(ssr_current_request){
		SSR_request *request = ssr_current_request;
		switch(request->type){
			case POSEPOSE:
				SSR_reply_pose(request); break;
			case POSESNAPSHOT:
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