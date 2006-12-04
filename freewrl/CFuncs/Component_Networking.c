/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Networking Component

*********************************************************************/

#include <math.h>
#include "headers.h"
#include "installdir.h"

void make_ReWireMidiControl (struct X3D_ReWireMidiControl *node) {
	printf ("make ReWire\n");
}
void prep_ReWireMidiControl (struct X3D_ReWireMidiControl *node) {
	printf ("prep ReWire\n");
}
void fin_ReWireMidiControl (struct X3D_ReWireMidiControl *node) {
	printf ("fin ReWire\n");
}
void changed_ReWireMidiControl (struct X3D_ReWireMidiControl *node) {
	printf ("changed ReWire\n");
}
void render_ReWireMidiControl (struct X3D_ReWireMidiControl *node) {
	printf ("render ReWire\n");
}

/* add up the characters in a string; return lowest 12 bits of the count */
unsigned int returnSumofString(struct Uni_String *str) {
	unsigned int sum;
	int count;
	int start;
	int end;

	sum = 0;
	if (str->len == 0) return sum;

	/* find the start and end - remove whitespace */
	start=0;
	while ((start < (str->len)) && ((str->strptr[start]) <= ' ')) start++;
	end = str->len-1;
	while ((end >= 0) && ((str->strptr[end]) <= ' ')) end--;

	/* printf ("returnSumofString start %d end %d len %d\n",start,end,str->len); */
	for (count = start; count <= end; count++) {
		sum += str->strptr[count];
	}

	sum &= 0x3ff;
	/* printf ("returning %x\n",sum); */
	return sum;
}


void compile_ReWireMidiControl (struct X3D_ReWireMidiControl *node) {
	unsigned int newEncodedName;
#ifdef DOREWIRE	
	newEncodedName = 0;
	if (node->bus < 0) 
		newEncodedName |= 0x0ff;
	else
		newEncodedName |= (node->bus & 0xff);

	/* shuffle bits about to make up a (hopefully unique) name */
	newEncodedName |= (returnSumofString(node->channel)  << 8);
	newEncodedName |= (returnSumofString(node->deviceName)  << 20);

	/* EncodedName - bits are as follows */
	/*
	printf ("compile ReWire\n");
	printf ("old encodedName %x newEncodedName %x\n",node->_encodedName, newEncodedName);
	*/
	if ((unsigned int) node->_encodedName != newEncodedName) {
		printf ("Name Changed!!\n");
		node->_encodedName = (unsigned int) newEncodedName;
	}

	printf ("worry about max/min values\n");

	MARK_NODE_COMPILED 
#endif
}

void do_ReWireMidiControl (void *this) {
#ifdef DOREWIRE
	struct X3D_ReWireMidiControl* node;
	int mySendValue;
	int possibleValueSpread;
	int minV, maxV;
	float fV;
	int sendEvent;

	node = (struct X3D_ReWireMidiControl*) this;

	/* printf ("do ReWireMidiControl for node %s\n",stringNodeType(node->_nodeType)); */
	if (NODE_ReWireMidiControl == node->_nodeType) {

		/* do we need to "compile" this node? */
		COMPILE_IF_REQUIRED

		/* printf ("ReWire change %d %d ", node->_ichange, node->_change); 

		printf ("bus %d channel :%s: controllerType :%s: device :%s: ",
			node->bus, node->channel->strptr, node->controllerType->strptr, node->deviceName->strptr);
		printf (" devp %d fv %f iv %d hr %d ct %d intVal %d max %d min %d\n",
			node->devicePresent, node->floatValue, node->intValue, node->highResolution,
			node->intControllerType, node->intValue, node->maxVal, node->minVal);
		*/

		/* find the min and max values; see what we want, and see what the device
		   can handle */
		minV = 0;  maxV = 100000;
		if (minV < node->deviceMinVal) minV = node->deviceMinVal;
		if (minV < node->minVal) minV = node->minVal;
		if (maxV > node->deviceMaxVal) maxV = node->deviceMaxVal;
		if (maxV > node->maxVal) maxV = node->maxVal;
		
		possibleValueSpread = maxV-minV +1;
		if (node->useIntValue) {
			mySendValue = node->intValue;

			fV = 0.0; /* fixme */
			fV = node->intValue * possibleValueSpread + minV;
			
		} else {
			/* convert the float to an int value */
			fV = node->floatValue * possibleValueSpread + minV;
			mySendValue =  (int) fV;
		}

		/* record our current value - may or may not actually send this */
		node->intValue = mySendValue;

		/* should we send this event? */
		sendEvent = FALSE;
		if (node->continuousEvents) {
			if (node->intValue != node->_oldintValue) {
				sendEvent = True;
				node->_oldintValue = node->intValue;
			}
		} else sendEvent = TRUE;

		if (sendEvent) {
			printf ("intValue changed - now is %d\n",node->intValue);
		
			/* calculate the floatValue from the intValue */
			printf ("fv %f minv %d, ps %d\n",fV, minV, possibleValueSpread);

			node->floatValue =  ((float) fV-minV)/((float)possibleValueSpread);

			printf ("sending %d %f ",mySendValue, node->floatValue);
			printf ("mins %d %d maxs %d %d ",node->deviceMinVal, node->minVal, node->deviceMaxVal, node->maxVal);
			printf ("float %f node->floatVal\n",node->floatValue);
		}
	}	
#endif
}


void render_LoadSensor (struct X3D_LoadSensor *node) {
	int total;
	int count;
	int nowLoading;
	int nowFinished;
	struct X3D_ImageTexture *tnode;
	struct X3D_MovieTexture *mnode;
	struct X3D_AudioClip *anode;
	struct X3D_Inline *inode;
	
	/* if not enabled, do nothing */
	if (!node) return;
	if (!node->enabled) return;

	/* we only need to look at this during the rendering pass - once per event loop */
	if (!render_geom) return;

	/* do we need to re-generate our internal variables? */
	if (node->_change != node->_ichange) {
		node->_ichange = node->_change;
		node->__loading = 0;
		node->__finishedloading = 0;
		node->progress = 0.0;
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
			if (isTextureLoaded(tnode->__textureTableIndex)) {
				/* is it finished loading? */
				nowFinished ++;
			}
				
			break;

		case NODE_MovieTexture:
			mnode = (struct X3D_MovieTexture *) tnode; /* change type to MovieTexture */
			/* printf ("opengl tex is %d\n",mnode->__texture0_); */
			/* is this texture thought of yet? */
			if (mnode->__texture0_ > 0) {
				nowLoading++;
				/* is it finished loading? */
				if (isTextureLoaded(mnode->__texture0_)) nowFinished ++;
			}
				
			break;

		case NODE_Inline:
			inode = (struct X3D_Inline *) tnode; /* change type to Inline */
			printf ("LoadSensor, Inline %d, type %d loadstatus %d at %d\n",inode,inode->_nodeType,inode->__loadstatus, &inode->__loadstatus);
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
		mark_event (node, offsetof (struct X3D_LoadSensor, isActive));

		node->isLoaded = 1;
		mark_event (node, offsetof (struct X3D_LoadSensor, isLoaded));

		node->progress = 1.0;
		mark_event (node, offsetof (struct X3D_LoadSensor, progress));

		node->loadTime = TickTime;
		mark_event (node, offsetof (struct X3D_LoadSensor, loadTime));
	}	

	/* have we NOW started loading? */
	if ((nowLoading > 0) && (node->__loading == 0)) {
		/* mark event isActive TRUE */
		node->isActive = 1;
		mark_event (node, offsetof (struct X3D_LoadSensor, isActive));

	
		node->__StartLoadTime = TickTime;
	}
	
	/* what is our progress? */
	if (node->isActive == 1) {
		node->progress = (float)(nowFinished)/(float)(node->watchList.n);
		mark_event (node, offsetof (struct X3D_LoadSensor, progress));
	}

	/* remember our status for next time. */
	node->__loading = nowLoading;
	node->__finishedloading = nowFinished;

	/* did we run out of time? */
	if (node->timeOut > 0.0001) {			/* we have a timeOut specified */
		if (node->__StartLoadTime > 0.001) {	/* we have a start Time recorded from the isActive = TRUE */
		
			/* ok, we should look at time outs */
			if ((TickTime - node->__StartLoadTime) > node->timeOut) {
				node->isLoaded = 0;
				mark_event (node, offsetof (struct X3D_LoadSensor, isLoaded));

				node->isActive = 0;
				mark_event (node, offsetof (struct X3D_LoadSensor, isActive));

				/* and, we will just assume that we have loaded everything next iteration */
				node->__finishedloading = node->watchList.n;
			}
		}
	}
}


void child_Anchor (struct X3D_Anchor *node) {
	int nc = (node->children).n;
	DIRECTIONAL_LIGHT_SAVE


	/* any children at all? */
	if (nc==0) return;

	#ifdef CHILDVERBOSE
	printf("RENDER ANCHOR START %d (%d)\n",node, nc);
	#endif

	/* do we have to sort this node? */
	if ((nc > 1 && !render_blend)) sortChildren(node->children);

	/* do we have a DirectionalLight for a child? */
	if(node->has_light) dirlightChildren(node->children);

	/* now, just render the non-directionalLight children */
	normalChildren(node->children);

	if (render_geom && (!render_blend)) {
		EXTENTTOBBOX
		BOUNDINGBOX
	}

	#ifdef CHILDVERBOSE
	printf("RENDER ANCHOR END %d\n",node);
	#endif

	DIRECTIONAL_LIGHT_OFF
}


void child_Inline (struct X3D_Inline *node) {
	int nc = (node->__children).n;
	DIRECTIONAL_LIGHT_SAVE

	#ifdef CHILDVERBOSE
	printf("RENDER INLINE START %d (%d)\n",node, nc);
	#endif

	/* lets see if we still have to load this one... */
	if ((node->__loadstatus)==0) loadInline(node);


	/* any children at all? */
	if (nc==0) return; 

	/* do we have to sort this node? */
	if ((nc > 1 && !render_blend)) sortChildren(node->__children);

	/* do we have a DirectionalLight for a child? */
	if(node->has_light) dirlightChildren(node->__children);

	/* now, just render the non-directionalLight children */
	normalChildren(node->__children);

	if (render_geom && (!render_blend)) {
		EXTENTTOBBOX
                /* pass the bounding box calculations on up the chain */
                propagateExtent((struct X3D_Box *)node);

		BOUNDINGBOX
	}

	#ifdef CHILDVERBOSE
	printf("RENDER INLINE END %d\n",node);
	#endif

	DIRECTIONAL_LIGHT_OFF
}

void changed_Anchor (struct X3D_Anchor *node) {
                int i;
                int nc = ((node->children).n);
                struct X3D_Box *p;
                struct X3D_Virt *v;

		INITIALIZE_EXTENT
		DIRECTIONAL_LIGHT_FIND
}

