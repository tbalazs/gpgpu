#version 130

out vec4 outcolor;
in vec2 fTexCoord;

uniform sampler2D inputTex;

void main()
{
	outcolor = texture(inputTex, fTexCoord);
	//outcolor = outcolor.bbbb / 400;
}
