#ifdef JAVASCRIPT_STUB
// stub
#include "../world_script_stub/jsVRMLBrowser.c"
#elif JAVASCRIPT_DUK
// duktape
#include "../world_script_stub/jsVRMLBrowser.c" //stub
//#include "../world_script_duk/FWTYPEs.c"
#else
// spidermonkey, the default
#include "../world_script/jsVRMLBrowser.c"
#endif