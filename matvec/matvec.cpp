#define _CRT_SECURE_NO_WARNINGS
#define PROGRAM_FILE "matvec.cl"
#define KERNEL_FUNC "matvec_mult"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef MAC
#include <OpenCL/cl.h>
#else  
#include <CL/cl.h>
#endif

#ifdef _WIN32
#include <Windows.h>
#include <time.h>
#else
#include <sys/time.h>
#include <sys/times.h>
#endif

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

int main() {

   /* Host/device data structures */
   cl_platform_id platform;
   cl_device_id device;
   cl_context context;
   cl_command_queue queue;
   cl_int i, err;

   /* Program/kernel data structures */
   cl_program program;
   FILE *program_handle;
   char *program_buffer, *program_log;
   size_t program_size, log_size;
   cl_kernel kernel;
   
   /* Data and buffers */
   float mat[16], vec[4], result[4];
   float correct[4] = {0.0f, 0.0f, 0.0f, 0.0f};
   cl_mem mat_buff, vec_buff, res_buff;
   size_t work_units_per_kernel;
	/*Time */
	Profiler prof;
   double exeTime = 0;

   /* Initialize data to be processed by the kernel */
   for(i=0; i<16; i++) {
      mat[i] = i * 2.0f;
	  printf("mat[%d]:%f\n",i,mat[i]);
   } 
   for(i=0; i<4; i++) {
      vec[i] = i * 3.0f;
      correct[0] += mat[i]    * vec[i];
      correct[1] += mat[i+4]  * vec[i];
      correct[2] += mat[i+8]  * vec[i];
      correct[3] += mat[i+12] * vec[i];
	  printf("mat[%d]*vec[%d]:%f * %f \n",i,i,mat[i],vec[i]);
	  printf("mat[%d]*vec[%d]:%f * %f \n",i+4,i,mat[i+4],vec[i]);
	  printf("mat[%d]*vec[%d]:%f * %f \n",i+8,i,mat[i+8],vec[i]);
	  printf("mat[%d]*vec[%d]:%f * %f \n",i+12,i,mat[i+12],vec[i]);
	  printf("correct:%f %f %f %f\n",correct[0],correct[1],correct[2],correct[3]);
   }
   printf("correct:%f %f %f %f\n",correct[0],correct[1],correct[2],correct[3]);

   /* Identify a platform */
   err = clGetPlatformIDs(1, &platform, NULL);
   if(err < 0) {
      perror("Couldn't find any platforms");
      exit(1);
   }

   /* Access a device */
   err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
   if(err < 0) {
      perror("Couldn't find any devices");
      exit(1);
   }

   /* Create the context */
   context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
   if(err < 0) {
      perror("Couldn't create a context");
      exit(1);   
   }

   /* Read program file and place content into buffer */
   program_handle = fopen(PROGRAM_FILE, "r");
   if(program_handle == NULL) {
      perror("Couldn't find the program file");
      exit(1);   
   }
   fseek(program_handle, 0, SEEK_END);
   program_size = ftell(program_handle);
   rewind(program_handle);
   program_buffer = (char*)malloc(program_size + 1);
   program_buffer[program_size] = '\0';
   fread(program_buffer, sizeof(char), program_size, program_handle);
   fclose(program_handle);

   /* Create program from file */
   program = clCreateProgramWithSource(context, 1, 
      (const char**)&program_buffer, &program_size, &err);
   if(err < 0) {
      perror("Couldn't create the program");
      exit(1);   
   }
   free(program_buffer);

   /* Build program */
   err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
   if(err < 0) {

      /* Find size of log and print to std output */
      clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 
            0, NULL, &log_size);
      program_log = (char*) malloc(log_size + 1);
      program_log[log_size] = '\0';
      clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 
            log_size + 1, program_log, NULL);
      printf("%s\n", program_log);
      free(program_log);
      exit(1);
   }

   /* Create kernel for the mat_vec_mult function */
   kernel = clCreateKernel(program, KERNEL_FUNC, &err);
   if(err < 0) {
      perror("Couldn't create the kernel");
      exit(1);   
   }

   /* Create CL buffers to hold input and output data */
   mat_buff = clCreateBuffer(context, CL_MEM_READ_ONLY | 
      CL_MEM_COPY_HOST_PTR, sizeof(float)*16, mat, &err);
   if(err < 0) {
      perror("Couldn't create a buffer object");
      exit(1);   
   }      
   vec_buff = clCreateBuffer(context, CL_MEM_READ_ONLY | 
      CL_MEM_COPY_HOST_PTR, sizeof(float)*4, vec, NULL);
   res_buff = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
      sizeof(float)*4, NULL, NULL);

   /* Create kernel arguments from the CL buffers */
   err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &mat_buff);
   if(err < 0) {
      perror("Couldn't set the kernel argument");
      exit(1);   
   }         
   clSetKernelArg(kernel, 1, sizeof(cl_mem), &vec_buff);
   clSetKernelArg(kernel, 2, sizeof(cl_mem), &res_buff);

   /* Create a CL command queue for the device*/
   queue = clCreateCommandQueue(context, device, 0, &err);
   if(err < 0) {
      perror("Couldn't create the command queue");
      exit(1);   
   }
   prof.resetProfiler();
   prof.startTime();
   /* Enqueue the command queue to the device */
   work_units_per_kernel = 4; /* 4 work-units per kernel */ 
   err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &work_units_per_kernel, 
      NULL, 0, NULL, NULL);
   if(err < 0) {
      perror("Couldn't enqueue the kernel execution command");
      exit(1);   
   }
   exeTime = prof.getDurationMS();
   printf("matvec kernel execution time:%f ms.\n", exeTime);
   /* Read the result */
   err = clEnqueueReadBuffer(queue, res_buff, CL_TRUE, 0, sizeof(float)*4, 
      result, 0, NULL, NULL);
   if(err < 0) {
      perror("Couldn't enqueue the read buffer command");
      exit(1);   
   }

   /* Test the result */
   if((result[0] == correct[0]) && (result[1] == correct[1])
      && (result[2] == correct[2]) && (result[3] == correct[3])) {
      printf("Matrix-vector multiplication successful.\n");
   }
   else {
      printf("Matrix-vector multiplication unsuccessful.\n");
   }

   /* Deallocate resources */
   clReleaseMemObject(mat_buff);
   clReleaseMemObject(vec_buff);
   clReleaseMemObject(res_buff);
   clReleaseKernel(kernel);
   clReleaseCommandQueue(queue);
   clReleaseProgram(program);
   clReleaseContext(context);

   return 0;
}

