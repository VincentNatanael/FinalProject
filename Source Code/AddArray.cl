__kernel void add_array(__global double* arr1, __global double* arr2, __global double* result) {
	uint i = get_global_id(0);
	result[i] = arr1[i] + arr2[i];
}

__kernel void add_to_array(__global double* arr, __global double* delta) {
	uint i = get_global_id(0);
	arr[i] += delta[i];
}