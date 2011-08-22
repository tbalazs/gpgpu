#version 130

out vec4 outcolor;
in vec2 fTexCoord;

uniform sampler2D inputTex;

void main()
{
	outcolor = vec4(0);
	ivec2 coord = ivec2(gl_FragCoord.xy);	
	if(coord.x == 0){
		if(texelFetch(inputTex, coord / 2, 0).z == 1 && mod(coord.y, 2) == 1)
			outcolor = vec4(1); 
	}
	else if(coord.y == 0){
		if(texelFetch(inputTex, coord / 2, 0).w == 1 && mod(coord.x, 2) == 1)
			outcolor = vec4(1); 
	}
	else{
		coord = coord - ivec2(1);
		ivec2 coordBase = coord / 2;
		ivec2 coordRel = ivec2(mod(vec2(coord), vec2(2)));
	
		vec4 Vp = texelFetch(inputTex, coordBase, 0);
	
		if(coordRel.x == 0 && coordRel.y == 0)
			outcolor = vec4(1);
		else if(coordRel.x == 1 && coordRel.y == 0 && Vp.x == 1)
			outcolor = vec4(1);
		else if(coordRel.x == 0 && coordRel.y == 1 && Vp.y == 1)
			outcolor = vec4(1);
	}
}
