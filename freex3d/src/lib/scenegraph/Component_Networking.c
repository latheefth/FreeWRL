/*


X3D Networking Component

*/


/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/



#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include "../vrml_parser/Structs.h"
#include "../vrml_parser/CRoutes.h"
#include "../main/headers.h"

#include "../input/EAIHeaders.h"
#include "../input/EAIHelpers.h"
#include "../opengl/Frustum.h"
#include "../opengl/Textures.h"

#include "Component_Networking.h"
#include "Children.h"

#include <libFreeWRL.h>
#include <list.h>
#include <io_http.h>
#ifdef WANT_OSC
	#include <lo/lo.h>
	#include "ringbuf.h"
	#define USE_OSC 1
	#define TRACK_OSC_MSG 0
#else
	#define USE_OSC 0
#endif

#if USE_OSC
#include "../vrml_parser/CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../world_script/JScript.h"
#include "../world_script/CScripts.h"
#include "../world_script/fieldSet.h"
#include "../vrml_parser/CParseParser.h"
#include "../vrml_parser/CParseLexer.h"
#include "../vrml_parser/CProto.h"
#include "../vrml_parser/CParse.h"
#endif

//OLDCODE #define BUTTON_PRESS_STRING "use_for_buttonPresses"

#if USE_OSC
/**************** START OF OSC node **************************/
/* DJTRACK_OSCSENSORS */

void error(int num, const char *m, const char *path);
void utilOSCcounts(char *types , int *intCount, int *fltCount, int *strCount, int *blobCount, int *midiCount, int *otherCount);

/* We actually want to keep this one inline, as it offers ease of editing with minimal infrastructure */
#include "OSCcallbacks.c"

int serverCount=0;
#define MAX_OSC_SERVERS 32
int serverPort[MAX_OSC_SERVERS] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
lo_server_thread oscThread[MAX_OSC_SERVERS] ;

static uintptr_t *OSC_Nodes = NULL;
static int num_OSC_Nodes = 0;
int curr_OSC_Node = 0;
int active_OSC_Nodes = FALSE;

void utilOSCcounts(char *types , int *intCount, int *fltCount, int *strCount, int *blobCount, int *midiCount, int *otherCount) {
	*intCount	= 0;
	*fltCount	= 0;
	*strCount	= 0;
	*blobCount	= 0;
	*midiCount	= 0;
	*otherCount	= 0;

	int i,j;

	j=strlen(types) ;
	/* what amount of storage */
	for (i=0 ; i < j ; i++) {
		switch (types[i]) {
		case 'i':
			(*intCount)++;
			break;
		case 'f':
			(*fltCount)++;
			break;
		case 's':
			(*strCount)++;
			break;
		case 'b':
			(*blobCount)++;
			break;
		case 'm':
			(*midiCount)++;
			break;
		default:
			(*otherCount)++;
			break;
		}
	}
}

void activate_OSCsensors() {
	curr_OSC_Node = 0 ;
	active_OSC_Nodes = TRUE ;
	struct X3D_OSC_Sensor *realnode ;
	char buf[32];
	int i ;
	/* what amount of storage */
	int fltCount;
	int intCount;
	int strCount;
	int blobCount;
	int midiCount;
	int otherCount;

	while (active_OSC_Nodes && curr_OSC_Node < num_OSC_Nodes) {
		realnode = (struct X3D_OSC_Sensor *) OSC_Nodes[curr_OSC_Node] ;
        if (checkNode(realnode,__FILE__,__LINE__)) {
		#if TRACK_OSC_MSG
		printf("activate_OSCsensors : %s,%d node=%p name=%s\n", __FILE__,__LINE__,realnode,realnode->description->strptr) ;
		#endif
		if (realnode->_status < 0) {
			printf("activate_OSCsensors : %s,%d Moving %s to ready.\n", __FILE__,__LINE__,realnode->description->strptr) ;
			realnode->_status = 0 ;
		} else if (realnode->_status == 0) {
			printf("activate_OSCsensors : %s,%d\n", __FILE__,__LINE__) ;
			printf("activate_OSCsensors : enabled=%d\n",	realnode->enabled) ;
			printf("activate_OSCsensors : gotEvents=%d\n",	realnode->gotEvents) ;
			printf("activate_OSCsensors : description=%s\n",realnode->description->strptr) ;
			printf("activate_OSCsensors : protocol=%s\n",	realnode->protocol->strptr) ;
			printf("activate_OSCsensors : port=%d\n",	realnode->port) ;
			printf("activate_OSCsensors : filter=%s\n",	realnode->filter->strptr) ;
			printf("activate_OSCsensors : handler=%s\n",	realnode->handler->strptr) ;
/*
11715                     if(allFields) {
11716                         spacer fprintf (fp,"\t_talkToNodes (MFNode):\n");
11717                         for (i=0; i<tmp->_talkToNodes.n; i++) { dump_scene(fp,level+1,tmp->_talkToNodes.p[i]); }
11718                     }
11719                     if(allFields) {
11720                         spacer fprintf (fp,"\t_status (SFInt32) \t%d\n",tmp->_status);
11721                     }
11722                     if(allFields) {
11723                         spacer fprintf (fp,"\t_floatInpFIFO (SFNode):\n"); dump_scene(fp,level+1,tmp->_floatInpFIFO);
11724                     }
11725                     if(allFields) {
11726                         spacer fprintf (fp,"\t_int32OutFIFO (SFNode):\n"); dump_scene(fp,level+1,tmp->_int32OutFIFO);
11727                     }
11728                         spacer fprintf (fp,"\ttalksTo (MFString): \n");
11729                         for (i=0; i<tmp->talksTo.n; i++) { spacer fprintf (fp,"                 %d: \t%s\n",i,tmp->talksTo.p[i]->strptr); }
*/
			printf("activate_OSCsensors : talksTo=[ ");
			for (i=0; i < realnode->talksTo.n; i++) {
				printf("\"%s\" ",realnode->talksTo.p[i]->strptr);
				/* This would be a good time to convert the name into an entry in _talkToNodes */
				struct X3D_Node * myNode;
				/* myNode = X3DParser_getNodeFromName(realnode->talksTo.p[i]->strptr); */
				myNode = parser_getNodeFromName(realnode->talksTo.p[i]->strptr);
				if (myNode != NULL) {
					printf("(%p) ",(void *)myNode);
				} else {
					printf("(..) ");
				}
			}
			printf("] (%d nodes) (Need to fix %s,%d)\n",realnode->talksTo.n , __FILE__,__LINE__);
			printf("activate_OSCsensors : listenfor=%s , expect %d parameters\n", realnode->listenfor->strptr , (int)strlen(realnode->listenfor->strptr)) ;
			printf("activate_OSCsensors : FIFOsize=%d\n",	realnode->FIFOsize) ;
			printf("activate_OSCsensors : _status=%d\n",	realnode->_status) ;

			if (realnode->FIFOsize > 0) {
				/* what amount of storage */
				utilOSCcounts(realnode->listenfor->strptr,&intCount,&fltCount,&strCount,&blobCount,&midiCount,&otherCount);
				intCount = realnode->FIFOsize * (intCount + midiCount); 
				fltCount = realnode->FIFOsize * fltCount;
				strCount = realnode->FIFOsize * (strCount + blobCount + otherCount); 
				printf("Allocate %d floats, %d ints for '%s'\n",fltCount,intCount,realnode->description->strptr);

				realnode->_int32InpFIFO = (void *) NewRingBuffer (intCount) ;
				realnode->_floatInpFIFO = (void *) NewRingBuffer (fltCount) ;
				realnode->_stringInpFIFO = (void *) NewRingBuffer (strCount) ;

			}

			/* start a new server on the required port */
			int foundCurrentPort = -1 ;
			for ( i=0 ; i < num_OSC_Nodes ; i++) {
				if(realnode->port == serverPort[i]) {
					foundCurrentPort=i;
					i = num_OSC_Nodes+1;
				}
			}
			if (foundCurrentPort < 0) {
				foundCurrentPort = serverCount ;
				serverPort[foundCurrentPort] = realnode->port ;
				serverCount++ ;

				sprintf (buf,"%d",realnode->port);

				if (strcmp("TCP",realnode->protocol->strptr)==0) {
					/* oscThread[foundCurrentPort] = lo_server_thread_new_with_proto(buf, LO_TCP, error); */
					oscThread[foundCurrentPort] = lo_server_thread_new(buf, error);
				} else if (strcmp("UNIX",realnode->protocol->strptr)==0) {
					/* oscThread[foundCurrentPort] = lo_server_thread_new_with_proto(buf, LO_UNIX, error); */
					oscThread[foundCurrentPort] = lo_server_thread_new(buf, error);
				} else {
					/* oscThread[foundCurrentPort] = lo_server_thread_new_with_proto(buf, LO_UDP, error); */
					oscThread[foundCurrentPort] = lo_server_thread_new(buf, error);
				}
				lo_server_thread_start(oscThread[foundCurrentPort]);
			}

			printf("%d servers; current server is running in slot %d on port %d\n",serverCount,foundCurrentPort,serverPort[foundCurrentPort]) ;

			/* add method (by looking up its name) that will in future use the required path */
			/* need to re-read the lo code to check that you can register the same callback twice (or more) with different paths) */
			/* See OSCcallbacks.c */
			int foundHandler = 0 ;
			for (i=0 ; i < OSCfuncCount ; i++) {
				printf("%d/%d : Check %s against %s\n",i,OSCfuncCount,realnode->handler->strptr,OSCfuncNames[i]);
				if (0 == strcmp(realnode->handler->strptr,OSCfuncNames[i])) {foundHandler = i;}
			}
			if (OSCcallbacks[foundHandler] != NULL) {
				printf("Going to hook '%s' to '%s' handler\n",realnode->description->strptr ,OSCfuncNames[foundHandler]) ;
				lo_server_thread_add_method(oscThread[foundCurrentPort], realnode->filter->strptr, realnode->listenfor->strptr,
					(OSCcallbacks[foundHandler]), realnode);
			}

			realnode->_status = 1 ;
			/* We only want one OSC node to become active in one slowtick */
			active_OSC_Nodes = FALSE ;
		} 
        } // end of checkNode conditional - JAS
            
		curr_OSC_Node++;
	}
}

void error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
}

void add_OSCsensor(struct X3D_Node * node) {
	uintptr_t *myptr;

	if (node == 0) {
		printf ("error in registerOSCNode; somehow the node datastructure is zero \n");
		return;
	}

	if (node->_nodeType != NODE_OSC_Sensor) return;

	OSC_Nodes = (uintptr_t *) REALLOC (OSC_Nodes,sizeof (uintptr_t *) * (num_OSC_Nodes+1));
	myptr = OSC_Nodes;

	/* now, put the node pointer into the structure entry */
	*myptr = (uintptr_t) node;

	num_OSC_Nodes++;
}
/***************** END OF OSC node ***************************/
#else
void add_OSCsensor(struct X3D_Node * node) {}
#endif


void render_LoadSensor (struct X3D_LoadSensor *node) {
	int count;
	int nowLoading;
	int nowFinished;
	struct X3D_ImageTexture *tnode;
#ifdef HAVE_TO_REIMPLEMENT_MOVIETEXTURES
	struct X3D_MovieTexture *mnode;
#endif /* HAVE_TO_REIMPLEMENT_MOVIETEXTURES */
	struct X3D_AudioClip *anode;
	//struct X3D_Inline *inode;
	
	/* if not enabled, do nothing */
	if (!node) return;
	if (node->__oldEnabled != node->enabled) {
		node->__oldEnabled = node->enabled;
		MARK_EVENT(X3D_NODE(node),offsetof (struct X3D_LoadSensor, enabled));
	}
	if (!node->enabled) return;

	/* we only need to look at this during the rendering pass - once per event loop */
	if (!renderstate()->render_geom) return;

	/* do we need to re-generate our internal variables? */
	if NODE_NEEDS_COMPILING {
		MARK_NODE_COMPILED
		node->__loading = 0;
		node->__finishedloading = 0;
		node->progress = (float) 0.0;
		node->__StartLoadTime = 0.0;
	}

	/* do we actually have any nodes to watch? */
	if (node->watchList.n<=0) return;

	/* are all nodes loaded? */
	if (node->__finishedloading == node->watchList.n) return;

	/* our current status... */
	nowLoading = 0;
	nowFinished = 0;

	/* go through node list, and check to see what the status is */
	/* printf ("have %d nodes to watch\n",node->watchList.n); */
	for (count = 0; count < node->watchList.n; count ++) {

		tnode = (struct X3D_ImageTexture *) node->watchList.p[count];

		/* printf ("node type of node %d is %d\n",count,tnode->_nodeType); */
		switch (tnode->_nodeType) {
		case NODE_ImageTexture:
			/* printf ("opengl tex is %d\n",tnode->__texture); */
			/* is this texture thought of yet? */
			nowLoading++;
			if (fwl_isTextureLoaded(tnode->__textureTableIndex)) {
				/* is it finished loading? */
				nowFinished ++;
			}
				
			break;

		case NODE_MovieTexture:
#ifdef HAVE_TO_REIMPLEMENT_MOVIETEXTURES
			mnode = (struct X3D_MovieTexture *) tnode; /* change type to MovieTexture */
			/* printf ("opengl tex is %d\n",mnode->__texture0_); */
			/* is this texture thought of yet? */
			if (mnode->__texture0_ > 0) {
				nowLoading++;
				/* is it finished loading? */
				if (fwl_isTextureLoaded(mnode->__texture0_)) nowFinished ++;
			}
#endif /* HAVE_TO_REIMPLEMENT_MOVIETEXTURES */
				
			break;

		case NODE_Inline:
			//inode = (struct X3D_Inline *) tnode; /* change type to Inline */
			/* printf ("LoadSensor, Inline %d, type %d loadstatus %d at %d\n",inode,inode->_nodeType,inode->__loadstatus, &inode->__loadstatus); */
			break;

		case NODE_Script:
			nowLoading ++; /* broken - assume that the url is ok for now */
			break;

		case NODE_AudioClip:
			anode = (struct X3D_AudioClip *) tnode; /* change type to AudioClip */
			/* AudioClip sourceNumber will be gt -1 if the clip is ok. see code for details */
			if (anode->__sourceNumber > -1) nowLoading ++;

			break;

		default :{} /* there should never be anything here, but... */
		}
	}
		

	/* ok, are we NOW finished loading? */
	if (nowFinished == node->watchList.n) {
		node->isActive = 0;
		MARK_EVENT (X3D_NODE(node), offsetof (struct X3D_LoadSensor, isActive));

		node->isLoaded = 1;
		MARK_EVENT (X3D_NODE(node), offsetof (struct X3D_LoadSensor, isLoaded));

		node->progress = (float) 1.0;
		MARK_EVENT (X3D_NODE(node), offsetof (struct X3D_LoadSensor, progress));

		node->loadTime = TickTime();
		MARK_EVENT (X3D_NODE(node), offsetof (struct X3D_LoadSensor, loadTime));
	}	

	/* have we NOW started loading? */
	if ((nowLoading > 0) && (node->__loading == 0)) {
		/* mark event isActive TRUE */
		node->isActive = 1;
		MARK_EVENT (X3D_NODE(node), offsetof (struct X3D_LoadSensor, isActive));

	
		node->__StartLoadTime = TickTime();
	}
	
	/* what is our progress? */
	if (node->isActive == 1) {
		node->progress = (float)(nowFinished)/(float)(node->watchList.n);
		MARK_EVENT (X3D_NODE(node), offsetof (struct X3D_LoadSensor, progress));
	}

	/* remember our status for next time. */
	node->__loading = nowLoading;
	node->__finishedloading = nowFinished;

	/* did we run out of time? */
	if (node->timeOut > 0.0001) {			/* we have a timeOut specified */
		if (node->__StartLoadTime > 0.001) {	/* we have a start Time recorded from the isActive = TRUE */
		
			/* ok, we should look at time outs */
			if ((TickTime() - node->__StartLoadTime) > node->timeOut) {
				node->isLoaded = 0;
				MARK_EVENT (X3D_NODE(node), offsetof (struct X3D_LoadSensor, isLoaded));

				node->isActive = 0;
				MARK_EVENT (X3D_NODE(node), offsetof (struct X3D_LoadSensor, isActive));

				/* and, we will just assume that we have loaded everything next iteration */
				node->__finishedloading = node->watchList.n;
			}
		}
	}
}


void child_Anchor (struct X3D_Anchor *node) {
	int nc = (node->children).n;
	LOCAL_LIGHT_SAVE

	/* printf ("child_Anchor node %u, vis %d\n",node,node->_renderFlags & VF_hasVisibleChildren); */

	/* any children at all? */
	if (nc==0) return;

	/* any visible children? */
	OCCLUSIONTEST

	#ifdef CHILDVERBOSE
	printf("RENDER ANCHOR START %d (%d)\n",node, nc);
	#endif

	/* do we have a local light for a child? */
	LOCAL_LIGHT_CHILDREN(node->children);

	/* now, just render the non-directionalLight children */
	normalChildren(node->children);

	#ifdef CHILDVERBOSE
	printf("RENDER ANCHOR END %d\n",node);
	#endif

	LOCAL_LIGHT_OFF
}



/* note that we get the resources in a couple of steps; this tries to keep the scenegraph running */
void load_Inline (struct X3D_Inline *node) {
	resource_item_t *res;
	// printf ("load_Inline %u, loadStatus %d loadResource %u\n",node, node->__loadstatus, node->__loadResource);

    //printf ("load_Inline, node %p loadStatus %d\n",node,node->load);
    
	if (node->load) {
		/* printf ("loading Inline\n");  */

		switch (node->__loadstatus) {
			case INLINE_INITIAL_STATE: /* nothing happened yet */

			if (node->url.n == 0) {
				node->__loadstatus = INLINE_STABLE; /* a "do-nothing" approach */
			} else {
				res = resource_create_multi(&(node->url));
				res->media_type = resm_unknown;
				node->__loadstatus = INLINE_REQUEST_RESOURCE;
				node->__loadResource = res;
			}
			break;

			case INLINE_REQUEST_RESOURCE:
			res = node->__loadResource;
			resource_identify(node->_parentResource, res);
			/* printf ("load_Inline, we have type  %s  status %s\n",
				resourceTypeToString(res->type), resourceStatusToString(res->status)); */
			res->actions = resa_download | resa_load; //not resa_parse which we do below
			//frontenditem_enqueue(ml_new(res));
			resitem_enqueue(ml_new(res));
			node->__loadstatus = INLINE_FETCHING_RESOURCE;
			break;

			case INLINE_FETCHING_RESOURCE:
			res = node->__loadResource;
			/* printf ("load_Inline, we have type  %s  status %s\n",
				resourceTypeToString(res->type), resourceStatusToString(res->status)); */
			if(res->complete){
				if (res->status == ress_loaded) {
					//determined during load process by resource_identify_type(): res->media_type = resm_vrml; //resm_unknown;
					res->whereToPlaceData = X3D_NODE(node);
					res->offsetFromWhereToPlaceData = offsetof (struct X3D_Inline, __children);
					res->actions = resa_process;
					node->__loadstatus = INLINE_PARSING; // a "do-nothing" approach 
					//tell it to instance (vs library)
					node->__protoFlags = ciflag_set(node->__protoFlags,1,0);
					res->complete = FALSE;
					//send_resource_to_parser(res);
					//send_resource_to_parser_if_available(res);
					resitem_enqueue(ml_new(res));
				} else if ((res->status == ress_failed) || (res->status == ress_invalid)) {
					//no hope left
					printf ("resource failed to load\n");
					node->__loadstatus = INLINE_STABLE; // a "do-nothing" approach 
				}
			}
			break;

			case INLINE_PARSING:
				res = node->__loadResource;

				//printf ("inline parsing.... %s\n",resourceStatusToString(res->status));
				//printf ("res complete %d\n",res->complete);
				if(res->complete){
					if (res->status == ress_parsed) {
						node->__loadstatus = INLINE_STABLE; 
					} 
				}

			break;
			case INLINE_STABLE:
			break;
		}

	} else {
		printf ("unloading Inline\n");
	}
}



void child_Inline (struct X3D_Inline *node) {
	int nc = (node->__children).n;

	LOCAL_LIGHT_SAVE

	#ifdef CHILDVERBOSE
	printf("RENDER INLINE START %d (%d)\n",node, nc);
	printf ("	child_Inline, %u, loadStatus %d, nc %d\n",node,node->__loadstatus, nc);
	#endif

	#ifdef CHILDVERBOSE
		{int i;
			for (i=0; i<nc; i++) { printf ("ch %d %s ",i,stringNodeType(X3D_NODE(node->children.p[i])->_nodeType));} 
		printf ("\n");}
	#endif

	/* any children at all? */
	if (nc==0) return; 

	/* do we have a local light for a child? */
	LOCAL_LIGHT_CHILDREN(node->__children);

	/* now, just render the non-directionalLight children */
	normalChildren(node->__children);

	#ifdef CHILDVERBOSE
	printf("RENDER INLINE END %d\n",node);
	#endif

	LOCAL_LIGHT_OFF
}
