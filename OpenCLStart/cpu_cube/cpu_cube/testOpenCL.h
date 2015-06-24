//copy the test example code from the book
//OpenCL Programming by Example

#include <CL/cl.h>
//OpenCL kernel which is run for every work item created.

const char *getErrorString(cl_int error)
{
	switch (error){
		// run-time and JIT compiler errors
	case 0: return "CL_SUCCESS";
	case -1: return "CL_DEVICE_NOT_FOUND";
	case -2: return "CL_DEVICE_NOT_AVAILABLE";
	case -3: return "CL_COMPILER_NOT_AVAILABLE";
	case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
	case -5: return "CL_OUT_OF_RESOURCES";
	case -6: return "CL_OUT_OF_HOST_MEMORY";
	case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
	case -8: return "CL_MEM_COPY_OVERLAP";
	case -9: return "CL_IMAGE_FORMAT_MISMATCH";
	case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
	case -11: return "CL_BUILD_PROGRAM_FAILURE";
	case -12: return "CL_MAP_FAILURE";
	case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
	case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
	case -15: return "CL_COMPILE_PROGRAM_FAILURE";
	case -16: return "CL_LINKER_NOT_AVAILABLE";
	case -17: return "CL_LINK_PROGRAM_FAILURE";
	case -18: return "CL_DEVICE_PARTITION_FAILED";
	case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

		// compile-time errors
	case -30: return "CL_INVALID_VALUE";
	case -31: return "CL_INVALID_DEVICE_TYPE";
	case -32: return "CL_INVALID_PLATFORM";
	case -33: return "CL_INVALID_DEVICE";
	case -34: return "CL_INVALID_CONTEXT";
	case -35: return "CL_INVALID_QUEUE_PROPERTIES";
	case -36: return "CL_INVALID_COMMAND_QUEUE";
	case -37: return "CL_INVALID_HOST_PTR";
	case -38: return "CL_INVALID_MEM_OBJECT";
	case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
	case -40: return "CL_INVALID_IMAGE_SIZE";
	case -41: return "CL_INVALID_SAMPLER";
	case -42: return "CL_INVALID_BINARY";
	case -43: return "CL_INVALID_BUILD_OPTIONS";
	case -44: return "CL_INVALID_PROGRAM";
	case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
	case -46: return "CL_INVALID_KERNEL_NAME";
	case -47: return "CL_INVALID_KERNEL_DEFINITION";
	case -48: return "CL_INVALID_KERNEL";
	case -49: return "CL_INVALID_ARG_INDEX";
	case -50: return "CL_INVALID_ARG_VALUE";
	case -51: return "CL_INVALID_ARG_SIZE";
	case -52: return "CL_INVALID_KERNEL_ARGS";
	case -53: return "CL_INVALID_WORK_DIMENSION";
	case -54: return "CL_INVALID_WORK_GROUP_SIZE";
	case -55: return "CL_INVALID_WORK_ITEM_SIZE";
	case -56: return "CL_INVALID_GLOBAL_OFFSET";
	case -57: return "CL_INVALID_EVENT_WAIT_LIST";
	case -58: return "CL_INVALID_EVENT";
	case -59: return "CL_INVALID_OPERATION";
	case -60: return "CL_INVALID_GL_OBJECT";
	case -61: return "CL_INVALID_BUFFER_SIZE";
	case -62: return "CL_INVALID_MIP_LEVEL";
	case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
	case -64: return "CL_INVALID_PROPERTY";
	case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
	case -66: return "CL_INVALID_COMPILER_OPTIONS";
	case -67: return "CL_INVALID_LINKER_OPTIONS";
	case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

		// extension errors
	case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
	case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
	case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
	case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
	case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
	case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
	default: return "Unknown OpenCL error";
	}
}

namespace SAXPY{
	const int SAXPY_VECTOR_SIZE = 128;
	const char *saxpy_kernel =
		"__kernel \n"
		"void saxpy_kernel(float alpha, \n"
		" __global float *A, \n"
		" __global float *B, \n"
		" __global float *C) \n"
		"{ \n"
		" //Get the index of the work-item \n"
		" int index = get_global_id(0); \n"
		" C[index] = alpha* A[index] + B[index]; \n"
		"} \n";
	float alpha = 2.0f;
	float *A = NULL, *B = NULL, *C = NULL;
	cl_platform_id *platforms = NULL;
	cl_uint num_platforms;
	cl_uint usePlatform = 0;

	cl_device_id *device_list = NULL;
	cl_uint num_devices;

	cl_context context = 0;

	cl_command_queue command_queue = 0;

	cl_mem A_clmem;
	cl_mem B_clmem;
	cl_mem C_clmem;

	cl_program program;
	cl_kernel kernel;

	size_t global_size = SAXPY_VECTOR_SIZE;
	size_t local_size = 64;

	void initSAXPY()
	{
		//allocating space
		A = new float[SAXPY_VECTOR_SIZE];
		B = new float[SAXPY_VECTOR_SIZE];
		C = new float[SAXPY_VECTOR_SIZE];

		for (int i = 0; i < SAXPY_VECTOR_SIZE; i++)
		{
			A[i] = i;
			B[i] = SAXPY_VECTOR_SIZE - i;
			C[i] = 0;
		}

		//NULL means "I don't need this info"

		//Only get num of platforms
		cl_int clStatus = clGetPlatformIDs(0, NULL, &num_platforms);
		cout << "num of platforms :" << num_platforms << endl;


		//Allocating yeah~~
		platforms = new cl_platform_id[num_platforms];
		//give all the platforms ID
		clStatus = clGetPlatformIDs(num_platforms, platforms, NULL);

		cout << "name of platforms : " << endl;
		for (int i = 0; i < num_platforms; i++)
		{
			cout << i + 1 << " ";
			cl_uint cl_platform_enum[] = { CL_PLATFORM_NAME, CL_PLATFORM_VENDOR, CL_PLATFORM_VERSION,
				CL_PLATFORM_EXTENSIONS, CL_PLATFORM_PROFILE };

			for (int j = 0; j < sizeof(cl_platform_enum) / sizeof(cl_uint); j++)
			{
				char *buffer;
				size_t num_char;
				clGetPlatformInfo(platforms[i], cl_platform_enum[j], 0, NULL, &num_char);
				buffer = new char[num_char + 1];
				clGetPlatformInfo(platforms[i], cl_platform_enum[j], num_char, buffer, &num_char);
				if (j>0)
					cout << ", ";
				cout << buffer;
				delete[] buffer;
			}


			cout << endl;
		}
		cout << endl;


		//get the device num
		clStatus = clGetDeviceIDs(platforms[usePlatform], CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices);

		device_list = new cl_device_id[num_devices];
		//get the device list
		clStatus = clGetDeviceIDs(platforms[usePlatform], CL_DEVICE_TYPE_GPU, num_devices, device_list, &num_devices);


		cout << "device list (GPU devices)" << endl;
		for (int i = 0; i < num_devices; i++)
		{
			cout << i + 1 << " ";
			char *buffer;
			size_t num_char;
			clGetDeviceInfo(device_list[i], CL_DEVICE_NAME, 0, NULL, &num_char);
			buffer = new char[num_char + 1];
			clGetDeviceInfo(device_list[i], CL_DEVICE_NAME, num_char, buffer, &num_char);
			cout << buffer << endl;
			delete[] buffer;
		}
		cout << endl;


		context = clCreateContext(NULL, num_devices, device_list, NULL, NULL, &clStatus);

		//create a command queue
		command_queue = clCreateCommandQueue(context, device_list[0], 0, &clStatus);


		//create memory buffers on the device
		A_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, SAXPY_VECTOR_SIZE*sizeof(float), NULL, &clStatus);
		B_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, SAXPY_VECTOR_SIZE*sizeof(float), NULL, &clStatus);
		C_clmem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, SAXPY_VECTOR_SIZE*sizeof(float), NULL, &clStatus);


		//create kernel, can be swap order?
		program = clCreateProgramWithSource(context, 1, (const char**)&saxpy_kernel, NULL, &clStatus);
		clStatus = clBuildProgram(program, 1, device_list, NULL, NULL, NULL);
		kernel = clCreateKernel(program, "saxpy_kernel", &clStatus);

		//set arguments
		clStatus = clSetKernelArg(kernel, 0, sizeof(float), (void*)&alpha);
		clStatus = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&A_clmem);
		clStatus = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&B_clmem);
		clStatus = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&C_clmem);

		return;
	}

	void checkSAXPY()
	{
		cout << "--------------------------------------" << endl;
		for (int i = 0; i < SAXPY_VECTOR_SIZE; i++)
			printf("%f * %f + %f = %f\n", alpha, A[i], B[i], C[i]);
		cout << "--------------------------------------" << endl;
	}

	void runSAXPY()
	{
		int iteration = 2;
		//add two times
		//read back
		//check answer

		while (iteration--)
		{

			cl_int clStatus;

			//update argument
			clStatus = clSetKernelArg(kernel, 0, sizeof(float), (void*)&alpha);

			//write buffer
			clStatus = clEnqueueWriteBuffer(command_queue, A_clmem, CL_TRUE, 0, SAXPY_VECTOR_SIZE*sizeof(float), A, 0, NULL, NULL);
			clStatus = clEnqueueWriteBuffer(command_queue, B_clmem, CL_TRUE, 0, SAXPY_VECTOR_SIZE*sizeof(float), B, 0, NULL, NULL);



			//run program
			clStatus = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);

			//read answer
			clStatus = clEnqueueReadBuffer(command_queue, C_clmem, CL_TRUE, 0, SAXPY_VECTOR_SIZE*sizeof(float), C, 0, NULL, NULL);

			clStatus = clFlush(command_queue);
			clStatus = clFinish(command_queue);

			checkSAXPY();

			//copy C to A
			memcpy(A, C, sizeof(float)*SAXPY_VECTOR_SIZE);
		}

	}



	void finalizeSAXPY()
	{
		//release resoucres
		if (A)
			delete[] A;
		if (B)
			delete[] B;
		if (C)
			delete[] C;

		if (platforms)
			delete platforms;
		if (device_list)
			delete device_list;

		cl_int clStatus;
		clStatus = clReleaseKernel(kernel);
		clStatus = clReleaseContext(context);
		clStatus = clReleaseMemObject(A_clmem);
		clStatus = clReleaseMemObject(B_clmem);
		clStatus = clReleaseMemObject(C_clmem);
		clStatus = clReleaseCommandQueue(command_queue);
		clStatus = clReleaseContext(context);

		return;
	}
	void testOpenCL()
	{
		initSAXPY();
		runSAXPY();
		finalizeSAXPY();
	}
}