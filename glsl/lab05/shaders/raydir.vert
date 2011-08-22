#version 130

uniform mat4 viewDirMatrix;
in vec4 position;
in vec2 texCoord;

out vec2 fTexCoord;
out vec3 viewDir;

void main(void) {
   gl_Position = position;
   fTexCoord = texCoord;
      vec4 hViewDir =  position * viewDirMatrix;
   viewDir = hViewDir.xyz / hViewDir.w;

}

