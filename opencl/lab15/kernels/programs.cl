__kernel
void map(__global float* data)
{
        int id = get_global_id(0);
        float square = data[id] * data[id];
        data[id] = square;
}

__kernel
void histogram_global(__global int* data, __global int* histogram)
{
        int id = get_global_id(0);
        //histogram[data[id]] += 1;
        atomic_add(&histogram[data[id]], 1);
}

__kernel
void histogram_local(__global int* data, __global int* histogram, __local int* lhistogram, const int histogramSize)
{
        int id = get_global_id(0);
        int lid = get_local_id(0);

        if(lid < histogramSize)
        {
                lhistogram[lid] = 0;
        }
        barrier(CLK_LOCAL_MEM_FENCE);

        atomic_add(&lhistogram[data[id]], 1);

        barrier(CLK_LOCAL_MEM_FENCE);

        if(lid < histogramSize)
        {
                histogram[lid] = lhistogram[lid];
        }
}

__kernel
void reduce_global(__global float* data)
{
        int id = get_global_id(0);

        for(unsigned int s = get_global_size(0) / 2; s > 0; s >>= 1)
        {
                if(id < s)
                {
                        data[id] = max(data[id], data[id + s]);
                }
                barrier(CLK_GLOBAL_MEM_FENCE);
        }
}

__kernel
void exscan_global(__global int* data)
{
        int id = get_global_id(0);
        data[id] = (id > 0) ? data[id-1] : 0;
        barrier(CLK_GLOBAL_MEM_FENCE);

        for(int s = 1; s < get_global_size(0); s *= 2)
        {
                int tmp = data[id];
                if(id + s < get_global_size(0))
                {
                        data[id + s] += tmp;
                }
                barrier(CLK_GLOBAL_MEM_FENCE);
        }
        if(id == 0)
        {
                data[0] = 0;
        }
}

__kernel
void compact_predicate(__global int* data, __global int* pred)
{
        int id = get_global_id(0);
        int predVal = data[id] < 50 ? 1 : 0;
        pred[id] = predVal;
}

__kernel
void compact_exscan(__global int* pred, __global int* prefSum)
{
        int id = get_global_id(0);
        prefSum[id] = (id > 0) ? pred[id-1] : 0;
        barrier(CLK_GLOBAL_MEM_FENCE);

        for(int s = 1; s < get_global_size(0); s *= 2)
        {
                int tmp = prefSum[id];
                if(id + s < get_global_size(0))
                {
                        prefSum[id + s] += tmp;
                }
                barrier(CLK_GLOBAL_MEM_FENCE);
        }
        if(id == 0)
        {
                prefSum[0] = 0;
        }
}

__kernel
void compact_compact(__global int* data, __global int* pred, __global int* prefSum)
{
        int id = get_global_id(0);
        int val = data[id];
        barrier(CLK_GLOBAL_MEM_FENCE);
        if(pred[id] == 1)
        {
                data[prefSum[id]]=val;
        }
}