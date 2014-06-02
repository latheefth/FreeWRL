#include <stdio.h>
#include <dllFreeWRL.h>
int main (int argc, char **argv)
{
	CdllFreeWRL *one, *two, *three; //, *four;
	char *sfiles1 [] = {"../../../../tests/1.wrl","../../../../tests/2.wrl","../../../../tests/3.wrl","E:/source2/tests/4.wrl","E:/source2/tests/5.wrl"};
	char *sfiles [] = {
		"../../../../tests/ProgrammableShaders/models/TwoCylinders.wrl",
		"../../../../tests/ProgrammableShaders/models/teapot-Toon.wrl",
		"../../../../tests/ProgrammableShaders/models/flutter2-ProgramShader.x3d",
		"../../../../tests/ProgrammableShaders/models/sobel-ComposedShader.wrl",
		"../../../../tests/ProgrammableShaders/models/teapot-noShaders.wrl",
		"../../../../tests/ProgrammableShaders/models/sobel-ComposedShader.wrl",
		"../../../../tests/ProgrammableShaders/models/flutter2-ComposedShader.x3d"};
	printf("hi from test_dll main\n");
	//one = new CdllFreeWRL(450,300); //
	one = new CdllFreeWRL();
	one->onInit(450,300,NULL,false,false);
#ifndef ANGLEPROJECT
	printf("load 3 freewrl instances from the same dll process\n");
	two = new CdllFreeWRL(450,300,0,false);
	three = new CdllFreeWRL(450,300,0,false);
#else
	printf("load 1 freewrl instance (unlike glew_mx, ANGLEPROJECT can only do one instance)\n");
#endif
	one->onLoad(sfiles[0]); 
#ifndef ANGLEPROJECT
	two->onLoad(sfiles[1]); 
	three->onLoad(sfiles[2]); 
#endif
	
	//do a 'q' in each window
	//one->onClose();
//#ifndef ANGLEPROJECT
//	two->onClose();
//	three->onClose();
//#endif
	printf("do a 'q' in each window, then press enter in the console to exit:");
	getchar();

}