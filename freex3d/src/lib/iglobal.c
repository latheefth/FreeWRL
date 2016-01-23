#include <internal.h> //needed for opengl_utils.h included in iglobal.h
#include <iglobal.h>


void display_init(struct tdisplay* d);
void internalc_init(struct tinternalc* ic);
void resources_init(struct tresources* t);
void resources_clear(struct tresources* t);
void threads_init(struct tthreads* t);

#if !defined(FRONTEND_DOES_SNAPSHOTS)
void Snapshot_init(struct tSnapshot *);
#endif

void EAI_C_CommonFunctions_init(struct tEAI_C_CommonFunctions*);
void EAIEventsIn_init(struct tEAIEventsIn* t);
void EAIHelpers_init(struct tEAIHelpers* t);

#if !defined(EXCLUDE_EAI)
void EAICore_init(struct tEAICore* t);
#endif

void SensInterps_init(struct tSensInterps *t);
void ConsoleMessage_init(struct tConsoleMessage *t);
void Mainloop_init(struct tMainloop *t);
void Mainloop_clear(struct tMainloop *t);
void ProdCon_init(struct tProdCon *t);
void ProdCon_clear(struct tProdCon *t);

#if defined (INCLUDE_NON_WEB3D_FORMATS)
void ColladaParser_init(struct tColladaParser *t);
#endif //INCLUDE_NON_WEB3D_FORMATS

#if defined (INCLUDE_STL_FILES)
void STL_Handler_init (struct tSTLHandler *t);
#endif // INCLUDE_STL_FILES



void Frustum_init(struct tFrustum *t);
void LoadTextures_init(struct tLoadTextures *t);
void OpenGL_Utils_init(struct tOpenGL_Utils *t);
void OpenGL_Utils_clear(struct tOpenGL_Utils *t);
#ifdef HAVE_OPENCL
void OpenCL_Utils_init(struct tOpenCL_Utils *t);
#endif
//void RasterFont_init(struct tRasterFont *t);
void RenderTextures_init(struct tRenderTextures *t);
void Textures_init(struct tTextures *t);
void Textures_clear(struct tTextures *t);
void PluginSocket_init(struct tPluginSocket *t);
void pluginUtils_init(struct tpluginUtils *t);
void collision_init(struct tcollision *t);
void collision_clear(struct tcollision *t);
void Component_EnvironSensor_init(struct tComponent_EnvironSensor *t);
void Component_Geometry3D_init(struct tComponent_Geometry3D *t);
void Component_Geometry3D_clear(struct tComponent_Geometry3D *t);
void Component_Geospatial_init(struct tComponent_Geospatial *t);
void Component_HAnim_init(struct tComponent_HAnim *t);
void Component_Layering_init(struct tComponent_Layering *t);
void Component_Layout_init(struct tComponent_Layout *t);
void Component_NURBS_init(struct tComponent_NURBS *t);
void Component_KeyDevice_init(struct tComponent_KeyDevice *t);

#ifdef OLDCODE
OLDCODEvoid Component_Networking_init(struct tComponent_Networking *t);
#endif

#ifdef DJTRACK_PICKSENSORS
void Component_Picking_init(struct tComponent_Picking *t);
#endif

void Component_Shape_init(struct tComponent_Shape *t);
void Component_Sound_init(struct tComponent_Sound *t);
void Component_Text_init(struct tComponent_Text *t);
void Component_Text_clear(struct tComponent_Text *t);
void Component_Layering_clear(struct tComponent_Layering *t);
void Component_Layout_clear(struct tComponent_Layout *t);

void RenderFuncs_init(struct tRenderFuncs *t);
void RenderFuncs_clear(struct tRenderFuncs *t);
void StreamPoly_init(struct tStreamPoly *t);
void Tess_init(struct tTess *t);
void Viewer_init(struct tViewer *t);

#if defined(STATUSBAR_HUD)
void statusbar_init(struct tstatusbar *t);
void statusbar_clear(struct tstatusbar *t);
#endif

void CParse_init(struct tCParse *t);
void CParse_clear(struct tCParse *t);
void CParseParser_init(struct tCParseParser *t);
void CProto_init(struct tCProto *t);
void CRoutes_init(struct tCRoutes *t);
void CRoutes_clear(struct tCRoutes *t);
void CScripts_init(struct tCScripts *t);
void JScript_init(struct tJScript *t);

#ifdef HAVE_JAVASCRIPT
void jsUtils_init(struct tjsUtils *t);
void jsVRMLBrowser_init(struct tjsVRMLBrowser *t);
void jsVRMLClasses_init(struct tjsVRMLClasses *t);
#endif
void Bindable_init(struct tBindable *t);
void Bindable_clear(struct tBindable *t);
void X3DParser_init(struct tX3DParser *t);
void X3DParser_clear(struct tX3DParser *t);

void X3DProtoScript_init(struct tX3DProtoScript *t);
void common_init(struct tcommon *t);
void common_clear(struct tcommon *t);
void CursorDraw_init(struct tCursorDraw *t);

//static ttglobal iglobal; //<< for initial development witn single instance
static int done_main_UI_thread_once = 0;
pthread_key_t threadSpecificKey;  //set like a global variable in the global scope in a .c file

ttglobal  iglobal_constructor() //(mainthreadID,parserthreadID,texturethreadID...)
{
    
    //JAS printf ("calling iglobal_constructor\n");
    
	//using Johns threadID method would:
	//1. create global struct
	// - malloc
	// - initialize any that have initializers
	//2. add 3 items to the thread2global[] list
	//3. for each of those 3 items:
	//   - set thread2global[threadID] = global
	// pthread_t uiThread;
	ttglobal iglobal = MALLOCV(sizeof(struct iiglobal));
	memset(iglobal,0,sizeof(struct iiglobal)); //set to zero/null by default
#ifdef DISABLER
    if(!done_main_UI_thread_once){
		pthread_key_create(&threadSpecificKey, NULL);
		done_main_UI_thread_once = 1; //this assumes the iglobal is created in the shared UI main thread
	}
	fwl_setCurrentHandle(iglobal,__FILE__,__LINE__); //probably redundant but no harm

#if defined(WRAP_MALLOC) || defined(DEBUG_MALLOC)
    iglobal->__memTable_ShouldRegisterAllocation = TRUE;
    freewrlInitMemTable();
#endif
#endif
	//call initializer for each sub-struct
	display_init(&iglobal->display);
	internalc_init(&iglobal->internalc);
	resources_init(&iglobal->resources);
	threads_init(&iglobal->threads);
    
    
	#if !defined(FRONTEND_DOES_SNAPSHOTS)
	Snapshot_init(&iglobal->Snapshot);
	#endif

	EAI_C_CommonFunctions_init(&iglobal->EAI_C_CommonFunctions);
#if !defined(EXCLUDE_EAI)
	EAIEventsIn_init(&iglobal->EAIEventsIn);
	EAIHelpers_init(&iglobal->EAIHelpers);
	EAICore_init(&iglobal->EAICore);
#endif //EXCLUDE_EAI

	SensInterps_init(&iglobal->SensInterps);
	ConsoleMessage_init(&iglobal->ConsoleMessage);
	Mainloop_init(&iglobal->Mainloop);
	ProdCon_init(&iglobal->ProdCon);

#if defined (INCLUDE_NON_WEB3D_FORMATS)
	ColladaParser_init(&iglobal->ColladaParser);
#endif //INCLUDE_NON_WEB3D_FORMATS

#if defined (INCLUDE_STL_FILES)
	STL_Handler_init (&iglobal->STLHandler);
#endif // INCLUDE_STL_FILES    

	Frustum_init(&iglobal->Frustum);
	LoadTextures_init(&iglobal->LoadTextures);
	OpenGL_Utils_init(&iglobal->OpenGL_Utils);
#ifdef HAVE_OPENCL
        OpenCL_Utils_init(&iglobal->OpenCL_Utils);
#endif

	//RasterFont_init(&iglobal->RasterFont);
	RenderTextures_init(&iglobal->RenderTextures);
	Textures_init(&iglobal->Textures);
#ifndef DISABLER	
#ifndef NO_PLUGINSOCKET
	PluginSocket_init(&iglobal->PluginSocket);
#endif
	pluginUtils_init(&iglobal->pluginUtils);
#endif	
	collision_init(&iglobal->collision);
	Component_EnvironSensor_init(&iglobal->Component_EnvironSensor);
	Component_Geometry3D_init(&iglobal->Component_Geometry3D);
	Component_Geospatial_init(&iglobal->Component_Geospatial);
	Component_HAnim_init(&iglobal->Component_HAnim);
	Component_Layering_init(&iglobal->Component_Layering);
	Component_Layout_init(&iglobal->Component_Layout);
	Component_NURBS_init(&iglobal->Component_NURBS);
	Component_KeyDevice_init(&iglobal->Component_KeyDevice);
#ifdef OLDCODE
OLDCODE	Component_Networking_init(&iglobal->Component_Networking);
#endif // OLDCODE
#ifdef DJTRACK_PICKSENSORS
	Component_Picking_init(&iglobal->Component_Picking);
#endif
	Component_Shape_init(&iglobal->Component_Shape);
	Component_Sound_init(&iglobal->Component_Sound);
	Component_Text_init(&iglobal->Component_Text);
    
	RenderFuncs_init(&iglobal->RenderFuncs);
	StreamPoly_init(&iglobal->StreamPoly);
	Tess_init(&iglobal->Tess);
	Viewer_init(&iglobal->Viewer);
#if defined(STATUSBAR_HUD)
	statusbar_init(&iglobal->statusbar);
#endif
	CParse_init(&iglobal->CParse);
	CParseParser_init(&iglobal->CParseParser);
	CProto_init(&iglobal->CProto);
	CRoutes_init(&iglobal->CRoutes);
	CScripts_init(&iglobal->CScripts);
	JScript_init(&iglobal->JScript);

#ifdef HAVE_JAVASCRIPT
	jsUtils_init(&iglobal->jsUtils);
	jsVRMLBrowser_init(&iglobal->jsVRMLBrowser);
	jsVRMLClasses_init(&iglobal->jsVRMLClasses);
#endif
	Bindable_init(&iglobal->Bindable);
	X3DParser_init(&iglobal->X3DParser);
	X3DProtoScript_init(&iglobal->X3DProtoScript);
	common_init(&iglobal->common);
#ifndef DISABLER	
	CursorDraw_init(&iglobal->CursorDraw);

	//uiThread = pthread_self();
	//set_thread2global(iglobal, uiThread ,"UI thread");
        
	if(!done_main_UI_thread_once){
		pthread_key_create(&threadSpecificKey, NULL);
		done_main_UI_thread_once = 1; //this assumes the iglobal is created in the shared UI main thread
	}
	fwl_setCurrentHandle(iglobal,__FILE__,__LINE__); //probably redundant but no harm
#endif	
	return iglobal;
}

void __iglobal_fields_destructor(ttglobal tg);
void remove_iglobal_from_table(ttglobal tg);
void __iglobal_destructor(ttglobal tg);

void iglobal_destructor(ttglobal tg)
{
    __iglobal_fields_destructor(tg);
	__iglobal_destructor(tg);
}

void __iglobal_fields_destructor(ttglobal tg)
{
    /* you should have stopped any worker threads for this instance */
	//call individual destructors in reverse order to constructor
	FREE_IF_NZ(tg->CursorDraw.prv);
	common_clear(&tg->common); FREE_IF_NZ(tg->common.prv);
	FREE_IF_NZ(tg->X3DProtoScript.prv);
	X3DParser_clear(&tg->X3DParser); FREE_IF_NZ(tg->X3DParser.prv);
	Bindable_clear(&tg->Bindable); FREE_IF_NZ(tg->Bindable.prv);
#ifdef HAVE_JAVASCRIPT
	FREE_IF_NZ(tg->jsVRMLClasses.prv);
	FREE_IF_NZ(tg->jsVRMLBrowser.prv);
	FREE_IF_NZ(tg->jsUtils.prv);
#endif
	FREE_IF_NZ(tg->JScript.prv);
	FREE_IF_NZ(tg->CScripts.prv);
	CRoutes_clear(&tg->CRoutes); FREE_IF_NZ(tg->CRoutes.prv);
	FREE_IF_NZ(tg->CProto.prv);
	FREE_IF_NZ(tg->CParseParser.prv);
	CParse_clear(&tg->CParse); FREE_IF_NZ(tg->CParse.prv);
#if defined(STATUSBAR_HUD)
	statusbar_clear(&tg->statusbar);
#endif
	FREE_IF_NZ(tg->statusbar.prv);
	FREE_IF_NZ(tg->Viewer.prv);
	FREE_IF_NZ(tg->Tess.prv);
	FREE_IF_NZ(tg->StreamPoly.prv);
	FREE_IF_NZ(tg->Component_Sound.prv);
	RenderFuncs_clear(&tg->RenderFuncs); FREE_IF_NZ(tg->RenderFuncs.prv);
	Component_Text_clear(&tg->Component_Text); FREE_IF_NZ(tg->Component_Text.prv);
	FREE_IF_NZ(tg->Component_Shape.prv);
#ifdef DJTRACK_PICKSENSORS
	FREE_IF_NZ(tg->Component_Picking.prv);
#endif
#ifdef OLDCODE
OLDCODE	FREE_IF_NZ(tg->Component_Networking.prv);
#endif
	FREE_IF_NZ(tg->Component_KeyDevice.prv);
	FREE_IF_NZ(tg->Component_NURBS.prv);
	Component_Layering_clear(&tg->Component_Layering); FREE_IF_NZ(tg->Component_Layering.prv);
	Component_Layout_clear(&tg->Component_Layout); FREE_IF_NZ(tg->Component_Layout.prv);
	FREE_IF_NZ(tg->Component_HAnim.prv);
	FREE_IF_NZ(tg->Component_Geospatial.prv);
	Component_Geometry3D_clear(&tg->Component_Geometry3D); FREE_IF_NZ(tg->Component_Geometry3D.prv);
	FREE_IF_NZ(tg->Component_EnvironSensor.prv);
	collision_clear(&tg->collision); FREE_IF_NZ(tg->collision.prv);
	FREE_IF_NZ(tg->pluginUtils.prv);
	FREE_IF_NZ(tg->PluginSocket.prv);
	Textures_clear(&tg->Textures); FREE_IF_NZ(tg->Textures.prv);
	FREE_IF_NZ(tg->RenderTextures.prv);
	//FREE_IF_NZ(tg->RasterFont.prv);
	OpenGL_Utils_clear(&tg->OpenGL_Utils); FREE_IF_NZ(tg->OpenGL_Utils.prv);
	FREE_IF_NZ(tg->LoadTextures.prv);
	FREE_IF_NZ(tg->Frustum.prv);
    
#if defined (INCLUDE_NON_WEB3D_FORMATS)
	FREE_IF_NZ(tg->ColladaParser.prv);
#endif //INCLUDE_NON_WEB3D_FORMATS

#if defined (INCLUDE_STL_FILES)
	FREE_IF_NZ(tg->STLHandler.prv);
#endif // INCLUDE_STL_FILES
    
	ProdCon_clear(&tg->ProdCon); FREE_IF_NZ(tg->ProdCon.prv);
	Mainloop_clear(&tg->Mainloop); FREE_IF_NZ(tg->Mainloop.prv);
	FREE_IF_NZ(tg->ConsoleMessage.prv);
	FREE_IF_NZ(tg->SensInterps.prv);
	FREE_IF_NZ(tg->EAICore.prv);
	FREE_IF_NZ(tg->EAIHelpers.prv);
	FREE_IF_NZ(tg->EAIEventsIn.prv);
	FREE_IF_NZ(tg->EAI_C_CommonFunctions.prv);
	FREE_IF_NZ(tg->Snapshot.prv);
    
	FREE_IF_NZ(tg->threads.prv);
	resources_clear(&tg->resources); FREE_IF_NZ(tg->resources.prv);
	FREE_IF_NZ(tg->internalc.prv);
	FREE_IF_NZ(tg->display.prv);
}

void __iglobal_destructor(ttglobal tg)
{
    //destroy iglobal
#ifndef DISABLER    
	FREE_IF_NZ(tg);
#else	
	free(tg);
#endif
	//remove_iglobal_from_table(tg);
	fwl_clearCurrentHandle(__FILE__,__LINE__);

}



void *fwl_getCurrentHandle(char *fi, int li){
	ttglobal currentHandle = (ttglobal)pthread_getspecific(threadSpecificKey); 
    //printf ("fwl_getCurrentHandle returning %p at %s:%d\n",currentHandle,fi,li);
	return (void*)currentHandle;
}
int fwl_setCurrentHandle(void *handle, char *fi, int li)
{    
    //printf ("fwl_setCurrentHandle at to %p thread %p at %s:%d\n",handle,pthread_self(),fi,li);
    
	pthread_setspecific(threadSpecificKey,handle);
	return 1; /* let caller know its not in the table yet */
}
void fwl_clearCurrentHandle(char *fi, int li)
{
	void *currentHandle = NULL;
    //printf ("fwl_clearCurrentHandle at %s:%d\n",fi,li);
	pthread_setspecific(threadSpecificKey,currentHandle);

}
ttglobal gglobal(char *fi, int *li){
	ttglobal tg = NULL;
#ifdef DISABLER	
    if (done_main_UI_thread_once)
#endif    
         tg = (ttglobal)pthread_getspecific(threadSpecificKey);
	if(!tg)
    {
		printf("Ouch - no state for this thread -- hit a key to exit\n");
#ifdef _MSC_VER
        	printf ("more info - thread %p\n\n",(void *)pthread_self().p);
#else
        	printf ("more info - thread %p\n\n",(void *)pthread_self());
#endif                
		getchar();
		exit(-1);
	}
	return tg;
}

ttglobal gglobal0()
{
#ifdef DISABLER
    if (!done_main_UI_thread_once)
    {
        return NULL;
    }
#endif    
	return (ttglobal)pthread_getspecific(threadSpecificKey); 
}
