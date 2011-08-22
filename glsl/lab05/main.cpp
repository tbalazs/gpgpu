#include <GL/glew.h>
#include <GL/freeglut.h>

#include <iostream>
#include <cstring>

#include "framebuffer.hpp"
#include "shader.hpp"
#include "quad.hpp"
#include "matrix4x4.hpp"
#include "camera.hpp"

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

int windowWidth = 600;
int windowHeight = 600;

Quad* fullscreenQuad;
Shader* simpleShader;

bool keysPressed[256];
Camera camera;

Vector noise[64];

Matrix4x4 quadrics[16];
struct Material {
	Vector diffuseColor;
	float idealParam; // kr | refr. index * -1
} materials[16];
int nQuadrics = 0;
int objectEnders[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int nObjects = 1;

int nRecursions = 3;

void addQuadric(const Matrix4x4& quadric, const Vector& color, float kr) {
	materials[nQuadrics].diffuseColor = color;
	materials[nQuadrics].idealParam = kr;
	quadrics[nQuadrics++] = quadric;
	objectEnders[nObjects] = nQuadrics;
}

void restartObject() {
	nObjects++;
	objectEnders[nObjects] = objectEnders[nObjects-1];
}

void init(){

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)	{
		std::cerr << "Error: "<< glewGetErrorString(err) << std::endl;
	} else {
		if (GLEW_VERSION_3_3)
		{
			std::cout<< "Driver supports OpenGL 3.3\nDetails:"<< std::endl;
			std::cout << "Using GLEW " << glewGetString(GLEW_VERSION)<< std::endl;
			std::cout << "Vendor: " << glGetString (GL_VENDOR)<< std::endl;
			std::cout << "Renderer: " << glGetString (GL_RENDERER)<< std::endl;
			std::cout << "Version: " << glGetString (GL_VERSION)<< std::endl;
			std::cout << "GLSL: " << glGetString (GL_SHADING_LANGUAGE_VERSION)<< std::endl;
		}
	}

	fullscreenQuad = new Quad();
	simpleShader = new Shader("shaders/raydir.vert", "shaders/raytrace.frag");

	memset(keysPressed, 0, sizeof(bool) * 256);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	for(int h=0; h<64; h++) {
		Vector v;
		do {
			v = Vector((float) rand() / RAND_MAX * 2 - 1, (float) rand() / RAND_MAX * 2 - 1, (float) rand() / RAND_MAX * 2 - 1);
		} while(v.dot(v) > 1);
		noise[h] = v;
	}

	Matrix4x4 q;
	addQuadric( q.makeQuadricSphere().quadricScale(Vector(2, 2, 2)), Vector(1, 0, 1), 0.5);
	addQuadric( q.makeQuadricParaboloid(), Vector(0, 1, 1), 0.5);

	restartObject();

	addQuadric( q.makeQuadricSphere().quadricScale(Vector(30, 100, 100)), Vector(1, 0, 0), 0 );
	addQuadric( q.makeQuadricSphere().quadricScale(Vector(100, 30, 100)), Vector(0, 1, 0), 0 );
	addQuadric( q.makeQuadricSphere().quadricScale(Vector(100, 100, 30)), Vector(0, 0, 1), 0 );

	restartObject();

	addQuadric( q.makeQuadricSphere().quadricScale(Vector(2, 2, 2)).quadricTranslate(Vector(6, 0, 0)), Vector(1, 1, 1), 0.5);

	restartObject();

	addQuadric( q.makeQuadricSphere().quadricScale(Vector(2, 2, 2)).quadricTranslate(Vector(-6, 0, 0)), Vector(0, 0, 0), -1.6);
}

void display(){
	glClearColor(0.17f, 0.4f, 0.6f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glutReshapeWindow(600, 600);
	simpleShader->enable();

	simpleShader->bindUniformVector("noise", (float*)noise, 64);
	Vector eye = camera.getEye();
	simpleShader->bindUniformFloat3("eye", eye.x, eye.y, eye.z);
	simpleShader->bindUniformMatrix("viewDirMatrix", camera.getViewDirMatrix().getPointer());
	simpleShader->bindUniformMatrix("quadrics", (float*)quadrics, nQuadrics);
	simpleShader->bindUniformFloat4Array("materials", (float*)materials, nQuadrics);
	simpleShader->bindUniformIntArray("objectEnders", objectEnders, nObjects+1);
	simpleShader->bindUniformInt("nObjects", nObjects);
	simpleShader->bindUniformInt("nRecursions", nRecursions);

	fullscreenQuad->render(simpleShader);
	simpleShader->disable();

	glutSwapBuffers();
}

void animate()
{
	static float lastTime = 0.0;
	long timeInMilliSecs = glutGet(GLUT_ELAPSED_TIME);
	float timeNow = timeInMilliSecs / 1000.0f;
	float deltaTime = timeNow - lastTime;
	lastTime = timeNow;

	camera.control(deltaTime, keysPressed);

	glutPostRedisplay();
}

void keyDown(unsigned char key, int x, int y){
	keysPressed[key] = true;
}

void keyUp(unsigned char key, int x, int y){
	keysPressed[key] = false;
	switch(key){
	case '+' : nRecursions++; break;
	case '-' : if(nRecursions > 1) nRecursions--; break;
	case 27:
		exit(0);
		break;

	}
}

void mouseClick(int button, int state, int x, int y){
	if(button == GLUT_LEFT_BUTTON)
		if(state == GLUT_DOWN)
			camera.startDrag(x, y);
}

void mouseMove(int x, int y)
{
	camera.drag(x, y);
}


void reshape(int width, int height){
	windowWidth = width;
	windowHeight = height;
	glViewport(0, 0, width, height);
	camera.setAspectRatio((float)width/height);
}

int main(int argc, char* argv[]){
	glutInit(&argc, argv);
	glutInitContextVersion (3, 3);
	glutInitContextFlags (GLUT_CORE_PROFILE | GLUT_DEBUG);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(windowWidth, windowHeight);
	glutCreateWindow("GPGPU 5. labor: raytracing");

	init();

	glutDisplayFunc(display);
	glutIdleFunc(animate);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyDown);
	glutKeyboardUpFunc(keyUp);
	glutMouseFunc(mouseClick);
	glutMotionFunc(mouseMove);

	glutMainLoop();

	return(0);
}
