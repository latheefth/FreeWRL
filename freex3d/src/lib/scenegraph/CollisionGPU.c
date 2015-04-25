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

#include "Viewer.h"
#include "RenderFuncs.h"
#include "../vrml_parser/Structs.h"

#include "../main/headers.h"

#include "LinearAlgebra.h"
#include "Collision.h"
#if !defined(_ANDROID) && !defined(IPHONE)
#include "../opencl/OpenCL_Utils.h"
#endif
#ifdef HAVE_OPENCL

static const char* collide_non_walk_kernel;
static const char* collide_non_walk_kernel_headers;

#define FLOAT_TOLERANCE 0.00000001

/********************************************************************************/
/*										*/
/* 	Collide kernel, generic structures, etc					*/
/*										*/
/********************************************************************************/

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
#endif  //_MSC_VER




bool collision_initGPUCollide (struct sCollisionGPU* initme) {
        ppOpenCL_Utils p;
        ttglobal tg = gglobal();
        p = (ppOpenCL_Utils)tg->OpenCL_Utils.prv;

	char *kp[2];
	cl_int err;
	size_t kernel_wg_size;
	size_t rvlen;

    //printf ("called initGPUCollide, p is %p\n",p);
    //printf ("called initGPUCollide, context is %p\n",p->CL_context);
    //printf ("called initGPUCollide, initme is %p\n",initme);

	kp[0] = (char *)collide_non_walk_kernel_headers;
	kp[1] = (char *)collide_non_walk_kernel;

	initme->CollideGPU_program = clCreateProgramWithSource(p->CL_context, 2, (const char **) kp, NULL, &err);
    //printf ("past the clCreateProgramWithSource call\n");

	if (!initme->CollideGPU_program || (err != CL_SUCCESS)) {
		printCLError("clCreateProgramWithSource",err);
		return FALSE;
	}
 
	// build the compute program executable
	//char *opts = "-Werror -cl-single-precision-constant -cl-nv-verbose  -g -cl-opt-disable -cl-strict-aliasing";
	//char *opts = "-Werror -cl-single-precision-constant -cl-opt-disable -cl-strict-aliasing";
	//err = clBuildProgram(p->program, 0, NULL, opts, NULL, NULL);
	//ConsoleMessage ("calling clBuildProgram with program %p\n",p->program);

	// build the program, hard code in devices to 1 device, with the device list, no options
	char *opts = NULL;
	err = clBuildProgram(initme->CollideGPU_program, 1, &(p->CL_device_id), opts, NULL, NULL);
    //printf ("past the clBuildProgram call\n");

	//ConsoleMessage ("called clBuildProgram error %d\n",err);
	if (err != CL_SUCCESS) {
        	size_t len;
        	char buffer[16384];
 
        	ConsoleMessage("Error: Failed to build program executable\n");           
		printCLError("clBuildProgram",err);
        	err = clGetProgramBuildInfo(initme->CollideGPU_program, p->CL_device_id, CL_PROGRAM_BUILD_LOG,
                                          sizeof(buffer), buffer, &len);
		TEST_ERR("clGetProgramBuildInfo",err);
		ConsoleMessage ("error string len %d\n",(int)len);
        	ConsoleMessage("%s\n", buffer);
        	return FALSE;
    	}
 
	// create the compute kernel
	initme->CollideGPU_kernel = clCreateKernel(initme->CollideGPU_program, "compute_collide", &err);
    // printf ("kernel is %p\n",initme->CollideGPU_kernel);

	if (!initme->CollideGPU_kernel || (err != CL_SUCCESS)) {
		printCLError("clCreateKernel",err);
        	return FALSE;
	}


	// Kernel Workgroup size
	// rv = clGetDeviceInfo (p->device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &wg_size, &rvlen);
	err = clGetKernelWorkGroupInfo (initme->CollideGPU_kernel, p->CL_device_id, 
		CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &kernel_wg_size, &rvlen);
	 
	if (err!=CL_SUCCESS) {
		printCLError( "clGetKernelWorkGroupInfo",err);
		return FALSE;
	}

	// try the smaller of the two
	if (kernel_wg_size < p->CL_default_workgroup_size) initme->CollideGPU_workgroup_size = kernel_wg_size;
	else initme->CollideGPU_workgroup_size = p->CL_default_workgroup_size;

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

int printedOnce = TRUE;


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

       	ppOpenCL_Utils p;
       	ttglobal tg = gglobal();
       	p = (ppOpenCL_Utils)tg->OpenCL_Utils.prv;

	// enough space for rv?
	if (me->CollideGPU_returnValues.n < ntri) {
		if (me->CollideGPU_returnValues.n != 0) {
			err = clReleaseMemObject(me->CollideGPU_output_buffer);	
			TEST_ERR("clReleaseMemObject",err);
		}

		me->CollideGPU_output_buffer = clCreateBuffer(p->CL_context, CL_MEM_WRITE_ONLY, sizeof(struct SFColorRGBA) * ntri,
                                                                  NULL, NULL);

		if (me->CollideGPU_matrix_buffer == NULL) {
		me->CollideGPU_matrix_buffer = clCreateBuffer(p->CL_context, CL_MEM_READ_ONLY, sizeof (cl_float16), NULL, NULL);
		}

		if (!(me->CollideGPU_output_buffer) || !(me->CollideGPU_matrix_buffer)) {
			printCLError("clCreateBuffer",10000);
		}

		me->CollideGPU_output_size = ntri;
		me->CollideGPU_returnValues.p = REALLOC(me->CollideGPU_returnValues.p, sizeof(struct SFColorRGBA) *ntri);
		me->CollideGPU_returnValues.n = ntri;
	}

	// update the current matrix transform
        err = clEnqueueWriteBuffer(p->CL_queue, me->CollideGPU_matrix_buffer, CL_TRUE, 0, sizeof(cl_float16), modelMat, 0, NULL, NULL);
	TEST_ERR("clEnqueueWriteBuffer",err);

	// lets get the openGL vertex buffer here
	me->CollideGPU_vertex_buffer=clCreateFromGLBuffer(p->CL_context, CL_MEM_READ_ONLY, vertex_vbo, &err);
	if (err != CL_SUCCESS) {
		printCLError("clCreateFromGLBuffer",err);
		return maxdispv;
	}

	// and the coordinate index buffer
	me->CollideGPU_index_buffer = clCreateFromGLBuffer(p->CL_context, CL_MEM_READ_ONLY, index_vbo, &err);
	if (err != CL_SUCCESS) {
		printCLError("clCreateFromGLBuffer",err);
		return maxdispv;
	}
	
	// set the args values
	count = (unsigned int) ntri;

	err = clSetKernelArg(me->CollideGPU_kernel, 0, sizeof(cl_mem), &me->CollideGPU_output_buffer);
	TEST_ERR("clSetKernelArg",err);

	err =clSetKernelArg(me->CollideGPU_kernel, 1, sizeof(unsigned int), &count);
	TEST_ERR("clSetKernelArg",err);

	err =clSetKernelArg(me->CollideGPU_kernel, 2, sizeof (cl_mem), &me->CollideGPU_matrix_buffer);
	TEST_ERR("clSetKernelArg",err);

	err =clSetKernelArg(me->CollideGPU_kernel, 3, sizeof (cl_mem), &me->CollideGPU_vertex_buffer);
	TEST_ERR("clSetKernelArg",err);

	err =clSetKernelArg(me->CollideGPU_kernel, 4, sizeof (cl_mem), &me->CollideGPU_index_buffer);
	TEST_ERR("clSetKernelArg",err);

	err =clSetKernelArg(me->CollideGPU_kernel, 5, sizeof(int), &face_ccw);
	TEST_ERR("clSetKernelArg",err);

	err =clSetKernelArg(me->CollideGPU_kernel, 6, sizeof(int), &face_flags);
	TEST_ERR("clSetKernelArg",err);

	err =clSetKernelArg(me->CollideGPU_kernel, 7, sizeof(int), &avatar_radius);
	TEST_ERR("clSetKernelArg",err);

	err =clSetKernelArg(me->CollideGPU_kernel, 8, sizeof(int), &ntri);
	TEST_ERR("clSetKernelArg",err);
	
	// global work group size
	#define MYWG (me->CollideGPU_workgroup_size)
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

if (!printedOnce) {
	cl_context myContext;
	cl_device_id myDevice;
	cl_uint myReference;


	err = clGetCommandQueueInfo(p->CL_queue, CL_QUEUE_CONTEXT, sizeof(myContext), &myContext, NULL);
printf ("queue context when commandqueue created %p, should be %p\n", myContext,p->CL_context);
	err = clGetCommandQueueInfo(p->CL_queue, CL_QUEUE_DEVICE, sizeof(myDevice), &myDevice, NULL);
printf ("queue Device when commandqueue created %p, should be %p\n", myDevice,p->CL_device_id);
	err = clGetCommandQueueInfo(p->CL_queue, CL_QUEUE_REFERENCE_COUNT, sizeof(myReference), &myReference, NULL);
printf ("queue Reference when commandqueue created %d\n", myReference);

}

  	err = clEnqueueNDRangeKernel(p->CL_queue, me->CollideGPU_kernel, 1, NULL, &global_work_size, &local_work_size, 0, NULL, NULL);
if (!printedOnce) {
	if (err != CL_SUCCESS) {
		printCLError("clEnqueueNDRangeKernel",err);
		return maxdispv;
	}
printedOnce = TRUE;
}
	
#ifdef TRY_FLUSH
	// wait for things to finish
	err = clFlush(p->CL_queue);
	if (err != CL_SUCCESS) {
		printCLError("clFlush",err);
		return maxdispv;
	} 

	err = clFinish(p->CL_queue);
	if (err != CL_SUCCESS) {
		printCLError("clFinish",err);
		return maxdispv;
	} 
#endif

	// get the data
    
    // get the data
	/* working code has:
     err = clEnqueueReadBuffer (me->queue, me->output_buffer,
                               CL_TRUE, 0, sizeof(struct SFColorRGBA) * ntri,
                               me->collide_rvs.p, 0, NULL, NULL);
*/
    
    
	err = clEnqueueReadBuffer (p->CL_queue, me->CollideGPU_output_buffer, 
		CL_TRUE, 0, sizeof(struct SFColorRGBA) * ntri, 
		me->CollideGPU_returnValues.p, 0, NULL, NULL);

	if (err != CL_SUCCESS) {
		printCLError("clEnqueueReadBuffer",err);
		return maxdispv;
	}


	for (i=0; i < ntri; i++) {
		/* XXX float to double conversion; make a vecdotf for speed */
		double disp;
        /* printf ("i %d rv %f %f %f %f\n",i,me->CollideGPU_returnValues.p[i].c[0],
                me->CollideGPU_returnValues.p[i].c[1],me->CollideGPU_returnValues.p[i].c[2],
                me->CollideGPU_returnValues.p[i].c[3]);
         */
		// we use the last float to indicate whether to bother here; saves us
		// doing unneeded calculations here

		if (me->CollideGPU_returnValues.p[i].c[3] > 1.0) {
			 //ConsoleMessage ("possibly triangle %d has some stuff for us\n",i);


			dispv.x = me->CollideGPU_returnValues.p[i].c[0];
			dispv.y = me->CollideGPU_returnValues.p[i].c[1];
			dispv.z = me->CollideGPU_returnValues.p[i].c[2];
			//ConsoleMessage ("GPU collide tri %d, disp %f %f %f\n",i,dispv.x,dispv.y,dispv.z);

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

	
	//ConsoleMessage ("OpenCL ntri %d - at end of opencl, maxdispv %f %f %f\n",ntri, maxdispv.x, maxdispv.y, maxdispv.z);

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

#if defined (TARGET_AQUA)
static const char* collide_non_walk_kernel_headers = " \
//#pragma OPENCL EXTENSION cl_khr_fp64 : enable \n\
#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable \n\
#pragma OPENCL EXTENSION CL_APPLE_gl_sharing : enable \n\
#pragma OPENCL EXTENSION CL_KHR_gl_sharing : enable \n\
#pragma OPENCL EXTENSION cl_khr_select_fprounding_mode : enable \n\
";
#else

// this seems to be ok on AMD drivers under Linux
static const char* collide_non_walk_kernel_headers = " \
#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable \n\
";
#endif // AQUA

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
#define FLOAT_TOLERANCE 0.0000001f \n\
#define PR_DOUBLESIDED 0x01  \n\
#define PR_FRONTFACING 0x02 /* overrides effect of doublesided. */  \n\
#define PR_BACKFACING 0x04 /* overrides effect of doublesided, all normals are reversed. */  \n\
 \n\
/********************************************************************************/ \n\
 \n\
 \n\
#define APPROX (a, b) (fabs(a-b) < FLOAT_TOLERANCE) \n\
#define VECSCALE(v,s) (float4)(v.x*s, v.y*s, v.z*s, 0.0f) \n\
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
	__global short *my_cindex, 	/* 4 */  \n\
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
	float4 matColumn1 = (float4)(convert_float(mymat[0]),convert_float(mymat[1]),convert_float(mymat[2]),0.0f); \n\
	float4 matColumn2 = (float4)(convert_float(mymat[4]),convert_float(mymat[5]),convert_float(mymat[6]),0.0f); \n\
	float4 matColumn3 = (float4)(convert_float(mymat[8]),convert_float(mymat[9]),convert_float(mymat[10]),0.0f); \n\
	float4 matColumn4 = (float4)(convert_float(mymat[12]),convert_float(mymat[13]),convert_float(mymat[14]),0.0f); \n\
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
		output[i_am_canadian] = (float4)(0.0f,0.0f,0.0f,0.0f); \n\
\
		return; \n\
	} \n\
 \n\
 \n\
	/* if we are down to here, we have to do this triangle */ \n\
 \n\
	if(!frontfacing) { /*can only be here in DoubleSided mode*/  \n\
		/*reverse polygon orientation, and do calculations*/  \n\
		norm = VECSCALE(norm,-1.0f); \n\
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
		output[i_am_canadian] = (float4)(0.0f,0.0f,0.0f,0.0f); \n\
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
		result.w = 100.0f; /* flag that this is a good one */ \n\
		output[i_am_canadian] = result; \n\
		return; \n\
	} \n\
 \n\
 \n\
	/* if we are down to here, we can just return zero */ \n\
	output[i_am_canadian] = (float4)(0.0f,0.0f,0.0f,0.0f); \n\
} \n\
";

#endif //HAVE_OPENCL
