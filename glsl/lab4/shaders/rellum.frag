#version 130

uniform sampler2D luminanceMap;
uniform sampler2D sat;
uniform float alpha;

in vec2 fTexCoord;
out vec4 outColor;

void main(void){
  vec2 mapSize = textureSize(sat, 0);
  float avg = texture(sat, vec2(1.0, 1.0)).r / mapSize.x / mapSize.y;

  outColor = alpha * texture(luminanceMap, fTexCoord) / avg;
}
