#include <EAI_C.h>
#define SPHERE          "Sphere"
#define CONE			"Cone"
#define BOX				"Box"
#define BLUE            "0.2 0.2 0.8"

X3DNode *root;
X3DNode *shape1;
X3DNode *touchSensor;

X3DEventIn *addChildren;
X3DEventOut *selectionEvent;
int opt = 0;


void* earthSelected();

/* simple function to make shapes up */
X3DNode *makeSimpleShape (char * shape, char *colour, char *posn) {
        char myline[2000];

        sprintf (myline, "Transform {translation %s children Shape{" \
                         "  appearance Appearance { \n" \
                         "    material Material {" \
                         "      diffuseColor %s" \
                         "    }" \
                         "  }" \
                         "  geometry %s {}" \
                         "}}", posn, colour, shape);

        return X3D_createVrmlFromString(myline);
}
X3DNode *makeSimpleShapeX3D (char * shape, char *colour, char *posn) {
        char myline[2000];

        sprintf (myline, "<Transform translation=\" %s \">\n"\
                         "<Shape>\n"\
                         "  <Appearance>\n" \
                         "    <Material diffuseColor=\" %s \"/>\n" \
                         "  </Appearance>\n" \
                         "  <%s/>\n" \
                         "</Shape>\n"\
                         "</Transform>\n", posn, colour, shape);

        return X3D_createX3DFromString(myline);
}
void loadObjects(){
 /* Get a pointer to the node called "ROOT" in the current scenegraph */
	printf("Step 2: X3D_getNode(ROOT)\n");
	root = X3D_getNode("ROOT");
	if (root == 0) {
		printf("ERROR: ROOT node not found!\n"); 
		exit(1);
	}
	else printf("Getting ROOT was OK! addr=%p type%d\n",root->X3D_SFNode.adr,root->X3D_SFNode.type);

 /* Get a pointer to the node called "TOUCH_SENSOR" in the current scenegraph */
	printf("Step 3: X3D_getNode(TOUCH_SENSOR)\n");
	touchSensor = X3D_getNode("TOUCH_SENSOR");
	if (touchSensor == 0) {
		printf("ERROR: node not found!\n");
		exit(1);
	}
	else printf("Getting TOUCH_SENSOR was OK!\n");

    /* Get a pointer to the eventIn called "addChildren" of the ROOT node in the current scenegraph */
	printf("Step 4: X3D_getEventOut(touchSensor, 'isActive')\n");
    	selectionEvent = X3D_getEventOut(touchSensor, "isActive");
   	if (selectionEvent == 0) {
        	printf("ERROR: Selection event not found!\n");
		exit(1);
    	}

	if(1){
		printf("Step 5: X3D_getEventIn(root, 'addChildren')\n");
		addChildren = X3D_getEventIn(root, "addChildren");
   		if (selectionEvent == 0) {
        		printf("ERROR: addChildren event not found!\n");
			exit(1);
    		}

		/* Utility function call which creates an X3D node - in this case a blue sphere */
		if(0) shape1 = makeSimpleShape(SPHERE, BLUE, "-2.3 2.1 0");
	}
   	//Call the function earthSelected when the event occurs.
    	X3DAdvise(selectionEvent, earthSelected);
}

int first = 1;
X3DNode* scene;
void * earthSelected(X3DNode* val, double dtime){
//	char * value = (char *) val;	
	int type, value;
	type = val->X3D_SFBool.type;
	value = val->X3D_SFBool.value;
	printf("Value of TOUCH_SENSOR was changed... to type %d value = %d  time= %lf\n", type, value, dtime);
	if( value == 0 ) return NULL;

    /* Set the value of the EventIn "addChildren" to a blue sphere. */
	if(first==1){
		printf("first click felt in earthSelected\n");
		if(0) X3D_setValue(addChildren, shape1); 
		first = 2;
	}else if(first == 2){
		printf("second click felt in earthSelected\n");
		first = 3;
	}else if(first == 3){
		printf("third click felt - making shape\n");
		shape1 = makeSimpleShape(SPHERE, BLUE, "-2.3 2.1 0");
		first = 4;
	}else if(first == 4){
		printf("fourth click felt - adding shape to ROOT\n");
		X3D_setValue(addChildren, shape1);
		first = 5;
	}else if(first ==5) {
		printf("5th click felt - getting scene = SYSTEMROOT:");
		scene = X3D_getExecutionContext();
		printf("scene node = %p type = %d\n",scene->X3D_SFNode.adr,scene->X3D_SFNode.type);
		first = 6;
	}else if(first ==6) {
		printf("6th click felt - adding shape to SYSTEMROOT\n");
		shape1 = makeSimpleShapeX3D(CONE, "0.2 0.8 0.2", "2.3 2.1 0");
		X3D_rootNodeHandling(scene,shape1,1);
		first = 7;
	}else if(first ==7) {
		printf("7th click felt - removing shape from SYSTEMROOT\n");
		scene = X3D_getExecutionContext();
		X3D_rootNodeHandling(scene,shape1,2);
		first = 8;
	}else{
		printf("yth click felt - press 'q' in graphics window or\n press Enter in this console to shutdown:");
	}
	return NULL;
}
//void unqueue_callback();  
//void dequeue_callback_ev();
void dequeue_callback_ev(int wait);
int main() {
    /* Initialization function.  Connects to FreeWRL EAI server.  This function must be called before any other EAI calls */
	printf("Step 1: X3D_initialize()\n");
	X3D_initialize("");

	loadObjects();

    printf("Sleeping...");
    /* Wait so we can observe the results */

  //  while(1){
		////unqueue_callback(); 
		////dequeue_callback_ev(0);
		////dequeue_callback_ev(1);
		//printf("*");
		//getchar();
  //  }
	getchar();
    /* Free memory */
    X3D_freeNode(root);
    X3D_freeNode(touchSensor);
    X3D_freeNode(shape1);
    X3D_freeEventIn(addChildren);
    X3D_freeEventOut(selectionEvent);

    /* Shutdown FreeWRL */
    X3D_shutdown();
	//   exit(0);

    return 0;
}

