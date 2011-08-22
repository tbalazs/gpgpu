#version 130  
  
out vec4 outcolor[2];  
in vec2 fTexCoord;  
  
uniform sampler2D inputTex1;  
uniform sampler2D inputTex2;  
uniform sampler2D noiseTex;  
uniform int level;  
uniform float noiseScale;  
uniform float noiseSeed;
 
 vec4 M[4] = vec4[4](vec4(1,1,0,0),  
						vec4(0,1,1,0),  
						vec4(1,0,0,1),  
						vec4(0,0,1,1));  

vec4 D[4] = vec4[4](vec4(0,0,1,0),  
						vec4(1,0,0,1),  
						vec4(0,0,0,0),  
						vec4(0,1,0,0));  
						  
vec4 inv(vec4 v){return vec4(1.0) - v;}  
vec4 uni(vec4 v1, vec4 v2){ return min(vec4(1.0), v1 + v2);} 

void main()  
{  
	ivec2 coord = ivec2(gl_FragCoord.xy);  

	vec4 R = textureLod(inputTex2, fTexCoord, level + 1);  
	vec4 rand = texelFetch(noiseTex,  ivec2(mod(coord + ivec2(noiseSeed, 0), ivec2(16))), 0);  
	  
	outcolor[1] = R + noiseScale * (rand - 0.5);    

	ivec2 coordBase = coord / 2;  
  
	ivec2 coordRel = ivec2(mod(vec2(coord), vec2(2)));  
	int index = coordRel.x + coordRel.y * 2;
	
	vec4 V;

	vec4 P = texelFetch(inputTex1, coordBase, level + 1); 

	V = inv(P * D[index]) * P; 
	V = uni(V, M[index]); 

		vec4 W1; // fuggoleges v. vizszintes  
	vec4 W2; // bal/lenn - jobb/fenn 

	if(R.x < 0.25){ // right  
		W1 = vec4(1,0,1,0);  
		W2 = vec4(M[index].x == 1);  
	}  
	else if(R.x < 0.5) { // top  
		W1 = vec4(0,1,0,1);  
		W2 = vec4(M[index].y == 1); 
	}  
	else if(R.x < 0.75){ // left  
		W1 = vec4(1,0,1,0);  
		W2 = vec4(M[index].z == 1);  
	}  
	else{ // bottom  
		W1 = vec4(0,1,0,1);  
		W2 = vec4(M[index].w == 1); 
	}  
  
	V = V * uni(uni(W1, W2), inv(M[index]));

	outcolor[0] = V;
} 