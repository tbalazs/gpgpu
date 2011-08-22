#version 130

uniform sampler2D hdr;

in vec2 fTexCoord;
out vec4 outColor;

void main(void){
  vec4 color = texture(hdr, fTexCoord);
  outColor = vec4(color.r * 0.2126, color.g * 0.7152, color.b * 0.0722, 1.0);
}
