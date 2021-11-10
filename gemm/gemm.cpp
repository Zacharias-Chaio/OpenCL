
#include <CL/cl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include <cmath>

#ifdef _WIN32
#include <Windows.h>
#include <time.h>
#else
#include <sys/time.h>
#include <sys/times.h>
#endif

#define SUCCESS 0
#define FAILURE 1
#define LOOP 10
#define M 4096   // row of A matrix
#define K 4096   // col of A matrix
#define N 4096   // row of B matrix
#define CHECK_RESULT 0

#define CHECK_ERROR(actual, msg) \
if (actual != 0) \
{ \
    std::cout << "Error: " << msg << " .error code: " << actual << std::endl; \
    std::cout << "Location: " << __FILE__ << " line: " << __LINE__ << std::endl; \
    return actual; \
}

#define CHECKED_MALLOC(var, type, count) \
{ \
    var = (type*)malloc(sizeof(type)* (count)); \
    if (!var) \
    { \
        printf("malloc of size %d failed. Location: %s, line: %d.\n", sizeof(type)* (count), __FILE__, __LINE__); \
    } \
}

#define CHECKED_FREE(var) \
{ \
    if (var) \
    { \
        free(var); \
    } \
}

using namespace std;

typedef struct _Profiler
{
#ifdef _WIN32

    LARGE_INTEGER m_freq, m_start, m_end;
    double m_ls, m_lms, m_lns;
    
    void startTime()
    {
        QueryPerformanceCounter(&m_start);
    }

    void startTime(LARGE_INTEGER& start)
    {
        QueryPerformanceCounter(&start);
    }

    void getTime(LARGE_INTEGER& start)
    {
        QueryPerformanceCounter(&start);
    }

    void resetProfiler()
    {
        m_start.QuadPart = 0;
        m_end.QuadPart = 0;
        QueryPerformanceFrequency(&m_freq);
    }

    double getDurationS()
    {
        return (m_end.QuadPart - m_start.QuadPart)*1.0 / m_freq.QuadPart;
    }

    double getDurationMS()
    {
        QueryPerformanceCounter(&m_end);
        return (m_end.QuadPart - m_start.QuadPart)*1000.0 / m_freq.QuadPart;
    }

    double getDurationMS(LARGE_INTEGER start, LARGE_INTEGER end)
    {
        return (end.QuadPart - start.QuadPart)*1000.0 / m_freq.QuadPart;
    }

    double getDurationUs()
    {
        return (m_end.QuadPart - m_start.QuadPart)*1000000.0 / m_freq.QuadPart;
    }
#else
    timeval m_start, m_end;

    void startTime(timeval& start)
    {
        gettimeofday(&start, NULL);
    }

    void startTime()
    {
        gettimeofday(&m_start, NULL);
    }


    void getTime(timeval& start)
    {
        gettimeofday(&start, NULL);
    }

    void resetProfiler()
    {
        //m_start.QuadPart = 0;
        //m_end.QuadPart = 0;
    }

    double getDurationS(timeval start, timeval end)
    {
        //return (m_end.QuadPart - m_start.QuadPart)*1.0 / m_freq.QuadPart;
        //return (end.tv_sec + end.tv_usec / 1000000) - (start.tv_sec + start.tv_usec / 1000000);

        double duration;
        duration = (end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);
        duration /= 1000000;

        return duration;
    }

    double getDurationMS()
    {
        //return (m_end.QuadPart - m_start.QuadPart)*1000.0 / m_freq.QuadPart;
        double duration;
        gettimeofday(&m_end, NULL);
        duration = (m_end.tv_sec * 1000000 + m_end.tv_usec) - (m_start.tv_sec * 1000000 + m_start.tv_usec);
        duration /= 1000;
        return duration;
    }

    double getDurationMS(timeval start, timeval end)
    {
        //return (end.tv_sec * 1000 + end.tv_usec / 1000) - (start.tv_sec * 1000.0 + start.tv_usec / 1000);
        double duration;
        duration = (end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);
        duration /= 1000;
        return duration;
}

    double getDurationUs(timeval start, timeval end)
    {
        //return (m_end.QuadPart - m_start.QuadPart)*1000000.0 / m_freq.QuadPart;
        double duration;
        duration = (end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);
        return duration;
    }
#endif

}Profiler;

template<typename T>
int fillRandom(
    T * arrayPtr,
    const int width,
    const int height,
    const T rangeMin,
    const T rangeMax,
    unsigned int seed=123)
{
    if(!arrayPtr)
    {
        printf("Cannot fill array. NULL pointer.");
        return -1;
    }

    if(!seed)
    {
        seed = (unsigned int)time(NULL);
    }

    srand(seed);

    double range = double(rangeMax - rangeMin) + 1.0;

    /* random initialisation of input */
    for(int i = 0; i < height; i++)
        for(int j = 0; j < width; j++)
        {
            int index = i*width + j;
            arrayPtr[index] = rangeMin + T(range*rand()/(RAND_MAX + 1.0));
        }

    return 0;
}

/* convert the kernel file into a string */
int convertToString(const char *filename, std::string& s, size_t& size)
{
    char*  str;
    std::fstream f(filename, (std::fstream::in | std::fstream::binary));

    if(f.is_open())
    {
        size_t fileSize;
        f.seekg(0, std::fstream::end);
        size = fileSize = (size_t)f.tellg();
        f.seekg(0, std::fstream::beg);
        str = new char[size+1];
        if(!str)
        {
            f.close();
            return 0;
        }

        f.read(str, fileSize);
        f.close();
        str[size] = '\0';
        s = str;
        delete[] str;
        return 0;
    }
    cout<<"Error: failed to open file\n:"<<filename<<endl;
    return FAILURE;
}

void saveProgramBin(const char* path, cl_program program, const char *dev_name, const char *dev_vendor, const char *driver_version)
{
    FILE *fp = NULL;
    fp = fopen(path, "wb");
    if (!fp)
    {
        //com_log("", "OpenCL: unable to open clbin file for write\n");
        return;
    }

    unsigned char *binary = NULL;

    size_t size = 0;
    cl_int status = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &size, NULL);
    if (status != CL_SUCCESS || !size)
    {
        //com_log(param, COM_LOG_INFO, "OpenCL: Unable to query program binary size, no cache file generated\n");
        goto fail;
    }

    CHECKED_MALLOC(binary, unsigned char, size);
    status = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char *), &binary, NULL);
    if (status != CL_SUCCESS)
    {
        //com_log(param, COM_LOG_INFO, "OpenCL: Unable to query program binary, no cache file generated\n");
        goto fail;
    }

    fputs(dev_name, fp);
    fputc('\n', fp);
    fputs(dev_vendor, fp);
    fputc('\n', fp);
    fputs(driver_version, fp);
    fputc('\n', fp);
    fwrite(binary, 1, size, fp);

fail:
    fclose(fp);
    CHECKED_FREE(binary);
    return;
}

cl_program loadProgramBin(cl_context context, cl_device_id device,
                          const char* filePath, const char *dev_name, const char *dev_vendor, const char *driver_version)
{
    FILE *fp = NULL;
    fp = fopen(filePath, "rb");
    if (!fp)
        return NULL;

    cl_program program = NULL;
    unsigned char* binary = NULL;

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    rewind(fp);
    CHECKED_MALLOC(binary, unsigned char, size);

    if (fread(binary, 1, size, fp) != size)
    {
        fclose(fp);
        CHECKED_FREE(binary);
        return program;
    }

    const unsigned char*ptr = (const unsigned char*)binary;

#define CHECK_STRING( STR )\
    do {\
        size_t len = strlen(STR); \
        if (size <= len || strncmp((char*)ptr, STR, len))\
            goto loadfail; \
            else { \
            size -= (len + 1); ptr += (len + 1); \
            }\
        } while (0)

    CHECK_STRING(dev_name);
    CHECK_STRING(dev_vendor);
    CHECK_STRING(driver_version);
#undef CHECK_STRING

    cl_int status;
    program = clCreateProgramWithBinary(context, 1, &device, &size, &ptr, NULL, &status);

    if (status != CL_SUCCESS)
        program = NULL;

loadfail:
    fclose(fp);
    CHECKED_FREE(binary);
    return program;
}

cl_program createProgramByBin(cl_context context, cl_device_id device, const char* fileName)
{
    int status = 0;

    char dev_name[64];
    char dev_vendor[64];
    char driver_version[64];

    status = clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(dev_name), dev_name, NULL);
    status |= clGetDeviceInfo(device, CL_DEVICE_VENDOR, sizeof(dev_vendor), dev_vendor, NULL);
    status |= clGetDeviceInfo(device, CL_DRIVER_VERSION, sizeof(driver_version), driver_version, NULL);
    if (status != CL_SUCCESS)
        return NULL;

    cl_program program = loadProgramBin(context, device, fileName, dev_name, dev_vendor, driver_version);

    if (program == NULL)
    {
        string sourceStr;
        size_t ret_size = 0;
        convertToString("gemm_kernel.cl", sourceStr, ret_size);
        const char *source = sourceStr.c_str();

        size_t sourceSize[] = { ret_size };
        cl_program program = clCreateProgramWithSource(context, 1, (const char **)&source, sourceSize, &status);
        if (status != SUCCESS)
        {
            printf("clCreateProgramWithSource error:%d\n", status);
        }

        status = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
        if (status != SUCCESS)
        {
            printf("clBuildProgram error:%d\n", status);
        }

        saveProgramBin(fileName, program, dev_name, dev_vendor, driver_version);

        return program;
    }
    status = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (status != SUCCESS)
    {
        printf("clBuildProgram error:%d\n", status);
    }

    return program;
}


void gemm_ref(cl_float * input0, cl_float * input1, cl_float * output,
    const cl_uint y, const cl_uint x, const cl_uint z)
{
    for(cl_uint i = 0; i < y; i++)
    {
        for(cl_uint j = 0; j < z; j++)
        {
            for(cl_uint k = 0; k < x; k++)
            {
                output[i * z + j] += (input0[i * x + k] * input1[k * z + j]);
            }
        }
    }
}

int main(int argc, char* argv[])
{
    cl_int  width = N;      //output width
    cl_int  height = M;     //output height
    cl_int  m = M;
    cl_int  k = K;
    cl_int  n = N;

    if(argc == 4)   // m  k  n
    {
        m = stoi(argv[1]);
        k = stoi(argv[2]);
        n = stoi(argv[3]);
    }
    else if(argc == 5)  // widthA  heightA   widthB  heightB
    {
        cl_int  k1 = K;
        k = stoi(argv[1]);
        m = stoi(argv[2]);
        n = stoi(argv[3]);
        k1 = stoi(argv[4]);
        if(k != k1)
        {
            printf("wrong input parameter(width of A should be equal with height of B), reset height of B\n");
        }
    }
    k = (k + 3) / 4 * 4;
    n = (n + 3) / 4 * 4;
    width = n;
    height = m;

    printf("Run gemm with inputA(w:%d, h:%d) inputB(w:%d, h:%d) or M:%d K:%d N:%d.\n", k, m, n, k, m, k, n);
    /*Step1: Getting platforms and choose an available one.*/
    cl_uint numPlatforms;    //the NO. of platforms
    cl_platform_id platform = NULL;    //the chosen platform
    cl_int    status = clGetPlatformIDs(0, NULL, &numPlatforms);
    CHECK_ERROR(status, "clGetPlatformIDs");

    /*For clarity, choose the first available platform. */
    if(numPlatforms > 0)
    {
        cl_platform_id* platforms = (cl_platform_id* )malloc(numPlatforms* sizeof(cl_platform_id));
        status = clGetPlatformIDs(numPlatforms, platforms, NULL);
        platform = platforms[0];
        free(platforms);
    }

    /*Step 2:Query the platform and choose the first GPU device if has one.Otherwise use the CPU as device.*/
    cl_uint                numDevices = 0;
    cl_device_id        *devices;
    status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);    
    if (numDevices == 0)    //no GPU available.
    {
        cout << "No GPU device available." << endl;
        cout << "Choose CPU as default device." << endl;
        status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 0, NULL, &numDevices);    
        devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
        status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, numDevices, devices, NULL);
    }
    else
    {
        devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
        status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);
    }

    /*Step 3: Create context.*/
    cl_context context = clCreateContext(NULL,1, devices,NULL,NULL,NULL);
    
    /*Step 4: Creating command queue associate with the context.*/
    cl_command_queue commandQueue = clCreateCommandQueue(context, devices[0], 0, NULL);

    /*Step 5: Create program object */
    const char *filename = "kernel.bin";
#if 0
    string sourceStr;
    size_t size;
    status = convertToString(filename, sourceStr, size);
    const char *source = sourceStr.c_str();
    size_t sourceSize[] = {strlen(source)};
    cl_program program = clCreateProgramWithSource(context, 1, &source, sourceSize, NULL);

    /*Step 6: Build program. */
    status = clBuildProgram(program, 1,devices,NULL,NULL,NULL);
    CHECK_ERROR(status, "clBuildProgram");
#else
    /*Step 6: Build program. */
    cl_program program = createProgramByBin(context, devices[0], filename);
#endif

    /*Step 7: Initial input,output for the host and create memory objects for the kernel*/

    cl_uint inputASizeBytes = m * k * sizeof(cl_float);
    cl_uint inputBSizeBytes = n * k * sizeof(cl_float);
    cl_uint outputSizeBytes = width * height * sizeof(cl_float);
    cl_float* inputA_hostPtr = (cl_float *) malloc( inputASizeBytes);
    cl_float* inputB_hostPtr = (cl_float *) malloc( inputBSizeBytes);
    cl_float* output_hostPtr = (cl_float *) malloc(outputSizeBytes);
    cl_float* golden = (cl_float *) malloc(outputSizeBytes);
    memset(output_hostPtr, 0, outputSizeBytes);
    memset(golden, 0, outputSizeBytes);
    fillRandom<cl_float>(inputA_hostPtr, k, m, 0, 255);
    fillRandom<cl_float>(inputB_hostPtr, n, k, 0, 255);

    cl_mem inputAbuf = clCreateBuffer(context,
                                 CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                 inputASizeBytes,
                                 inputA_hostPtr,
                                 &status);
    CHECK_ERROR(status, "clCreateBuffer");
    cl_mem inputBbuf = clCreateBuffer(context,
                                 CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                 inputBSizeBytes,
                                 inputB_hostPtr,
                                 &status);
    CHECK_ERROR(status, "clCreateBuffer");
    cl_mem outputBuf = clCreateBuffer(context,
                                 CL_MEM_WRITE_ONLY,
                                 outputSizeBytes,
                                 NULL,
                                 &status);
    CHECK_ERROR(status, "clCreateBuffer");


    /*Step 8: Create kernel object */
    cl_kernel kernel;
    kernel = clCreateKernel(program,"gemm_block4x4_F32", NULL);

    /*Step 9: Sets Kernel arguments.*/
    size_t index = 0;
    status = clSetKernelArg(kernel, index++, sizeof(cl_mem), (void *)&inputAbuf);
    status |= clSetKernelArg(kernel, index++, sizeof(cl_mem), (void *)&inputBbuf);
    status |= clSetKernelArg(kernel, index++, sizeof(cl_mem), (void *)&outputBuf);
    status |= clSetKernelArg(kernel, index++, sizeof(cl_int), (void *)&m);
    status |= clSetKernelArg(kernel, index++, sizeof(cl_int), (void *)&k);
    status |= clSetKernelArg(kernel, index++, sizeof(cl_int), (void *)&n);
    CHECK_ERROR(status, "clSetKernelArg");

    Profiler prof;
    int loop = LOOP;
    double exeTime = 0;
    double cpuTime = 0;

    printf("kernel calculates as block 4x4. input size:%d x %d.\n", width, height);
    /*Step 10: Running the kernel.*/
    size_t global_work_size[2] = {width/4, height/4};
    // warm up
    status = clEnqueueNDRangeKernel(commandQueue, kernel, 2, NULL, global_work_size, NULL, 0, NULL, NULL);
    CHECK_ERROR(status, "clEnqueueNDRangeKernel");
    clFinish(commandQueue);

    prof.resetProfiler();
    prof.startTime();
    for(int i = 0; i < loop; i++)
    {
        status = clEnqueueNDRangeKernel(commandQueue, kernel, 2, NULL, global_work_size, NULL, 0, NULL, NULL);
        CHECK_ERROR(status, "clEnqueueNDRangeKernel");
    }
    clFinish(commandQueue);

    exeTime = prof.getDurationMS() / loop;
    printf("gemm kernel execution time:%f ms.\n", exeTime);

    /*Step 11: Read the cout put back to host memory.*/

    status = clEnqueueReadBuffer(commandQueue,
                                outputBuf,
                                CL_TRUE,
                                0,
                                outputSizeBytes,
                                output_hostPtr,
                                0,
                                NULL,
                                NULL);
    CHECK_ERROR(status, "clEnqueueReadBuffer");
    clFinish(commandQueue);

    prof.startTime();
    //gemm_ref(inputA_hostPtr, inputB_hostPtr, golden, m, k, n);
    cpuTime = prof.getDurationMS();
    //printf("cpu gemm reference execution time:%f ms.\n", cpuTime);

    int failFlg = 0;
#if CHECK_RESULT
    for(int i = 0; i < width * height; i++)
    {
        if(fabs(output_hostPtr[i] - golden[i]) > 1.0)
        {
            printf("failed=id:%d, gpu:%f, cpu:%f.\n", i, output_hostPtr[i], golden[i]);
            failFlg = 1;
            break;
        }
    }
#endif

    /*Step 12: Clean the resources.*/
    status = clReleaseKernel(kernel);                  //Release kernel.
    status |= clReleaseProgram(program);                //Release the program object.
    status |= clReleaseMemObject(inputAbuf);            //Release mem object.
    status |= clReleaseMemObject(inputBbuf);
    status |= clReleaseMemObject(outputBuf);
    status |= clReleaseCommandQueue(commandQueue);      //Release  Command queue.
    status |= clReleaseContext(context);                //Release context.
    CHECK_ERROR(status, "clReleaseContext");

    if (inputA_hostPtr != NULL)
    {
        free(inputA_hostPtr);
        inputA_hostPtr = NULL;
    }

    if (inputB_hostPtr != NULL)
    {
        free(inputB_hostPtr);
        inputB_hostPtr = NULL;
    }

    if (output_hostPtr != NULL)
    {
        free(output_hostPtr);
        output_hostPtr = NULL;
    }

    if (golden != NULL)
    {
        free(golden);
        golden = NULL;
    }

    if (devices != NULL)
    {
        free(devices);
        devices = NULL;
    }

    if(failFlg)
    {
        std::cout<<"Failed!\n";
    }
    else
    {
        std::cout<<"Passed!\n";
    }
    return SUCCESS;
}