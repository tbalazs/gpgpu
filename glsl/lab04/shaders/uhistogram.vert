#version 130

uniform sampler2D inputBuffer;

in vec4 position;
out vec4 color;

void main(void){
  vec2 resolution = textureSize(inputBuffer, 0);
  float luminance = texture(inputBuffer, position.xy + (0.5 / resolution)).x;
  gl_Position = vec4(2.0 * (luminance * (1.0 - 1.0 / 255.0) + 0.5 / 255.0 - 0.5) , 0.5, 0.0, 1.0);
}

