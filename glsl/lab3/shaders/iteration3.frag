#version 130

uniform sampler2D inputMap;

out vec4 outColor;

const float deltat = 0.00001;
const float deltax = 0.01;
const float sqrc = 200.0;

const float sigma = 0.0006;

void main(void){

  vec4 data = texelFetch(inputMap, ivec2(gl_FragCoord), 0);

  float u0 = data.x;
  float u = data.x;

  float v0 = data.y;
  float v = data.y;

  float ud2 = texelFetch(inputMap, ivec2(gl_FragCoord) + ivec2(-1,  0), 0).x +
              texelFetch(inputMap, ivec2(gl_FragCoord) + ivec2( 1,  0), 0).x +
              texelFetch(inputMap, ivec2(gl_FragCoord) + ivec2( 0, -1), 0).x +
              texelFetch(inputMap, ivec2(gl_FragCoord) + ivec2( 0,  1), 0).x -
              4 * u0;

  float a = sqrc * ud2 / deltax / deltax;
  u =  u0 +  v * deltat + a / 2.0 * deltat * deltat;
  v = (1.0 - sigma * deltat) * v0  + a * deltat;


  ud2 = texelFetch(inputMap, ivec2(gl_FragCoord) + ivec2(-1,  0), 0).x +
              texelFetch(inputMap, ivec2(gl_FragCoord) + ivec2( 1,  0), 0).x +
              texelFetch(inputMap, ivec2(gl_FragCoord) + ivec2( 0, -1), 0).x +
              texelFetch(inputMap, ivec2(gl_FragCoord) + ivec2( 0,  1), 0).x -
              4 * u;

  a = sqrc * ud2 / deltax / deltax;
  u =  u0 +  v * deltat + a / 2.0 * deltat * deltat;
  v = (1.0 - sigma * deltat) * v0  + a * deltat;

  outColor = vec4(u, v, 0.0, 1.0);
}
