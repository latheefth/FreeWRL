//#include <libversion.h>
const char *libFreeWRL_get_version(void);// {return FW_LIB_VERSION_STR;}
const char *freewrl_get_version(void) { return libFreeWRL_get_version();}

