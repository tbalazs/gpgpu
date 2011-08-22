#version 130

uniform sampler2D trajectories;
uniform int timeStep;

out vec4 outColor;

const float dt = 0.00396854;
const float drift = 0.15;
const float volatility = 0.079;
const float r = 200.0;

// Permutation polynomial evaluation for 1 to 4 components in parallel
float permute( float x, vec3 p ) { return floor(mod( (p.y*x + p.z) * x , p.x )); }
vec2 permute( vec2 x, vec3 p )   { return floor(mod( (p.y*x + p.z) * x , p.x )); }
vec3 permute( vec3 x, vec3 p )   { return floor(mod( (p.y*x + p.z) * x , p.x )); }
vec4 permute( vec4 x, vec3 p )   { return floor(mod( (p.y*x + p.z) * x , p.x )); }

// Example constant with a 289 element permutation
const vec4 pParam = vec4( 17.0*17.0, 34.0, 1.0, 7.0);

float simplexNoise(vec2 v){
  const vec2 C = vec2(0.211324865405187134, // (3.0-sqrt(3.0))/6.0
                      0.366025403784438597); // 0.5*(sqrt(3.0)-1.0)
  const vec3 D = vec3( 0.0, 0.5, 2.0) * 3.14159265358979312;

  // First corner
  vec2 i  = floor(v + dot(v, C.yy) );
  vec2 x0 = v -   i + dot(i, C.xx);

  // Other corners
  vec2 i1  =  (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0) ;
   //  x0 = x0 - 0.0 + 0.0 * C
  vec2 x1 = x0 - i1 + 1.0 * C.xx ;
  vec2 x2 = x0 - 1.0 + 2.0 * C.xx ;

  // Permutations
  i = mod(i, pParam.x);
  vec3 p = permute( permute(
             i.y + vec3(0.0, i1.y, 1.0 ), pParam.xyz)
           + i.x + vec3(0.0, i1.x, 1.0 ), pParam.xyz);

  // ( N points uniformly over a line, mapped onto a diamond.)
  vec3 x = fract(p / pParam.w) ;
  vec3 h = 0.5 - abs(x) ;

  vec3 sx = vec3(lessThan(x,vec3(0.0))) *2.0 - 1.0;
  vec3 sh = vec3(lessThan(h,vec3(0.0)));

  vec3 a0 = x + sx*sh;
  vec2 p0 = vec2(a0.x,h.x);
  vec2 p1 = vec2(a0.y,h.y);
  vec2 p2 = vec2(a0.z,h.z);

  vec3 g = 2.0 * vec3( dot(p0, x0), dot(p1, x1), dot(p2, x2) );
  // Mix contributions from the three corners
  vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x1,x1), dot(x2,x2)), 0.0);
  m = m*m;

  return  r * 1.66666 * 70.0 * dot(m*m, g);
}

void main(void){
  ivec2 coord = ivec2(gl_FragCoord);
  if(coord.y == timeStep){
    float Sp = texelFetch(trajectories, ivec2(coord.x, coord.y - 1.0), 0).x;
    float Sc = Sp + drift * Sp * dt + volatility * Sp * simplexNoise(vec2(gl_FragCoord) + vec2(0.0, timeStep)) * dt;
    if(Sc < 0.0){
      Sc = 0.0;
    }
    outColor = vec4(Sc, Sc*Sc, 0.0, 0.0);
  } else {
    discard;
  }
}
