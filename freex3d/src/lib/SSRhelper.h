#include <pthread.h>
enum {
	SSR_INITPOSE = 0,
	SSR_POSEPOSE = 1,
	SSR_POSESNAPSHOT = 2,
};
typedef struct SSR_request {
	int type;
	pthread_mutex_t requester_mutex;
	pthread_cond_t requester_condition;
	double quat4[4]; //rot
	double vec3[3]; //trans
	char *blob;
	int len;
	int answered;
	//added for ssr2 api
	int LOD;
	int levels_available;
	int status;
	double extent[6];
	int isInside;
	double avatarHeight;
	double fov;
	//double aspect;
} SSR_request;
