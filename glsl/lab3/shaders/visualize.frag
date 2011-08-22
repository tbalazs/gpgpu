#version 130

uniform sampler2D inputMap;

out vec4 outColor;

void main(void){
  outColor = vec4(texelFetch(inputMap, ivec2(gl_FragCoord.xy), 0).x * 100);
}
