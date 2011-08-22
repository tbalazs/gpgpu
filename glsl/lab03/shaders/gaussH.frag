#version 130

uniform sampler2D colorMap;
const float kernel[3] = float[3]( 1.0, 2.0, 1.0 );
out vec4 outColor;

void main(){
 outColor = vec4(0.0);
  vec2 texSize = textureSize(colorMap, 0);
 for(int x=-1; x<=1; ++x)
  outColor += texelFetch(colorMap, 
                        ivec2(gl_FragCoord.x,texSize.y - gl_FragCoord.y) + ivec2(x,0), 0) * kernel[x+1] / 4.0;
}
