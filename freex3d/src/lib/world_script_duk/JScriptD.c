#ifdef JAVASCRIPT_STUB
// stub
#include "../world_script_stub/jsVRMLClasses.c"
#elif JAVASCRIPT_DUK
// duktape
#include "../world_script_stub/jsVRMLClasses.c" //take the stub
#else
// spidermonkey, the default
#include "../world_script/jsVRMLClasses.c"
#endif