#include <stdio.h>
#include <dllFreeWRL.h>
int main (int argc, char **argv)
{
	int i,j,k;
	CdllFreeWRL *one, *two, *three; //, *four;
	char *sfiles1 [] = {"../../../../tests/1.wrl","../../../../tests/2.wrl","../../../../tests/3.wrl","E:/source2/tests/4.wrl","E:/source2/tests/5.wrl"};
	char *sfiles [] = {
		"../../../../tests/ProgrammableShaders/models/TwoCylinders.wrl",
		"../../../../tests/ProgrammableShaders/models/teapot-Toon.wrl",
		"../../../../tests/ProgrammableShaders/models/flutter2-ProgramShader.x3d",
		"../../../../tests/ProgrammableShaders/models/sobel-ComposedShader.wrl",
		"../../../../tests/ProgrammableShaders/models/teapot-noShaders.wrl",
		"../../../../tests/ProgrammableShaders/models/sobel-ComposedShader.wrl",
		"../../../../tests/ProgrammableShaders/models/flutter2-ComposedShader.x3d",
		"../../../../tests/1.x3d",
		"../../../../tests/2.x3d",
		"../../../../tests/3.x3d",
		};
	int simaltaneous = 0;
	i =0; //7; //0;
	j=i+1;
	k=i+2;
	if(simaltaneous){

		printf("hi from test_dll main\n");
		//one = new CdllFreeWRL(450,300); //
		one = new CdllFreeWRL();
		one->onInit(450,300,NULL,false,false);
#undef ANGLEPROJECT
#ifndef ANGLEPROJECT
		//unfortunately angleproject -a way to wrap directX with GLES2 API as used for FireFox, Chrome- 
		// cannot do multi-threaded something
		//It seems to bomb in the shader compiler, or shader code - possibly it caches shader things to static storage (rather
		//than using TSD thread specific data -looking up data by thread as freewrl does with gglobal().
		//however it should still be OK for singlethreaded (meaning one displaythread, can have other auxiliary threads)
		// and possibly apartment threaded apps if the apartment thread is the display thread
		printf("load 3 freewrl instances from the same dll process\n");
		two = new CdllFreeWRL(450,300,0,false);
		three = new CdllFreeWRL(450,300,0,false);
#else
		printf("load 1 freewrl instance (unlike glew_mx, ANGLEPROJECT can only do one instance)\n");
#endif
		one->onLoad(sfiles[i]); 
#ifndef ANGLEPROJECT
		two->onLoad(sfiles[j]); 
		three->onLoad(sfiles[k]); 
#endif
	
		//do a 'q' in each window
		//one->onClose();
//#ifndef ANGLEPROJECT
		//two->onClose();
		//three->onClose();
//#endif
	}else{ //simaltaneous
		//seuquence - attempt to simulate IE/Activex or FF/npapi click-on-x3d-link, backbutton, click-another-x3d, backbutton, ...
		one = new CdllFreeWRL();

		one->onInit(450,300,NULL,false,false);
		one->onLoad(sfiles[i]); 
		getchar();
		one->onClose();
		one = new CdllFreeWRL();
		one->onInit(450,300,NULL,false,false);
		one->onLoad(sfiles[j]);
		getchar();
		one->onClose();
		one = new CdllFreeWRL();
		one->onInit(450,300,NULL,false,false);
		one->onLoad(sfiles[k]);
		getchar();
		one->onClose();
	}
	printf("do a 'q' in each window, then press enter in the console to exit:");
	getchar();

}