#include <stdio.h>
#include <dllFreeWRL.h>
int main (int argc, char **argv)
{
	//void *zero=0,*one=0,*two=0,*three=0,*four=0; //window handles - pass null so lib creates
	CdllFreeWRL *one, *two, *three; //, *four;
	//char *sfiles [] = {"E:/source2/tests/1.wrl","E:/source2/tests/2.wrl","E:/source2/tests/3.wrl","E:/source2/tests/4.wrl","E:/source2/tests/5.wrl"};
	char *sfiles [] = {
		"E:/source2/tests/ProgrammableShaders/models/teapot-Toon.wrl",
		"E:/source2/tests/ProgrammableShaders/models/TwoCylinders.wrl",
		"E:/source2/tests/ProgrammableShaders/models/flutter2-ProgramShader.x3d",
		"E:/source2/tests/ProgrammableShaders/models/teapot-noShaders.wrl",
		"E:/source2/tests/ProgrammableShaders/models/sobel-ComposedShader.wrl",
		"E:/source2/tests/ProgrammableShaders/models/flutter2-ComposedShader.x3d"};
	printf("hi from test_dll main\n");
	printf("load 3 freewrl instances from the same dll process\n");
	one = new CdllFreeWRL(450,300); //
	one->onLoad(sfiles[0]); //"E:/source2/tests/1.wrl");
	//getchar();
	two = new CdllFreeWRL(450,300,0,false);
	three = new CdllFreeWRL(450,300,0,false);
	two->onLoad(sfiles[1]); //"E:/source2/tests/2.wrl");
	//getchar();
	//CdllFreeWRL::onInit(four,450,300,false);
	three->onLoad(sfiles[2]); //"E:/source2/tests/3.wrl");
	//CdllFreeWRL::onLoad(four,"E:/source2/tests/4.wrl");
	printf("press enter to exit:");
	getchar();
	
	one->onClose();
	//two->onClose();
	three->onClose();
	//CdllFreeWRL::onClose(four);

}