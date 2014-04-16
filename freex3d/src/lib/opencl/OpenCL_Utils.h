/*


Collision ???

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


#ifndef __FREEWRL_OPENCL_H__
#define __FREEWRL_OPENCL_H__

#ifdef HAVE_OPENCL
#if defined (__APPLE__) || defined(MACOSX) || defined(TARGET_AQUA)
        #include <OpenCL/opencl.h>
        #include <OpenGL/CGLDevice.h>
#elif defined(_MSC_VER)
    #include <windows.h>  //WGL prototyped in wingdi.h
    #include <CL/opencl.h>
        #define DEBUG
#else  //LINUX
    #include <CL/opencl.h>
    #include <CL/cl_gl.h>
#endif

#define TEST_ERR(aa,bb) if (bb!=CL_SUCCESS) printCLError(aa,bb)

typedef struct pOpenCL_Utils{
        cl_context CL_context;
        cl_command_queue CL_queue;
        cl_device_id CL_device_id;
        size_t CL_default_workgroup_size;
    cl_program coordinateInterpolatorProgram;
    cl_kernel coordinateInterpolatorKernel;
    size_t coordinateInterpolator_workgroup_size;
}* ppOpenCL_Utils;



//void runOpenCLInterpolator(struct CRStruct *route, struct X3D_Node * toNode, int toOffset);
void printCLError(const char *where, cl_int err);
void fwl_OpenCL_startup(struct tOpenCL_Utils *t);

#endif // HAVE_OPENCL
#endif /* __FREEWRL_OPENCL_H__ */
