#version 130

uniform vec2 center;
uniform float zoom;
uniform float iteration;
uniform bool fractalType;
uniform vec2 k;

in vec2 fTexCoord;

out vec4 outColor;

void main(){
  vec2 z, c;

  c.x = fTexCoord.x * zoom + center.x;
  c.y = fTexCoord.y * zoom + center.y;

  int i;
  z = c;
  for(i=0; i<iteration; i++) {
    if(fractalType){
      z = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + c;
    } else {
      z = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + k;
    }
    if (dot(z, z) > 4.0) break;
  }

  outColor = vec4(vec3(1.0 - (i/iteration)), 1.0);
}

