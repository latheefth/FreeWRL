/*
 *  Simple EAI test - draw a blue sphere.
 *  Author:  Sarah Dumoulin
 */

#include <FreeWRLEAI/EAI_C.h>

#define SPHERE          "Sphere"
#define BLUE            "0.2 0.2 0.8"

X3DNode *myRoot;
X3DNode *shape1;
X3DEventIn *addChildren;

/* simple function to make shapes up */
X3DNode *makeSimpleShape (char * shape, char *colour, char *posn) {
        char myline[2000];

        sprintf (myline, " <Shape> <Appearance> <Material/> </Appearance> <Sphere/> </Shape>");


        return X3D_createX3DFromString(myline);
}

int main() {
    /* Initialization function.  Connects to FreeWRL EAI server.  This function must be called before any other EAI calls */

    X3D_initialize("");
printf ("simple, past X3D_initialize\n");
printf ("simple, sleeping...\n");
sleep(2);


    /* Get a pointer to the node called "ROOT" in the current scenegraph */
printf ("simple, calling X3D_getNode\n");

    myRoot = X3D_getNode("ROOTNODE");
printf ("simple, got ROOTNODE\n");


    if (myRoot == 0) {
        printf("ERROR: node not found!\n");
        return;
    }

printf ("simple, myRoot %p\n",myRoot);


    /* Get a pointer to the eventIn called "addChildren" of the ROOT node in the current scenegraph */
    addChildren = X3D_getEventIn(myRoot, "addChildren");
printf ("simple, addChildren eventIn %p\n",addChildren);


    if (addChildren == 0) {
        printf("ERROR: event not found!\n");
        return;
    }

    /* Utility function call which creates an X3D node - in this case a blue sphere */
    shape1 = makeSimpleShape(SPHERE, BLUE, "-2.3 2.1 0");

printf ("simple, result from makeSimpleShape shape1 %p\n",shape1);
printf ("simple,shape1 has %d nodes\n",shape1->X3D_MFNode.n);


printf ("simple, adding child\n");
    /* Set the value of the EventIn "addChildren" to a blue sphere. */
    X3D_setValue(addChildren, shape1);                 
printf ("simple, addChildren, shape1 done \n");


    /* Wait so we can observe the results */
    printf("We just drew a sphere!\npausing for 100 seconds so the results can be observed...\n");
    sleep(100);

printf ("simple, shutting down...\n");

    /* Free memory */
    X3D_freeNode(myRoot);
    X3D_freeNode(shape1);
    X3D_freeEventIn(addChildren);

    /* Shutdown FreeWRL */
    X3D_shutdown();
}
