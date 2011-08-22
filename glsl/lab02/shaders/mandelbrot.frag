#version 130

const vec2 center = vec2(-1.2,-1.2);
const float zoom = 2.35;
const float iteration = 100;
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
    z = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + c;
    if (dot(z, z) > 4)break;
  }

  outColor = vec4(1.0 - i/iteration);
}

