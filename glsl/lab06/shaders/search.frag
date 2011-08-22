#version 130

out vec4 outcolor;
in vec4 celldata;

void main()
{
	outcolor = celldata;
}