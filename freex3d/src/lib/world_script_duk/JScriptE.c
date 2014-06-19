#ifdef JAVASCRIPT_STUB
// stub
#include "../world_script_stub/jsVRML_SFClasses.c"
#elif JAVASCRIPT_DUK
// duktape
#include "../world_script_duk/duktape.c"  
//#include "../world_script_stub/jsVRML_SFClasses.c"  //take the stub
#else
// spidermonkey, the default
#include "../world_script/jsVRML_SFClasses.c"
#endif