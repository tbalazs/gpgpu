#ifndef _SHADER_
#define _SHADER_

#include <string>

#include <GL/glew.h>

class Shader{
private:
  GLuint shaderProgram;

  Shader();
  bool fileToString(const char* path, char* &out, int& len);

public:
  Shader(const char* vertexShaderPath, const char* fragmentShaderPath);
  virtual ~Shader();

  void shaderFromFile(const char* shaderPath, GLenum shaderType, GLuint& handle);
  void linkShaders(GLuint& vertexShader, GLuint& fragmentShader, GLuint& handle);

  std::string getShaderInfoLog(GLuint& object);
  std::string getProgramInfoLog(GLuint& object);

  void enable();
  void disable();

  GLuint getHandle(){
    return shaderProgram;
  }

  void bindUniformInt(const char* name, int i);
  void bindUniformFloat(const char* name, float f);
  void bindUniformFloat2(const char* name, float f1, float f2);
  void bindUniformFloat3(const char* name, float f1, float f2, float f3);
  void bindUniformTexture(const char* name, GLuint texture, GLuint unit);

  void bindAttribLocation(GLuint id, const char* name);
};

#endif
