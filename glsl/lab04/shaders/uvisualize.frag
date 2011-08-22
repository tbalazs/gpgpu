#version 130

uniform sampler2D inputBuffer;
uniform sampler2D original;

uniform int maxValue;

in vec2  fTexCoord;

out vec4 outColor;

float I (vec2 coord){
  vec4 color = texture(original, coord);
  return(dot(color.rgb, vec3(0.21, 0.39, 0.4)));
}

void main() {
  float data = texture(inputBuffer, fTexCoord).x;
  if(data / float(maxValue) > fTexCoord.y){
    //    outColor = vec4(0.5f + 0.5f * I(fTexCoord));
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
  } else {
    outColor = vec4(I(fTexCoord));
  }
}

