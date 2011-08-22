/* -*- mode: c++ -*- */

__constant float dt = 0.1f;

__kernel
void resetSimulation(const int gridResolution,
		     __global float2* velocityBuffer,
		     __global float* pressureBuffer,
		     __global float4* densityBuffer){
  int2 id = (int2)(get_global_id(0), get_global_id(1));

  if( id.x < gridResolution && id.y < gridResolution){
    velocityBuffer[id.x + id.y * gridResolution] = (float2)(0.0f);
    pressureBuffer[id.x + id.y * gridResolution] = 0.0f;
    densityBuffer[id.x + id.y * gridResolution] = (float4)(0.0f);
  }
}

// bilinear interpolation
float2 getBil(float2 p, int gridResolution, __global float2* buffer){
  p = clamp(p, (float2)(0.0f), (float2)(gridResolution));

  float2 p00 = buffer[(int)(p.x) + (int)(p.y) * gridResolution];
  float2 p10 = buffer[(int)(p.x) + 1 + (int)(p.y) * gridResolution];
  float2 p11 = buffer[(int)(p.x) + 1 + (int)(p.y + 1.0f) * gridResolution];
  float2 p01 = buffer[(int)(p.x) + (int)(p.y + 1.0f) * gridResolution];

  __private float flr;
  float t0 = fract(p.x, &flr);
  float t1 = fract(p.y, &flr);

  float2 v0 = mix(p00, p10, t0);
  float2 v1 = mix(p01, p11, t0);

  return mix(v0, v1, t1);
}

float4 getBil4(float2 p, int gridResolution, __global float4* buffer){
  p = clamp(p, (float2)(0.0f), (float2)(gridResolution));

  float4 p00 = buffer[(int)(p.x) + (int)(p.y) * gridResolution];
  float4 p10 = buffer[(int)(p.x) + 1 + (int)(p.y) * gridResolution];
  float4 p11 = buffer[(int)(p.x) + 1 + (int)(p.y + 1.0f) * gridResolution];
  float4 p01 = buffer[(int)(p.x) + (int)(p.y + 1.0f) * gridResolution];

  __private float flr;
  float t0 = fract(p.x, &flr);
  float t1 = fract(p.y, &flr);

  float4 v0 = mix(p00, p10, t0);
  float4 v1 = mix(p01, p11, t0);

  return mix(v0, v1, t1);
}

__kernel
void advection(const int gridResolution,
	       __global float2* inputVelocityBuffer,
	       __global float2* outputVelocityBuffer){
  int2 id = (int2)(get_global_id(0), get_global_id(1));

  if(id.x > 0 && id.x < gridResolution - 1 &&
     id.y > 0 && id.y < gridResolution - 1){
    float2 velocity = inputVelocityBuffer[id.x + id.y * gridResolution];

    float2 p = (float2)((float)id.x - dt * velocity.x, (float)id.y - dt * velocity.y);

    outputVelocityBuffer[id.x + id.y * gridResolution] = getBil(p, gridResolution, inputVelocityBuffer);
  } else {
    if(id.x == 0) outputVelocityBuffer[id.x + id.y * gridResolution] = -inputVelocityBuffer[id.x + 1 + id.y * gridResolution];
    if(id.x == gridResolution - 1) outputVelocityBuffer[id.x + id.y * gridResolution] = -inputVelocityBuffer[id.x - 1 + id.y * gridResolution];
    if(id.y == 0) outputVelocityBuffer[id.x + id.y * gridResolution] = -inputVelocityBuffer[id.x + 1 + (id.y + 1) * gridResolution];
    if(id.y == gridResolution - 1) outputVelocityBuffer[id.x + id.y * gridResolution] = -inputVelocityBuffer[id.x + 1 + (id.y - 1) * gridResolution];
  }
}

__kernel
void advectionDensity(const int gridResolution,
		      __global float2* velocityBuffer,
		      __global float4* inputDensityBuffer,
		      __global float4* outputDensityBuffer){
  int2 id = (int2)(get_global_id(0), get_global_id(1));

  if(id.x > 0 && id.x < gridResolution - 1 &&
     id.y > 0 && id.y < gridResolution - 1){
    float2 velocity = velocityBuffer[id.x + id.y * gridResolution];

    float2 p = (float2)((float)id.x - dt * velocity.x, (float)id.y - dt * velocity.y);

    outputDensityBuffer[id.x + id.y * gridResolution] = getBil4(p, gridResolution, inputDensityBuffer);
  } else {
    outputDensityBuffer[id.x + id.y * gridResolution] = 0.0f;
  }
}

__kernel
void diffusion(const int gridResolution,
	       __global float2* inputVelocityBuffer,
	       __global float2* outputVelocityBuffer){
  int2 id = (int2)(get_global_id(0), get_global_id(1));

  float viscousity = 0.01f;
  float alpha = 1.0f / (viscousity * dt);
  float beta = 1.0f / (4.0f + alpha);

  if(id.x > 0 && id.x < gridResolution - 1 &&
     id.y > 0 && id.y < gridResolution - 1){
    float2 vL = inputVelocityBuffer[id.x - 1 + id.y * gridResolution];
    float2 vR = inputVelocityBuffer[id.x + 1 + id.y * gridResolution];
    float2 vB = inputVelocityBuffer[id.x + (id.y - 1) * gridResolution];
    float2 vT = inputVelocityBuffer[id.x + (id.y + 1) * gridResolution];

    float2 velocity = inputVelocityBuffer[id.x + id.y * gridResolution];

    outputVelocityBuffer[id.x + id.y * gridResolution] = (vL + vR + vB + vT + alpha * velocity) * beta;
  } else {
    outputVelocityBuffer[id.x + id.y * gridResolution] = inputVelocityBuffer[id.x + id.y * gridResolution];
  }
}

__kernel
void vorticity(const int gridResolution, __global float2* velocityBuffer,
	       __global float* vorticityBuffer){
  int2 id = (int2)(get_global_id(0), get_global_id(1));

  if(id.x > 0 && id.x < gridResolution - 1 &&
     id.y > 0 && id.y < gridResolution - 1){
    float2 vL = velocityBuffer[id.x - 1 + id.y * gridResolution];
    float2 vR = velocityBuffer[id.x + 1 + id.y * gridResolution];
    float2 vB = velocityBuffer[id.x + (id.y - 1) * gridResolution];
    float2 vT = velocityBuffer[id.x + (id.y + 1) * gridResolution];

    vorticityBuffer[id.x + id.y * gridResolution] = (vR.y - vL.y) - (vT.x - vB.x);
  } else {
    vorticityBuffer[id.x + id.y * gridResolution] = 0.0f;
  }
}

__kernel
void addVorticity(const int gridResolution, __global float* vorticityBuffer,
		  __global float2* velocityBuffer){
  int2 id = (int2)(get_global_id(0), get_global_id(1));

  const float scale = 0.2f;

  if(id.x > 0 && id.x < gridResolution - 1 &&
     id.y > 0 && id.y < gridResolution - 1){
    float vL = vorticityBuffer[id.x - 1 + id.y * gridResolution];
    float vR = vorticityBuffer[id.x + 1 + id.y * gridResolution];
    float vB = vorticityBuffer[id.x + (id.y - 1) * gridResolution];
    float vT = vorticityBuffer[id.x + (id.y + 1) * gridResolution];

    float4 gradV = (float4)(vR - vL, vT - vB, 0.0f, 0.0f);
    float4 z = (float4)(0.0f, 0.0f, 1.0f, 0.0f);

    if(dot(gradV, gradV)){
      float4 vorticityForce = scale * cross(gradV, z);
      velocityBuffer[id.x + id.y * gridResolution] += vorticityForce.xy * dt;
    }
  }
}

__kernel
void divergence(const int gridResolution, __global float2* velocityBuffer,
		__global float* divergenceBuffer){
  int2 id = (int2)(get_global_id(0), get_global_id(1));

  if(id.x > 0 && id.x < gridResolution - 1 &&
     id.y > 0 && id.y < gridResolution - 1){
    float2 vL = velocityBuffer[id.x - 1 + id.y * gridResolution];
    float2 vR = velocityBuffer[id.x + 1 + id.y * gridResolution];
    float2 vB = velocityBuffer[id.x + (id.y - 1) * gridResolution];
    float2 vT = velocityBuffer[id.x + (id.y + 1) * gridResolution];

    divergenceBuffer[id.x + id.y * gridResolution] = 0.5f * ((vR.x - vL.x) + (vT.y - vB.y));
  } else {
    divergenceBuffer[id.x + id.y * gridResolution] = 0.0f;
  }
}

__kernel
void pressureJacobi(const int gridResolution,
		    __global float* inputPressureBuffer,
		    __global float* outputPressureBuffer,
		    __global float* divergenceBuffer){
  int2 id = (int2)(get_global_id(0), get_global_id(1));

  if(id.x > 0 && id.x < gridResolution - 1 &&
     id.y > 0 && id.y < gridResolution - 1){

    float alpha = -1.0f;
    float beta = 0.25f;

    float vL = inputPressureBuffer[id.x - 1 + id.y * gridResolution];
    float vR = inputPressureBuffer[id.x + 1 + id.y * gridResolution];
    float vB = inputPressureBuffer[id.x + (id.y - 1) * gridResolution];
    float vT = inputPressureBuffer[id.x + (id.y + 1) * gridResolution];

    float divergence = divergenceBuffer[id.x + id.y * gridResolution];

    outputPressureBuffer[id.x + id.y * gridResolution] = (vL + vR + vB + vT + alpha * divergence) * beta;
  } else {
    if(id.x == 0) outputPressureBuffer[id.x + id.y * gridResolution] = inputPressureBuffer[id.x + 1 + id.y * gridResolution];
    if(id.x == gridResolution - 1) outputPressureBuffer[id.x + id.y * gridResolution] = inputPressureBuffer[id.x - 1 + id.y * gridResolution];
    if(id.y == 0) outputPressureBuffer[id.x + id.y * gridResolution] = inputPressureBuffer[id.x + (id.y + 1) * gridResolution];
    if(id.y == gridResolution - 1) outputPressureBuffer[id.x + id.y * gridResolution] = inputPressureBuffer[id.x + (id.y - 1) * gridResolution];
  }
}

__kernel
void projection(const int gridResolution,
		__global float2* inputVelocityBuffer,
		__global float* pressureBuffer,
		__global float2* outputVelocityBuffer){
  int2 id = (int2)(get_global_id(0), get_global_id(1));

  if(id.x > 0 && id.x < gridResolution - 1 &&
     id.y > 0 && id.y < gridResolution - 1){
    float pL = pressureBuffer[id.x - 1 + id.y * gridResolution];
    float pR = pressureBuffer[id.x + 1 + id.y * gridResolution];
    float pB = pressureBuffer[id.x + (id.y - 1) * gridResolution];
    float pT = pressureBuffer[id.x + (id.y + 1) * gridResolution];

    float2 velocity = inputVelocityBuffer[id.x + id.y * gridResolution];

    outputVelocityBuffer[id.x + id.y * gridResolution] = velocity -  /* 0.5f **//* (1.0f / 256.0f) **/ (float2)(pR - pL, pT - pB);
  } else {
    if(id.x == 0) outputVelocityBuffer[id.x + id.y * gridResolution] = -inputVelocityBuffer[id.x + 1 + id.y * gridResolution];
    if(id.x == gridResolution - 1) outputVelocityBuffer[id.x + id.y * gridResolution] = -inputVelocityBuffer[id.x - 1 + id.y * gridResolution];
    if(id.y == 0) outputVelocityBuffer[id.x + id.y * gridResolution] = -inputVelocityBuffer[id.x + 1 + (id.y + 1) * gridResolution];
    if(id.y == gridResolution - 1) outputVelocityBuffer[id.x + id.y * gridResolution] = -inputVelocityBuffer[id.x + 1 + (id.y - 1) * gridResolution];
  }
}

__kernel
void addForce(const float x, const float y, const float2 force,
	      const int gridResolution, __global float2* velocityBuffer,
	      const float4 density, __global float4* densityBuffer){
  int2 id = (int2)(get_global_id(0), get_global_id(1));

  float dx = ((float)id.x / (float)gridResolution) - x;
  float dy = ((float)id.y / (float)gridResolution) - y;

  float radius = 0.001f;

  float c = exp( - (dx * dx + dy * dy) / radius ) * dt;

  velocityBuffer[id.x + id.y * gridResolution] += c * force;
  densityBuffer[id.x + id.y * gridResolution] += c * density;
}

// *************
// Visualization
// *************

__kernel
void visualizationDensity(const int width, const int height, __global float4* visualizationBuffer,
			  const int gridResolution, __global float4* densityBuffer){
  int2 id = (int2)(get_global_id(0), get_global_id(1));

  if( id.x < width && id.y < height){
    float4 density = densityBuffer[id.x + id.y * width];
    visualizationBuffer[id.x + id.y * width] = density;
  }
}

__kernel
void visualizationVelocity(const int width, const int height, __global float4* visualizationBuffer,
			   const int gridResolution, __global float2* velocityBuffer){
  int2 id = (int2)(get_global_id(0), get_global_id(1));

  if( id.x < width && id.y < height){
    float2 velocity = velocityBuffer[id.x + id.y * width];
    visualizationBuffer[id.x + id.y * width] = (float4)((1.0f + velocity)/2.0f, 0.0f, 0.0f);
  }
}

__kernel
void visualizationPressure(const int width, const int height, __global float4* visualizationBuffer,
			   const int gridResolution, __global float* pressureBuffer){
  int2 id = (int2)(get_global_id(0), get_global_id(1));

  if( id.x < width && id.y < height){
    float pressure = pressureBuffer[id.x + id.y * width];
    visualizationBuffer[id.x + id.y * width] = (float4)((1.0f + pressure)/2.0f);
  }
}
