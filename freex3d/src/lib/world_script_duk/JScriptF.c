#ifdef JAVASCRIPT_STUB
// stub
#include "../world_script_stub/jsVRML_MFClasses.c"
#elif JAVASCRIPT_DUK
// duktape
#include "../world_script_stub/jsVRML_MFClasses.c"  //take the stub
#else
// spidermonkey, the default
#include "../world_script/jsVRML_MFClasses.c"
#endif