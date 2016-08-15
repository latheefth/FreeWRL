/*


Render the children of nodes.

*/

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


#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../scenegraph/Viewer.h"
#include "../scenegraph/RenderFuncs.h"
#include "../vrml_parser/Structs.h"

#include "../main/headers.h"
#include "../vrml_parser/CRoutes.h"
#include "../scenegraph/Collision.h"

#include "OpenCL_Utils.h"

#ifdef HAVE_OPENCL


static bool initialize_OpenCL();
static void createGPUInterpolators();
static const char* coordinateInterpolator_kernel;
static const char* interpolator_headers;


static void *OpenCL_Utils_constructor(){
	void *v = malloc(sizeof(struct pOpenCL_Utils));
	memset(v,0,sizeof(struct pOpenCL_Utils));
	return v;
}


void OpenCL_Utils_init(struct tOpenCL_Utils *t)
{
	//printf ("start calling OpenCL_Utils_init - t is %p\n",t);
	//private
	t->prv = OpenCL_Utils_constructor();
	{
		//ppOpenCL_Utils p = (ppOpenCL_Utils)t->prv;
	}

	t->OpenCL_Initialized = FALSE;
	t->OpenCL_OK = FALSE;
	//printf ("done calling OpenCL_Utils_init\n");

}

void fwl_OpenCL_startup(struct tOpenCL_Utils *t) {
    //printf ("called fwl_OpenCL_startup...\n");
if (t->OpenCL_Initialized) printf (".... fwl_opencl already done?\n"); else printf ("..... OpenCL init currently false\n");
    
	initialize_OpenCL();

    //printf ("past the initialize_OpenCL call\n");
	createGPUCollisionProgram();

    createGPUInterpolators();
    //printf ("finished called fwl_OpenCL_startup...\n");
    t->OpenCL_Initialized = TRUE;
    
}


static char *getCLErrorString(cl_int err) {
	switch (err) {
#ifdef CL_VERSION_1_2
 		case CL_COMPILE_PROGRAM_FAILURE: return "CL_COMPILE_PROGRAM_FAILURE"; break;
		case CL_LINKER_NOT_AVAILABLE: return "CL_LINKER_NOT_AVAILABLE"; break;
		case CL_LINK_PROGRAM_FAILURE: return "CL_LINK_PROGRAM_FAILURE"; break;
		case CL_DEVICE_PARTITION_FAILED: return "CL_DEVICE_PARTITION_FAILED"; break;
		case CL_KERNEL_ARG_INFO_NOT_AVAILABLE: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE"; break;
		case CL_INVALID_IMAGE_DESCRIPTOR: return "CL_INVALID_IMAGE_DESCRIPTOR"; break;
		case CL_INVALID_COMPILER_OPTIONS: return "CL_INVALID_COMPILER_OPTIONS"; break;
		case CL_INVALID_LINKER_OPTIONS: return "CL_INVALID_LINKER_OPTIONS"; break;
		case CL_INVALID_DEVICE_PARTITION_COUNT: return "CL_INVALID_DEVICE_PARTITION_COUNT"; break;
           
#endif //CL_VERSION_1_2
            
		case CL_DEVICE_NOT_FOUND: return "CL_DEVICE_NOT_FOUND"; break;
		case CL_DEVICE_NOT_AVAILABLE: return "CL_DEVICE_NOT_AVAILABLE"; break;
		case CL_COMPILER_NOT_AVAILABLE: return "CL_COMPILER_NOT_AVAILABLE"; break;
		case CL_MEM_OBJECT_ALLOCATION_FAILURE: return "CL_MEM_OBJECT_ALLOCATION_FAILURE"; break;
		case CL_OUT_OF_RESOURCES: return "CL_OUT_OF_RESOURCES"; break;
		case CL_OUT_OF_HOST_MEMORY: return "CL_OUT_OF_HOST_MEMORY"; break;
		case CL_PROFILING_INFO_NOT_AVAILABLE: return "CL_PROFILING_INFO_NOT_AVAILABLE"; break;
		case CL_MEM_COPY_OVERLAP: return "CL_MEM_COPY_OVERLAP"; break;
		case CL_IMAGE_FORMAT_MISMATCH: return "CL_IMAGE_FORMAT_MISMATCH"; break;
		case CL_IMAGE_FORMAT_NOT_SUPPORTED: return "CL_IMAGE_FORMAT_NOT_SUPPORTED"; break;
		case CL_BUILD_PROGRAM_FAILURE: return "CL_BUILD_PROGRAM_FAILURE"; break;
		case CL_MAP_FAILURE: return "CL_MAP_FAILURE"; break;
		case CL_MISALIGNED_SUB_BUFFER_OFFSET: return "CL_MISALIGNED_SUB_BUFFER_OFFSET"; break;
		case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST"; break;
		case CL_INVALID_VALUE: return "CL_INVALID_VALUE"; break;
		case CL_INVALID_DEVICE_TYPE: return "CL_INVALID_DEVICE_TYPE"; break;
		case CL_INVALID_PLATFORM: return "CL_INVALID_PLATFORM"; break;
		case CL_INVALID_DEVICE: return "CL_INVALID_DEVICE"; break;
		case CL_INVALID_CONTEXT: return "CL_INVALID_CONTEXT"; break;
		case CL_INVALID_QUEUE_PROPERTIES: return "CL_INVALID_QUEUE_PROPERTIES"; break;
		case CL_INVALID_COMMAND_QUEUE: return "CL_INVALID_COMMAND_QUEUE"; break;
		case CL_INVALID_HOST_PTR: return "CL_INVALID_HOST_PTR"; break;
		case CL_INVALID_MEM_OBJECT: return "CL_INVALID_MEM_OBJECT"; break;
		case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR"; break;
		case CL_INVALID_IMAGE_SIZE: return "CL_INVALID_IMAGE_SIZE"; break;
		case CL_INVALID_SAMPLER: return "CL_INVALID_SAMPLER"; break;
		case CL_INVALID_BINARY: return "CL_INVALID_BINARY"; break;
		case CL_INVALID_BUILD_OPTIONS: return "CL_INVALID_BUILD_OPTIONS"; break;
		case CL_INVALID_PROGRAM: return "CL_INVALID_PROGRAM"; break;
		case CL_INVALID_PROGRAM_EXECUTABLE: return "CL_INVALID_PROGRAM_EXECUTABLE"; break;
		case CL_INVALID_KERNEL_NAME: return "CL_INVALID_KERNEL_NAME"; break;
		case CL_INVALID_KERNEL_DEFINITION: return "CL_INVALID_KERNEL_DEFINITION"; break;
		case CL_INVALID_KERNEL: return "CL_INVALID_KERNEL"; break;
		case CL_INVALID_ARG_INDEX: return "CL_INVALID_ARG_INDEX"; break;
		case CL_INVALID_ARG_VALUE: return "CL_INVALID_ARG_VALUE"; break;
		case CL_INVALID_ARG_SIZE: return "CL_INVALID_ARG_SIZE"; break;
		case CL_INVALID_KERNEL_ARGS: return "CL_INVALID_KERNEL_ARGS"; break;
		case CL_INVALID_WORK_DIMENSION: return "CL_INVALID_WORK_DIMENSION"; break;
		case CL_INVALID_WORK_GROUP_SIZE: return "CL_INVALID_WORK_GROUP_SIZE"; break;
		case CL_INVALID_WORK_ITEM_SIZE: return "CL_INVALID_WORK_ITEM_SIZE"; break;
		case CL_INVALID_GLOBAL_OFFSET: return "CL_INVALID_GLOBAL_OFFSET"; break;
		case CL_INVALID_EVENT_WAIT_LIST: return "CL_INVALID_EVENT_WAIT_LIST"; break;
		case CL_INVALID_EVENT: return "CL_INVALID_EVENT"; break;
		case CL_INVALID_OPERATION: return "CL_INVALID_OPERATION"; break;
		case CL_INVALID_GL_OBJECT: return "CL_INVALID_GL_OBJECT"; break;
		case CL_INVALID_BUFFER_SIZE: return "CL_INVALID_BUFFER_SIZE"; break;
		case CL_INVALID_MIP_LEVEL: return "CL_INVALID_MIP_LEVEL"; break;
		case CL_INVALID_GLOBAL_WORK_SIZE: return "CL_INVALID_GLOBAL_WORK_SIZE"; break;
		case CL_INVALID_PROPERTY: return "CL_INVALID_PROPERTY"; break;
		case CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR"; break;
		default :{return "hmmm - error message makes no sense";}
	}

}
void printCLError(const char *where, cl_int err) {
	printf ("OpenCL fn %s, error %s (%d)\n",where,getCLErrorString(err),err);
}

/********************************************************************************/
/*                                                                              */
/*                                                                              */
/********************************************************************************/

static bool initialize_OpenCL() {

        ppOpenCL_Utils p;
        ttglobal tg = gglobal();
        p = (ppOpenCL_Utils)tg->OpenCL_Utils.prv;

	cl_int err;

	// debugging information
	cl_int rv;
	size_t rvlen;

	#ifdef GPU_DEBUG
	size_t wg_size;
	cl_ulong longish;
	size_t xyz;
	char rvstring[1000];
	int gpu;
	#endif // GPU_DEBUG

// get the current context.
// windows - IntPtr curDC = wglGetCurrentDC();
// then in the new compute context, we pass in the context

	/* initialized yet? */
	//if (p->kernel != NULL) return false;


	// get the device id

// OLD_IPHONE_AQUA #if defined (TARGET_AQUA)
// OLD_IPHONE_AQUA     {
// OLD_IPHONE_AQUA         printf ("testing...\n");
// OLD_IPHONE_AQUA         cl_uint num_devices, i;
// OLD_IPHONE_AQUA         clGetDeviceIDs (NULL, CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices);
// OLD_IPHONE_AQUA         cl_device_id *devices = calloc(sizeof(cl_device_id), num_devices);
// OLD_IPHONE_AQUA         clGetDeviceIDs(NULL, CL_DEVICE_TYPE_ALL, num_devices, devices, NULL);
// OLD_IPHONE_AQUA         char buf[128];
// OLD_IPHONE_AQUA         for (i=0; i<num_devices; i++) {
// OLD_IPHONE_AQUA             clGetDeviceInfo(devices[i], CL_DEVICE_NAME, 128, buf, NULL);
// OLD_IPHONE_AQUA             printf ("Device %s supports ",buf);
// OLD_IPHONE_AQUA             clGetDeviceInfo(devices[i],CL_DEVICE_VERSION, 128, buf, NULL);
// OLD_IPHONE_AQUA             printf ("%s\n",buf);
// OLD_IPHONE_AQUA         }
// OLD_IPHONE_AQUA         free (devices);
// OLD_IPHONE_AQUA     }
// OLD_IPHONE_AQUA 	CGLContextObj kCGLContext = CGLGetCurrentContext();
// OLD_IPHONE_AQUA 	CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
// OLD_IPHONE_AQUA 	cl_context_properties properties[] = {
// OLD_IPHONE_AQUA 		CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)kCGLShareGroup, 0 };
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	err = clGetDeviceIDs(NULL, CL_DEVICE_TYPE_GPU, 1, &p->CL_device_id, NULL);
// OLD_IPHONE_AQUA     {
// OLD_IPHONE_AQUA         char buf[128];
// OLD_IPHONE_AQUA         clGetDeviceInfo(p->CL_device_id, CL_DEVICE_NAME, 128, buf, NULL);
// OLD_IPHONE_AQUA         printf ("Device %s supports ",buf);
// OLD_IPHONE_AQUA         clGetDeviceInfo(p->CL_device_id,CL_DEVICE_VERSION, 128, buf, NULL);
// OLD_IPHONE_AQUA         printf ("%s\n",buf);
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA     }
// OLD_IPHONE_AQUA 	if (err != CL_SUCCESS) {
// OLD_IPHONE_AQUA 		printCLError("clGetDeviceIDs",err);
// OLD_IPHONE_AQUA 		return FALSE;
// OLD_IPHONE_AQUA 	}
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	p->CL_context =clCreateContext(properties,0,0,clLogMessagesToStderrAPPLE,0,&err);
// OLD_IPHONE_AQUA #endif // TARGET_AQUA

#if defined (_MSC_VER)

	if(1)
		err = extraInitFromNvidiaSamples(p);
	else
	{
		cl_int ciErrNum;
		cl_platform_id cpPlatform = NULL;          // OpenCL platform
		// Get the NVIDIA platform
		//ciErrNum = oclGetPlatformID(&cpPlatform);
		{
			/* from OpenCL Programming Guide, pg 338 */
			cl_context_properties properties[] = {
				CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
				CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
				CL_CONTEXT_PLATFORM, (cl_context_properties)cpPlatform,
				0};

			p->context = clCreateContext(properties, 1, &p->device_id, NULL, NULL, &err);
		}
	}
#endif // _MSC_VER

/* is this Linux? */
#if defined (__linux__)

	#define MAX_OPENCL_PLATFORMS 10
	#define MAX_OPENCL_DEVICES 32
	cl_platform_id platforms[MAX_OPENCL_PLATFORMS];
	cl_device_id devices[MAX_OPENCL_DEVICES];
	cl_uint numPlats;
	cl_uint numDevs;

	// we may have OpenCL, but maybe we dont have cl_khr_gl_sharing, so look for it
	int selectedPlatform = -1;
	int selectedDevice = -1;

	// printf ("have linux uere\n");
	// printf ("OpenCL - before clGetPlatformIDs\n");

	err = clGetPlatformIDs(10,platforms,&numPlats);
	TEST_ERR("clGetPlatformIDs",err);

	//printf ("looking for up to 10 platforms, got %d\n",numPlats);
	if (numPlats <1) {
		printf ("OpenCL init - numPlats is %d, OpenCL device not found\n",numPlats);
		return FALSE;
	}

	// bounds check
	if (numPlats >= MAX_OPENCL_PLATFORMS) {
		printf ("OpenCL init - numPlats is %d, setting to %d\n",numPlats,MAX_OPENCL_PLATFORMS);
		numPlats = MAX_OPENCL_PLATFORMS;
	}
	
	/* not sure what platform to choose, if more than 1...
	{
		int i;

		//printf ("printing out the platform names:\n");
		for (i=0; i<numPlats; i++) {
			char platname[500];
                        cl_int err = clGetPlatformInfo(platforms[i],CL_PLATFORM_NAME,sizeof(platname),platname,NULL);
			TEST_ERR("clGetPlatformInfo",err);
			printf ("GetPlatfromInfo for %d is :%s:\n",i,platname);
		}

	}
	*/

	//printf ("now, trying to get the device IDS\n");




	{
		int i,j;
		printf ("printing out the device names for each platform found\n");
		for (i=0; i<numPlats; i++) {
			err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, MAX_OPENCL_DEVICES, devices, &numDevs);
			printf ("done the clGetDeviceIDS call for platform %d - devices %d\n",i,numDevs);
			
/* XXX */
			for (j=0; j<numDevs; j++) {
				char crv[1000];
				size_t crvs;

				/*
				err = clGetDeviceInfo(devices[j],CL_DEVICE_NAME,1000,crv,&crvs); 
				printf ("NAME for %d is %s\n",j,crv);
				err = clGetDeviceInfo(devices[j],CL_DEVICE_VENDOR,1000,crv,&crvs); 
				printf ("VENDOR for %d is %s\n",j,crv);
				err = clGetDeviceInfo(devices[j],CL_DEVICE_PROFILE,1000,crv,&crvs); 
				printf ("PROFILE for %d is %s\n",j,crv);
				*/

				err = clGetDeviceInfo(devices[j],CL_DEVICE_EXTENSIONS,1000,crv,&crvs); 
				if (err != CL_SUCCESS) {
					printCLError("clGetDeviceIDs",err);
					return FALSE;
				}
				// printf ("EXTENSIONS for %d is %s\n",j,crv);
				
				if (strstr(crv,"cl_khr_gl_sharing") != NULL) {
					printf ("**** Found cl_khr_gl_sharing ****\n");
					selectedPlatform = i;
					selectedDevice = j;
					p->CL_device_id = devices[j];
				}
			}
		}
	}

	//printf ("Linux, have device id...\n");
	if ((selectedPlatform <0) || (selectedDevice<0)) {
		printCLError("No good OpenCL device or platform found, error ",err);
		return FALSE;
	}

	// redo the calls, now that we have (the best?) match
	if ((selectedPlatform != 0) && (selectedDevice != 0)) {
	//printf ("regetting platform %d and device %d\n",selectedPlatform, selectedDevice);
	err = clGetDeviceIDs(platforms[selectedPlatform], CL_DEVICE_TYPE_GPU, MAX_OPENCL_DEVICES, devices, &numDevs);
	}

	// now save the device id
	p->CL_device_id = devices[selectedDevice];


	// printf ("\n.....now doing the context sharing getting.....\n\n");

	cl_context_properties properties[] = {
		CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
		CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
		CL_CONTEXT_PLATFORM, (cl_context_properties)platforms[selectedPlatform],
		0 };

// function pointer typedefs must use the
// following naming convention
typedef CL_API_ENTRY cl_int
     (CL_API_CALL *clGetGLContextInfoKHR_fn)(
              const cl_context_properties * /* properties */,
              cl_gl_context_info /* param_name */,
              size_t /* param_value_size */,
              void * /* param_value */,
              size_t * /*param_value_size_ret*/);

clGetGLContextInfoKHR_fn clGetGLContextInfoKHR = NULL;

#ifdef CL_VERSION_1_2
clGetGLContextInfoKHR = (clGetGLContextInfoKHR_fn)clGetExtensionFunctionAddressForPlatform(platforms[selectedPlatform],"clGetGLContextInfoKHR");
#else
#ifdef CL_VERSION_1_1
clGetGLContextInfoKHR = (clGetGLContextInfoKHR_fn)clGetExtensionFunctionAddress("clGetGLContextInfoKHR");
#endif
#endif
	// find CL capable devices in the current GL context
	size_t size;
	err = clGetGLContextInfoKHR(properties, CL_DEVICES_FOR_GL_CONTEXT_KHR,
		MAX_OPENCL_DEVICES*sizeof(cl_device_id), devices, &size);

	TEST_ERR("clGetGLContextInfoKHR",err);

	printf ("clGetGLContextInfoKHR returns size of %d\n",size);

	//printf ("going to clCreateContextFromType:\n");
	//printf ("just so we know, p is %p\n",p);
	//printf ("just so we know, p->CL_context is %p\n",p->CL_context);

	p->CL_context=clCreateContextFromType(properties, CL_DEVICE_TYPE_GPU, NULL, NULL, &err);

	//printf ("done the createContextFromType, it is %p\n",p->CL_context);

	TEST_ERR("clCreateContextFromType",err);

#endif // linux

/* how about Android (and maybe IPHONE) using OpenCL-ES 2.0? */
#ifdef GL_ES_VERSION_2_0

	cl_platform_id platforms[10];
	cl_uint numPlats;

	err = getFunctionHandles();


 
	if (err != CL_SUCCESS) {
		printCLError("clCreateContext",err);
		return FALSE;
	}


	err = clGetPlatformIDs(10,platforms,&numPlats);
	TEST_ERR("clGetPlatformIDs",err);
	printf ("looking for up to 10 platforms, got %d",numPlats);
	
	cl_platform_id platform;
	err = clGetPlatformIDs(1,&platform,NULL);
	TEST_ERR("clGetPlatformIDs",err);

	err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &p->device_id, NULL);

	if (err != CL_SUCCESS) {
		printCLError("clGetDeviceIDs",err);
		return FALSE;
	}

/*
	cl_context_properties properties[] = {
		CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
		CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
		CL_CONTEXT_PLATFORM, (cl_context_properties)platform,
		0 };
	p->context=clCreateContextFromType(properties, CL_DEVICE_TYPE_GPU, NULL, NULL, &err);
	TEST_ERR("clCreateContextFromType",err);
*/

	p->CL_context=clCreateContextFromType(NULL, CL_DEVICE_TYPE_ANY, NULL, NULL, &err);
	TEST_ERR("clCreateContextFromType",err);

	printf ("remember - building currently without the CL_KHR_gl_sharing enabled - the clCreateFromGLBuffer will error out, so return code removed.");

#endif //GL_ES_VERSION_2_0


	// create a command queue

	p->CL_queue = clCreateCommandQueue(p->CL_context, p->CL_device_id, 0, &err);
    //printf ("CL_queue is %p for context %p, device %d\n",p->CL_queue, p->CL_context, p->CL_device_id);


	if (!p->CL_queue || (err != CL_SUCCESS)) {
		printCLError("clCreateCommandQueue",err);
		return FALSE;
	}

	rv = clGetDeviceInfo (p->CL_device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &(p->CL_default_workgroup_size), &rvlen);
	if ((rv != CL_SUCCESS) || (err != CL_SUCCESS)) {
		printCLError("clGetDeviceInfo",err);
		return FALSE;
	}


	#ifdef GPU_DEBUG
	// Find the work group size
	rv = clGetDeviceInfo (p->CL_device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &wg_size, &rvlen);
	TEST_ERR("clGetDeviceInfo",rv);
	printf ("CL_DEVICE_MAX_WORK_GROUP_SIZE %d\n",wg_size);

	// debugging information
// OLD_IPHONE_AQUA #ifndef AQUA
	rv = clGetPlatformInfo(platforms[0],CL_PLATFORM_PROFILE,1000,rvstring,&rvlen);
	printf ("CL_PLATFORM_PROFILE :%s:\n",rvstring);
	rv = clGetPlatformInfo(platforms[0],CL_PLATFORM_VERSION,1000,rvstring,&rvlen);
	printf ("CL_PLATFORM_VERSION :%s:\n",rvstring);
	rv = clGetPlatformInfo(platforms[0],CL_PLATFORM_NAME,1000,rvstring,&rvlen);
	printf ("CL_PLATFORM_NAME :%s:\n",rvstring);
	rv = clGetPlatformInfo(platforms[0],CL_PLATFORM_VENDOR,1000,rvstring,&rvlen);
	printf ("CL_PLATFORM_VENDOR :%s:\n",rvstring);
	rv = clGetPlatformInfo(platforms[0],CL_PLATFORM_EXTENSIONS,1000,rvstring,&rvlen);
	printf ("CL_PLATFORM_EXTENSIONS :%s:\n",rvstring);
// OLD_IPHONE_AQUA #endif

	rv = clGetDeviceInfo (p->CL_device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(size_t), &xyz, &rvlen);
	printf ("CL_DEVICE_MAX_COMPUTE_UNITS %d\n",xyz);
	rv = clGetDeviceInfo (p->CL_device_id, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, sizeof(cl_ulong), &longish, &rvlen);
	printf ("CL_DEVICE_GLOBAL_MEM_CACHE_SIZE %lu\n",longish);
	rv = clGetDeviceInfo (p->CL_device_id, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &longish, &rvlen);
	printf ("CL_DEVICE_GLOBAL_MEM_SIZE %lu\n",longish);
	rv = clGetDeviceInfo (p->CL_device_id, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &longish, &rvlen);
	printf ("CL_DEVICE_LOCAL_MEM_SIZE %lu\n",longish);
	rv= clGetDeviceInfo (p->CL_device_id,CL_DEVICE_EXTENSIONS, 1000, rvstring,&rvlen);
	printf ("CL_DEVICE_EXTENSIONS :%s:\n",rvstring);
	rv= clGetDeviceInfo (p->CL_device_id,CL_DEVICE_PROFILE, 1000, rvstring,&rvlen);
	printf ("CL_DEVICE_PROFILE :%s:\n",rvstring);
	rv= clGetDeviceInfo (p->CL_device_id,CL_DEVICE_NAME, 1000, rvstring,&rvlen);
	printf ("CL_DEVICE_NAME :%s:\n",rvstring);
	rv= clGetDeviceInfo (p->CL_device_id,CL_DEVICE_VENDOR, 1000, rvstring,&rvlen);
	printf ("CL_DEVICE_VENDOR :%s:\n",rvstring);
	rv= clGetDeviceInfo (p->CL_device_id,CL_DEVICE_VERSION, 1000, rvstring,&rvlen);
	printf ("CL_DEVICE_VERSION :%s:\n",rvstring);


	#endif //GPU_DEBUG
#undef GPU_DEBUG

	// do this when we need collision - collision_initGPUCollide(p);

	return TRUE;
}


/********************************************************************************/
/*                                                                              */
/* Android, (code might work on IPHONE) OpenGL ES 2.0, CL integration		*/
/*                                                                              */
/********************************************************************************/

#ifdef GL_ES_VERSION_2_0

#include <dlfcn.h> // possibly Android only

#define clGetPlatformIDs(aa,bb,cc) rclGetPlatformIDs(aa,bb,cc)
#define clGetPlatformInfo(aa,bb,cc,dd,ee) rclGetPlatformInfo(aa,bb,cc,dd,ee)
#define clGetDeviceIDs(aa,bb,cc,dd,ee) rclGetDeviceIDs(aa,bb,cc,dd,ee)
#define clGetDeviceInfo(aa,bb,cc,dd,ee) rclGetDeviceInfo(aa,bb,cc,dd,ee)
#define clCreateKernel(aa,bb,cc) rclCreateKernel(aa,bb,cc)
#define clBuildProgram(aa,bb,cc,dd,ee,ff) rclBuildProgram(aa,bb,cc,dd,ee,ff)
#define clCreateBuffer(aa,bb,cc,dd,ee) rclCreateBuffer(aa,bb,cc,dd,ee)
#define clCreateCommandQueue(aa,bb,cc,dd) rclCreateCommandQueue(aa,bb,cc,dd)
#define clCreateContextFromType(aa,bb,cc,dd,ee) rclCreateContextFromType(aa,bb,cc,dd,ee)
#define clCreateFromGLBuffer(aa,bb,cc,dd) rclCreateFromGLBuffer(aa,bb,cc,dd)
#define clCreateProgramWithSource(aa,bb,cc,dd,ee) rclCreateProgramWithSource(aa,bb,cc,dd,ee)
#define clEnqueueNDRangeKernel(aa,bb,cc,dd,ee,ff,gg,hh,ii) rclEnqueueNDRangeKernel(aa,bb,cc,dd,ee,ff,gg,hh,ii)
#define clEnqueueReadBuffer(aa,bb,cc,dd,ee,ff,gg,hh,ii) rclEnqueueReadBuffer(aa,bb,cc,dd,ee,ff,gg,hh,ii)
#define clEnqueueWriteBuffer(aa,bb,cc,dd,ee,ff,gg,hh,ii) rclEnqueueWriteBuffer(aa,bb,cc,dd,ee,ff,gg,hh,ii)
#define clGetKernelWorkGroupInfo(aa,bb,cc,dd,ee,ff) rclGetKernelWorkGroupInfo(aa,bb,cc,dd,ee,ff)
#define clGetProgramBuildInfo(aa,bb,cc,dd,ee,ff) rclGetProgramBuildInfo(aa,bb,cc,dd,ee,ff)
#define clReleaseMemObject(aa) rclReleaseMemObject(aa)
#define clSetKernelArg(aa,bb,cc,dd) rclSetKernelArg(aa,bb,cc,dd)

static void *getCLHandle(){
	void *res = NULL;
	int which=0;

	res = dlopen("/system/lib/libOpenCL.so",RTLD_LAZY);
	if(res==NULL){
		res = dlopen("/system/vendor/lib/egl/libGLES_mali.so",RTLD_LAZY);
		which = 1;
	}
	if(res==NULL){
		res = dlopen("/system/lib/libllvm-a3xx.so",RTLD_LAZY);
		which = 2;
	}
	if(res==NULL) {
		ConsoleMessage("Could not open library :(\n");
		return NULL;
	}

	if (which==0) {
		ConsoleMessage ("OpenCL lib - libOpenCL.so");
	} else if (which == 1) {
		ConsoleMessage ("OpenCL lib libGLES_mali.so");
	} else if (which == 2) {
		ConsoleMessage ("OpenCL Lib - liblvm-a3xx.so");
	}
	return res;
}

cl_int (*rclGetPlatformIDs)(cl_uint          /* num_entries */,
                 cl_platform_id * /* platforms */,
                 cl_uint *        /* num_platforms */);


cl_int (*rclGetPlatformInfo)(cl_platform_id   /* platform */, 
                  cl_platform_info /* param_name */,
                  size_t           /* param_value_size */, 
                  void *           /* param_value */,
                  size_t *         /* param_value_size_ret */);

cl_int (*rclGetDeviceIDs)(cl_platform_id   /* platform */,
               cl_device_type   /* device_type */, 
               cl_uint          /* num_entries */, 
               cl_device_id *   /* devices */, 
               cl_uint *        /* num_devices */);


cl_int (*rclGetDeviceInfo)(cl_device_id    /* device */,
                cl_device_info  /* param_name */, 
                size_t          /* param_value_size */, 
                void *          /* param_value */,
                size_t *        /* param_value_size_ret */);

cl_kernel (*rclCreateKernel)(cl_program /*program */,
		const char * /* kernel name */,
		cl_int *	/* errorcode_ret */);

cl_int (*rclBuildProgram)(cl_program           /* program */,
               cl_uint              /* num_devices */,
               const cl_device_id * /* device_list */,
               const char *         /* options */,
               void (CL_CALLBACK *  /* pfn_notify */)(cl_program /* program */, void * /* user_data */),
               void *               /* user_data */);

cl_mem (*rclCreateBuffer)(cl_context   /* context */,
               cl_mem_flags /* flags */,
               size_t       /* size */,
               void *       /* host_ptr */,
               cl_int *     /* errcode_ret */);

cl_command_queue (*rclCreateCommandQueue)(cl_context                     /* context */,
                     cl_device_id                   /* device */,
                     cl_command_queue_properties    /* properties */,
                     cl_int *                       /* errcode_ret */);

	
cl_context (*rclCreateContextFromType)(const cl_context_properties * /* properties */,
                        cl_device_type          /* device_type */,
                        void (CL_CALLBACK *     /* pfn_notify*/ )(const char *, const void *, size_t, void *),
                        void *                  /* user_data */,
                        cl_int *                /* errcode_ret */);

cl_program (*rclCreateProgramWithSource)(cl_context        /* context */,
                          cl_uint           /* count */,
                          const char **     /* strings */,
                          const size_t *    /* lengths */,
                          cl_int *          /* errcode_ret */);


cl_int (*rclEnqueueNDRangeKernel)(cl_command_queue /* command_queue */,
                       cl_kernel        /* kernel */,
                       cl_uint          /* work_dim */,
                       const size_t *   /* global_work_offset */,
                       const size_t *   /* global_work_size */,
                       const size_t *   /* local_work_size */,
                       cl_uint          /* num_events_in_wait_list */,
                       const cl_event * /* event_wait_list */,
                       cl_event *       /* event */);

cl_int (*rclEnqueueReadBuffer)(cl_command_queue    /* command_queue */,
                    cl_mem              /* buffer */,
                    cl_bool             /* blocking_read */,
                    size_t              /* offset */,
                    size_t              /* size */,
                    void *              /* ptr */,
                    cl_uint             /* num_events_in_wait_list */,
                    const cl_event *    /* event_wait_list */,
                    cl_event *          /* event */);


cl_int (*rclEnqueueWriteBuffer)(cl_command_queue   /* command_queue */,
                     cl_mem             /* buffer */,
                     cl_bool            /* blocking_write */,
                     size_t             /* offset */,
                     size_t             /* size */,
                     const void *       /* ptr */,
                     cl_uint            /* num_events_in_wait_list */,
                     const cl_event *   /* event_wait_list */,
                     cl_event *         /* event */);


cl_int (*rclGetKernelWorkGroupInfo)(cl_kernel                  /* kernel */,
                         cl_device_id               /* device */,
                         cl_kernel_work_group_info  /* param_name */,
                         size_t                     /* param_value_size */,
                         void *                     /* param_value */,
                         size_t *                   /* param_value_size_ret */);


cl_int (*rclReleaseMemObject)(cl_mem /* memobj */);

cl_int (*rclSetKernelArg)(cl_kernel    /* kernel */,
               cl_uint      /* arg_index */,
               size_t       /* arg_size */,
               const void * /* arg_value */);

cl_mem (*rclCreateFromGLBuffer)(cl_context, cl_mem_flags, GLuint, cl_int *);

cl_int (*rclGetProgramBuildInfo)(cl_program, cl_device_id, cl_program_build_info, size_t, void *, size_t *);

static int getFunctionHandles(){
	static void* getCLHandle();

	void *handle = getCLHandle();
	if(handle==NULL) return CL_DEVICE_NOT_AVAILABLE;
	rclGetPlatformIDs = (cl_int (*)(cl_uint,cl_platform_id *,cl_uint*))dlsym(handle,"clGetPlatformIDs");
	rclGetPlatformInfo = (cl_int (*)(cl_platform_id, cl_platform_info, size_t, void *, size_t*))dlsym(handle,"clGetPlatformInfo");
	rclGetDeviceIDs = (cl_int (*)(cl_platform_id, cl_device_type, cl_uint, cl_device_id *, cl_uint*))dlsym(handle,"clGetDeviceIDs");
	rclGetDeviceInfo = (cl_int (*)(cl_device_id, cl_device_info, size_t, void *, size_t*))dlsym(handle,"clGetDeviceInfo");
	rclBuildProgram = (cl_int (*)(cl_program,cl_uint,const cl_device_id *, const char *, void (CL_CALLBACK*)(cl_program,void*), void *))dlsym(handle,"clBuildProgram");
	rclCreateBuffer = (cl_mem (*)(cl_context, cl_mem_flags, size_t, void *, cl_int *))dlsym(handle,"clCreateBuffer");
	rclCreateKernel = (cl_kernel (*)(cl_program,const char*,cl_int*))dlsym(handle,"clCreateKernel");
	rclCreateCommandQueue = (cl_command_queue (*) (cl_context,cl_device_id,cl_command_queue_properties,cl_int*))dlsym(handle,"clCreateCommandQueue");
	rclCreateContextFromType = (cl_context (*)(const cl_context_properties*,cl_device_type,void(CL_CALLBACK*)(const char*,const void*,size_t,void*),void*,cl_int*))dlsym(handle,"clCreateContextFromType");
	rclCreateProgramWithSource = (cl_program (*) (cl_context,cl_uint,const char**,const size_t*,cl_int*))dlsym(handle,"clCreateProgramWithSource");
	rclEnqueueNDRangeKernel=(cl_int(*)(cl_command_queue,cl_kernel,cl_uint,const size_t*,const size_t*,const size_t*,cl_uint,const cl_event*,cl_event*))dlsym(handle,"clEnqueueNDRangeKernel");
	rclEnqueueReadBuffer = (cl_int (*)(cl_command_queue,cl_mem,cl_bool,size_t,size_t,void*,cl_uint,const cl_event*,cl_event*))dlsym(handle,"clEnqueueReadBuffer");
	rclEnqueueWriteBuffer = (cl_int (*)(cl_command_queue,cl_mem,cl_bool,size_t,size_t,const void*,cl_uint,const cl_event*,cl_event*))dlsym(handle,"clEnqueueWriteBuffer");
	rclGetKernelWorkGroupInfo = (cl_int (*)(cl_kernel,cl_device_id,cl_kernel_work_group_info,size_t, void*, size_t *))dlsym(handle,"clGetKernelWorkGroupInfo");
	rclReleaseMemObject = (cl_int (*)(cl_mem))dlsym(handle,"clReleaseMemObject");
	rclSetKernelArg = (cl_int (*)(cl_kernel,cl_uint,size_t,const void *))dlsym(handle,"clSetKernelArg");
	rclGetProgramBuildInfo = (cl_int (*)(cl_program, cl_device_id, cl_program_build_info, size_t, void *, size_t *))dlsym(handle,"clGetProgramBuildInfo");
	rclCreateFromGLBuffer = (cl_mem (*)(cl_context, cl_mem_flags, GLuint, cl_int *))dlsym(handle,"clCreateFromGLBuffer");



	if (!(rclGetPlatformIDs) || !(rclGetPlatformInfo) || !(rclGetDeviceIDs) || !(rclGetDeviceInfo) ||
		!(rclBuildProgram) || !(rclCreateBuffer) || !(rclCreateKernel) || !(rclCreateCommandQueue) ||
		!(rclCreateContextFromType) || !(rclCreateProgramWithSource) || !(rclEnqueueNDRangeKernel) || !(rclEnqueueReadBuffer) ||
		!(rclEnqueueWriteBuffer) || !(rclGetKernelWorkGroupInfo) || !(rclReleaseMemObject) || !(rclSetKernelArg) ||
		!(rclGetProgramBuildInfo) || !(rclCreateFromGLBuffer)) {
			ConsoleMessage ("did not find one of the functions in this OpenCL Library");
			if (!rclGetPlatformIDs) ConsoleMessage ("did not find rclGetPlatformIDs");
			if (!rclGetPlatformInfo) ConsoleMessage ("did not find rclGetPlatformInfo");
			if (!rclGetDeviceIDs) ConsoleMessage ("did not find rclGetDeviceIDs");
			if (!rclGetDeviceInfo) ConsoleMessage ("did not find rclGetDeviceInfo");
			if (!rclBuildProgram) ConsoleMessage ("did not find rclBuildProgram");
			if (!rclCreateBuffer) ConsoleMessage ("did not find rclCreateBuffer");
			if (!rclCreateKernel) ConsoleMessage ("did not find rclCreateKernel");
			if (!rclCreateCommandQueue) ConsoleMessage ("did not find rclCreateCommandQueue");
			if (!rclCreateContextFromType) ConsoleMessage ("did not find rclCreateContextFromType");
			if (!rclCreateProgramWithSource) ConsoleMessage ("did not find rclCreateProgramWithSource");
			if (!rclEnqueueNDRangeKernel) ConsoleMessage ("did not find rclEnqueueNDRangeKernel");
			if (!rclEnqueueReadBuffer) ConsoleMessage ("did not find rclEnqueueReadBuffer");
			if (!rclEnqueueWriteBuffer) ConsoleMessage ("did not find rclEnqueueWriteBuffer");
			if (!rclGetKernelWorkGroupInfo) ConsoleMessage ("did not find rclGetKernelWorkGroupInfo");
			if (!rclReleaseMemObject) ConsoleMessage ("did not find rclReleaseMemObject");
			if (!rclSetKernelArg) ConsoleMessage ("did not find rclSetKernelArg");
			if (!rclGetProgramBuildInfo) ConsoleMessage ("did not find rclGetProgramBuildInfo");
			if (!rclCreateFromGLBuffer) ConsoleMessage ("did not find rclCreateFromGLBuffer");

		// JAS return !CL_SUCCESS;
	}
	return CL_SUCCESS;
}


#endif

/* create the Interpolators for the GPU */
static void createGPUInterpolators() {
    ppOpenCL_Utils p;
    ttglobal tg = gglobal();
    p = (ppOpenCL_Utils)tg->OpenCL_Utils.prv;

    //bool collision_initGPUCollide (struct sCollisionGPU* initme) {
    char *kp[2];
        cl_int err;
        size_t kernel_wg_size;
        size_t rvlen;
        
        //printf ("called initGPUCollide, p is %p\n",p);
        //printf ("called initGPUCollide, context is %p\n",p->CL_context);
        //printf ("called initGPUCollide, initme is %p\n",initme);
        
        kp[0] = (char *)interpolator_headers;
        kp[1] = (char *)coordinateInterpolator_kernel;
        
        p->coordinateInterpolatorProgram = clCreateProgramWithSource(p->CL_context, 2, (const char **) kp, NULL, &err);
        //printf ("past the clCreateProgramWithSource call\n");
        
        if (!p->coordinateInterpolatorProgram || (err != CL_SUCCESS)) {
            printCLError("clCreateProgramWithSource",err);
            return;
        }
        
        // build the compute program executable
        //char *opts = "-Werror -cl-single-precision-constant -cl-nv-verbose  -g -cl-opt-disable -cl-strict-aliasing";
        //char *opts = "-Werror -cl-single-precision-constant -cl-opt-disable -cl-strict-aliasing";
        //err = clBuildProgram(p->program, 0, NULL, opts, NULL, NULL);
        //ConsoleMessage ("calling clBuildProgram with program %p\n",p->program);
        
        // build the program, hard code in devices to 1 device, with the device list, no options
        char *opts = NULL;
        err = clBuildProgram(p->coordinateInterpolatorProgram, 1, &(p->CL_device_id), opts, NULL, NULL);
        //printf ("past the clBuildProgram call\n");
        
        //ConsoleMessage ("called clBuildProgram error %d\n",err);
        if (err != CL_SUCCESS) {
        	size_t len;
        	char buffer[16384];
            
        	ConsoleMessage("Error: Failed to build program executable\n");
            printCLError("clBuildProgram",err);
        	err = clGetProgramBuildInfo(p->coordinateInterpolatorProgram, p->CL_device_id, CL_PROGRAM_BUILD_LOG,
                                        sizeof(buffer), buffer, &len);
            TEST_ERR("clGetProgramBuildInfo",err);
            ConsoleMessage ("error string len %d\n",(int)len);
        	ConsoleMessage("%s\n", buffer);
        	return;
    	}
        
        // create the compute kernel
        p->coordinateInterpolatorKernel = clCreateKernel(p->coordinateInterpolatorProgram, "compute_collide", &err);
         //printf ("kernel is %p %p\n",p, p->coordinateInterpolatorKernel);
        
        if (!p->coordinateInterpolatorKernel || (err != CL_SUCCESS)) {
            printCLError("clCreateKernel",err);
        	return;
        }
        
        
        // Kernel Workgroup size
        // rv = clGetDeviceInfo (p->device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &wg_size, &rvlen);
        err = clGetKernelWorkGroupInfo (p->coordinateInterpolatorKernel, p->CL_device_id,
                                        CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &kernel_wg_size, &rvlen);
        
        if (err!=CL_SUCCESS) {
            printCLError( "clGetKernelWorkGroupInfo",err);
            return;
        }
        
        // try the smaller of the two
        if (kernel_wg_size < p->CL_default_workgroup_size) p->coordinateInterpolator_workgroup_size = kernel_wg_size;
        else p->coordinateInterpolator_workgroup_size = p->CL_default_workgroup_size;
        
#ifdef GPU_DEBUG
        ConsoleMessage ("MAX_WORK_GROUP_SIZE %d\n",kernel_wg_size);
        ConsoleMessage ("We are going to set our workgroup size to %d\n",wg_size);
        
        
        /*
         1. Get workGroupSize from clGetDeviceInfo with CL_DEVICE_mum of two values and use that value as your optimal workGroupSize
         2. Get KernelWorkGroupSize from from clGetKernelWorkGroupInfo with CL_KERNEL_WORK_GPOUP_SIZE
         3. Get minimum of two values and use that value as your optimal workGroupSize
         */
        
#endif // GPU_DEBUG
}

static int printOnce = FALSE;

/* actually do the interpolator for each and every parent - eg, a Coordinate destination may have multiple
 parents, of course, (DEF, multi USE) */

#define  TEST_GLOBAL
#ifdef TEST_GLOBAL
cl_mem myK = NULL;
cl_mem myKV = NULL;
cl_mem myVert = NULL;
#endif


static void runItOnce(cl_kernel myKernel, GLuint keyVBO, GLuint keyValueVBO, GLuint destVBO, int keysIn, int keyValuesIn, float frac) {
    cl_int err;
    size_t global_work_size;
    size_t local_work_size;
  
	//printf ("runItOnce...\n");

#ifdef TESTING
    /* TESTING */
        float rvs[2000];
    int i;
    cl_mem output_buffer;
#endif //TESTING
   
#ifdef TESTING
    printf ("calling glFinish()\n");
    glFinish();
    
    printf ("runItOnce, frac %f keysIn %d keyValuesIn %d\n",frac,keysIn,keyValuesIn);
 
#endif //TESTING
    
#ifndef TEST_GLOBAL
    // set up pointers to buffers
    cl_mem myK, myKV,myVert;
#endif
    
    ppOpenCL_Utils p;
    ttglobal tg = gglobal();
    p = (ppOpenCL_Utils)tg->OpenCL_Utils.prv;

#ifdef TEST_GLOBAL
    if (myK==NULL) myK = clCreateFromGLBuffer(p->CL_context, CL_MEM_READ_ONLY, keyVBO, &err);
    TEST_ERR("clCreateFromGLBuffer 1",err);
    
    if (myKV == NULL) myKV = clCreateFromGLBuffer(p->CL_context, CL_MEM_READ_ONLY, keyValueVBO, &err);
    TEST_ERR("clCreateFromGLBuffer 2",err);
    
    
    if (myVert==NULL) myVert = clCreateFromGLBuffer(p->CL_context, CL_MEM_WRITE_ONLY, destVBO, &err);
    TEST_ERR("clCreateFromGLBuffer 3",err);
    clFinish(p->CL_queue);

#else
    myK = clCreateFromGLBuffer(p->CL_context, CL_MEM_READ_ONLY, keyVBO, &err);
    TEST_ERR("clCreateFromGLBuffer 1",err);

    myKV = clCreateFromGLBuffer(p->CL_context, CL_MEM_READ_ONLY, keyValueVBO, &err);
    TEST_ERR("clCreateFromGLBuffer 2",err);
    
    
    myVert = clCreateFromGLBuffer(p->CL_context, CL_MEM_WRITE_ONLY, destVBO, &err);
    TEST_ERR("clCreateFromGLBuffer 3",err);
    clFinish(p->CL_queue);
#endif //TEST_GLOBAL
    
    

    /* TESTING */
    //printf ("acquiring objects\n");
    err = clEnqueueAcquireGLObjects(p->CL_queue, 1, &myVert, 0, NULL, NULL);
    TEST_ERR("clEnqueueAcquire",err);
    

    
    
    //send along the values as arguments to the CL kernel
    err = clSetKernelArg(myKernel, 0, sizeof(cl_mem), &myK);
	TEST_ERR("clSetKernelArg",err);
    err = clSetKernelArg(myKernel, 1, sizeof(cl_mem), &myKV);
	TEST_ERR("clSetKernelArg",err);
    err = clSetKernelArg(myKernel, 2, sizeof(cl_mem), &myVert);
	TEST_ERR("clSetKernelArg",err);
    err =clSetKernelArg(myKernel, 3, sizeof(int), &keysIn);
	TEST_ERR("clSetKernelArg",err);
    err =clSetKernelArg(myKernel, 4, sizeof(int), &keyValuesIn);
	TEST_ERR("clSetKernelArg",err);
    err =clSetKernelArg(myKernel, 5, sizeof(float), &frac);
	TEST_ERR("clSetKernelArg",err);
    
    
    /* testing */
#ifdef TESTING
    output_buffer = clCreateBuffer(p->CL_context, CL_MEM_WRITE_ONLY, sizeof(float) * keyValuesIn/keysIn, NULL, NULL);
    err = clSetKernelArg(myKernel, 6, sizeof(cl_mem), &output_buffer);
	TEST_ERR("clSetKernelArg",err);
#endif //TESTING




    
    // global work group size
#define MYWG (p->CL_default_workgroup_size)
	// find out how many "blocks" we can have
	if (MYWG > 0)
		global_work_size = (size_t) (keysIn) / MYWG;
	else global_work_size = 0;
    
	// add 1 to it, because we have to round up
	global_work_size += 1;
    
	// now, global_work_size will be an exact multiple of local_work_size
	global_work_size *= MYWG;
    
	//ConsoleMessage ("global_work_size is %d %x right now...\n",global_work_size, global_work_size);
    
	local_work_size = MYWG;
	//ConsoleMessage ("local_work_size %d\n",local_work_size);
	//ConsoleMessage ("ntri %d, global_work_size %d, local_work_size %d\n",ntri,global_work_size,local_work_size);
    
    //printf ("calling kernel local_work_size %d, global_Work_size %d\n",local_work_size, global_work_size);
    err = clEnqueueNDRangeKernel(p->CL_queue, myKernel, 1, NULL, &global_work_size, &local_work_size, 0, NULL, NULL);
    TEST_ERR("clEnqueueNDRangeKernel", err);
    
    //printf ("called kernel\n");
 
#ifdef TESTING
    clFinish(p->CL_queue);
    
    printf ("past clFinish\n");
#endif //TESTING

    
        /* TESTING */
 /*   glBindBuffer(GL_ARRAY_BUFFER,destVBO);
    glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof (float) * 24, rvs);
    for (i=0; i<8; i++) {
        printf ("glBufferData is %d %f\n",i,rvs[i]);
    }
  */
    
#ifndef TEST_GLOBAL
    err = clEnqueueReleaseGLObjects(p->CL_queue, 1, &myKV, 0, NULL, NULL);
    TEST_ERR("clEnqueueRelease",err);

    err = clReleaseMemObject(myK) || clReleaseMemObject(myKV) || clReleaseMemObject(myVert);
    TEST_ERR("clReleaseMemObject",err);
 
#endif // TEST_GLOBAL

#ifdef TESTING

    err = clEnqueueReadBuffer (p->CL_queue, output_buffer,
                               CL_TRUE, 0, sizeof(float) * 6 /* keyValuesIn/keysIn */,
                               rvs, 0, NULL, NULL);
    
	if (err != CL_SUCCESS) {
		printCLError("clEnqueueReadBuffer",err);
		return;
	}
    
    
	for (i=0; i < (keyValuesIn/keysIn); i++) {
        printf ("rv %d is %f\n", i, rvs[i]);
    }
    clReleaseMemObject(output_buffer);
    
#endif //TESTING

    

    
}

/* do an interpolator on the GPU - the destination will be on the GPU, too */
void runOpenCLInterpolator(struct CRStruct *route, struct X3D_Node * toNode, int toOffset) {
    GLuint keyVBO = 0;
    GLuint keyValueVBO = 0;
    GLuint destVBO = 0;
    int keysIn = 0;
    int keyValuesIn = 0;
    float frac = 0.0;
    
  
    
    if (!printOnce) {
    printf ("RUNNING OPENCL INTERPOLATOR PROGRAM %p\n",route->CL_Interpolator);
    printf ("it is coming from a %s\n",stringNodeType(route->routeFromNode->_nodeType));
    printf ("and, it is going to a %s\n",stringNodeType(toNode->_nodeType));
    printf ("with a length of %d\n",route->len);
    }
    
    if (route->CL_Interpolator == NULL) {
        printf ("runCLInterpolator  - interpolator is NULL??\n");
        return;
    }
    
    if ((toNode == NULL) || (route->routeFromNode == NULL)) {
        printf ("runCLInterpolator - error - destination or source NULL\n");
        return;
    }
    
    // gather info here
    switch (route->routeFromNode->_nodeType) {
        case NODE_CoordinateInterpolator: {
            struct X3D_CoordinateInterpolator *px = (struct X3D_CoordinateInterpolator *) route->routeFromNode;
            keyVBO = px->_keyVBO;
            keyValueVBO = px->_keyValueVBO;
            keysIn = px->key.n;
            keyValuesIn = px->keyValue.n;
            frac = px->set_fraction;
            break;
        }
        default: ConsoleMessage ("do not route from a node of %s on the  GPU - help!\n",
                                 stringNodeType(route->routeFromNode->_nodeType));
    }
    
    if ((keyVBO == 0) || (keyValueVBO == 0)) {
        printf ("runCLInterpolator - error - source VBOS are %d %d, should not be zero\n",
                keyVBO, keyValueVBO);
        return;
    }
    
    switch (toNode->_nodeType) {
        case NODE_Coordinate: {
            struct X3D_Coordinate *px = X3D_COORDINATE(toNode);
            int i;
		//printf ("haveCoordinateHere...\n");

            for (i=0; i<vectorSize(px->_parentVector); i++) {
                struct X3D_Node * me = vector_get(struct X3D_Node *, px->_parentVector, i);
                //printf ("parent %d of %d is %s\n",i,vectorSize(px->_parentVector), stringNodeType(me->_nodeType));
                struct X3D_PolyRep pr = *(me->_intern);
                //printf ("polyrep buffer is %d\n",pr.VBO_buffers[VERTEX_VBO]);
                destVBO = pr.VBO_buffers[VERTEX_VBO];
                
                if (destVBO != 0) {
                    runItOnce(route->CL_Interpolator, keyVBO, keyValueVBO, destVBO, keysIn, keyValuesIn, frac);
                }
            }
            //destVBO = px->
            break;
        }
        default: ConsoleMessage ("do not route from a node of %s on the  GPU - help!\n",
                                 stringNodeType(route->routeFromNode->_nodeType));
    }
    
    if (!printOnce) {
        printf ("so, if we were to run the interp, keyVBO %d keyValueVBO %d destVBO %d keysIn %d keyValuesIn %d frac %f\n",
                keyVBO, keyValueVBO, destVBO, keysIn, keyValuesIn, frac);
        printOnce = TRUE;
    }
}

#ifdef GL_ES_VERSION_2_0
static const char* interpolator_headers = " \
//#pragma OPENCL EXTENSION cl_khr_fp64 : enable \n\
#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable \n\
//#pragma OPENCL EXTENSION CL_APPLE_gl_sharing : enable \n\
//#pragma OPENCL EXTENSION CL_KHR_gl_sharing : enable \n\
//#pragma OPENCL EXTENSION cl_khr_select_fprounding_mode : enable \n\
";
#else

// OLD_IPHONE_AQUA #if defined (TARGET_AQUA)
// OLD_IPHONE_AQUA static const char* interpolator_headers = " \
// OLD_IPHONE_AQUA //#pragma OPENCL EXTENSION cl_khr_fp64 : enable \n\
// OLD_IPHONE_AQUA #pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable \n\
// OLD_IPHONE_AQUA #pragma OPENCL EXTENSION CL_APPLE_gl_sharing : enable \n\
// OLD_IPHONE_AQUA #pragma OPENCL EXTENSION CL_KHR_gl_sharing : enable \n\
// OLD_IPHONE_AQUA #pragma OPENCL EXTENSION cl_khr_select_fprounding_mode : enable \n\
// OLD_IPHONE_AQUA ";
// OLD_IPHONE_AQUA #else

// this seems to be ok on AMD drivers under Linux
static const char* interpolator_headers = " \
#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable \n\
//#pragma OPENCL EXTENSION cl_khr_gl_sharing : enable \n\
";
// OLD_IPHONE_AQUA #endif // AQUA

#endif

static const char* coordinateInterpolator_kernel = " \
/* Function prototypes */ \n \
int find_key (int kin, float frac, __global float *keys); \n \
\n \
int find_key (int kin, float frac, __global float *keys) { \n \
int counter; \n \
\
for (counter=1; counter <= kin; counter++) { \n \
    if (frac <keys[counter]) { \n \
        return counter; \n \
    } \n \
} \n \
return kin;     /* huh? not found! */ \n \
} \
\
/********************************************************************************/ \n\
\n\
__kernel void compute_collide (  \n\
__global    float *keys,            /* 0 */  \n\
__global    float *keyValues,       /* 1 */ \n \
__global    float *destVertices,    /* 2 */ \n \
const       int kin,                /* 3 */  \n\
const       int kvin,               /* 4 */ \n \
const       float frac              /* 5 */ \n \
/* , __global    float *output  */           \n \
) {   \n\
\n\
    int i_am_canadian = get_global_id(0); \n\
\
    /* get keysPerKeyValue */ \n \
    int kpkv = kvin/kin; \n\
if (i_am_canadian > kpkv) return; /* this invocation is above our bounds */ \n\
\
//output[i_am_canadian] = -999.9f; /* convert_float(get_global_id(0)); */ /* keys[kin-1]; */ \n \
\
\
//output[i_am_canadian] = destVertices[i_am_canadian]; \n \
 \
    /* set fraction less than or greater than keys */ \n\
    if (frac <= keys[0]) {  \n\
//output[i_am_canadian] = -100.0f; \n \
       destVertices[i_am_canadian*3+0] = keyValues[i_am_canadian*3+0]; \n \
        destVertices[i_am_canadian*3+1] = keyValues[i_am_canadian*3+1]; \n \
        destVertices[i_am_canadian*3+2] = keyValues[i_am_canadian*3+2]; \n \
    } else if (frac >=keys[kin-1]) { \n \
//output[i_am_canadian] = 100.0f; \n\
        destVertices[i_am_canadian*3+0] = keyValues[(kvin - kpkv + i_am_canadian)*3+0]; \n \
        destVertices[i_am_canadian*3+1] = keyValues[(kvin - kpkv + i_am_canadian)*3+1]; \n \
        destVertices[i_am_canadian*3+2] = keyValues[(kvin - kpkv + i_am_canadian)*3+2]; \n \
    } else { \n \
        int myKey = find_key(kin,frac,keys); \n \
        float interval = (frac - keys[myKey-1]) / (keys[myKey] - keys[myKey-1]); \n \
//output[i_am_canadian] =  convert_float(myKey*100+kpkv); \n \
\
        int thisone = myKey*kpkv*3; \n \
        int prevone = (myKey-1) * kpkv *3; \n \
//output[i_am_canadian] = convert_float(thisone * 100 + prevone)+interval; \n \
    prevone = prevone + i_am_canadian*3; \n \
    thisone = thisone + i_am_canadian*3; \n \
        destVertices[i_am_canadian*3+0] = keyValues[prevone+0] + interval*(keyValues[thisone+0]-keyValues[prevone+0]); \n \
        destVertices[i_am_canadian*3+1] = keyValues[prevone+1] + interval*(keyValues[thisone+1]-keyValues[prevone+1]); \n \
        destVertices[i_am_canadian*3+2] = keyValues[prevone+2] + interval*(keyValues[thisone+2]-keyValues[prevone+2]); \n \
    } \n \
    //output[i_am_canadian] = destVertices[i_am_canadian*3]; \n \
\n \
}";



#endif //HAVE_OPENCL
