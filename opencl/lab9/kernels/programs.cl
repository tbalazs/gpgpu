__kernel
void simpleMV(const int n, const int m, __global float* y, __global float* A, __global float* x, __global float* b){
  int i = get_global_id(0);
  if(i < n){
    float yi = b[i];
    for(int j = 0; j < m; ++j){
      yi += A[j + i * m ] * x[j];
    }
    y[i] = yi;
  }
}

#define M 32
__kernel
void reduceMV(const int n, __global float* y, __global float* A, __global float* x, __global float* b){
  int i = get_group_id(0);
  int j = get_local_id(0);

  __local float Q[M];

  Q[j] = A[i * M + j] * x[j];

  for(int stride = M / 2; stride > 0; stride >>= 1){
    barrier(CLK_LOCAL_MEM_FENCE);
    if(j + stride < M){
      Q[j] += Q[j + stride];
    }
  }

  if(j == 0){
    y[i] = Q[0] + b[i];
  }
}

#define T 8
#define Z 2
__kernel
void largeMV(const int n, const int m, __global float* y, __global float* A, __global float* x, __global float* b){
  __local float Q[T * Z];

  int t = get_local_id(0) / Z;
  int z = get_local_id(0) % Z;

  for(int i = t; i < n; i += T){
    Q[t * Z + z] = 0.0f;
    for(int j = z; j < m; j+=Z){
      Q[t * Z + z] += A[j + i * m] * x[j];
    }

    for(int stride = Z / 2; stride > 0; stride >>= 1){
      barrier(CLK_LOCAL_MEM_FENCE);
      if(z + stride < Z){
	Q[t * Z + z] += Q[t * Z + z + stride];
      }
    }

    if(z == 0){
      y[i] = Q[t * Z + 0] + b[i];
    }
  }
}

__kernel void gaussian(const int n, const int m, __global float* A){
  int id = get_local_id(0);

  for(int ma = 0; ma < m; ++ma){
    float pp = A[ma + ma * n];
    float coeff = A[ma + id * n] / pp;
    barrier(CLK_GLOBAL_MEM_FENCE);
    if(id != ma){
      for(int na = 0; na < n; ++na){
	A[na + id * n] = A[na + id * n] - coeff * A[na + n * ma];
      }
    }
    barrier(CLK_GLOBAL_MEM_FENCE);
  }

  float coeff = A[id + id * n];
  for(int na = 0; na < n; ++na){
    A[na + id * n] = A[na + id * n] / coeff;
  }
}

