#ifdef JAVASCRIPT_STUB
// stub
#include "../world_script_stub/JScript.c"
#elif JAVASCRIPT_DUK
// duktape
#include "../world_script_duk/JScript.c"
#else
// spidermonkey, the default
#include "../world_script/JScript.c"
#endif
