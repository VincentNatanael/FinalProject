#include "CLWrapper.hpp"

#include <fstream>
#include <string>

namespace {
	/// <summary>
	/// Read the content of a text file and put it into a string
	/// </summary>
	/// <param name="fileName">(the file name to be read)</param>
	/// <returns>An std::string with the file content</returns>
	std::string readFile(const char* fileName) {
		std::string result = "";
		std::string line = "";

		std::ifstream file;
		file.open(fileName, std::ios::in);
		if (!file.is_open()) return result;

		std::getline(file, result);
		while (std::getline(file, line)) result += "\n" + line;

		file.close();
		return result;
	}
}

bool startCL(const char* cl_code_filename, const char* cl_function_name,
	cl_uint dimension, size_t* work_size, size_t* local_size,
	CLArg* cl_args, cl_uint cl_arg_num, bool throw_log_on_build_error)
{
	std::string file = readFile(cl_code_filename);
	const char* cl_code = file.c_str();
	cl_device_id device = nullptr;
	cl_int result;

	cl_platform_id platforms[64];
	cl_uint platformCount;
	result = clGetPlatformIDs(64, platforms, &platformCount);

	if (result != CL_SUCCESS) return false;

	for (cl_uint i = 0; i < platformCount; i++) {
		char platformInfo[256];
		size_t platformInfoLength;

		//*
		result = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 256, (void*)platformInfo, &platformInfoLength);
		if (result != CL_SUCCESS) return false;
		/**/

		cl_device_id devices[64];
		cl_uint deviceCount;
		result = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 64, devices, &deviceCount);
		if (result != CL_SUCCESS) return false;

		for (cl_uint v = 0; v < deviceCount; v++) {
			char deviceInfo[256];
			size_t deviceInfoLength;
			result = clGetDeviceInfo(devices[v], CL_DEVICE_VENDOR, 256, deviceInfo, &deviceInfoLength);
			if (result == CL_SUCCESS && (
				std::string(deviceInfo).substr(0, deviceInfoLength) == "NVIDIA Corporation" ||
				std::string(platformInfo).substr(0, platformInfoLength) == "AMD Accelerated Parallel Processing" ||
				(platformCount == i + 1 && deviceCount == v + 1)
				)
				) {
				device = devices[v];
				break; break;
			}
		}
	}





	cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &result);
	if (result != CL_SUCCESS) return false;

	cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, 0, &result);
	if (result != CL_SUCCESS) return false;





	size_t length = 0;
	cl_program program = clCreateProgramWithSource(context, 1, &cl_code, &length, &result);
	if (result != CL_SUCCESS) return false;

	result = clBuildProgram(program, 1, &device, "", nullptr, nullptr);
	if (result != CL_SUCCESS) {
		if (throw_log_on_build_error) {
			char log[256];
			size_t logLength;
			result = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 256, log, &logLength);
			//if (result == CL_SUCCESS)
			throw(log);
		}
		else return false;
	}

	cl_kernel kernel = clCreateKernel(program, cl_function_name, &result);
	if (result != CL_SUCCESS) return false;

	cl_mem* buffers = new cl_mem[cl_arg_num];
	for (cl_uint i = 0; i < cl_arg_num; i++) {
		buffers[i] = clCreateBuffer(context, cl_args[i].flags, cl_args[i].byte_size, nullptr, &result);
		if (result != CL_SUCCESS) return false;
		if (cl_args[i].flags == CL_MEM_READ_ONLY || cl_args[i].flags == CL_MEM_READ_WRITE) {
			result = clEnqueueWriteBuffer(queue, buffers[i], CL_TRUE, 0, cl_args[i].byte_size, cl_args[i].values, 0, nullptr, nullptr);
			if (result != CL_SUCCESS) return false;
		}
		result = clSetKernelArg(kernel, i, sizeof(cl_mem), &buffers[i]);
		if (result != CL_SUCCESS) return false;
	}

	result = clEnqueueNDRangeKernel(queue, kernel, dimension, 0, work_size, local_size, 0, nullptr, nullptr);
	if (result != CL_SUCCESS) return false;

	for (cl_uint i = 0; i < cl_arg_num; i++) {
		if (cl_args[i].flags == CL_MEM_WRITE_ONLY || cl_args[i].flags == CL_MEM_READ_WRITE) {
			result = clEnqueueReadBuffer(queue, buffers[i], CL_TRUE, 0, cl_args[i].byte_size, cl_args[i].values, 0, nullptr, nullptr);
			if (result != CL_SUCCESS) return false;
		}
	}

	clFinish(queue);

	for (cl_uint i = 0; i < cl_arg_num; i++) {
		clReleaseMemObject(buffers[i]);
	}
	delete[] buffers;
	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);
	clReleaseDevice(device);

	return true;
}