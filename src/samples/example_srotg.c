/* ************************************************************************
 * Copyright 2013 Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * ************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Include CLBLAS header. It automatically includes needed OpenCL header,
 * so we can drop out explicit inclusion of cl.h header.
 */
#include <clBLAS.h>

/* This example uses predefined matrices and their characteristics for
 * simplicity purpose.
 */
static cl_float SA = 11;
static cl_float SB = 21;
static cl_float C = 0.2;
static cl_float S = 0.5;

static void
printResult(void)
{
    printf("\nResult:\n");
    printf("SA: %f\tSB: %f\t C: %f\tS: %f\n", SA, SB, C, S);
}

int
main(void)
{
    cl_int err;
    cl_platform_id platform = 0;
    cl_device_id device = 0;
    cl_context_properties props[3] = { CL_CONTEXT_PLATFORM, 0, 0 };
    cl_context ctx = 0;
    cl_command_queue queue = 0;
    cl_mem bufSA, bufSB, bufC, bufS;
    cl_event event = NULL;
    int ret = 0;

    /* Setup OpenCL environment. */
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        printf( "clGetPlatformIDs() failed with %d\n", err );
        return 1;
    }

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        printf( "clGetDeviceIDs() failed with %d\n", err );
        return 1;
    }

    props[1] = (cl_context_properties)platform;
    ctx = clCreateContext(props, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        printf( "clCreateContext() failed with %d\n", err );
        return 1;
    }

    queue = clCreateCommandQueue(ctx, device, 0, &err);
    if (err != CL_SUCCESS) {
        printf( "clCreateCommandQueue() failed with %d\n", err );
        clReleaseContext(ctx);
        return 1;
    }

    /* Setup clblas. */
    err = clblasSetup();
    if (err != CL_SUCCESS) {
        printf("clblasSetup() failed with %d\n", err);
        clReleaseCommandQueue(queue);
        clReleaseContext(ctx);
        return 1;
    }

    /* Prepare OpenCL memory objects and place matrices inside them. */
    bufSA = clCreateBuffer(ctx, CL_MEM_READ_WRITE, sizeof(cl_float), NULL, &err);
    bufSB = clCreateBuffer(ctx, CL_MEM_READ_WRITE, sizeof(cl_float), NULL, &err);
    bufC  = clCreateBuffer(ctx, CL_MEM_READ_WRITE, sizeof(cl_float), NULL, &err);
    bufS  = clCreateBuffer(ctx, CL_MEM_READ_WRITE, sizeof(cl_float), NULL, &err);

    err = clEnqueueWriteBuffer(queue, bufSA, CL_TRUE, 0, sizeof(cl_float), &SA, 0, NULL, NULL);
    err = clEnqueueWriteBuffer(queue, bufSB, CL_TRUE, 0, sizeof(cl_float), &SB, 0, NULL, NULL);
    err = clEnqueueWriteBuffer(queue, bufC, CL_TRUE, 0, sizeof(cl_float), &C, 0, NULL, NULL);
    err = clEnqueueWriteBuffer(queue, bufS, CL_TRUE, 0, sizeof(cl_float), &S, 0, NULL, NULL);


    /* Call clblas function. */
    err = clblasSrotg(bufSA, 0, bufSB, 0, bufC, 0, bufS, 0, 1, &queue, 0, NULL, &event);
    if (err != CL_SUCCESS) {
        printf("clblasSrotg() failed with %d\n", err);
        ret = 1;
    }
    else {
        /* Wait for calculations to be finished. */
        err = clWaitForEvents(1, &event);

        /* Fetch results of calculations from GPU memory. */
        err = clEnqueueReadBuffer(queue, bufSA, CL_TRUE, 0, sizeof(cl_float), &SA, 0, NULL, NULL);
        err = clEnqueueReadBuffer(queue, bufSB, CL_TRUE, 0, sizeof(cl_float), &SB, 0, NULL, NULL);
        err = clEnqueueReadBuffer(queue, bufC, CL_TRUE, 0, sizeof(cl_float), &C, 0, NULL, NULL);
        err = clEnqueueReadBuffer(queue, bufS, CL_TRUE, 0, sizeof(cl_float), &S, 0, NULL, NULL);

        /* At this point you will get the result of SROTG placed in vector Y. */
        printResult();
    }

    /* Release OpenCL events. */
    clReleaseEvent(event);

    /* Release OpenCL memory objects. */
    clReleaseMemObject(bufSA);
    clReleaseMemObject(bufSB);
    clReleaseMemObject(bufC);
    clReleaseMemObject(bufS);

    /* Finalize work with clblas. */
    clblasTeardown();

    /* Release OpenCL working objects. */
    clReleaseCommandQueue(queue);
    clReleaseContext(ctx);

    return ret;
}
