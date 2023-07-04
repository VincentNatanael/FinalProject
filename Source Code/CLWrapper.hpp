#pragma once

#include <CL/cl.h>



/// <summary>
/// A struct for CL memory buffer and its parameters that will be passed as argument to the CL kernel by the wrapper.
/// </summary>
struct CLArg {
	void* values;
	size_t byte_size;
	cl_mem_flags flags;
};



/// <summary>
/// A simple wrapper for running an OpenCL kernel function.
/// The first NVIDIA or AMD GPU found will be used to run the kernel.
/// If no NVIDIA or AMD GPU is found, the last GPU found will be used instead.
/// </summary>
/// <param name="cl_code_filename">(the text file containing the CL kernel code)</param>
/// <param name="cl_function_name">(the name of the CL kernel function that will be executed)</param>
/// <param name="dimension">(dimension of the work load)</param>
/// <param name="work_size">(an array containing global work size for each dimension)</param>
/// <param name="local_size">(an array containing local work size for each dimension)</param>
/// <param name="cl_args">(an array of CLArg to be passed as CL kernel arguments)</param>
/// <param name="cl_arg_num">(the number of arguments passed)</param>
/// <param name="throw_log_on_build_error">(if true, the function will throw the build error log instead of returning false)</param>
/// <returns>A boolean that indicates if the kernel successfully run</returns>
bool startCL(const char* cl_code_filename, const char* cl_function_name,
	cl_uint dimension, size_t* work_size, size_t* local_size,
	CLArg* cl_args, cl_uint cl_arg_num, bool throw_log_on_build_error = false
);