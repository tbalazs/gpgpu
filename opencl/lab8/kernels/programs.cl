__kernel void globalAddress(__global float* data){
  int id = get_global_id(0);
  data[id] = id;
}

__kernel void localAddress(__global float* data){
  int id = get_local_id(0);

  data[get_local_id(0) + get_group_id(0) * get_local_size(0)] = id;
}

__kernel void address2D(__global float4* data){
  int localIDX = get_local_id(0);
  int localIDY = get_local_id(1);
  int globalIDX = get_global_id(0);
  int globalIDY = get_global_id(1);

  data[globalIDX + get_global_size(0) * globalIDY] = (float4)(globalIDX, globalIDY, localIDX, localIDY);
}

__kernel void square(__global float* inputData,
		     __global float* outputData,
		     const int data_size){
  int id = get_global_id(0);
  outputData[id] = inputData[id] * inputData[id];
}

__kernel void function2D(__global float4* data){
  int2 id = (int2)(get_global_id(0), get_global_id(1));
  int2 globalSize = (int2)(get_global_size(0), get_global_size(1));

  float2 point = (float2)(id.x / (float)globalSize.x * 6.0, id.y / (float)globalSize.y * 6.0f);

  data[id.x + id.y * globalSize.x] = (float4)(id.x, id.y, sin(point.x) * cos(point.y), 0);
}
