__kernel void adder(__global const unsigned int* a, __global unsigned int* result)
{
	int idx = get_global_id(0);
	result[idx] = a[idx] + 256 * (idx%3);
}
