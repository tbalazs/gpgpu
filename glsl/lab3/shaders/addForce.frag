#version 130

uniform ivec2 center;

out vec4 outColor;

void main(void){
  if(ivec2(gl_FragCoord.xy) == center){
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
  } else {
    discard;
  }
}
