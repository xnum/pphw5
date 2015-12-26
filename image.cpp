#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <fstream>
#include <iostream>
#include <vector>

#include <CL/cl.h>

using namespace std;

cl_program load_program(cl_context context, const char* filename)
{
	std::ifstream in(filename, std::ios_base::binary);
	if(!in.good()) {
		return 0;
	}

	// get file length
	in.seekg(0, std::ios_base::end);
	size_t length = in.tellg();
	in.seekg(0, std::ios_base::beg);

	// read program source
	std::vector<char> data(length + 1);
	in.read(&data[0], length);
	data[length] = 0;

	// create and build program 
	const char* source = &data[0];
	cl_program program = clCreateProgramWithSource(context, 1, &source, 0, 0);
	if(program == 0) {
		return 0;
	}

	if(clBuildProgram(program, 0, 0, 0, 0, 0) != CL_SUCCESS) {
		return 0;
	}

	return program;
}

unsigned int * histogram ( unsigned int * image_data , unsigned int _size )
{
	unsigned int DATA_SIZE = _size;

	cl_int err;
	cl_uint num;
	err = clGetPlatformIDs(0, 0, &num);
	if(err != CL_SUCCESS) {
		std::cerr << "Unable to get platforms\n";
		exit(1);
	}

	std::vector<cl_platform_id> platforms(num);
	err = clGetPlatformIDs(num, &platforms[0], &num);
	if(err != CL_SUCCESS) {
		std::cerr << "Unable to get platform ID\n";
		exit(1);
	}

	cl_context_properties prop[] = { CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platforms[0]), 0 };
	cl_context context = clCreateContextFromType(prop, CL_DEVICE_TYPE_DEFAULT, NULL, NULL, NULL);
	if(context == 0) {
		std::cerr << "Can't create OpenCL context\n";
		exit(1);
	}

	size_t cb;
	clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &cb);
	std::vector<cl_device_id> devices(cb / sizeof(cl_device_id));
	clGetContextInfo(context, CL_CONTEXT_DEVICES, cb, &devices[0], 0);

	clGetDeviceInfo(devices[0], CL_DEVICE_NAME, 0, NULL, &cb);
	std::string devname;
	devname.resize(cb);
	clGetDeviceInfo(devices[0], CL_DEVICE_NAME, cb, &devname[0], 0);
	std::cout << "Device: " << devname.c_str() << "\n";

	cl_command_queue queue = clCreateCommandQueue(context, devices[0], 0, 0);
	if(queue == 0) {
		std::cerr << "Can't create command queue\n";
		clReleaseContext(context);
		exit(1);
	}
	
	cl_mem cl_a = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_uint) * DATA_SIZE, &image_data[0], NULL);
	cl_mem cl_res = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_uint) * DATA_SIZE, NULL, NULL);
	if(cl_a == 0 || cl_res == 0) {
		std::cerr << "Can't create OpenCL buffer\n";
		clReleaseMemObject(cl_a);
		clReleaseMemObject(cl_res);
		clReleaseCommandQueue(queue);
		clReleaseContext(context);
		exit(1);
	}

	cl_program program = load_program(context, "shader.cl");
	if(program == 0) {
		std::cerr << "Can't load or build program\n";
		clReleaseMemObject(cl_a);
		clReleaseMemObject(cl_res);
		clReleaseCommandQueue(queue);
		clReleaseContext(context);
		exit(1);
	}

	cl_kernel adder = clCreateKernel(program, "adder", 0);
	if(adder == 0) {
		std::cerr << "Can't load kernel\n";
		clReleaseProgram(program);
		clReleaseMemObject(cl_a);
		clReleaseMemObject(cl_res);
		clReleaseCommandQueue(queue);
		clReleaseContext(context);
		exit(1);
	}

	clSetKernelArg(adder, 0, sizeof(cl_mem), &cl_a);
	clSetKernelArg(adder, 1, sizeof(cl_mem), &cl_res);

	size_t work_size = DATA_SIZE;
	err = clEnqueueNDRangeKernel(queue, adder, 1, 0, &work_size, 0, 0, 0, 0);

	unsigned int *res = (unsigned int*)calloc( DATA_SIZE , sizeof(unsigned int));
	if(err == CL_SUCCESS) {
		err = clEnqueueReadBuffer(queue, cl_res, CL_TRUE, 0, sizeof(unsigned int) * DATA_SIZE, &res[0], 0, 0, 0);
	}

	unsigned int * img = image_data;
	unsigned int * ref_histogram_results;
	unsigned int * ptr;
	ref_histogram_results = ( unsigned int *) malloc (256 * 3 * sizeof
			(unsigned int)) ;
	ptr = ref_histogram_results;
	memset ( ref_histogram_results , 0x0 , 256 * 3 * sizeof ( unsigned
				int));

	for( int i = 0 ; i < _size ; ++i ) {
		//cout << res[i] << endl;
		ref_histogram_results[ res[i] ]++;
	}
	/*
	// histogram of R
	for ( unsigned int i = 0; i < _size ; i += 3)
	{
		unsigned int index = img[i];
		ptr[index]++;
	}
	// histogram of G
	ptr += 256;
	for ( unsigned int i = 1; i < _size ; i += 3)
	{
		unsigned int index = img[i];
		ptr[index]++;
	}
	// histogram of B
	ptr += 256;
	for ( unsigned int i = 2; i < _size ; i += 3)
	{
		unsigned int index = img[i];
		ptr[index]++;
	}
	*/


	clReleaseKernel(adder);
	clReleaseProgram(program);
	clReleaseMemObject(cl_a);
	clReleaseMemObject(cl_res);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);

	return ref_histogram_results;
}
int main ( int argc , char const * argv [])
{
	unsigned int * histogram_results;
	unsigned int i =0 , a , input_size;
	std :: fstream inFile ( "input" , std::ios_base::in );
	std :: ofstream outFile ( "0456095.out" , std::ios_base::out );
	inFile >> input_size;
	unsigned int * image = new unsigned int [input_size];
	while ( inFile >> a ) {
		image[i++] = a;
	}
	histogram_results = histogram ( image , input_size );
	for ( unsigned int i = 0; i < 256 * 3; ++i ) {
		if ( i % 256 == 0 && i != 0)
			outFile << std::endl;
		outFile << histogram_results[i] << ' ';
	}
	inFile.close();
	outFile.close();
	return 0;
}
