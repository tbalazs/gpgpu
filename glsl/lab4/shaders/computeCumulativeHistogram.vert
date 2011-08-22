#version 130

in vec4 position;
in vec2 texCoord;

out vec2 fTexCoord;

void main(){
  gl_Position = position;
  fTexCoord = texCoord;
}
