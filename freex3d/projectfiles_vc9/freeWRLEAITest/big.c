#include <stdio.h>
#include "Eai_C.h"

X3DNode *myRoot;

X3DNode *X3DClock;
X3DNode *X3DColumnpath;

X3DEventOut *X3DClock_frac_changed;
int verbose = 1;
void myListener (void *mydata) {
	if(verbose){
		printf ("LISTENER CALLED");
		if(mydata) printf("%f",*((float*)mydata));
		printf("\n");
	}
}

int main () {

	X3D_initialize ("");

	myRoot = X3D_getNode ("ROOT");

	X3DClock = X3D_getNode("CLOCK");

	X3DClock_frac_changed = X3D_getEventOut(X3DClock,"fraction_changed");
	
	/* Lets throw this one - catch the output of the Clock and print it */
	X3DAdvise (X3DClock_frac_changed, myListener);

#ifdef _MSC_VER
	Sleep(1000);
#else
	sleep(10);
#endif
	//there doesn't seem to be a X3DUnAdvise(X3DClock_frac_changed);
	verbose = 0;
	printf("press Enter to shutdown:");
	getchar();
	X3D_shutdown ();
}
