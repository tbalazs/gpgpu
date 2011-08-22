#version 130

uniform sampler2D inputBuffer;

in vec4 position;

out vec4 color;

float I (vec2 coord){
  vec4 color = texture(inputBuffer, coord);
  return(dot(color.rgb, vec3(0.21, 0.39, 0.4)));
}

void main(void){
  vec2 resolution = textureSize(inputBuffer, 0);
  float luminance = I(position.xy + (0.5 / resolution));

  gl_Position = vec4(2.0 * (luminance * (1.0 - 1.0 / 255.0) + 0.5 / 255.0 - 0.5) , 0.0, 0.0, 1.0);
  color = vec4(1.0);
}
