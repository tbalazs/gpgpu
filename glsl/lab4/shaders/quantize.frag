#version 130

uniform sampler2D inputBuffer;
uniform int levels;

in vec2 fTexCoord;

out vec4 outColor;

float I (vec2 coord){
  vec4 color = texture(inputBuffer, coord);
  return(dot(color.rgb, vec3(0.21, 0.39, 0.4)));
}

void main(){
  float data = I(fTexCoord);

  float threshold = 1.0 / levels;

  while(data > threshold){
    threshold += 1.0 / levels;
  }
  outColor = vec4(threshold);
}
