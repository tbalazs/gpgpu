#version 130
#extension GL_EXT_geometry_shader4 : enable

uniform sampler2D inputTex1; //cell information
out vec4 celldata;

void main(void)
{

	vec2 windowSize = textureSize(inputTex1, 0);
	vec4 vertexdata = gl_PositionIn[0];
			
	vec4 storedCelldata = texelFetch(inputTex1, ivec2(vertexdata.xy), 0);
	
	if(storedCelldata.z == 0)
		return;

	if(storedCelldata.w == 1) // right
	{
		celldata = vertexdata;
		celldata.x += 1;
		vec2 wCoord = (celldata.xy * 2 + 1) / (windowSize * 2 + 1) * 2.0 - 1.0;
		gl_Position = vec4(wCoord,0,1);
		EmitVertex();	
	}
	else if(storedCelldata.w == 2) // up
	{
		celldata = vertexdata;
		celldata.y += 1;
		vec2 wCoord = (celldata.xy * 2 + 1) / (windowSize * 2 + 1) * 2.0 - 1.0;
		gl_Position = vec4(wCoord,0,1);
		EmitVertex();	
	}
	else if(storedCelldata.w == 3) // left
	{
		celldata = vertexdata;
		celldata.x -= 1;
		vec2 wCoord = (celldata.xy * 2 + 1) / (windowSize * 2 + 1) * 2.0 - 1.0;
		gl_Position = vec4(wCoord,0,1);
		EmitVertex();	
	}
	else if(storedCelldata.w == 4) // down
	{
		celldata = vertexdata;
		celldata.y -= 1;
		vec2 wCoord = (celldata.xy * 2 + 1) / (windowSize * 2 + 1) * 2.0 - 1.0;
		gl_Position = vec4(wCoord,0,1);
		EmitVertex();	
	}	
	EndPrimitive(); 	
}