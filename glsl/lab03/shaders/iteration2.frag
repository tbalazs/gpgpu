#version 130

uniform sampler2D inputMap;

uniform bool pass;

out vec4 outColor;

const float deltat = 0.001;
const float deltax = 0.01;
const float sqrc = 200.0;

void main(void){

  vec4 data = texelFetch(inputMap, ivec2(gl_FragCoord), 0);

  if(pass == bool(0)){
    float u = data.x;
    float v = data.y;

    float ud2 = texelFetch(inputMap, ivec2(gl_FragCoord) + ivec2(-1,  0), 0).x +
      texelFetch(inputMap, ivec2(gl_FragCoord) + ivec2( 1,  0), 0).x +
      texelFetch(inputMap, ivec2(gl_FragCoord) + ivec2( 0, -1), 0).x +
      texelFetch(inputMap, ivec2(gl_FragCoord) + ivec2( 0,  1), 0).x -
      4 * u;

    float um = u + v * deltat / 2.0;
    float vm = v + sqrc * ud2 * deltat / 2.0;
    outColor = vec4(u, v, um, vm);
    return;
  } else {
    float u = data.x;
    float v = data.y;
    float um = data.z;
    float vm = data.w;

    float ud2 = texelFetch(inputMap, ivec2(gl_FragCoord) + ivec2(-1,  0), 0).z +
      texelFetch(inputMap, ivec2(gl_FragCoord) + ivec2( 1,  0), 0).z +
      texelFetch(inputMap, ivec2(gl_FragCoord) + ivec2( 0, -1), 0).z +
      texelFetch(inputMap, ivec2(gl_FragCoord) + ivec2( 0,  1), 0).z -
      4 * u;

    u = u + vm * deltat;
    v = v + sqrc * ud2 * deltat;

    outColor = vec4(u, v, 0.0, 0.0);
  }
}