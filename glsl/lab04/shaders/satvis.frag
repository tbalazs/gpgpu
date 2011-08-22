#version 130

uniform sampler2D inputMap;

in vec2 fTexCoord;
out vec4 outColor;

void main(void){
  outColor = vec4(texture(inputMap, fTexCoord).r / 74000);
}
