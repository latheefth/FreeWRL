/*
=INSERT_TEMPLATE_HERE=

$Id$

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

#include "Viewer.h"
#include "RenderFuncs.h"
#include "../vrml_parser/Structs.h"

#include "../main/headers.h"

#include "LinearAlgebra.h"
#include "Collision.h"

#ifdef DO_COLLISION_GPU

static const char* collide_non_walk_kernel;
static const char* collide_non_walk_kernel_headers;

#define FLOAT_TOLERANCE 0.00000001

/********************************************************************************/
/*										*/
/* 	Collide kernel, generic structures, etc					*/
/*										*/
/********************************************************************************/

static char *getErrorString(cl_int err) {
	switch (err) {
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
		case CL_COMPILE_PROGRAM_FAILURE: return "CL_COMPILE_PROGRAM_FAILURE"; break;
		case CL_LINKER_NOT_AVAILABLE: return "CL_LINKER_NOT_AVAILABLE"; break;
		case CL_LINK_PROGRAM_FAILURE: return "CL_LINK_PROGRAM_FAILURE"; break;
		case CL_DEVICE_PARTITION_FAILED: return "CL_DEVICE_PARTITION_FAILED"; break;
		case CL_KERNEL_ARG_INFO_NOT_AVAILABLE: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE"; break;
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
		case CL_INVALID_IMAGE_DESCRIPTOR: return "CL_INVALID_IMAGE_DESCRIPTOR"; break;
		case CL_INVALID_COMPILER_OPTIONS: return "CL_INVALID_COMPILER_OPTIONS"; break;
		case CL_INVALID_LINKER_OPTIONS: return "CL_INVALID_LINKER_OPTIONS"; break;
		case CL_INVALID_DEVICE_PARTITION_COUNT: return "CL_INVALID_DEVICE_PARTITION_COUNT"; break;
		default :{return "hmmm - error message makes no sense";}
	}

}
static void printCLError(const char *where, cl_int err) {
	ConsoleMessage ("OpenCL fn %s, error %s",getErrorString(err));
}

#define TEST_ERR(aa,bb) if (bb!=CL_SUCCESS) printCLError(aa,bb)


/********************************************************************************/
/*										*/
/*										*/
/********************************************************************************/
#ifdef _MSC_VER
cl_platform_id cpPlatform = NULL;          // OpenCL platform
cl_device_id* cdDevices = NULL;     // device list
cl_uint uiTargetDevice = 0;	        // Default Device to compute on



cl_int ciErrNum;			        // Error code var
enum LOGMODES 
{
    LOGCONSOLE = 1, // bit to signal "log to console" 
    LOGFILE    = 2, // bit to signal "log to file" 
    LOGBOTH    = 3, // convenience union of first 2 bits to signal "log to both"
    APPENDMODE = 4, // bit to set "file append" mode instead of "replace mode" on open
    MASTER     = 8, // bit to signal master .csv log output
    ERRORMSG   = 16, // bit to signal "pre-pend Error" 
    CLOSELOG   = 32  // bit to close log file, if open, after any requested file write
};
int bQATest = 0;			// false = normal GL loop, true = run No-GL test sequence
bool bGLinteropSupported = false;	// state var for GL interop supported or not
cl_uint uiNumDevsUsed = 1;          // Number of devices used in this sample 
bool bGLinterop = false;			// state var for GL interop or not
/*
void Cleanup(int iExitCode)
{
    // Cleanup allocated objects
    //shrLog("\nStarting Cleanup...\n\n");
    if(program)clReleaseProgram(program);
    if(context)clReleaseContext(context);
    if(cdDevices)free(cdDevices);

    // Cleanup GL objects if used
    if (!bQATest)
    {
        //DeInitGL();
    }

    // finalize logs and leave
    //shrLog("%s\n\n", iExitCode == 0 ? "PASSED" : "FAILED"); 
    if ((bQATest))
    {
       // shrLogEx(LOGBOTH | CLOSELOG, 0, "oclBoxFilter.exe Exiting...\n");
    }
    else 
    {
        //shrLogEx(LOGBOTH | CLOSELOG, 0, "oclBoxFilter.exe Exiting...\nPress <Enter> to Quit\n");
        #ifdef WIN32
            getchar();
        #endif
    }
    exit (iExitCode);
}
void (*pCleanup)(int) = &Cleanup;
*/
#define shrLog printf
cl_int oclGetPlatformID(cl_platform_id* clSelectedPlatformID)
{
    char chBuffer[1024];
    cl_uint num_platforms; 
    cl_platform_id* clPlatformIDs;
    cl_int ciErrNum;
    *clSelectedPlatformID = NULL;

    // Get OpenCL platform count
    ciErrNum = clGetPlatformIDs (0, NULL, &num_platforms);
    if (ciErrNum != CL_SUCCESS)
    {
        shrLog(" Error %i in clGetPlatformIDs Call !!!\n\n", ciErrNum);
        return -1000;
    }
    else 
    {
        if(num_platforms == 0)
        {
            shrLog("No OpenCL platform found!\n\n");
            return -2000;
        }
        else 
        {
			cl_uint i;
            // if there's a platform or more, make space for ID's
            if ((clPlatformIDs = (cl_platform_id*)malloc(num_platforms * sizeof(cl_platform_id))) == NULL)
            {
                shrLog("Failed to allocate memory for cl_platform ID's!\n\n");
                return -3000;
            }

            // get platform info for each platform and trap the NVIDIA platform if found
            ciErrNum = clGetPlatformIDs (num_platforms, clPlatformIDs, NULL);
            for(i = 0; i < num_platforms; ++i)
            {
                ciErrNum = clGetPlatformInfo (clPlatformIDs[i], CL_PLATFORM_NAME, 1024, &chBuffer, NULL);
                if(ciErrNum == CL_SUCCESS)
                {
                    if(strstr(chBuffer, "NVIDIA") != NULL)
                    {
                        *clSelectedPlatformID = clPlatformIDs[i];
                        break;
                    }
                }
            }

            // default to zeroeth platform if NVIDIA not found
            if(*clSelectedPlatformID == NULL)
            {
                shrLog("WARNING: NVIDIA OpenCL platform not found - defaulting to first platform!\n\n");
                *clSelectedPlatformID = clPlatformIDs[0];
            }

            free(clPlatformIDs);
        }
    }

    return CL_SUCCESS;
}
int extraInitFromNvidiaSamples(struct sCollisionGPU* initme)
{
    cl_uint uiNumDevices = 0;           // Number of devices available
    cl_uint uiTargetDevice = 0;	        // Default Device to compute on
    cl_uint uiNumComputeUnits;          // Number of compute units (SM's on NV GPU)

	// Get the NVIDIA platform
    ciErrNum = oclGetPlatformID(&cpPlatform);
    //oclCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);
    shrLog("clGetPlatformID...\n"); 

    //Get all the devices
    //shrLog("Get the Device info and select Device...\n");
    ciErrNum = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 0, NULL, &uiNumDevices);
    //oclCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);
    cdDevices = (cl_device_id *)malloc(uiNumDevices * sizeof(cl_device_id) );
    ciErrNum = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, uiNumDevices, cdDevices, NULL);
    //oclCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);

    // Set target device and Query number of compute units on uiTargetDevice
    shrLog("  # of Devices Available = %u\n", uiNumDevices); 
    //if(shrGetCmdLineArgumentu(argc, (const char**)argv, "device", &uiTargetDevice)== shrTRUE) 
    //{
    //    uiTargetDevice = CLAMP(uiTargetDevice, 0, (uiNumDevices - 1));
    //}
    shrLog("  Using Device %u: ", uiTargetDevice); 
    //oclPrintDevName(LOGBOTH, cdDevices[uiTargetDevice]);
	initme->device_id = cdDevices[uiTargetDevice];
    ciErrNum = clGetDeviceInfo(cdDevices[uiTargetDevice], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(uiNumComputeUnits), &uiNumComputeUnits, NULL);
    //oclCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);
    shrLog("\n  # of Compute Units = %u\n", uiNumComputeUnits); 

    // Check for GL interop capability (if using GL)
    if(!bQATest)
    {
        char extensions[1024];
        ciErrNum = clGetDeviceInfo(cdDevices[uiTargetDevice], CL_DEVICE_EXTENSIONS, 1024, extensions, 0);
        //oclCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);
        
        #if defined (__APPLE__) || defined(MACOSX)
            bGLinteropSupported = strstr(extensions,"cl_APPLE_gl_sharing") != NULL;
        #else
            bGLinteropSupported = strstr(extensions,"cl_khr_gl_sharing") != NULL;
        #endif
    }

    //Create the context
    if(bGLinteropSupported) 
    {
        // Define OS-specific context properties and create the OpenCL context
        #if defined (__APPLE__)
            CGLContextObj kCGLContext = CGLGetCurrentContext();
            CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
            cl_context_properties props[] = 
            {
                CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)kCGLShareGroup, 
                0 
            };
            cxGPUContext = clCreateContext(props, 0,0, NULL, NULL, &ciErrNum);
        #else
            #ifdef UNIX
                cl_context_properties props[] = 
                {
                    CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(), 
                    CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(), 
                    CL_CONTEXT_PLATFORM, (cl_context_properties)cpPlatform, 
                    0
                };
                cxGPUContext = clCreateContext(props, uiNumDevsUsed, &cdDevices[uiTargetDevice], NULL, NULL, &ciErrNum);
            #else // Win32
                cl_context_properties props[] = 
                {
                    CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(), 
                    CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(), 
                    CL_CONTEXT_PLATFORM, (cl_context_properties)cpPlatform, 
                    0
                };
                initme->context = clCreateContext(props, uiNumDevsUsed, &cdDevices[uiTargetDevice], NULL, NULL, &ciErrNum);
				printf("ciErrNum=%d\n",ciErrNum);
            #endif
        #endif
        shrLog("clCreateContext, GL Interop supported...\n"); 
    } 
    else 
    {
        bGLinterop = false;
        initme->context = clCreateContext(0, uiNumDevsUsed, &cdDevices[uiTargetDevice], NULL, NULL, &ciErrNum);
        shrLog("clCreateContext, GL Interop %s...\n", bQATest ? "N/A" : "not supported"); 
    }
   // oclCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);
	return ciErrNum;
}
#endif  //_MSC_VER


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



/********************************************************************************/
/*                                                                              */
/*                                                                              */
/********************************************************************************/

bool init_GPU_collide(struct sCollisionGPU* initme) {

	cl_int err;

	// debugging information
	cl_int rv;
	size_t rvlen;
	size_t wg_size;
	size_t kernel_wg_size;

	#ifdef GPU_DEBUG
	cl_ulong longish;
	size_t xyz;
	char rvstring[1000];
	int gpu;
	#endif // GPU_DEBUG



// get the current context.
// windows - IntPtr curDC = wglGetCurrentDC();
// then in the new compute context, we pass in the context

	/* initialized yet? */
	if (initme->kernel != NULL) return false;


	// get the device id

#if defined (TARGET_AQUA)
	CGLContextObj kCGLContext = CGLGetCurrentContext();
	CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
	cl_context_properties properties[] = {
		CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)kCGLShareGroup, 0 };

	err = clGetDeviceIDs(NULL, CL_DEVICE_TYPE_GPU, 1, &initme->device_id, NULL);

	if (err != CL_SUCCESS) {
		printCLError("clGetDeviceIDs",err);
		return FALSE;
	}

	initme->context =clCreateContext(properties,0,0,clLogMessagesToStderrAPPLE,0,&err);
#endif // TARGET_AQUA

#if defined (_MSC_VER)

	if(1)
		err = extraInitFromNvidiaSamples(initme);
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

			initme->context = clCreateContext(properties, 1, &initme->device_id, NULL, NULL, &err);
		}
	}
#endif // _MSC_VER

/* is this Linux? */
#if !(defined(TARGET_AQUA) || defined(_MSC_VER) || defined(_ANDROID)) 
	cl_platform_id platforms[10];
	cl_uint numPlats;

	err = clGetPlatformIDs(10,platforms,&numPlats);
	TEST_ERR("clGetPlatformIDs",err);

	ConsoleMessage ("looking for up to 10 platforms, got %d",numPlats);
	
/* not sure what platform to choose, if more than 1...
	{
		int i;

		for (i=0; i<numPlats; i++) {
			char platname[500];
                        cl_int err = clGetPlatformInfo(platforms[i],CL_PLATFORM_NAME,sizeof(platname),platname,NULL);
			TEST_ERR("clGetPlatformInfo",err);
			ConsoleMessage ("GetPlatfromInfo for %d is :%s:",i,platname);
		}

	}
	*/


	err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 1, &initme->device_id, NULL);

	if (err != CL_SUCCESS) {
		printCLError("clGetDeviceIDs",err);
		return FALSE;
	} else {
		printf ("Linux, have device id...\n");
	}


	cl_context_properties properties[] = {
		CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
		CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
		CL_CONTEXT_PLATFORM, (cl_context_properties)platforms[0],
		0 };

	initme->context=clCreateContextFromType(properties, CL_DEVICE_TYPE_GPU, NULL, NULL, &err);
	TEST_ERR("clCreateContextFromType",err);
#endif

/* how about Android (and maybe IPHONE) using OpenGL-ES 2.0? */
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
	ConsoleMessage ("looking for up to 10 platforms, got %d",numPlats);
	
	cl_platform_id platform;
	err = clGetPlatformIDs(1,&platform,NULL);
	TEST_ERR("clGetPlatformIDs",err);

	err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &initme->device_id, NULL);

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
	initme->context=clCreateContextFromType(properties, CL_DEVICE_TYPE_GPU, NULL, NULL, &err);
	TEST_ERR("clCreateContextFromType",err);
*/

	initme->context=clCreateContextFromType(NULL, CL_DEVICE_TYPE_GPU, NULL, NULL, &err);
	TEST_ERR("clCreateContextFromType",err);

	ConsoleMessage ("remember - building currently without the CL_KHR_gl_sharing enabled - the clCreateFromGLBuffer will error out, so return code removed.");

#endif //GL_ES_VERSION_2_0


	// create a command queue

	initme->queue = clCreateCommandQueue(initme->context, initme->device_id, 0, &err);
	if (!initme->queue || (err != CL_SUCCESS)) {
		printCLError("clCreateCommandQueue",err);
		return FALSE;
	}
 
	{
	char *kp[2];

	kp[0] = (char *)collide_non_walk_kernel_headers;
	kp[1] = (char *)collide_non_walk_kernel;

	// Find the work group size
	rv = clGetDeviceInfo (initme->device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &wg_size, &rvlen);
	TEST_ERR("clGetDeviceInfo",rv);

	#ifdef GPU_DEBUG
	// debugging information
	rv = clGetPlatformInfo(platforms[0],CL_PLATFORM_PROFILE,1000,rvstring,&rvlen);
	ConsoleMessage ("CL_PLATFORM_PROFILE :%s:\n",rvstring);
	rv = clGetPlatformInfo(platforms[0],CL_PLATFORM_VERSION,1000,rvstring,&rvlen);
	ConsoleMessage ("CL_PLATFORM_VERSION :%s:\n",rvstring);
	rv = clGetPlatformInfo(platforms[0],CL_PLATFORM_NAME,1000,rvstring,&rvlen);
	ConsoleMessage ("CL_PLATFORM_NAME :%s:\n",rvstring);
	rv = clGetPlatformInfo(platforms[0],CL_PLATFORM_VENDOR,1000,rvstring,&rvlen);
	ConsoleMessage ("CL_PLATFORM_VENDOR :%s:\n",rvstring);
	rv = clGetPlatformInfo(platforms[0],CL_PLATFORM_EXTENSIONS,1000,rvstring,&rvlen);
	ConsoleMessage ("CL_PLATFORM_EXTENSIONS :%s:\n",rvstring);
	ConsoleMessage ("CL_DEVICE_MAX_WORK_GROUP_SIZE %d\n",wg_size);
	rv = clGetDeviceInfo (initme->device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(size_t), &xyz, &rvlen);
	ConsoleMessage ("CL_DEVICE_MAX_COMPUTE_UNITS %d\n",xyz);

	rv = clGetDeviceInfo (initme->device_id, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, sizeof(cl_ulong), &longish, &rvlen);
	ConsoleMessage ("CL_DEVICE_GLOBAL_MEM_CACHE_SIZE %ld\n",longish);
	rv = clGetDeviceInfo (initme->device_id, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &longish, &rvlen);
	ConsoleMessage ("CL_DEVICE_GLOBAL_MEM_SIZE %ld\n",longish);
	rv = clGetDeviceInfo (initme->device_id, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &longish, &rvlen);
	ConsoleMessage ("CL_DEVICE_LOCAL_MEM_SIZE %ld\n",longish);


	#endif //GPU_DEBUG


	initme->program = clCreateProgramWithSource(initme->context, 2, (const char **) kp, NULL, &err);
	if (!initme->program || (err != CL_SUCCESS)) {
		printCLError("clCreateProgramWithSource",err);
		return FALSE;
	}
	}


 
	// build the compute program executable
	//char *opts = "-Werror -cl-single-precision-constant -cl-nv-verbose  -g -cl-opt-disable -cl-strict-aliasing";
	//char *opts = "-Werror -cl-single-precision-constant -cl-opt-disable -cl-strict-aliasing";
	//err = clBuildProgram(initme->program, 0, NULL, opts, NULL, NULL);
	//ConsoleMessage ("calling clBuildProgram with program %p\n",initme->program);

	// build the program, hard code in devices to 1 device, with the device list, no options
	char *opts = NULL;
	err = clBuildProgram(initme->program, 1, &(initme->device_id), opts, NULL, NULL);
	ConsoleMessage ("called clBuildProgram error %d\n",err);
	if (err != CL_SUCCESS) {
        	size_t len;
        	char buffer[16384];
 
        	ConsoleMessage("Error: Failed to build program executable\n");           
		printCLError("clBuildProgram",err);
        	err = clGetProgramBuildInfo(initme->program, initme->device_id, CL_PROGRAM_BUILD_LOG,
                                          sizeof(buffer), buffer, &len);
		TEST_ERR("clGetProgramBuildInfo",err);
		ConsoleMessage ("error string len %d\n",(int)len);
        	ConsoleMessage("%s\n", buffer);
        	return FALSE;
    	}
 
	// create the compute kernel
	initme->kernel = clCreateKernel(initme->program, "compute_collide", &err);
	if (!initme->kernel || (err != CL_SUCCESS)) {
		printCLError("clCreateKernel",err);
	}


	// Kernel Workgroup size
	err = clGetKernelWorkGroupInfo (initme->kernel, initme->device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &kernel_wg_size, &rvlen);
	TEST_ERR("clGetKernelWorkGroupInfo",err);

	// try the smaller of the two
	if (kernel_wg_size < wg_size) wg_size = kernel_wg_size;
	initme->workgroup_size = wg_size;

	#ifdef GPU_DEBUG
	ConsoleMessage ("MAX_WORK_GROUP_SIZE %d\n",kernel_wg_size);
	ConsoleMessage ("We are going to set our workgroup size to %d\n",wg_size);


/*
1. Get workGroupSize from clGetDeviceInfo with CL_DEVICE_mum of two values and use that value as your optimal workGroupSize
2. Get KernelWorkGroupSize from from clGetKernelWorkGroupInfo with CL_KERNEL_WORK_GPOUP_SIZE
3. Get minimum of two values and use that value as your optimal workGroupSize
*/

	#endif // GPU_DEBUG

	return TRUE;
}

/********************************************************************************/
/*										*/
/*										*/
/********************************************************************************/

struct point_XYZ run_non_walk_collide_program(GLuint vertex_vbo, GLuint index_vbo, float *modelMat,int ntri,
		int face_ccw, int face_flags, float avatar_radius) {
 
	int i;
	cl_int err;
	size_t local_work_size;
	size_t global_work_size;
	unsigned int count;

	double maxdisp = 0.0;
	struct point_XYZ dispv, maxdispv = {0,0,0};

	struct sCollisionGPU* me = GPUCollisionInfo();

	// enough space for rv?
	if (me->collide_rvs.n < ntri) {
		if (me->collide_rvs.n != 0) {
			err = clReleaseMemObject(me->output_buffer);	
			TEST_ERR("clReleaseMemObject",err);
		}

		me->output_buffer = clCreateBuffer(me->context, CL_MEM_WRITE_ONLY, sizeof(struct SFColorRGBA) * ntri,
                                                                  NULL, NULL);

		if (me->matrix_buffer == NULL) {
		me->matrix_buffer = clCreateBuffer(me->context, CL_MEM_READ_ONLY, sizeof (cl_float16), NULL, NULL);
		}

		if (!(me->output_buffer) || !(me->matrix_buffer)) {
			printCLError("clCreateBuffer",10000);
		}

		me->output_size = ntri;
		me->collide_rvs.p = REALLOC(me->collide_rvs.p, sizeof(struct SFColorRGBA) *ntri);
		me->collide_rvs.n = ntri;
	}

	// update the current matrix transform
        err = clEnqueueWriteBuffer(me->queue, me->matrix_buffer, CL_TRUE, 0, sizeof(cl_float16), modelMat, 0, NULL, NULL);
	TEST_ERR("clEnqueueWriteBuffer",err);

	// lets get the openGL vertex buffer here
	me->vertex_buffer=clCreateFromGLBuffer(me->context, CL_MEM_READ_ONLY, vertex_vbo, &err);
	if (err != CL_SUCCESS) {
		printCLError("clCreateFromGLBuffer",err);
		return maxdispv;
	}

	// and the coordinate index buffer
	me->index_buffer = clCreateFromGLBuffer(me->context, CL_MEM_READ_ONLY, index_vbo, &err);
	if (err != CL_SUCCESS) {
		printCLError("clCreateFromGLBuffer",err);
		return maxdispv;
	}
	
	// set the args values
	count = (unsigned int) ntri;

	err = clSetKernelArg(me->kernel, 0, sizeof(cl_mem), &me->output_buffer);
	TEST_ERR("clSetKernelArg",err);

	err =clSetKernelArg(me->kernel, 1, sizeof(unsigned int), &count);
	TEST_ERR("clSetKernelArg",err);

	err =clSetKernelArg(me->kernel, 2, sizeof (cl_mem), &me->matrix_buffer);
	TEST_ERR("clSetKernelArg",err);

	err =clSetKernelArg(me->kernel, 3, sizeof (cl_mem), &me->vertex_buffer);
	TEST_ERR("clSetKernelArg",err);

	err =clSetKernelArg(me->kernel, 4, sizeof (cl_mem), &me->index_buffer);
	TEST_ERR("clSetKernelArg",err);

	err =clSetKernelArg(me->kernel, 5, sizeof(int), &face_ccw);
	TEST_ERR("clSetKernelArg",err);

	err =clSetKernelArg(me->kernel, 6, sizeof(int), &face_flags);
	TEST_ERR("clSetKernelArg",err);

	err =clSetKernelArg(me->kernel, 7, sizeof(int), &avatar_radius);
	TEST_ERR("clSetKernelArg",err);

	err =clSetKernelArg(me->kernel, 8, sizeof(int), &ntri);
	TEST_ERR("clSetKernelArg",err);
	
	// global work group size
	#define MYWG (me->workgroup_size)
	// find out how many "blocks" we can have
	if (MYWG > 0)
		global_work_size = (size_t) (ntri) / MYWG;
	else global_work_size = 0;

	// add 1 to it, because we have to round up
	global_work_size += 1;

	// now, global_work_size will be an exact multiple of local_work_size
	global_work_size *= MYWG;

	//ConsoleMessage ("global_work_size is %d %x right now...\n",global_work_size, global_work_size);

	local_work_size = MYWG;
	//ConsoleMessage ("local_work_size %d\n",local_work_size);
	//ConsoleMessage ("ntri %d, global_work_size %d, local_work_size %d\n",ntri,global_work_size,local_work_size);

  	err = clEnqueueNDRangeKernel(me->queue, me->kernel, 1, NULL, &global_work_size, &local_work_size, 0, NULL, NULL);
	if (err != CL_SUCCESS) {
		printCLError("clEnqueueNDRangeKernel",err);
		return maxdispv;
	}
	

#ifdef TRY_FLUSH
	// wait for things to finish
	err = clFlush(me->queue);
	if (err != CL_SUCCESS) {
		printCLError("clFlush",err);
		return maxdispv;
	} 

	err = clFinish(me->queue);
	if (err != CL_SUCCESS) {
		printCLError("clFinish",err);
		return maxdispv;
	} 
#endif

	// get the data
	err = clEnqueueReadBuffer (me->queue, me->output_buffer, 
		CL_TRUE, 0, sizeof(struct SFColorRGBA) * ntri, 
		me->collide_rvs.p, 0, NULL, NULL);

	if (err != CL_SUCCESS) {
		printCLError("clEnqueueReadBuffer",err);
		return maxdispv;
	}


	for (i=0; i < ntri; i++) {
		/* XXX float to double conversion; make a vecdotf for speed */
		double disp;

		// we use the last float to indicate whether to bother here; saves us
		// doing unneeded calculations here

		if (me->collide_rvs.p[i].c[3] > 1.0) {
			 //ConsoleMessage ("possibly triangle %d has some stuff for us\n",i);


			dispv.x = me->collide_rvs.p[i].c[0];
			dispv.y = me->collide_rvs.p[i].c[1];
			dispv.z = me->collide_rvs.p[i].c[2];
			//ConsoleMessage ("GPU tri %d, disp %f %f %f\n",i,dispv.x,dispv.y,dispv.z);

                        /*keep result only if:
                          displacement is positive
                          displacement is smaller than minimum displacement up to date
                         */

			disp = vecdot (&dispv,&dispv);
			if ((disp > FLOAT_TOLERANCE) && (disp>maxdisp)) {
				maxdisp = disp;
				maxdispv = dispv;
			}
		}

	}

	
	//ConsoleMessage ("OpenCL - at end of opencl, maxdispv %f %f %f\n",maxdispv.x, maxdispv.y, maxdispv.z); 

	return maxdispv;
}


#ifdef GL_ES_VERSION_2_0
static const char* collide_non_walk_kernel_headers = " \
//#pragma OPENCL EXTENSION cl_khr_fp64 : enable \n\
#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable \n\
//#pragma OPENCL EXTENSION CL_APPLE_gl_sharing : enable \n\
//#pragma OPENCL EXTENSION CL_KHR_gl_sharing : enable \n\
//#pragma OPENCL EXTENSION cl_khr_select_fprounding_mode : enable \n\
";
#else
static const char* collide_non_walk_kernel_headers = " \
//#pragma OPENCL EXTENSION cl_khr_fp64 : enable \n\
#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable \n\
#pragma OPENCL EXTENSION CL_APPLE_gl_sharing : enable \n\
#pragma OPENCL EXTENSION CL_KHR_gl_sharing : enable \n\
#pragma OPENCL EXTENSION cl_khr_select_fprounding_mode : enable \n\
";
#endif

static const char* collide_non_walk_kernel = " \
 \n\
/********************************************************************************/ \n\
/*										*/ \n\
/*	Collide kernel for fly and examine modes				*/ \n\
/*										*/ \n\
/********************************************************************************/ \n\
/* Function prototypes */ \n\
float4 closest_point_on_plane(float4 point_a, float4 point_b, float4 point_c); \n\
 \n\
/* start the collide process. \n\
 \n\
1) transform the vertex. \n\
2) calculate normal \n\
3) if triangle is visible to us, get ready for collide calcs \n\
 \n\
*/ \n\
 \n\
 \n\
#define DOUGS_FLOAT_TOLERANCE 0.00000001 \n\
#define FLOAT_TOLERANCE 0.0000001 \n\
#define PR_DOUBLESIDED 0x01  \n\
#define PR_FRONTFACING 0x02 /* overrides effect of doublesided. */  \n\
#define PR_BACKFACING 0x04 /* overrides effect of doublesided, all normals are reversed. */  \n\
 \n\
/********************************************************************************/ \n\
 \n\
 \n\
#define APPROX (a, b) (fabs(a-b) < FLOAT_TOLERANCE) \n\
#define VECSCALE(v,s) (float4)(v.x*s, v.y*s, v.z*s, 0.0) \n\
#define VECLENGTH(v) (float)sqrt((float)dot((float4)v,(float4)v)) \n\
 \n\
 \n\
 \n\
/********************************************************************************/ \n\
/*										*/ \n\
/*	Three vertices; find the closest one which intersects the Z plane;	*/ \n\
/*	either we choose a Vertex, on an edge, or fabricate one in the		*/ \n\
/*	middle of the triangle somewhere.					*/ \n\
/*										*/ \n\
/*	Adapted from \"Real time Collision Detection\", Christer Ericson.		*/ \n\
/*										*/ \n\
/********************************************************************************/ \n\
 \n\
 \n\
float4 closest_point_on_plane(float4 point_a, float4 point_b, float4 point_c) { \n\
	float4 vector_ab = (point_b - point_a); // b - a \n\
	float4 vector_ac = (point_c - point_a); // c - a \n\
	float4 vector_bc = (point_c - point_b); // c - b \n\
	float4 vector_ba = (point_a - point_b); // a - b \n\
	float4 vector_ca = (point_a - point_c); // a - c \n\
	float4 vector_cb = (point_b - point_c); // b - c \n\
 \n\
 \n\
	// we have moved points, so our bounding sphere is at (0,0,0) so p = (0,0,0) \n\
	float4 vector_ap = point_a * (float4)(-1.0f, -1.0f, -1.0f, -1.0f); // p - a \n\
	float4 vector_bp = point_b * (float4)(-1.0f, -1.0f, -1.0f, -1.0f); // p - b \n\
	float4 vector_cp = point_c * (float4)(-1.0f, -1.0f, -1.0f, -1.0f); // p - c \n\
	#define vector_pa point_a    /* a - p */ \n\
	#define vector_pb point_b    /* b - p */ \n\
	#define vector_pc point_c    /* c - p */ \n\
	 \n\
	// Step 2. Compute parametric position s for projection P' of P on AB, \n\
	// P' = A + s*AB, s = snom/(snom+sdenom) \n\
 \n\
	float snom = dot(vector_ap, vector_ab); // (p - a, ab); \n\
	float sdenom = dot(vector_bp, vector_ba); // (p - b, a - b); \n\
 \n\
	// Step 3. \n\
	// Compute parametric position t for projection P' of P on AC, \n\
	// P' = A + t*AC, s = tnom/(tnom+tdenom) \n\
	float tnom = dot(vector_ap, vector_ac); // (p - a, ac); \n\
	float tdenom = dot(vector_cp, vector_ca); //  (p - c, a - c); \n\
 \n\
	// Step 4. \n\
	if (snom <= 0.0f && tnom <= 0.0f) { \n\
		return point_a; \n\
	} \n\
 \n\
	// Step 5. \n\
	// Compute parametric position u for projection P' of P on BC, \n\
	// P' = B + u*BC, u = unom/(unom+udenom) \n\
	float unom = dot(vector_bp, vector_bc); //(p - b, bc) \n\
	float udenom = dot(vector_cp, vector_cb); // (p - c, b - c); \n\
 \n\
	// Step 6. \n\
	if (sdenom <= 0.0f && unom <= 0.0f) { \n\
		return point_b; \n\
	} \n\
 \n\
	if (tdenom <= 0.0f && udenom <= 0.0f) { \n\
		return point_c; \n\
	} \n\
 \n\
 \n\
	// Step 7. \n\
	// P is outside (or on) AB if the triple scalar product [N PA PB] <= 0 \n\
	float4 n; \n\
	float4 tmp; \n\
	float vc; \n\
 \n\
	n = cross(vector_ab, vector_ac); // (b - a, c - a); \n\
	tmp = cross(vector_pa, vector_pb); // veccross (a-p, b-p); \n\
 \n\
	// vc = dot(n, veccross(a - p, b - p)); \n\
	vc = dot(n, tmp); \n\
 \n\
 \n\
	// If P outside AB and within feature region of AB, \n\
	// return projection of P onto AB \n\
	if (vc <= 0.0f && snom >= 0.0f && sdenom >= 0.0f) { \n\
		return point_a  + snom / (snom + sdenom) * vector_ab; \n\
	} \n\
 \n\
 \n\
 \n\
	// Step 8. \n\
	// P is outside (or on) BC if the triple scalar product [N PB PC] <= 0 \n\
	tmp = cross (vector_pb, vector_pc); \n\
 \n\
	float va = dot(n, tmp); // Cross(b - p, c - p)); \n\
	 \n\
	// If P outside BC and within feature region of BC, \n\
	// return projection of P onto BC \n\
	if (va <= 0.0f && unom >= 0.0f && udenom >= 0.0f) { \n\
		return point_b + unom / (unom + udenom) * vector_bc; \n\
	} \n\
 \n\
	// Step 9. \n\
	// P is outside (or on) CA if the triple scalar product [N PC PA] <= 0 \n\
	tmp = cross (vector_pc, vector_pa); \n\
 \n\
	float vb = dot(n, tmp); //  Cross(c - p, a - p)); \n\
	// If P outside CA and within feature region of CA, \n\
	// return projection of P onto CA \n\
	if (vb <= 0.0f && tnom >= 0.0f && tdenom >= 0.0f) { \n\
		return point_a + tnom / (tnom + tdenom) * vector_ac; \n\
	} \n\
 \n\
	// 10. \n\
	// P must project inside face region. Compute Q using barycentric coordinates \n\
	float u = va / (va + vb + vc); \n\
	float v = vb / (va + vb + vc); \n\
	float w = 1.0f - u - v; // = vc / (va + vb + vc) \n\
	float4 u4 = (float4)(u); \n\
	float4 v4 = (float4)(v); \n\
	float4 w4 = (float4)(w); \n\
 \n\
	//return u * point_a + v * point_b + w * point_c; \n\
	float4 rv = mad(point_a,u4,mad(point_b,v4,point_c*w4)); \n\
	return rv; \n\
} \n\
 \n\
/********************************************************************************/ \n\
 \n\
	__kernel void compute_collide (  \n\
	__global float4 *output,  	/* 0 */  \n\
        const unsigned int count,	/* 1 */  \n\
	__global float *mymat,   	/* 2 */  \n\
	__global float *my_vertex,	/* 3 */  \n\
	__global int *my_cindex, 	/* 4 */  \n\
	const int face_ccw,		/* 5 */ \n\
	const int face_flags,		/* 6 */  \n\
	const float avatar_radius,	/* 7 */ \n\
	const int ntri			/* 8 */ \n\
	) {   \n\
  \n\
	/* which index this instantation is working on */ \n\
	int i_am_canadian = get_global_id(0);  \n\
	if (i_am_canadian >= ntri) return; /* allows for workgroup size sizes */ \n\
 \n\
	/* vertices for this triangle */  \n\
	/* transformed by matrix */  \n\
	float4 tv1;  \n\
	float4 tv2;  \n\
	float4 tv3;  \n\
 \n\
	/* starting index in my_vertex of this vertex */  \n\
	/* we work in triangles; each triangle has 3 vertices */  \n\
	#define COORD_1 (my_cindex[i_am_canadian*3+0]*3) \n\
	#define COORD_2 (my_cindex[i_am_canadian*3+1]*3) \n\
	#define COORD_3 (my_cindex[i_am_canadian*3+2]*3) \n\
 \n\
	/* do matrix transform, 4 floats wide. */ \n\
	float4 matColumn1 = (float4)(mymat[0],mymat[1],mymat[2],0.0); \n\
	float4 matColumn2 = (float4)(mymat[4],mymat[5],mymat[6],0.0); \n\
	float4 matColumn3 = (float4)(mymat[8],mymat[9],mymat[10],0.0); \n\
	float4 matColumn4 = (float4)(mymat[12],mymat[13],mymat[14],0.0); \n\
 \n\
	/* first vertex */ \n\
	float4 Vertex_X = (float4)(my_vertex[COORD_1+0]); \n\
	float4 Vertex_Y = (float4)(my_vertex[COORD_1+1]); \n\
	float4 Vertex_Z = (float4)(my_vertex[COORD_1+2]); \n\
	tv1 = mad(matColumn1,Vertex_X,mad(matColumn2,Vertex_Y,mad(matColumn3,Vertex_Z,matColumn4))); \n\
 \n\
	/* second vertex */ \n\
	Vertex_X = (float4)(my_vertex[COORD_2+0]); \n\
	Vertex_Y = (float4)(my_vertex[COORD_2+1]); \n\
	Vertex_Z = (float4)(my_vertex[COORD_2+2]); \n\
	tv2 = mad(matColumn1,Vertex_X,mad(matColumn2,Vertex_Y,mad(matColumn3,Vertex_Z,matColumn4))); \n\
 \n\
	/* third vertex */ \n\
	Vertex_X = (float4)(my_vertex[COORD_3+0]); \n\
	Vertex_Y = (float4)(my_vertex[COORD_3+1]); \n\
	Vertex_Z = (float4)(my_vertex[COORD_3+2]); \n\
	tv3 = mad(matColumn1,Vertex_X,mad(matColumn2,Vertex_Y,mad(matColumn3,Vertex_Z,matColumn4))); \n\
 \n\
 \n\
	/* calculate normal for face from transformed vertices */  \n\
	/* this replicates polynormalf for opencl */ \n\
	#define VEC_DIST_1 (tv2-tv1) \n\
	#define VEC_DIST_2 (tv3-tv1) \n\
	float4 norm = normalize(cross(VEC_DIST_1,VEC_DIST_2));  \n\
 \n\
	/* from polyrep_disp_rec2, see that function for full comments */  \n\
	bool frontfacing;  \n\
 \n\
	/* how we view it from the avatar */  \n\
	if (face_ccw) frontfacing = (dot(norm,tv1) < 0);   \n\
	else frontfacing = (dot(norm,tv1) >= 0);  \n\
 \n\
	/* now, is solid false, or ccw or ccw winded triangle? */  \n\
	/* if we should do this triangle, the if statement is true */  \n\
 \n\
	bool should_do_this_triangle =  \n\
	((frontfacing && !(face_flags & PR_DOUBLESIDED) )  \n\
		|| ( (face_flags & PR_DOUBLESIDED)  && !(face_flags & (PR_FRONTFACING | PR_BACKFACING) )  )  \n\
		|| (frontfacing && (face_flags & PR_FRONTFACING))  \n\
		|| (!frontfacing && (face_flags & PR_BACKFACING))  ); \n\
 \n\
 \n\
	if (!should_do_this_triangle) { \n\
		output[i_am_canadian] = (float4)(0.0,0.0,0.0,0.0); \n\
		return; \n\
	} \n\
 \n\
 \n\
	/* if we are down to here, we have to do this triangle */ \n\
 \n\
	if(!frontfacing) { /*can only be here in DoubleSided mode*/  \n\
		/*reverse polygon orientation, and do calculations*/  \n\
		norm = VECSCALE(norm,-1.0); \n\
	} \n\
 \n\
	/********************************************************************************/ \n\
	/*										*/ \n\
	/* Collide Kernel Step 2: do hit calculations					*/ \n\
	/* replicate Dougs get_poly_min_disp_with_sphere function 			*/  \n\
	/*										*/ \n\
	/********************************************************************************/ \n\
 \n\
	float4 closest_point = closest_point_on_plane(tv1,tv2,tv3); \n\
 \n\
	float get_poly_mindisp = dot(closest_point,closest_point); \n\
	 \n\
	if (get_poly_mindisp > (avatar_radius * avatar_radius)) { \n\
		output[i_am_canadian] = (float4)(0.0,0.0,0.0,0.0); \n\
		return; \n\
	} \n\
 \n\
	/* do we have a movement here? */ \n\
	if (VECLENGTH(closest_point) > FLOAT_TOLERANCE) { \n\
		float poly_min_rt = sqrt(get_poly_mindisp); \n\
		float sFactor = (avatar_radius -poly_min_rt) /VECLENGTH(closest_point); \n\
 \n\
		float4 result = VECSCALE(closest_point,sFactor); \n\
		/* copy over the result */ \n\
		result.w = 100.0; /* flag that this is a good one */ \n\
		output[i_am_canadian] = result; \n\
		return; \n\
	} \n\
 \n\
 \n\
	/* if we are down to here, we can just return zero */ \n\
	output[i_am_canadian] = (float4)(0.0,0.0,0.0,0.0); \n\
} \n\
";

#endif //DO_COLLISION_GPU
