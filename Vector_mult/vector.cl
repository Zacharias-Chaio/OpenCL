__kernel void findmax(__global float *data,
	  __global float *array,
	  __local float *temp,
      __global float *result) {

	int lid = get_local_id(0);
	int localSize = get_local_size(0);
		
	temp[lid] = data[get_global_id(0)] * array[get_global_id(0)];
	barrier(CLK_LOCAL_MEM_FENCE);

	for(int i = localSize/2; i > 0; i >>= 1){
		if(lid < i) 
		{
			temp[lid] = fmax(temp[lid], temp[lid+i]);
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	}
	if (lid == 0)
	{
		result[get_group_id(0)] = temp[0];
	}

}