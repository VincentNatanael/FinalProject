__kernel void copy_array(__global double* source, __global double* target) {
	uint i = get_global_id(0);
	target[i] = source[i];
}