// include file for sound engine client/server

#define SNDMAXMSGSIZE 256
typedef struct {
	long	mtype;	// message type
	char	msg[SNDMAXMSGSIZE]; // message data
} FWSNDMSG;

// states of the sound engine
#define SOUND_FAILED  2
#define SOUND_STARTED 1
#define SOUND_NEEDS_STARTING 3

#define MAXSOUNDS 20

