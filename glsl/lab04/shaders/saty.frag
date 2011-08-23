#version 130

uniform sampler2D inputMap;
uniform float offset;

in vec2 fTexCoord;
out vec4 outColor;

void main(void){
  float c = texture(inputMap, fTexCoord).r;
  vec2 sampleOff = fTexCoord + vec2(0.0, offset);
  float addValue = 0.0f;

  if(sampleOff.y >= 0.0){
    addValue = texture(inputMap, sampleOff).r;
  }
  outColor = vec4(c + addValue);
}