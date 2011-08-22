#version 130

uniform sampler2D colorMap;
const float kernel[3] = float[3]( 1.0, 2.0, 1.0 );
out vec4 outColor;

void main(){
 outColor = vec4(0.0);
  vec2 texSize = textureSize(colorMap, 0);
 for(int y=-1; y<=1; ++y)
  outColor += texelFetch(colorMap, 
                         ivec2(gl_FragCoord) + ivec2(0,y), 0) * kernel[y+1] / 4.0;
}
