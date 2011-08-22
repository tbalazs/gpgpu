#version 130

out vec4 outcolor;
in vec2 fTexCoord;

void main()
{
	outcolor = vec4(fTexCoord,0.0,1.0);
}
