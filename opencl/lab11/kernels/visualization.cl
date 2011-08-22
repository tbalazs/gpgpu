// -*- mode: c++ -*-

//**************
// Visualization
//**************

struct ray{
  float4 origin;
  float4 direction;
};

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

float getDensityFromVolume(const float4 p, const int resolution, __global float* volumeData){
  int x = p.x * resolution;
  int y = p.y * resolution;
  int z = p.z * resolution;

  if(x > resolution -1 || x < 0) return(0.0f);
  if(y > resolution -1 || y < 0) return(0.0f);
  if(z > resolution -1 || z < 0) return(0.0f);

  return volumeData[x + y * resolution + z * resolution * resolution];
}

float4 getNormalFromVolume(const float4 p, const int resolution, __global float* volumeData){
  float4 normal;

  normal.x = getDensityFromVolume((float4)(p.x + 2.0f/resolution, p.y, p.z, 0.0f), resolution, volumeData) -
    getDensityFromVolume((float4)(p.x - 2.0f/resolution, p.y, p.z, 0.0f), resolution, volumeData);
  normal.y = getDensityFromVolume((float4)(p.x, p.y + 2.0f/resolution, p.z, 0.0f), resolution, volumeData) -
    getDensityFromVolume((float4)(p.x, p.y - 2.0f/resolution, p.z, 0.0f), resolution, volumeData);
  normal.z = getDensityFromVolume((float4)(p.x, p.y, p.z + 2.0f/resolution, 0.0f), resolution, volumeData) -
    getDensityFromVolume((float4)(p.x, p.y, p.z - 2.0f/resolution, 0.0f), resolution, volumeData);
  normal.w = 0.0f;

  if(dot(normal, normal) < 0.001f){
    normal = (float4)(0.0f, 0.0f, 1.0f, 0.0f);
  }

  return normalize(normal);
}

// Iso-sourface raycasting
__kernel
void isosurface(const int width, const int height, __global float4* visualizationBuffer,
                   const int resolution, __global float* volumeData,
		   const float isoValue, const float16 invViewMatrix){
  int2 id = (int2)(get_global_id(0), get_global_id(1));

  float2 uv = (float2)( (id.x / (float) width)*2.0f-1.0f, (id.y / (float) height)*2.0f-1.0f );

  float4 boxMin = (float4)(-1.0f, -1.0f, -1.0f,1.0f);
  float4 boxMax = (float4)(1.0f, 1.0f, 1.0f,1.0f);

  // calculate eye ray in world space
  struct ray eyeRay;

  eyeRay.origin = (float4)(invViewMatrix.sC, invViewMatrix.sD, invViewMatrix.sE, invViewMatrix.sF);

  float4 temp = normalize(((float4)(uv.x, uv.y, -2.0f, 0.0f)));
  eyeRay.direction.x = dot(temp, ((float4)(invViewMatrix.s0,invViewMatrix.s1,invViewMatrix.s2,invViewMatrix.s3)));
  eyeRay.direction.y = dot(temp, ((float4)(invViewMatrix.s4,invViewMatrix.s5,invViewMatrix.s6,invViewMatrix.s7)));
  eyeRay.direction.z = dot(temp, ((float4)(invViewMatrix.s8,invViewMatrix.s9,invViewMatrix.sA,invViewMatrix.sB)));
  eyeRay.direction.w = 0.0f;

  float4 color = (float4)(0.0f);

  float tnear, tfar;
  int hit = intersectBox(eyeRay.origin, eyeRay.direction, boxMin, boxMax, &tnear, &tfar);
  if(hit){
    if(tnear < 0.0f) tnear = 0.0f;
    float maxStep = 256.0f;
    float step = (tfar - tnear) / maxStep;
    float t = tnear + 0.0001f;
    for(int i=0; i < maxStep; ++i){
      float4 pos = ((eyeRay.origin + t * eyeRay.direction) + 1.0f) / 2.0f;
      float density = getDensityFromVolume(pos, resolution, volumeData);

      // simple iso
      if( density > isoValue ){
	// color = clamp(dot(normalize((float4)(0.3f, -2.0f, 0.0f, 0.0f)), getNormalFromVolume(pos, resolution, volumeData)), 0.0f, 1.0f);
	color = 0.5f + 0.5f * dot(normalize((float4)(0.3f, -2.0f, 0.0f, 0.0f)), getNormalFromVolume(pos, resolution, volumeData));
	break;
      }

      t += step;
      if(t>tfar) break;
    }
  }

  if(id.x < width && id.y < height){
    visualizationBuffer[id.x + id.y * width] = color;
  }
}

// Alpha blended
__kernel
void alphaBlended(const int width, const int height, __global float4* visualizationBuffer,
		  const int resolution, __global float* volumeData,
		  const float alphaExponent, const float alphaCenter,
		  const float16 invViewMatrix){
  int2 id = (int2)(get_global_id(0), get_global_id(1));

  float2 uv = (float2)( (id.x / (float) width)*2.0f-1.0f, (id.y / (float) height)*2.0f-1.0f );

  float4 boxMin = (float4)(-1.0f, -1.0f, -1.0f,1.0f);
  float4 boxMax = (float4)(1.0f, 1.0f, 1.0f,1.0f);

  // calculate eye ray in world space
  struct ray eyeRay;

  eyeRay.origin = (float4)(invViewMatrix.sC, invViewMatrix.sD, invViewMatrix.sE, invViewMatrix.sF);

  float4 temp = normalize(((float4)(uv.x, uv.y, -2.0f, 0.0f)));
  eyeRay.direction.x = dot(temp, ((float4)(invViewMatrix.s0,invViewMatrix.s1,invViewMatrix.s2,invViewMatrix.s3)));
  eyeRay.direction.y = dot(temp, ((float4)(invViewMatrix.s4,invViewMatrix.s5,invViewMatrix.s6,invViewMatrix.s7)));
  eyeRay.direction.z = dot(temp, ((float4)(invViewMatrix.s8,invViewMatrix.s9,invViewMatrix.sA,invViewMatrix.sB)));
  eyeRay.direction.w = 0.0f;

  float4 sum = (float4)(0.0f);

  float tnear, tfar;
  int hit = intersectBox(eyeRay.origin, eyeRay.direction, boxMin, boxMax, &tnear, &tfar);
  if(hit){
    if(tnear < 0.0f) tnear = 0.0f;
    float maxStep = 256.0f;
    float step = (tfar - tnear) / maxStep;
    float t = tfar - 0.0001f;
    for(int i=0; i < maxStep; ++i){
      float4 pos = ((eyeRay.origin + t * eyeRay.direction) + 1.0f) / 2.0f;
      float density = getDensityFromVolume(pos, resolution, volumeData);
      // float alpha = pow(density, alphaExponent) * step;
      float alpha = clamp(alphaExponent * (density - alphaCenter) + 0.5f, 0.0f, 1.0f);
      float4 color = (float4)(0.5f + 0.5f * dot(normalize((float4)(0.3f, -2.0f, 0.0f, 0.0f)), getNormalFromVolume(pos, resolution, volumeData)));

      // float p1 = clamp(3 * (density - 0.1f), 0.0f, 1.0f);
      // float p2 = clamp(3 * (density - 0.4f), 0.0f, 1.0f);
      // float p3 = clamp(30 * (density - 0.6f), 0.0f, 1.0f);
      // color *= clamp((float4)(p1 - p2 + p3, 0.3f + p2, 0.2f + p3, 1.0f), 0.0f, 1.0f);

      color *= (float4)(density, density * density, density * density * density, 1.0f) + 0.1f;

      sum = (1.0f - alpha) * sum + alpha * color;

      t -= step;
      if(t<tnear) break;
    }
  }

  if(id.x < width && id.y < height){
    visualizationBuffer[id.x + id.y * width] = (float4)(sum);
  }
}

