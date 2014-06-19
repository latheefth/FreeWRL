#ifdef JAVASCRIPT_STUB
// stub
#include "../world_script_stub/jsUtils.c"
#elif JAVASCRIPT_DUK
// duktape
#include "../world_script_stub/jsUtils.c" //stub
//#include "../world_script_duk/5FunctionMethod.c"
#else
// spidermonkey, the default
#include "../world_script/jsUtils.c"
#endif