#version 130

uniform sampler2D colorMap;

const float kernel[9] = float[9]( 1.0, 2.0, 1.0,
			      2.0, 4.0, 2.0,
			      1.0, 2.0, 1.0);
out vec4 outColor;

void main(){
 outColor = vec4(0.0);
 vec2 texSize = textureSize(colorMap, 0);
 for(int x=-1; x<=1; ++x)
  for(int y=-1; y<=1; ++y)
   outColor += texelFetch(colorMap, ivec2(gl_FragCoord.x,texSize.y - gl_FragCoord.y) + ivec2(x,y),0) *
					kernel[(x+1)+(y+1)*3] / 16.0;
}
