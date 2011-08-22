#version 130

uniform sampler2D inputMap;
uniform int offset;

out vec4 outColor;

void main(void){
  vec2 v = texelFetch(inputMap, ivec2(gl_FragCoord), 0).xy;
  ivec2 sampleOff = ivec2(gl_FragCoord) + ivec2(offset, 0);

  vec2 addV = vec2(0.0f);
  if(sampleOff.x >= 0){
    addV = texelFetch(inputMap, sampleOff, 0).xy;
  }

  outColor = vec4(v + addV, 0.0, 0.0);
}
