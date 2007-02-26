#include "Eai_C.h"
void _handleReWireCallback (char *buf) {
printf ("handleReWireCallback for %s\n",buf);

}

void sendMIDITableToFreeWRL(char *buf) {
	char *ptr;
        ptr = _X3D_make1StringCommand(REWIREMIDIINFO,buf);
}
