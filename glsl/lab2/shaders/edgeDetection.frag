#version 130

uniform sampler2D textureMap;
uniform vec2 textureSize;

in vec2 fTexCoord;
out vec4 outColor;

float I (vec2 coord){
  vec4 color = texture(textureMap, coord);
  return(dot(color.rgb, vec3(0.21, 0.39, 0.4)));
}

// Prewitt
const float kernelX[9] = float[9](-1.0/6.0, 0.0, 1.0/6.0,
				  -1.0/6.0, 0.0, 1.0/6.0,
				  -1.0/6.0, 0.0, 1.0/6.0);

const float kernelY[9] = float[9](1.0/6.0, 1.0/6.0, 1.0/6.0,
				  0.0, 0.0, 0.0,
				  -1.0/6.0, -1.0/6.0, -1.0/6.0); 

// Sobel
/* const float kernelX[9] = float[9](-1.0/8.0, 0.0, 1.0/8.0, */
/* 				  -2.0/8.0, 0.0, 2.0/8.0, */
/* 				  -1.0/8.0, 0.0, 1.0/8.0); */

/* const float kernelY[9] = float[9](1.0/8.0, 2.0/8.0, 1.0/8.0, */
/* 				  0.0, 0.0, 0.0, */
/* 				  -1.0/8.0, -2.0/8.0, -1.0/8.0); */
 

void main(){
 
 float step_w = 1.0/textureSize.x;
  float step_h = 1.0/textureSize.y;
  vec2 offset[9] = vec2[9]( vec2(-step_w, -step_h), vec2(0.0, -step_h), vec2(step_w, -step_h),
			    vec2(-step_w, 0.0), vec2(0.0, 0.0), vec2(step_w, 0.0),
			    vec2(-step_w, step_h), vec2(0.0, step_h), vec2(step_w, step_h) );

  int i = 0;
  outColor = vec4(0.0); 

  float gradX = 0.0;
  float gradY = 0.0;
  for( i=0; i<9; i++ ) {
      gradX += I(fTexCoord.xy + offset[i])*kernelX[i];
      gradY += I(fTexCoord.xy + offset[i])*kernelY[i];
  }
  outColor = vec4(sqrt(gradX * gradX + gradY * gradY) * 8.0); 
 }
