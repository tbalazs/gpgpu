/* -*- mode: c++ -*- */

// Linear Congruential Generator
uint stepLCG(uint *z, uint A, uint C){
  return (*z) = (A * (*z) + C);
}

__kernel
void randomLCG(const int randomNumbers, __global float* randomsSeed, __global float* randomGPU){
  int id = get_global_id(0);
  int maxID = get_global_size(0);

  uint rng = randomsSeed[id];
  for(int i=0; i < randomNumbers; ++i){
    randomGPU[id + i * maxID] =  (float)stepLCG(&rng, 1664525, 1013904223UL) / 0xffffffff;
  }
}

uint stepLFG(uint *z, __global uint *znmk, uint A, uint C){
  return (*znmk) = (*z) = (A * (*z) + C) + (*znmk);
}

// Lagged Fibonacci Generator
__kernel
void randomLFG(const int randomNumbers, __global float* randomsSeed, const int randomStateSize, __global uint* randomState, __global float* randomGPU){
  int id = get_global_id(0);
  int maxID = get_global_size(0);

  // bootstrap
  uint rng = randomsSeed[id];
  for(int i=0; i < randomStateSize; ++i){
    randomState[id + i * maxID] =  stepLCG(&rng, 1664525, 1013904223UL);
  }

  // Lagged Fibonacci Generator
  int nmkIndex = 0;
  for(int i=0; i < randomNumbers; ++i){
    randomGPU[id + i * maxID] = (float)stepLFG(&rng, &randomState[nmkIndex], 1664525, 1013904223UL) / 0xffffffff;
    nmkIndex = (nmkIndex + 1) % randomStateSize;
  }

}

// Combined Tausworthe Generator
uint stepCTG(uint *z, uint S1, uint S2, uint S3, uint M){
  uint b=((((*z)<<S1)^(*z))>>S2);
  return (*z) = ((((*z)&M)<<S3)^b);
}

__kernel
void randomCTG(const int randomNumbers, __global float2* randomsSeed, __global float* randomGPU){
  int id = get_global_id(0);
  int maxID = get_global_size(0);

  uint rng1 = randomsSeed[id].x;
  uint rng2 = randomsSeed[id].y;
  for(int i=0; i < randomNumbers; ++i){
    randomGPU[id + i * maxID] =  (float)(stepCTG(&rng1, 13, 19, 12, 4294967294UL) ^
					 stepCTG(&rng2, 2, 25, 4, 4294967288UL)) / 0xffffffff;
  }
}

// Hybrid RNG
float stepHybrid(uint* rng1, uint* rng2, uint* rng3, uint* rng4){
  return 2.3283064365387e-10 * (
				stepCTG(rng1, 13, 19, 12, 4294967294UL) ^
				stepCTG(rng2, 2, 25, 4, 4294967288UL) ^
				stepCTG(rng3, 3, 11, 17, 4294967280UL) ^
				stepLCG(rng4,1664525,1013904223UL)
				);
}

__kernel
void hybridRNG(const int randomNumbers, __global float* randomsSeed, __global float* randomGPU){
  int id = get_global_id(0);
  int maxID = get_global_size(0);

  uint rng1 = randomsSeed[id * 4 + 0];
  uint rng2 = randomsSeed[id * 4 + 1];
  uint rng3 = randomsSeed[id * 4 + 2];
  uint rng4 = randomsSeed[id * 4 + 3];

  for(int i = 0; i < randomNumbers; ++i){
    randomGPU[id + i * maxID] =  (float)stepHybrid(&rng1, &rng2, &rng3, &rng4);
  }
}

// Halton sequence
float stepHalton(float *value, float inv_base){
  float r = 1.0 - (*value) - 0.0000000001;
  if(inv_base < r) {
    (*value) += inv_base;
  } else {
    float h = inv_base, hh;
    do{
      hh = h;
      h *= inv_base;
    } while (h >= r);
    (*value) += hh + h - 1.0;
  }
  return (*value);
}

void seedHalton(ulong i, int base, float* inv_base, float* value){
  float f = (*inv_base) = 1.0/base;
  (*value) = 0.0;
  while( i > 0){
    (*value) += f * (float)(i % base);
    i /= base;
    f *= (*inv_base);
  }
}

__kernel
void haltonSequence(const int randomNumbers, const int base, __global float* randomGPU){
  int id = get_global_id(0);
  int maxID = get_global_size(0);

  float inv_base = 0.0;
  float rng = 0.0;
  seedHalton(id * randomNumbers, base, &inv_base, &rng);

  for(int i=0; i < randomNumbers; ++i){
    randomGPU[id + i * maxID] =  stepHalton(&rng, inv_base);
  }
}

// 1D uniformity test
__kernel
void testUniform1D(const int randomNums, __global float* randoms, const int bucketNum, __global int* buckets){
  int id = get_local_id(0);
  int maxID = get_global_size(0);

  for(int i=0; i<bucketNum; ++i){
    buckets[id + i * maxID] = 0;
  }

  for(int i=0; i < randomNums; ++i){
    int bucket = floor(randoms[id + i * maxID] / (1.0 / bucketNum));
    buckets[id + bucket * maxID] += 1;
  }
}

// 1D Monte-Carlo integral
#define M_PIP2 1.57796327f
__kernel
void mcInt1D(const int sampleNumber, __global float4* seed, __global float* integral){
  int id = get_global_id(0);

  uint rng1 = seed[id].x;
  uint rng2 = seed[id].y;
  uint rng3 = seed[id].z;
  uint rng4 = seed[id].w;

  float w = 1.0f / sampleNumber;
  float partialIntegral = 0.0f;
  for(int i = 0; i < sampleNumber; ++i){
    float rnd = (float)stepHybrid(&rng1, &rng2, &rng3, &rng4);
    partialIntegral += sin(rnd * M_PIP2) * w * M_PIP2;
  }

  integral[id] = partialIntegral;
}
