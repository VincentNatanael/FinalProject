__kernel void relu(__global double* values) {
	uint i = get_global_id(0);
	if (values[i] < 0) values[i] *= 0.5;
}