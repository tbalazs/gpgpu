#version 130

uniform sampler2D inputBuffer;
in vec2  fTexCoord;

out vec4 outColor;

void main() {
  outColor = texture(inputBuffer, fTexCoord)/4.0;
}

