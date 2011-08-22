#version 130

uniform sampler2D inputMap;
uniform sampler2D histogram;
uniform float level;
uniform float numLevel;

in vec2 fTexCoord;
out vec4 outColor;

float I (vec2 coord){
  vec4 color = texture(inputMap, coord);
  return(dot(color.rgb, vec3(0.21, 0.39, 0.4)));
}


void main(void){
  vec2 size = textureSize(inputMap, 0);
  float current = I(fTexCoord /*- vec2(0.5 / size.x, 0.5 / size.y)*/ ); // 0..1
  /* outColor = vec4(current); */
  /* return; */
    float accum = texture(histogram, vec2(current, 0.0)).x; // 0..Px
  // float accum = texture(histogram, fTexCoord).x; // 0..Px

  float bin = accum / size.x / size.y;
    //    float bin = accum / 262144.0;

  outColor = vec4(bin) ;
  //  outColor = vec4(0.5);
}

