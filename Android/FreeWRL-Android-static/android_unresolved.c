
/*
 * ANDROID Build of FreeWRL
 * unresolved externals - these are functions used by the library that should not be there!
*/

#include <config.h>
#include <system.h>

#if defined( _ANDROID )

void resetGeometry(void)
{
	// fwCommonX11.x
}

//void killErrantChildren(void)
//{
	// pluginUtils.c
//}

//int doBrowserAction()
//{
	// pluginUtils.c
//	return 1;
//}

int fv_open_display(void)
{
	return 1;
}

bool fv_create_GLcontext(void)
{
	return 1;
}

int fv_create_main_window(int argc, char *argv[])
{
	return 1;
}

bool bind_GLcontext()
{
	return 1;
}

void setCursor(void) {
	//MainLoop.c 744
}

void setWindowTitle(void) {
	//display.c 172
}


#endif
