#version 130

uniform sampler2D textureMap;
uniform vec2 textureSize;

in vec2 fTexCoord;
out vec4 outColor;

const float kernel[9] = float[9](1.0, 2.0, 1.0,
				 2.0, 4.0, 2.0,
				 1.0, 2.0, 1.0);

void main(){

  float step_w = 1.0/textureSize.x;
  float step_h = 1.0/textureSize.y;
  vec2 offset[9] = vec2[9]( vec2(-step_w, -step_h), vec2(0.0, -step_h), vec2(step_w, -step_h),
			    vec2(-step_w, 0.0), vec2(0.0, 0.0), vec2(step_w, 0.0),
			    vec2(-step_w, step_h), vec2(0.0, step_h), vec2(step_w, step_h) );

  int i = 0;
  outColor = vec4(0.0);

  if(fTexCoord.y<0.495) {
    for( i=0; i<9; i++ ) {
      outColor += texture2D(textureMap, fTexCoord.xy + offset[i]) * kernel[i] / 16.0f;
    }
  } else if( fTexCoord.y>0.505 ) {
    outColor = texture2D(textureMap, fTexCoord.xy);
  } else {
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
  }
}

