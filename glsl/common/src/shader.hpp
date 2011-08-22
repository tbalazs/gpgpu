#ifndef _SHADER_
#define _SHADER_

#include <string>

#include <GL/glew.h>

class Shader{
private:
  GLuint shaderProgram;
  GLuint vertexProgram;
  GLuint fragmentProgram;
  GLuint geometryProgram;

  Shader();
  bool fileToString(const char* path, char* &out, int& len);

public:
  Shader(const char* vertexShaderPath, const char* fragmentShaderPath, const char* geometryShaderPath = 0);
  virtual ~Shader();

  GLuint getProgram(){return shaderProgram;}
  GLuint getGeometryProgram(){return geometryProgram;}

  void shaderFromFile(const char* shaderPath, GLenum shaderType, GLuint& handle);
  void linkShaders(GLuint& vertexShader, GLuint& fragmentShader, GLuint& geometryShader, GLuint& handle);

  std::string getShaderInfoLog(GLuint& object);
  std::string getProgramInfoLog(GLuint& object);

  void enable();
  void disable();

  GLuint getHandle(){
    return shaderProgram;
  }

  void bindUniformInt(const char* name, int i);
  void bindUniformInt2(const char* name, int i1, int i2);
  void bindUniformFloat(const char* name, float f);
  void bindUniformFloat2(const char* name, float f1, float f2);
  void bindUniformFloat3(const char* name, float f1, float f2, float f3);
  void bindUniformTexture(const char* name, GLuint texture, GLuint unit);

  void bindUniformMatrix(const char* name, float* m, unsigned int arraySize = 1);
  void bindUniformVector(const char* name, float* m, unsigned int arraySize = 1);
  void bindUniformFloat4Array(const char* name, float* m, unsigned int arraySize = 1);
  void bindUniformIntArray(const char* name, int* iv, unsigned int arraySize);

  void bindAttribLocation(GLuint id, const char* name);
};

#endif
