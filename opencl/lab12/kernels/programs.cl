/* -*- mode: c++ -*- */
__kernel
void resetSimulation(const int resolution, __global float* simulationBuffer){
  int4 id = (int4)(get_global_id(0), get_global_id(1), get_global_id(2), 0);
  simulationBuffer[id.x + id.y * resolution + id.z * resolution * resolution] = 0.0f;
}

// intersect ray with a box
// http://www.siggraph.org/education/materials/HyperGraph/raytrace/rtinter3.htm

int intersectBox(float4 r_o, float4 r_d, float4 boxmin, float4 boxmax, float *tnear, float *tfar)
{
    // compute intersection of ray with all six bbox planes
    float4 invR = (float4)(1.0f,1.0f,1.0f,1.0f) / r_d;
    float4 tbot = invR * (boxmin - r_o);
    float4 ttop = invR * (boxmax - r_o);

    // re-order intersections to find smallest and largest on each axis
    float4 tmin = min(ttop, tbot);
    float4 tmax = max(ttop, tbot);

    // find the largest tmin and the smallest tmax
    float largest_tmin = max(max(tmin.x, tmin.y), max(tmin.x, tmin.z));
    float smallest_tmax = min(min(tmax.x, tmax.y), min(tmax.x, tmax.z));

        *tnear = largest_tmin;
        *tfar = smallest_tmax;

        return smallest_tmax > largest_tmin;
}

struct module
{
    float4 origin;
    float4 axial;
    float4 transAxial;
    float4 n;
};

int getLorIndex( int pair, int axial1, int axial2, int transAxial1, int transAxial2, int nAxial, int nTransAxial )
{
	int lorNumber = nAxial*nAxial*nTransAxial*nTransAxial;
	int u = axial1 * nTransAxial + transAxial1;
	int v = axial2 * nTransAxial + transAxial2;
	return lorNumber*pair + u*nAxial*nTransAxial + v;
}

int2 getModules(int pair)
{
	switch ( pair )
	{
		case 0: return (int2)(0,2);
		case 1: return (int2)(1,3);
	};
	return (int2)(0,0);
}

float getIntensity(const float4 p, const int resolution, __global float* intensityBuffer){
  int x = p.x * resolution;
  int y = p.y * resolution;
  int z = p.z * resolution;

  if(x > resolution -1 || x < 0) return(0.0f);
  if(y > resolution -1 || y < 0) return(0.0f);
  if(z > resolution -1 || z < 0) return(0.0f);
  
  return intensityBuffer[x + y * resolution + z * resolution * resolution];
}

__kernel
void parallelProjection(
	const int nAxial,
	const int nTransAxial,
	__global float* inputLors,
	__global float* outputLors
){
	int iPair = get_global_id(0);
	int iAxial = get_global_id(1);
	int iTransAxial = get_global_id(2);
	
	// a két panel tranzaxiális iránya egymás tükörképe!
	int lorIndex = getLorIndex( iPair, iAxial, iAxial, iTransAxial, nTransAxial-iTransAxial-1, nAxial, nTransAxial );
	int parallelIndex = iAxial + iTransAxial*nAxial + iPair*nAxial*nTransAxial;
	
	outputLors[parallelIndex] = inputLors[lorIndex];
}

__kernel
void forwardProjection(
	const int nAxial,
	const int nTransAxial,
	const int resolution,
	const float detectorArea,
	float4 volumeMin,
	float4 volumeMax,
	__global struct module* modules,
	__global float* measuredLors,
	__global float* estimatedLors,
	__global float* volumeBuffer,
	int measurementGeneration
){
	int iPair = get_global_id(0);
	int iAxial1 = get_global_id(1) % nAxial;
	int iTransAxial1 = get_global_id(1) / nAxial;
	int iAxial2 = get_global_id(2) % nAxial;
	int iTransAxial2 = get_global_id(2) / nAxial;
	
	int lorIndex = getLorIndex( iPair, iAxial1, iAxial2, iTransAxial1, iTransAxial2, nAxial, nTransAxial );
	
	float y_estimated = 0.0f;
	int2 iModule = getModules(iPair);
	float4 z1 = modules[iModule.x].origin + modules[iModule.x].transAxial*(iTransAxial1+0.5f)/nTransAxial + modules[iModule.x].axial*(iAxial1+0.5f)/nAxial;
	float4 z2 = modules[iModule.y].origin + modules[iModule.y].transAxial*(iTransAxial2+0.5f)/nTransAxial + modules[iModule.y].axial*(iAxial2+0.5f)/nAxial;
	float4 dir = z2-z1;
	
	float tnear, tfar;
	
	if ( intersectBox( z1, dir, volumeMin, volumeMax, &tnear, &tfar ) )
	{
		float G = -detectorArea*detectorArea * dot(modules[iModule.x].n,dir)*dot(modules[iModule.y].n,dir) / (2.0f*M_PI*dot(dir,dir)*dot(dir,dir));
	
		float4 start = z1+tnear*dir;
		float4 end = z1+tfar*dir;
		float4 step = (end - start) / resolution;		// step of the ray marching
		float dl = length(step);						// step length of the ray marching
		
		float4 voxel = start;
		for ( int i = 0; i < resolution; ++i )
		{
			float x = getIntensity( (voxel - volumeMin) / (volumeMax - volumeMin), resolution, volumeBuffer );
			y_estimated += G*x*dl;
			voxel += step;
		}
	}
	
	if ( 0.0f != y_estimated )
	{
		if ( measurementGeneration )
			measuredLors[lorIndex] = y_estimated;
		else
			estimatedLors[lorIndex] = measuredLors[lorIndex] / y_estimated;
	}
	else
		estimatedLors[lorIndex] = 0.0f;
}

__kernel
void backProjection(
	const int nPairs,
	const int nAxial,
	const int nTransAxial,
	const int resolution,
	float4 volumeMin,
	float4 volumeMax,
	__global struct module* modules,
	__global float* estimatedLors,
	__global float* volumeBuffer
){
	int4 iVoxel = (int4)(get_global_id(0),get_global_id(1),get_global_id(2),0);
	int linearIndex = iVoxel.x + iVoxel.y*resolution + iVoxel.z*resolution*resolution;
	float4 voxel = (float4)(iVoxel.x,iVoxel.y,iVoxel.z,0);
	voxel /= resolution;                                 // [0,1]
	voxel = (volumeMax-volumeMin)*voxel + volumeMin;     // [volumeMin,VolumeMax]
	
	float numerator = 0.0f;
	float denominator = 0.0f;
	for ( int iPair = 0; iPair < nPairs; ++iPair )
	{
		int2 iModule = getModules(iPair);
		struct module primary = modules[iModule.x];     // far detector
		struct module secondary = modules[iModule.y];   // near detector
		bool switchDetectors = false;
		if ( dot(voxel-primary.origin,primary.n) < dot(voxel-secondary.origin,secondary.n) )
		{
			switchDetectors = true;
			struct module tmp;
			tmp = primary;
			primary = secondary;
			secondary = tmp;
		}
		
		float sTransAxial = nTransAxial / dot(secondary.transAxial,secondary.transAxial);
		float sAxial = nAxial / dot(secondary.axial,secondary.axial);
		for ( int p = 0; p < nAxial; ++p )
			for ( int q = 0; q < nTransAxial; ++q )
			{
				float4 z1 = primary.origin + primary.axial*(p+0.5f)/nAxial + primary.transAxial*(q+0.5f)/nTransAxial;
				float4 dv = voxel - z1;
				// using similar triangle for projection
				float t = dot(secondary.n, secondary.origin-z1) / dot(secondary.n,dv);
				float4 z2 = z1 + dv*t;
				
				float fr = dot(z2-secondary.origin,secondary.axial) * sAxial;
				float fs = dot(z2-secondary.origin,secondary.transAxial) * sTransAxial;
				if ( 0 <= fr && fr < nAxial && 
					0 <= fs && fs < nTransAxial )
				{
					int r = (int)fr;
					int s = (int)fs;
					
					int lorIndex;
					if ( switchDetectors )
					{
						lorIndex = getLorIndex( iPair, r, p, s, q, nAxial, nTransAxial );
					}
					else
					{
						lorIndex = getLorIndex( iPair, p, r, q, s, nAxial, nTransAxial );
					}
					float y_div_yw = estimatedLors[lorIndex];
					float ddv = length(dv);
					float Alv = dot(primary.n,dv) / (ddv*ddv*ddv);
					
					numerator += y_div_yw * Alv;
					denominator += Alv;
				}
			}
	}
	
	if ( denominator > 0.0f )
	{
		volumeBuffer[linearIndex] *= numerator / denominator;
	}
	else
	{
		volumeBuffer[linearIndex] = 0.0f;
	}
}

//**************
// Visualization
//**************

float getIllumination(const float4 p, const int resolution, __global float* simulationBuffer){
  int x = p.x * resolution;
  int y = p.y * resolution;
  int z = p.z * resolution;

  if(x > resolution -1 || x < 0) return(0.0f);
  if(y > resolution -1 || y < 0) return(0.0f);
  if(z > resolution -1 || z < 0) return(0.0f);

  return simulationBuffer[x + y * resolution + z * resolution * resolution];
}

struct photon
{
	float4 origin;
	float4 direction;
};

__kernel
void visualization(const int width, const int height, __global float4* visualizationBuffer,
                   const int resolution, __global float* simulationBuffer,
				   const int iteration, const float16 invViewMatrix){
  int2 id = (int2)(get_global_id(0), get_global_id(1));

  float2 uv = (float2)( (id.x / (float) width)*2.0f-1.0f, (id.y / (float) height)*2.0f-1.0f );

  float4 boxMin = (float4)(-1.0f, -1.0f, -1.0f,1.0f);
  float4 boxMax = (float4)(1.0f, 1.0f, 1.0f,1.0f);

  // calculate eye ray in world space
  struct photon eyeRay;

  eyeRay.origin = (float4)(invViewMatrix.sC, invViewMatrix.sD, invViewMatrix.sE, invViewMatrix.sF);

  float4 temp = normalize(((float4)(uv.x, uv.y, -2.0f, 0.0f)));
  eyeRay.direction.x = dot(temp, ((float4)(invViewMatrix.s0,invViewMatrix.s1,invViewMatrix.s2,invViewMatrix.s3)));
  eyeRay.direction.y = dot(temp, ((float4)(invViewMatrix.s4,invViewMatrix.s5,invViewMatrix.s6,invViewMatrix.s7)));
  eyeRay.direction.z = dot(temp, ((float4)(invViewMatrix.s8,invViewMatrix.s9,invViewMatrix.sA,invViewMatrix.sB)));
  eyeRay.direction.w = 0.0f;

  float sum = 0.0f;
  float transparency = 1.0f;

  float tnear, tfar;
  int hit = intersectBox(eyeRay.origin, eyeRay.direction, boxMin, boxMax, &tnear, &tfar);
  if(hit){
    if(tnear < 0.0f) tnear = 0.0f;
	float maxStep = 256.0f;
	float step = (tfar - tnear) / maxStep;
	float t = tfar - 0.0001f;
	//float t = tnear + 0.0001f;
	for(int i=0; i < maxStep; ++i){
		float4 pos = ((eyeRay.origin + t * eyeRay.direction) + 1.0f) / 2.0f;
		//float illumination = getIllumination(pos, resolution, simulationBuffer) / (iteration * 500) / (4.0f * M_PI) / pown(1.0f / resolution, 3) / albedo  * step;
		float illumination = getIllumination(pos, resolution, simulationBuffer) * step;
		//float alpha = getDensity(pos) * step;

		//sum = (1.0f-alpha) * sum + illumination;
		//transparency *= (1-alpha);
		sum += illumination;

        t -= step;
        if(t<tnear) break;
	}
  }

  if(id.x < width && id.y < height){
	float4 sky = (float4)(0.17f, 0.4f, 0.6f, 0.0f);
	sky = sky * transparency;

	float4 tmpColor = sky + sum;

	float gamma = 0.8f;
	tmpColor.x = pow(tmpColor.x, gamma);
    tmpColor.y = pow(tmpColor.y, gamma);
    tmpColor.z = pow(tmpColor.z, gamma);

	visualizationBuffer[id.x + id.y * width] = tmpColor;
  }
}

