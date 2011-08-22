#version 130
#extension GL_EXT_geometry_shader4 : enable

uniform sampler2D inputTex1; //neighbour information
uniform sampler2D inputTex2; //celldata texture (shortest path + path dir)
out vec4 celldata;

void main(void)
{
	vec2 windowSize = textureSize(inputTex2, 0);
	vec4 vertexdata = gl_PositionIn[0];
			
	//read neighbour information
	vec4 neighInf = texelFetch(inputTex1, ivec2(vertexdata.xy), 0);
	vec4 storedCelldata = texelFetch(inputTex2, ivec2(vertexdata.xy), 0);

	if(storedCelldata.z > vertexdata.z || storedCelldata.w == 0)
	{
		celldata = vertexdata;		
		vec2 wCoord = vertexdata.xy / windowSize * 2.0 - 1.0;
		gl_Position = vec4(wCoord,0,1);
		EmitVertex();	
	}

	//emit neighbours if connected	
	//right
	if(neighInf.r == 1)
	{
		celldata = vertexdata;
		celldata.x += 1;
		celldata.z += 1;
		celldata.w = 3;	
		storedCelldata = texelFetch(inputTex2, ivec2(celldata.xy), 0);
		if((storedCelldata.z > celldata.z || storedCelldata.w == 0) && celldata.x < windowSize.x)		
		{
			vec2 wCoord = celldata.xy / windowSize * 2.0 - 1.0;
			gl_Position = vec4(wCoord,0,1);
			EmitVertex();	
		}
	}
	//left
	if(neighInf.b == 1)
	{
		celldata = vertexdata;
		celldata.x -= 1;
		celldata.z += 1;
		celldata.w = 1;			
		storedCelldata = texelFetch(inputTex2, ivec2(celldata.xy), 0);
		if((storedCelldata.z > celldata.z || storedCelldata.w == 0) && celldata.x >= 0)
		{
			vec2 wCoord = celldata.xy / windowSize * 2.0 - 1.0;
			gl_Position = vec4(wCoord,0,1);
			EmitVertex();	
		}
	}
	//up
	if(neighInf.g == 1)
	{
		celldata = vertexdata;
		celldata.y += 1;
		celldata.z += 1;
		celldata.w = 4;			
		storedCelldata = texelFetch(inputTex2, ivec2(celldata.xy), 0);
		if((storedCelldata.z > celldata.z || storedCelldata.w == 0) && celldata.y < windowSize.y)
		{
			vec2 wCoord = celldata.xy / windowSize * 2.0 - 1.0;
			gl_Position = vec4(wCoord,0,1);
			EmitVertex();	
		}
	}
	//down
	if(neighInf.a == 1)
	{
		celldata = vertexdata;
		celldata.y -= 1;
		celldata.z += 1;
		celldata.w = 2;			
		storedCelldata = texelFetch(inputTex2, ivec2(celldata.xy), 0);
		if((storedCelldata.z > celldata.z || storedCelldata.w == 0) && celldata.y >= 0)
		{
			vec2 wCoord = celldata.xy / windowSize * 2.0 - 1.0;
			gl_Position = vec4(wCoord,0,1);
			EmitVertex();	
		}
	}
	
	EndPrimitive(); 	
}