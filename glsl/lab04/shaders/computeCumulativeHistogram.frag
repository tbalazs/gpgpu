#version 130

uniform sampler2D inputMap;
uniform float offset;

in vec2 fTexCoord;

out vec4 outColor;

void main(void){
  float current = texture(inputMap, fTexCoord).x;
  float sampleOff = fTexCoord.x + offset;
  float addValue = 0.0;
  if(sampleOff >= 0.0){
    addValue = texture(inputMap, vec2(sampleOff, 0.0)).x;
  }
  outColor = vec4(current + addValue);
}
