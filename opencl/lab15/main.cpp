/*
 *
 * Copyright © 2010-2013 Balázs Tóth <tbalazs@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <cmath>
#include <limits>
#include <CL/opencl.h>

#include "clwrapper.hpp"

ClWrapper cl;
cl_program clProgram;

// *****************
// OpenCL processing
// *****************

void printTimeStats(cl_event event)
{
    cl_int err = CL_SUCCESS;
    if(event == NULL)
    {
        std::cerr << "No event object returned!" << std::endl;
    }
    else
    {
        clWaitForEvents(1, &event);
    }
    cl_ulong execStart, execEnd;
    err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START,
                                  sizeof(cl_ulong), &execStart, NULL);
    if(err != CL_SUCCESS)
    {
        std::cerr << "Error during profile query: CL_PROFILING_COMMAND_START [" << err << "]." << std::endl;
    }

    err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END,
                                  sizeof(cl_ulong), &execEnd, NULL);
    if(err != CL_SUCCESS)
    {
        std::cerr << "Error during profile query: CL_PROFILING_COMMAND_END [" << err << "]." << std::endl;
    }
    std::cout << "[start] " << execStart << " [end] " << execEnd << " [time] " << (execEnd - execStart) / 1e+06 << "ms." << std::endl;
}

// Map primitive
void map()
{
    std::cout << "Map" << std::endl;

    const size_t dataSize = 1024;
    cl_int err = CL_SUCCESS;

    cl_kernel mapKernel = cl.createKernel(clProgram, "map");

    float* hData = new float[dataSize];
    for(size_t i = 0; i < dataSize; ++i)
    {
        hData[i] = (float)i;
    }

    cl_mem gData = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(float) * dataSize, NULL, NULL);
    clEnqueueWriteBuffer(cl.cqueue(), gData, CL_TRUE, 0, sizeof(float) * dataSize, hData, 0, NULL, NULL);

    clSetKernelArg(mapKernel, 0, sizeof(cl_mem), &gData);
    size_t workSize = dataSize;
    cl_event kernelEvent;
    clEnqueueNDRangeKernel(cl.cqueue(), mapKernel,
                           1, NULL, &workSize, NULL,
                           0, NULL, &kernelEvent);

    printTimeStats(kernelEvent);

    clEnqueueReadBuffer(cl.cqueue(), gData, CL_TRUE, 0, sizeof(float) * dataSize, hData, 0, NULL, NULL);

    for(size_t i = 0; i < dataSize; ++i)
    {
        if(hData[i] != (float)i * (float)i)
        {
            std::cerr << "Wrong value at " << i << ". Value is " << hData[i] << " but expected " << (float)i * (float)i << "." << std::endl;
            break;
        }
    }

    delete[] hData;
    clReleaseMemObject(gData);
    clReleaseKernel(mapKernel);

    std::cout << std::endl;
}

// Histogram primitive (global version)
void histogram_global()
{
    std::cout << "Global histogram" << std::endl;
    const size_t dataSize = 1024;
    const size_t histogramSize = 32;
    cl_int err = CL_SUCCESS;

    cl_kernel histogramGlobalKernel = cl.createKernel(clProgram, "histogram_global");

    int *hData = new int[dataSize];
    for(size_t i = 0; i < dataSize; ++i)
    {
        hData[i] = i % histogramSize;
    }

    int *hHist = new int[histogramSize];
    memset(hHist, 0, sizeof(int)*histogramSize);

    cl_mem gData = clCreateBuffer(cl.context(), CL_MEM_READ_ONLY, sizeof(int) * dataSize, NULL, NULL);
    clEnqueueWriteBuffer(cl.cqueue(), gData, CL_TRUE, 0, sizeof(int) * dataSize, hData, 0, NULL, NULL);

    cl_mem gHist = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(int) * histogramSize, NULL, NULL);
    clEnqueueWriteBuffer(cl.cqueue(), gHist, CL_TRUE, 0, sizeof(int) * histogramSize, hHist, 0, NULL, NULL);

    clSetKernelArg(histogramGlobalKernel, 0, sizeof(cl_mem), &gData);
    clSetKernelArg(histogramGlobalKernel, 1, sizeof(cl_mem), &gHist);

    cl_event kernelEvent;
    const size_t workSize = dataSize;
    clEnqueueNDRangeKernel(cl.cqueue(), histogramGlobalKernel,
                           1, NULL, &workSize, NULL,
                           0, NULL, &kernelEvent);

    printTimeStats(kernelEvent);

    clEnqueueReadBuffer(cl.cqueue(), gHist, CL_TRUE, 0, sizeof(int) * histogramSize, hHist, 0, NULL, NULL);

    // reference
    int *refHist = new int[histogramSize];
    memset(refHist, 0, sizeof(int)*histogramSize);
    for(size_t i = 0; i < dataSize; ++i)
    {
        refHist[hData[i]] += 1;
    }

    for(size_t i = 0; i < histogramSize; ++i)
    {
        if(refHist[i] != hHist[i])
        {
            std::cerr << "Wrong value at " << i << ". Value is " << hHist[i] << " but expected " << refHist[i] << "." << std::endl;
            break;
        }
    }

    delete[] refHist;
    clReleaseMemObject(gHist);
    clReleaseMemObject(gData);
    delete[] hHist;
    delete[] hData;
    clReleaseKernel(histogramGlobalKernel);

    std::cout << std::endl;
}

// Histogram primitive (local version)
void histogram_local()
{
    std::cout << "Local histogram" << std::endl;

    const size_t dataSize = 1024;
    const size_t histogramSize = 32;
    cl_int err = CL_SUCCESS;

    cl_kernel histogramLocalKernel = cl.createKernel(clProgram, "histogram_local");

    int *hData = new int[dataSize];
    for(size_t i = 0; i < dataSize; ++i)
    {
        hData[i] = i % histogramSize;
    }

    int *hHist = new int[histogramSize];
    memset(hHist, 0, sizeof(int)*histogramSize);

    cl_mem gData = clCreateBuffer(cl.context(), CL_MEM_READ_ONLY, sizeof(int) * dataSize, NULL, NULL);
    clEnqueueWriteBuffer(cl.cqueue(), gData, CL_TRUE, 0, sizeof(int) * dataSize, hData, 0, NULL, NULL);

    cl_mem gHist = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(int) * histogramSize, NULL, NULL);
    clEnqueueWriteBuffer(cl.cqueue(), gHist, CL_TRUE, 0, sizeof(int) * histogramSize, hHist, 0, NULL, NULL);

    clSetKernelArg(histogramLocalKernel, 0, sizeof(cl_mem), &gData);
    clSetKernelArg(histogramLocalKernel, 1, sizeof(cl_mem), &gHist);
    clSetKernelArg(histogramLocalKernel, 2, sizeof(int) * histogramSize, NULL);
    clSetKernelArg(histogramLocalKernel, 3, sizeof(int), &histogramSize);

    cl_event kernelEvent;
    const size_t workSize = dataSize;
    clEnqueueNDRangeKernel(cl.cqueue(), histogramLocalKernel,
                           1, NULL, &workSize, NULL,
                           0, NULL, &kernelEvent);

    printTimeStats(kernelEvent);

    clEnqueueReadBuffer(cl.cqueue(), gHist, CL_TRUE, 0, sizeof(int) * histogramSize, hHist, 0, NULL, NULL);

    // reference
    int *refHist = new int[histogramSize];
    memset(refHist, 0, sizeof(int)*histogramSize);
    for(size_t i = 0; i < dataSize; ++i)
    {
        refHist[hData[i]] += 1;
    }

    for(size_t i = 0; i < histogramSize; ++i)
    {
        if(refHist[i] != hHist[i])
        {
            std::cerr << "Wrong value at " << i << ". Value is " << hHist[i] << " but expected " << refHist[i] << "." << std::endl;
            break;
        }
    }

    delete[] refHist;
    clReleaseMemObject(gHist);
    clReleaseMemObject(gData);
    delete[] hHist;
    delete[] hData;
    clReleaseKernel(histogramLocalKernel);

    std::cout << std::endl;
}

// Reduce primitive (global version)
void reduce_global()
{
    std::cout << "Global reduce" << std::endl;

    const size_t dataSize = 1024;
    cl_int err = CL_SUCCESS;

    cl_kernel reduceKernel = cl.createKernel(clProgram, "reduce_global");

    float* hData = new float[dataSize];
    for(size_t i = 0; i < dataSize; ++i)
    {
        hData[i] = (float)rand() / (float)RAND_MAX;
    }

    cl_mem gData = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(float) * dataSize, NULL, NULL);
    clEnqueueWriteBuffer(cl.cqueue(), gData, CL_TRUE, 0, sizeof(float) * dataSize, hData, 0, NULL, NULL);

    clSetKernelArg(reduceKernel, 0, sizeof(cl_mem), &gData);

    size_t workSize = dataSize;
    cl_event kernelEvent;
    clEnqueueNDRangeKernel(cl.cqueue(), reduceKernel,
                           1, NULL, &workSize, NULL,
                           0, NULL, &kernelEvent);

    printTimeStats(kernelEvent);

    clEnqueueReadBuffer(cl.cqueue(), gData, CL_TRUE, 0, sizeof(float) * dataSize, hData, 0, NULL, NULL);

    // reference
    float minVal = std::numeric_limits<float>::min();
    for(size_t i = 0; i < dataSize; ++i)
    {
        minVal = fmax(minVal, hData[i]);
    }
    if(minVal != hData[0])
    {
        std::cerr << "Wrong result. Value is " << hData[0] << " but expected " << minVal << std::endl;
    }

    delete[] hData;
    clReleaseMemObject(gData);
    clReleaseKernel(reduceKernel);

    std::cout << std::endl;
}

// Exclusive scan primitive
void exclusive_scan()
{
    std::cout << "Global exclusive scan" << std::endl;

    const size_t dataSize = 1024;
    cl_int err = CL_SUCCESS;

    cl_kernel exscanKernel = cl.createKernel(clProgram, "exscan_global");

    int* hData = new int[dataSize];
    for(size_t i = 0; i < dataSize; ++i)
    {
        hData[i] = ((float)rand() / (float)RAND_MAX) < 0.5 ? 0 : 1;
    }
    int* oData = new int[dataSize];
    memcpy(oData, hData, sizeof(int)*dataSize);

    cl_mem gData = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(float) * dataSize, NULL, NULL);
    clEnqueueWriteBuffer(cl.cqueue(), gData, CL_TRUE, 0, sizeof(float) * dataSize, hData, 0, NULL, NULL);

    clSetKernelArg(exscanKernel, 0, sizeof(cl_mem), &gData);

    size_t workSize = dataSize;
    cl_event kernelEvent;
    clEnqueueNDRangeKernel(cl.cqueue(), exscanKernel,
                           1, NULL, &workSize, NULL,
                           0, NULL, &kernelEvent);

    printTimeStats(kernelEvent);

    clEnqueueReadBuffer(cl.cqueue(), gData, CL_TRUE, 0, sizeof(float) * dataSize, hData, 0, NULL, NULL);

    // reference
    int runningSum = 0;
    for(size_t i = 0; i < dataSize; ++i)
    {
        int tmp = oData[i];
        oData[i] = runningSum;
        runningSum += tmp;
    }

    for(size_t i = 0; i < dataSize; ++i)
    {
        if(oData[i] != hData[i])
        {
            std::cerr << "Wrong value at " << i << ". Value is " << hData[i] << " but expected " << oData[i] << "." << std::endl;
            break;
        }
    }

    delete[] oData;
    delete[] hData;
    clReleaseMemObject(gData);
    clReleaseKernel(exscanKernel);

    std::cout << std::endl;
}

int main(int argc, char* argv[])
{
    clProgram = cl.createProgram("kernels/programs.cl");
    map();
    histogram_global();
    histogram_local();
    reduce_global();
    exclusive_scan();
    return(0);
}
