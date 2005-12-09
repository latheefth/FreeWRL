/* headers for EAI and java CLASS invocation */

/* function prototypes */
void handle_Listener (void);
void EAI_Convert_mem_to_ASCII (int id, char *reptype, int type, char *memptr, char *buf);
unsigned int EAI_SendEvent (char *ptr);
void EAI_RNewW(char *bufptr);
void EAI_RW(char *bufptr);


#define MAXEAIHOSTNAME	255		/* length of hostname on command line */
#define EAIREADSIZE	2048		/* maximum we are allowed to read in from socket */
#define EAIBASESOCKET   9877		/* socket number to start at */


/* these are commands accepted from the EAI client */
#define GETNODE		'A'
#define UPDATEROUTING 	'B'
#define SENDCHILD 	'C'
#define SENDEVENT	'D'
#define GETVALUE	'E'
#define GETTYPE		'F'
#define	REGLISTENER	'G'
#define	ADDROUTE	'H'
#define REREADWRL	'I'
#define	DELETEROUTE	'J'
#define GETNAME		'K'
#define	GETVERSION	'L'
#define GETCURSPEED	'M'
#define GETFRAMERATE	'N'
#define	GETURL		'O'
#define	REPLACEWORLD	'P'
#define	LOADURL		'Q'
#define CREATEVS	'S'
#define	CREATEVU	'T'
#define	STOPFREEWRL	'U'
#define	NEXTVIEWPOINT	'V'
#define UNREGLISTENER   'W'

/* Subtypes - types of data to get from EAI  - we don't use the ones defined in
   headers.h, because we want ASCII characters */

/* KEEP THESE IN STEP WITH THE EQUIVS IN headers.h */

#define	EAI_SFUNKNOWN		'a'
#define	EAI_SFBOOL		'b'
#define	EAI_SFCOLOR		'c'
#define	EAI_SFFLOAT		'd'
#define	EAI_SFTIME		'e'
#define	EAI_SFINT32		'f'
#define	EAI_SFSTRING		'g'
#define	EAI_SFNODE		'h'
#define	EAI_SFROTATION		'i'
#define	EAI_SFVEC2F		'j'
#define	EAI_SFIMAGE		'k'
#define	EAI_MFCOLOR		'l'
#define	EAI_MFFLOAT		'm'
#define	EAI_MFTIME		'n'
#define	EAI_MFINT32		'o'
#define	EAI_MFSTRING		'p'
#define	EAI_MFNODE		'q'
#define	EAI_MFROTATION		'r'
#define	EAI_MFVEC2F		's'
#define EAI_MFVEC3F		't'
#define EAI_SFVEC3F		'u'


/* Function Prototype for plugins, Java Class Invocation */
int createUDPSocket();
int conEAIorCLASS(int socketincrement, int *sockfd, int *listenfd);
void EAI_send_string (char *str, int listenfd);
char *read_EAI_socket(char *bf, int *bfct, int *bfsz, int *listenfd);
extern int EAIlistenfd;
extern int EAIsockfd;
extern int EAIport;
extern int EAIwanted;
extern int EAIbufsize;
extern char *EAIbuffer;
extern int EAIbufcount;
extern char EAIListenerData[EAIREADSIZE];
extern char EAIListenerArea[40];

