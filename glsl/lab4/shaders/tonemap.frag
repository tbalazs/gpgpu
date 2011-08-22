#version 130

uniform sampler2D hdr;
uniform sampler2D rellum;
uniform float W;

in vec2 fTexCoord;
out vec4 outColor;

void main(void){
  float Yr = texture(rellum, fTexCoord).x;
  vec4 hdrColor = texture(hdr, fTexCoord);
  outColor = hdrColor * ( Yr * ( 1 + Yr / W / W) / (1 + Yr) );
}
