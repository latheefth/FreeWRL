/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

//#define USE_SNAPSHOT_TESTING 1
#ifndef USE_SNAPSHOT_TESTING
//stubs for options
void fwl_set_modeRecord()
{
}
void fwl_set_modeFixture()
{
}
void fwl_set_modePlayback()
{
}
void fwl_set_nameTest(char *nameTest)
{
}
#endif //ifndef USE_SNAPSHOT_TESTING


#ifdef USE_SNAPSHOT_TESTING

/* SnapshotTesting
- a process to do regression testing
- implemented as an insertible 'layer' in freewrl

http://freewrl.sourceforge.net/use.html
Commmand Line Options - Testing

Developers working on desktop configurations of freewrl can automate testing by comparing 
before- and after- snapshot images and log files.

There are 3 steps to the testing process:
A) generate test playback files:
	freewrl 1.wrl -R -N t1
	generates /recording/t1.fwplay
	In order to generate test fixtures, you need to take a snapshot of the graphics window 
	using the 'x' key, and/or for non-visual tests, log something to the log file by toggling 
	logging on with the "`" key, doing some functions that write to the console such as "/" command, 
	toggle logging off with "`", and press 'q' to exit gracefully. 
	(some temporary files will appear in /freewrl_tmp but are not needed).
	During this sesssion the frame rate is slowed -to minimize the size of the .fwplay file 
	and speed up test playback- and your mouse and keyboard actions are recorded on each frame, 
	to the .fwplay file.
B) before making a change to the code, run the playback files to generate test fixtures:
	freewrl -F -N t1
	generates /fixture/t1.0001.bmp test fixture, assuming you hit the 'x' snapshot during the recording step, 
	and /fixture/t1.log if you toggled logging on during recording
C) after making each small code change:
	i) re-run the playback files
		freewrl -P -N t1
		generates /playback/t1.0001.bmp and/or /playback/t1.log
	ii) compare /playback/testname.* to the /fixture/testname.* files
		For this you'll need a perl script doing a file compare between the /fixture and /playback versions.
		If any tests break by not comparing identically playback to fixture, 
		that means you changed the functionality of the code.
		Sometimes that's what you want. In that case check which tests failed to see if 
		there are unintended side-effects. If all OK, then generate new fixtures.
		Sometimes you want to refactor code: change its structure without changing its functionality. 
		In this case if you break any test, roll back your last code change.

Summary of testing command line options:

-N or --nametest testname
	Sets the .fwplay, snapshot image and logfile prefix to testname. 
	During recording /recording/testname.fwplay test playback file is generated, 
	and during fixture it's read, and /fixture/testname.0001.bmp (or .png) and/or /fixture/testname.log 
	is generated, depending on what actions you took during recording. 
	If you don't set this option, testname defaults to your scene basename, 
	except with the .suffix changed to _suffix. Example 1.wrl defaults to 1_wrl.fwplay.
-R or --record
	Generates /recording/testname.fwplay test playback file.
-F or --fixture
	Generates /fixture/testname.0001.bmp or .png, and/or /fixture/testname.log depending 
	on whether you did a snapshot and/or log command during recording.
-P or --playback
	Identical to option -F or --fixture except puts test fixtures into /playback folder.

*/




#include <stdio.h>
#include <iglobal.h>
#include <internal.h>
void set_snapshotModeTesting(int value);  //snapshot.c
int isSnapshotModeTesting(); //snapshot.c


#define FALSE 0
#define TRUE 1

struct keypressTuple{
	int key;
	int type;
};
struct mouseTuple{
	int mev;
	unsigned int button;
	float x;
	float y;
	int ix;
	int iy;
	int ID;
};
struct playbackRecord {
	int frame;
	double dtime;
	//should we use more general Touch instead of mouse-specific?
	int *mousetuples; //x,y,button chord
	int mouseCount; //# mouse tuples
	char *keystrokes;
	int keyCount;
};

typedef struct tSnapshotTesting{
		void *prv;
	} tSnapshotTesting;
static tSnapshotTesting SnapshotTesting;


typedef struct pSnapshotTesting{
	FILE* recordingFile;
	char* recordingFName;
	int modeRecord;
	int modeFixture;
	int modePlayback;
	int fwplayOpened;
	char *nameTest;
	int frameNum; //for Record, Playback - frame# =0 after scene loaded
	struct playbackRecord* playback;
	int playbackCount;

	struct keypressTuple keypressQueue[50]; //for Record,Playback where keypresses are applied just once per frame for consistency
	int keypressQueueCount;
	struct mouseTuple mouseQueue[50];
	int mouseQueueCount;

}* ppSnapshotTesting;
void *SnapshotTesting_constructor(){
	void *v = MALLOCV(sizeof(struct pSnapshotTesting));
	memset(v,0,sizeof(struct pSnapshotTesting));
	return v;
}
void SnapshotTesting_init(struct tSnapshotTesting *t){
	//public
	//private
	t->prv = SnapshotTesting_constructor();
	{
		ppSnapshotTesting p = (ppSnapshotTesting)t->prv;
		p->recordingFile = NULL;
		p->recordingFName = NULL;
		p->modeRecord = FALSE;
		p->modeFixture = FALSE;
		p->modePlayback = FALSE;
		p->nameTest = NULL;
		p->frameNum = 0;
		p->playbackCount = 0;
		p->playback = NULL;
		p->fwplayOpened = 0;

		p->keypressQueueCount=0;
		p->mouseQueueCount=0;
	}
}
void SnapshotTesting_setHandlers();

static int rtestinit = 0;
static ppSnapshotTesting get_ppSnapshotTesting(){
	if(!rtestinit){
		SnapshotTesting_init(&SnapshotTesting);
		rtestinit = 1;
		ppSnapshotTesting p = (ppSnapshotTesting)SnapshotTesting.prv;
		SnapshotTesting_setHandlers();
	}
	return (ppSnapshotTesting)SnapshotTesting.prv;
}

int dequeueKeyPress(ppSnapshotTesting p,int *key, int *type){
	if(p->keypressQueueCount > 0){
		int i;
		p->keypressQueueCount--;
		*key = p->keypressQueue[0].key;
		*type = p->keypressQueue[0].type;
		for(i=0;i<p->keypressQueueCount;i++){
			p->keypressQueue[i].key = p->keypressQueue[i+1].key;
			p->keypressQueue[i].type = p->keypressQueue[i+1].type;
		}
		return 1;
	}
	return 0;
}

void queueKeyPress(ppSnapshotTesting p, int key, int type){
	if(p->keypressQueueCount < 50){
		p->keypressQueue[p->keypressQueueCount].key = key;
		p->keypressQueue[p->keypressQueueCount].type = type;
		p->keypressQueueCount++;
	}
}

int dequeueMouse(ppSnapshotTesting p, int *mev, unsigned int *button, float *x, float *y){
	if(p->mouseQueueCount > 0){
		int i;
		p->mouseQueueCount--;
		*mev = p->mouseQueue[0].mev;
		*button = p->mouseQueue[0].button;
		*x = p->mouseQueue[0].x;
		*y = p->mouseQueue[0].y;
		for(i=0;i<p->mouseQueueCount;i++){
			p->mouseQueue[i].mev = p->mouseQueue[i+1].mev;
			p->mouseQueue[i].button = p->mouseQueue[i+1].button;
			p->mouseQueue[i].x = p->mouseQueue[i+1].x;
			p->mouseQueue[i].y = p->mouseQueue[i+1].y;
		}
		return 1;
	}
	return 0;
}
int dequeueMouseMulti(ppSnapshotTesting p, int *mev, unsigned int *button, int *ix, int *iy, int *ID){
	if(p->mouseQueueCount > 0){
		int i;
		p->mouseQueueCount--;
		*mev = p->mouseQueue[0].mev;
		*button = p->mouseQueue[0].button;
		*ix = p->mouseQueue[0].ix;
		*iy = p->mouseQueue[0].iy;
		*ID = p->mouseQueue[0].ID;
		for(i=0;i<p->mouseQueueCount;i++){
			p->mouseQueue[i].mev = p->mouseQueue[i+1].mev;
			p->mouseQueue[i].button = p->mouseQueue[i+1].button;
			p->mouseQueue[i].ix = p->mouseQueue[i+1].ix;
			p->mouseQueue[i].iy = p->mouseQueue[i+1].iy;
			p->mouseQueue[i].ID = p->mouseQueue[i+1].ID;
		}
		return 1;
	}
	return 0;
}

void queueMouseMulti(ppSnapshotTesting p, const int mev, const unsigned int button, const int ix, const int iy, int ID){
	if(p->mouseQueueCount < 50){
		p->mouseQueue[p->mouseQueueCount].mev = mev;
		p->mouseQueue[p->mouseQueueCount].button = button;
		p->mouseQueue[p->mouseQueueCount].ix = ix;
		p->mouseQueue[p->mouseQueueCount].iy = iy;
		p->mouseQueue[p->mouseQueueCount].ID = ID;
		p->mouseQueueCount++;
	}
}
void queueMouse(ppSnapshotTesting p, const int mev, const unsigned int button, const float x, const float y){
	if(p->mouseQueueCount < 50){
		p->mouseQueue[p->mouseQueueCount].mev = mev;
		p->mouseQueue[p->mouseQueueCount].button = button;
		p->mouseQueue[p->mouseQueueCount].x = x;
		p->mouseQueue[p->mouseQueueCount].y = y;
		p->mouseQueueCount++;
	}
}


void handleTESTING(const int mev, const unsigned int button, const float x, const float y)
{
	ppSnapshotTesting p;
	//ttglobal tg = gglobal();
	//p = (ppSnapshotTesting)tg->SnapshotTesting.prv;
	p = get_ppSnapshotTesting();

	//if(0)
	if(p->modeRecord || p->modeFixture || p->modePlayback){
		if(p->modeRecord){
			queueMouse(p,mev,button,x,y);
		}
		//else ignor so test isn't ruined by random mouse movement during playback
		return;
	}
	handle0(mev, button, x, y);
}
void fwl_do_keyPress0(int key, int type);
void fwl_do_rawKeyPressTESTING(int key, int type) {
	ppSnapshotTesting p;
	//ttglobal tg = gglobal();
	//p = (ppSnapshotTesting)tg->SnapshotTesting.prv;
	p = get_ppSnapshotTesting();

	if(p->modeRecord){
		queueKeyPress(p,key,type);
	}else{
		fwl_do_keyPress0(key,type);
	}
}
void fwl_handle_aqua_multiNORMAL(const int mev, const unsigned int button, int x, int y, int ID);
void fwl_handle_aqua_multiTESTING(const int mev, const unsigned int button, int x, int y, int ID)
{
	ppSnapshotTesting p;
	//ttglobal tg = gglobal();
	//p = (ppSnapshotTesting)tg->SnapshotTesting.prv;
	p = get_ppSnapshotTesting();

	if(p->modeRecord || p->modeFixture || p->modePlayback){
		if(p->modeRecord){
			queueMouseMulti(p,mev,button,x,y,ID);
		}
		//else ignor so test isn't ruined by random mouse movement during playback
		return;
	}
	fwl_handle_aqua_multiNORMAL(mev, button, x, y, ID);
}

void fwl_set_modeRecord()
{
	ppSnapshotTesting p;
	//ttglobal tg = gglobal();
	//p = (ppSnapshotTesting)tg->SnapshotTesting.prv;
	p = get_ppSnapshotTesting();
    p->modeRecord = TRUE;
}
void fwl_set_modeFixture()
{
	ppSnapshotTesting p;
	//ttglobal tg = gglobal();
	//p = (ppSnapshotTesting)tg->SnapshotTesting.prv;
	p = get_ppSnapshotTesting();
    p->modeFixture = TRUE;
}
void fwl_set_modePlayback()
{
	ppSnapshotTesting p;
	//ttglobal tg = gglobal();
	//p = (ppSnapshotTesting)tg->SnapshotTesting.prv;
	p = get_ppSnapshotTesting();
    p->modePlayback = TRUE;
}
void fwl_set_nameTest(char *nameTest)
{
	ppSnapshotTesting p;
	//ttglobal tg = gglobal();
	//p = (ppSnapshotTesting)tg->SnapshotTesting.prv;
	p = get_ppSnapshotTesting();
    p->nameTest = STRDUP(nameTest);
}

char *nameLogFileFolderTESTING(char *logfilename, int size){
	ppSnapshotTesting p;
	ttglobal tg = gglobal();
	//p = (ppSnapshotTesting)tg->SnapshotTesting.prv;
	p = get_ppSnapshotTesting();

	if(p->modePlayback || p->modeFixture){
		if(p->modePlayback)
			strcat(logfilename,"playback");
		else
			strcat(logfilename,"fixture");
		fw_mkdir(logfilename);
		strcat(logfilename,"/");
		if(p->nameTest){
			//  /fixture/test1.log
			strcat(logfilename,p->nameTest);
		}else if(tg->Mainloop.scene_name){
			//  /fixture/1_wrl.log
			strcat(logfilename,tg->Mainloop.scene_name);
			if(tg->Mainloop.scene_suff){
				strcat(logfilename,"_");
				strcat(logfilename,tg->Mainloop.scene_suff);
			}
		}
	}else{
		nameLogFileFolderNORMAL(logfilename,size);
	}
	return logfilename;
}


int fw_mkdir(char* path);
void fwl_RenderSceneUpdateScene0(double dtime);
void fwl_RenderSceneUpdateSceneTESTING() {
	double dtime;
	//ttglobal tg = gglobal();
	//ppMainloop p = (ppMainloop)tg->Mainloop.prv;
	ppSnapshotTesting p;
	ttglobal tg = gglobal();
	//p = (ppSnapshotTesting)tg->SnapshotTesting.prv;
	p = get_ppSnapshotTesting();


	dtime = Time1970sec();
	if((p->modeRecord || p->modeFixture || p->modePlayback)) //commandline --record/-R and --playback/-P, for automated testing
	{
		//functional testing support options May 2013
		//records frame#, dtime, keyboard, mouse to an ASCII .fwplay file for playback
		//to record, run a scene file with -R or --record option
		//copy the .fwplay between platforms
		//before starting refactoring, run scenes with -F or --fixture option,
		//  and hit the 'x' key to save a snapshot one or more times per fixture run
		//after each refactoring step, run scenes with -P or --playback option,
		//  and (with perl script) do a file compare(fixture_snapshot,playback_snapshot)
		//
		//on the command line use:
		//-R to just record the .fwplay file
		//-F to play recording and save as fixture
		//-P to play recording and save as playback
		//-R -F to record and save as fixture in one step
		//command line long option equivalents: -R --record, -F --fixture, -P --playback
		int key;
		int type;
		int mev,ix,iy,ID;
		unsigned int button;
		float x,y;
		char buff[1000], keystrokes[200], mouseStr[1000];
		int namingMethod;
		char *folder;
		char sceneName[1000];
		//naming method for related files (and folders)
		//0=default: recording.fwplay, fixture.bmp playback.bmp - will overwrite for each scene
		//1=folders: 1_wrl/recording.fwplay, 1_wrl/fixture/17.bmp, 1_wrl/playback/17.bmp
		//2=flattened: 1_wrl.fwplay, 1_wrl_fixture_17.bmp, 1_wrl_playback_17.bmp (17 is frame#)
		//3=groupfolders: /tests, /recordings/*.fwplay, /fixtures/1_wrl_17.bmp /playbacks/1_wrl_17.bmp
		//4=groupfolders: /tests, /recordings/*.fwplay, /fixtures/1_wrl_17.bmp /playbacks/1_wrl_17.bmp
		//  - 4 same as 3, except done to harmonize with linux/aqua naming approach:
		//  - fwl_set_SnapFile(path = {"fixture" | "playback" }); to set mytmp
		//  -
		folder = NULL;
		namingMethod = 4;
		//if(p->frameNum == 1){
		if(!p->fwplayOpened){
			char recordingName[1000];
			int j,k;
			p->fwplayOpened = 1;
			recordingName[0] = '\0';
			sceneName[0] = '\0';
			if(tg->Mainloop.scene_name){
				strcat(sceneName,tg->Mainloop.scene_name);
				if(tg->Mainloop.scene_suff){
					strcat(sceneName,".");
					strcat(sceneName,tg->Mainloop.scene_suff);
				}
			}
			if(namingMethod==3 || namingMethod==4){
				strcpy(recordingName,"recording");
				fw_mkdir(recordingName);
				strcat(recordingName,"/");
			}
			if(namingMethod>0){
				if(p->nameTest){
					strcat(recordingName,p->nameTest);
				}else{
					strcat(recordingName,tg->Mainloop.scene_name);
					k = strlen(recordingName);
					if(k){
						//1.wrl -> 1_wrl
						j = strlen(tg->Mainloop.scene_suff);
						if(j){
							strcat(recordingName,"_");
							strcat(recordingName,tg->Mainloop.scene_suff);
						}
					}
				}
			}
			if(namingMethod==1){
				fw_mkdir(recordingName);
				strcat(recordingName,"/recording"); //recording.fwplay a generic name, in case there's no scene name
			}
			if(namingMethod==0)
				strcat(recordingName,"recording");
			strcat(recordingName,".fwplay"); //1_wrl.fwplay
			p->recordingFName = STRDUP(recordingName);

			if(p->modeFixture  || p->modePlayback){
				if(!p->modeRecord){
					p->recordingFile = fopen(p->recordingFName, "r");
					if(p->recordingFile == NULL){
						printf("ouch recording file %s not found\n", p->recordingFName);
						fw_exit(1);
					}
					if( fgets(buff, 1000, p->recordingFile) != NULL){
						char window_widthxheight[100], equals[50];
						int width, height;
						//window_wxh = 600,400
						if( sscanf(buff,"%s %s %d, %d\n",window_widthxheight,equals, &width,&height) == 4) {
							if(width != tg->display.screenWidth || height != tg->display.screenHeight){
								//right now all we can do is passively complain
								printf("Ouch - the test playback window size is different than recording:\n");
								printf("recording %d x %d playback %d x %d\n",width,height,
									tg->display.screenWidth,tg->display.screenHeight);
								printf("hit Enter:");
								getchar();
							}
						}
					}
					if( fgets(buff, 1000, p->recordingFile) != NULL){
						char scenefile[100], equals[50];
						//scenefile = 1.wrl
						if( sscanf(buff,"%s %s %s \n",scenefile,equals, sceneName) == 3) {
							if(!tg->Mainloop.scene_name){
								char* suff = NULL;
								char* local_name = NULL;
								char* url = NULL;
								if(strlen(sceneName)) url = STRDUP(sceneName);
								if(url){
									splitpath_local_suffix(url, &local_name, &suff);
									gglobal()->Mainloop.url = url;
									gglobal()->Mainloop.scene_name = local_name;
									gglobal()->Mainloop.scene_suff = suff;
									fwl_resource_push_single_request(url);
								}
							}
						}
					}
				}
			}
		}
		int doEvents = (!fwl_isinputThreadParsing()) && (!fwl_isTextureParsing()) && fwl_isInputThreadInitialized();
		//printf("frame %d doevents=%d\n",p->frameNum,p->doEvents);
		if(!doEvents)
			return; //for Record and Playback, don't start doing things until scene and textures are loaded
		if(p->modeRecord)
			if(dtime - tg->Mainloop.TickTime < .5) return; //slow down frame rate to 2fps to reduce empty meaningless records
		p->frameNum++; //for record, frame relative to when scene is loaded

		if(p->modeRecord){
			int i;
			char temp[1000];
			if(p->frameNum == 1){
				p->recordingFile = fopen(p->recordingFName, "w");
				if(p->recordingFile == NULL){
					printf("ouch recording file %s not found\n", p->recordingFName);
					fw_exit(1);
				}
				//put in a header record, passively showing window widthxheight
				fprintf(p->recordingFile,"window_wxh = %d, %d \n",tg->display.screenWidth,tg->display.screenHeight);
				fprintf(p->recordingFile,"scenefile = %s \n",tg->Mainloop.url); //sceneName);
			}
			strcpy(keystrokes,"\"");
			while(dequeueKeyPress(p,&key,&type)){
				sprintf(temp,"%d,%d,",key,type);
				strcat(keystrokes,temp);
			}
			strcat(keystrokes,"\"");
			strcpy(mouseStr,"\"");
			i = 0;
			while(dequeueMouseMulti(p,&mev, &button, &ix, &iy, &ID)){
				sprintf(temp,"%d,%d,%d,%d,%d;",mev,button,ix,iy,ID);
				strcat(mouseStr,temp);
				i++;
			}
			strcat(mouseStr,"\"");
			fprintf(p->recordingFile,"%d %.6lf %s %s\n",p->frameNum,dtime,keystrokes,mouseStr);
			//in case we are -R -F together,
			//we need to round dtime for -F like it will be coming out of .fwplay for -P
			sprintf(temp,"%.6lf",dtime);
			sscanf(temp,"%lf",&dtime);
			//folder = "fixture";
			folder = NULL;
		}
		if(p->modeFixture  || p->modePlayback){
			if(!p->modeRecord){
				// playback[i] = {iframe, dtime, keystrokes or NULL, mouse (xy,button sequence) or NULL, snapshot URL or NULL, scenegraph_dump URL or NULL, ?other?}
				if( fgets( buff, 1000, p->recordingFile ) != NULL ) {
					if(sscanf(buff,"%d %lf %s %s\n",&p->frameNum,&dtime,keystrokes,mouseStr) == 4){ //,snapshotURL,scenegraphURL) == 6){
						if(0) printf("%d %lf %s %s\n",p->frameNum,dtime,keystrokes,mouseStr);
					}
				}
			}
			if(p->modeFixture)  folder = "fixture";
			if(p->modePlayback) folder = "playback";
		}
		//for all 3 - read the keyboard string and the mouse string
		if(p->modeRecord || p->modeFixture || p->modePlayback){
			if(strlen(keystrokes)>2){ // "x,1," == 6
				char *next,*curr;
				//count the number of ','
				//for(i=0,n=0;i<strlen(keystrokes);i++) if(keystrokes[i] == ',') n++; //(strlen(keystrokes) -2)/4;
				//n /= 2; //each keystroke has 2 commas: (char),(type),
				curr = &keystrokes[1]; //skip leading "
				while(curr && strlen(curr)>1){
					//for(i=0;i<n;i++){
					//ii = i*4 +1;
					//sscanf(&keystrokes[ii],"%d,%d",&key,&type);
					sscanf(curr,"%d",&key);
					next = strchr(curr,',');
					curr = &next[1];
					sscanf(curr,"%d",&type);
					next = strchr(curr,',');
					curr = &next[1];
					if(p->modeFixture || p->modePlayback){
						//we will catch the snapshot keybaord command and prepare the
						//snapshot filename and folder/directory for fixture and playback
						if(key == 'x'){
							//prepare snapshot folder(scene/ + fixture ||playback)
							// and file name(frame#)
							char snapfile[5];
#ifdef _MSC_VER
							char *suff = ".bmp";
#else
							char *suff = ".snap";
#endif
							sprintf(snapfile,"%d",p->frameNum);
							if(namingMethod == 0){
								//default: recording.bmp, playback.bmp
								char snappath[100];
								strcpy(snappath,folder);
								strcat(snappath,suff);
								fwl_set_SnapFile(snappath);
							}
							if(namingMethod==1){
								//nested folder approach
								//1=folders: 1_wrl/recording.fwplay, 1_wrl/fixture/17.bmp, 1_wrl/playback/17.bmp
								int k,j;
								char snappath[100];
								strcpy(snappath,tg->Mainloop.scene_name);
								k = strlen(snappath);
								if(k){
									//1.wrl -> 1_wrl
									j = strlen(tg->Mainloop.scene_suff);
									if(j){
										strcat(snappath,"_");
										strcat(snappath,tg->Mainloop.scene_suff);
									}
								}
								strcat(snappath,"/");
								strcat(snappath,folder);
								fw_mkdir(snappath); //1_wrl/fixture
								//fwl_set_SnapTmp(snappath); //sets the folder for snaps
								strcat(snappath,"/");
								strcat(snappath,snapfile);
								strcat(snappath,suff); //".bmp");
								//fwl_set_SnapFile(snapfile);
								fwl_set_SnapFile(snappath); //1_wrl/fixture/17.bmp
							}
							if(namingMethod == 2){
								//flattened filename approach with '_'
								//if snapshot 'x' is on frame 17, and fixture,
								//   then 1_wrl_fixture_17.snap or .bmp
								char snappath[100];
								int j, k;
								strcpy(snappath,tg->Mainloop.scene_name);
								k = strlen(snappath);
								if(k){
									j= strlen(tg->Mainloop.scene_suff);
									if(j){
										strcat(snappath,"_");
										strcat(snappath,tg->Mainloop.scene_suff);
									}
									strcat(snappath,"_");
								}
								strcat(snappath,folder);
								strcat(snappath,"_");
								strcat(snappath,snapfile);
								strcat(snappath,suff); //".bmp");
								fwl_set_SnapFile(snappath);
							}
							if(namingMethod == 3){
								//group folder
								//if snapshot 'x' is on frame 17, and fixture,
								//   then fixture/1_wrl_17.snap or .bmp
								char snappath[100];
								int j, k;
								strcpy(snappath,folder);
								fw_mkdir(snappath); // /fixture
								strcat(snappath,"/");
								strcat(snappath,tg->Mainloop.scene_name); // /fixture/1
								k = strlen(tg->Mainloop.scene_name);
								if(k){
									j= strlen(tg->Mainloop.scene_suff);
									if(j){
										strcat(snappath,"_");
										strcat(snappath,tg->Mainloop.scene_suff);
									}
									strcat(snappath,"_");
								}
								strcat(snappath,snapfile);
								strcat(snappath,suff); //".bmp");
								fwl_set_SnapFile(snappath);  //  /fixture/1_wrl_17.bmp
							}
							if(namingMethod == 4){
								//group folder
								//if snapshot 'x' is the first one .0001, and fixture,
								//   then fixture/1_wrl.0001.rgb or .bmp
								char snappath[100];
								char *sep = "_"; // "." or "_" or "/"
								set_snapshotModeTesting(TRUE);
								//if(isSnapshotModeTesting())
								//	printf("testing\n");
								//else
								//	printf("not testing\n");
								strcpy(snappath,folder);
								fw_mkdir(snappath); // /fixture
								fwl_set_SnapTmp(snappath);

								snappath[0] = '\0';
								if(p->nameTest){
									strcat(snappath,p->nameTest);
								}else{
									if(tg->Mainloop.scene_name){
										strcat(snappath,tg->Mainloop.scene_name); // /fixture/1
										if(tg->Mainloop.scene_suff)
										{
											strcat(snappath,sep); // "." or "_");
											strcat(snappath,tg->Mainloop.scene_suff);
										}
									}
								}
								fwl_set_SnapFile(snappath);  //  /fixture/1_wrl.001.bmp

							}
						}
					}
					fwl_do_keyPress0(key, type);
				}
			}
			if(strlen(mouseStr)>2){
				int i,ii,len;
				int mev;
				unsigned int button;
				float x,y;
				len = strlen(mouseStr);
				ii=1;
				do{
					for(i=ii;i<len;i++)
						if(mouseStr[i] == ';') break;

					sscanf(&mouseStr[ii],"%d,%d,%d,%d,%d;",&mev,&button,&ix,&iy,&ID);
					fwl_handle_aqua_multiNORMAL(mev,button,ix,iy,ID);

					//printf("%d,%d,%f,%f;",mev,button,x,y);
					ii=i+1;
				}while(ii<len-1);
			}
		}
	}
	fwl_RenderSceneUpdateScene0(dtime);
}
extern void (*fwl_do_rawKeyPressPTR)(int key, int type);
extern void (*fwl_handle_aqua_multiPTR)(const int mev, const unsigned int button, int x, int y, int ID);
extern void (*fwl_RenderSceneUpdateScenePTR)();
extern void (*handlePTR)(const int mev, const unsigned int button, const float x, const float y);
extern char * (*nameLogFileFolderPTR)(char *logfilename, int size);
void SnapshotTesting_setHandlers(){
	fwl_do_rawKeyPressPTR = fwl_do_rawKeyPressTESTING;
	fwl_handle_aqua_multiPTR = fwl_handle_aqua_multiTESTING;
	fwl_RenderSceneUpdateScenePTR = fwl_RenderSceneUpdateSceneTESTING;
	handlePTR = handleTESTING;
	nameLogFileFolderPTR = nameLogFileFolderTESTING;
}

#endif //USE_SNAPSHOT_TESTING
