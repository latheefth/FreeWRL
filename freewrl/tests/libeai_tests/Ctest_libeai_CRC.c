/* Advanced EAI test
 * Draw a sphere and then move it around some
 *
 * Author:  dug9
 */

#include <FreeWRLEAI/EAI_C.h>
#define WIN32
#define SPHERE          "Sphere"
#define BLUE            "0.2 0.2 0.8"
X3DNode *myRoot;
X3DNode *shape1;
X3DEventIn *addChildren;
/* simple function to make shapes up */
X3DNode *makeSimpleShape (char * shape, char *colour, char *posn) {
	char myline[2000];
	sprintf (myline, "Transform{translation %s children Shape{" \
						"  appearance Appearance { \n" \
						"    material Material {" \
						"      diffuseColor %s" \
						"    }" \
						"  }" \
						"  geometry %s {}" \
						"}}",posn,colour,shape);
        return X3D_createVrmlFromString(myline);
}
int main() {
	/* Initialization function.  Connects to FreeWRL EAI server.  This function must be called before any other EAI calls */
    X3D_initialize("");
	/* Get a pointer to the node called "ROOT" in the current scenegraph */
    myRoot = X3D_getNode("ROOTNODE");
	if (myRoot == 0) {
		printf("ERROR: node not found!\n");
		getchar();
		return;
	}
	/* Get a pointer to the eventIn called "addChildren" of the ROOT node in the current scenegraph */ 
    addChildren = X3D_getEventIn(myRoot, "addChildren");
	if (addChildren == 0) {
		printf("ERROR: event not found!\n");
		getchar();
		return;
	}
    /* Utility function call which creates an X3D node - in this case a blue sphere */
	shape1 = makeSimpleShape(SPHERE, BLUE, "-2.3 2.1 0");
	/* Set the value of the EventIn "addChildren" to a blue sphere. */
	X3D_setValue(addChildren, shape1);
	/* Wait so we can observe the results */
/*	sleep(10);                          */
	printf("press [Enter] to exit when finished...");
	getchar();
    /* Free memory */
    X3D_freeNode(myRoot);
	X3D_freeNode(shape1);
    X3D_freeEventIn(addChildren);
	/* Shutdown FreeWRL */
	X3D_shutdown();
}

