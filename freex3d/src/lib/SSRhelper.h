#include <pthread.h>
enum {
	SSR_POSEPOSE = 0,
	SSR_POSESNAPSHOT = 1
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
} SSR_request;
