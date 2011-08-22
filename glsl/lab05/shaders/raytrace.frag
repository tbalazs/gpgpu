#version 130

out vec4 outcolor;
in vec2 fTexCoord;
in vec3 viewDir;

uniform mat4 quadrics[16];
uniform vec4 materials[16];
uniform int objectEnders[16];
uniform int nObjects;
uniform vec3 eye;

uniform vec3 noise[64];
vec3 texProc(vec3 x, vec3 mainColor)
{
float r = 0;
for(int i=0; i<64; i++)
r += fract(dot(noise[i] , x ));
r /= 64;
return r * mainColor;
}
//vec3 texProc(vec3 x, vec3 mainColor)
//{
	//x *= 100;
    //float r = 0;
    //for(int i=0; i<32; i++)
		//r += sin(dot(noise[i] , x ));
    //float e = 0;
    //for(int i=32; i<64; i++)
		//e += sin(dot(noise[i] , x ));
    //return (1 - smoothstep(5, 0, abs(r)) * smoothstep(-4, 4, e)) * mainColor;
//}

vec2 intersectQuadric(mat4 A, vec4 o, vec4 d, vec2 tMinMax, out bvec2 visible)
{
	float a = dot(d * A,  d);
	float b = dot(d * A, o) + dot(o * A, d);
	float c = dot(o * A, o);
	float det = b*b - 4 * a * c;
	if(det < 0)
		return tMinMax.yx;

	vec2 t = (-vec2(b) + sqrt(det) * vec2(1, -1)) / (2 * a);
	if(t.x > t.y) t = t.yx;

	visible = bvec2(true, true);
	if(t.x < tMinMax.x)
	{
		t.x = tMinMax.x;
		visible.x = false;
	}
	if(t.y > tMinMax.y)
	{
		t.y = tMinMax.y;
		visible.y = false;
	}
	return t;
}

void processObject(int iObject, in vec4 o, in vec4 d, inout float bt, inout int bQuadric, inout bool bBackFacing)
{
	vec2 t = vec2(0, 100000);
	ivec2 visibleQuadric = ivec2(0, 0);
	for(int iQuadric=objectEnders[iObject]; iQuadric < objectEnders[iObject+1]; iQuadric++)
	{
		bvec2 visible;
		t = intersectQuadric(quadrics[iQuadric], o, d, t, visible);
		if(t.x > t.y)
			return;
		visibleQuadric.x = visible.x?iQuadric:visibleQuadric.x;
		visibleQuadric.y = visible.y?iQuadric:visibleQuadric.y;
	}
	bool backFacing = false;
	if(t.x == 0)
	{
		backFacing = true;
		t.x = t.y;
		visibleQuadric.x = visibleQuadric.y;
	}
	if(t.x < bt)
	{
		bt = t.x;
		bQuadric = visibleQuadric.x;
		bBackFacing = backFacing;
	}
}

vec3 trace(inout vec4 o, inout vec4 d, inout float contrib)
{
	int quadric = -1;
	bool backFacing = false;
	float t = 100000;
	for(int iObject=0; iObject < nObjects; iObject++)
		processObject(iObject, o, d, t, quadric, backFacing);

	if(quadric == -1)
		return vec3(0, 0, 0);
	
	vec4 p = o + d * t;
	vec3 normal = normalize((p * quadrics[quadric] + quadrics[quadric] * p).xyz) * (backFacing?-1:1);
	vec3 lightDir = normalize(vec3(-1, 1, 1));
	vec3 result = contrib * texProc(p.xyz, materials[quadric].xyz) * (clamp(dot(normal, lightDir), 0, 1) + 0.3);

	float kr = materials[quadric].w;
	if(kr < 0) 
	{
        vec3 rdir = refract(d.xyz, normal, -(backFacing?(kr):(1/kr)));
        if(dot(rdir, rdir) > 0.001) {
            contrib *= 0.99;
            o = p;
            o.xyz -= normal * 0.01;
            d = vec4(rdir, 0);
        }
		else
		 	kr = 1;	// total internal relfection
	}
	if(kr >= 0) {
		contrib *= kr;
		o = p;
		o.xyz += normal * 0.01;	// induló felületet ne találja +
		d = vec4(reflect(d.xyz, normal), 0);
	}
	return result;
}

uniform int nRecursions = 5;
void main() 
{
	vec4 o = vec4(eye, 1);
	vec4 d = vec4(normalize(viewDir), 0);
	outcolor = vec4(0, 0, 0, 1);
	float contrib = 1;
	for(int iReflection=0; iReflection<nRecursions && contrib > 0.01; iReflection++)
		outcolor.xyz += trace(o, d, contrib);
}


