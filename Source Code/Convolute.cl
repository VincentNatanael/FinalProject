__kernel void convolute(
	__global double* input, __global uint* input_size,
	__global double* layer, __global uint* layer_size,
	__global double* output
) {
	uint x = get_global_id(0);
	uint y = get_global_id(1);
	uint x_max = get_global_size(0);
	uint y_max = get_global_size(1);

	double result = 0;
	for (uint i = 0; i < layer_size[0]; i++) {
		for (uint v = 0; v < layer_size[1]; v++) {
			result += input[(x + i) * input_size[1] + y + v] * layer[i * layer_size[1] + v];
		}
	}

	output[x*get_global_size(1)+y] = result;
}