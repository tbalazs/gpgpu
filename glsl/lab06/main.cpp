#include <GL/glew.h>
#include <GL/freeglut.h>

#include <iostream>

#include "framebuffer.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "quad.hpp"

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

int windowWidth = 600;
int windowHeight = 600;
int windowScale = 8;

Quad* fullscreenQuad;
Texture2D* noiseTexture;
Shader* simpleShader;

Framebuffer* dataBuffer;
Shader* generateShader;
int labyrinthSize = 64;
bool recalcLabyrinth = true;
float noiseSeed = (float) rand() / (float) RAND_MAX;
float noiseStart = 0.5;
float noiseScale = 0.5;

Shader* visualizeShader;
Framebuffer* visualizeTex;

Shader* searchShader;
Framebuffer* shortestPath;
GLuint streamoutBufffer[2];
GLuint outputQuery;
bool startPointChanged = true;
bool endpointChanged = true;
float startX = 0;
float startY = labyrinthSize -1;
float endX = labyrinthSize - 1;
float endY = labyrinthSize - 1;

Shader* walkPathShader;

void recursion( float startX,
				float startY,
				float startZ,
				float startW)
{
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER,0,streamoutBufffer[0]);

	glBeginTransformFeedback(GL_POINTS);
	glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, outputQuery);
	glPointSize(1);
	glBegin(GL_POINTS);
	glVertex4f(startX, startY, startZ, startW);
	glEnd();
	glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

	glEndTransformFeedback();

	GLint outPointCount = 0;
	GLint succ = 0;

	while(!succ)
		glGetQueryObjectiv(outputQuery, GL_QUERY_RESULT_AVAILABLE, &succ);
	glGetQueryObjectiv(outputQuery, GL_QUERY_RESULT, &outPointCount);
	//std::cout << "points written: " << outPointCount << std::endl;
	succ = 0;

	glEnableClientState(GL_VERTEX_ARRAY);
	int bb = 0;
	while(outPointCount > 0)
	{
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER,0,streamoutBufffer[(bb+1)%2]);
		glBeginTransformFeedback(GL_POINTS);

		glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, outputQuery);

		glBindBufferARB(GL_ARRAY_BUFFER_ARB, streamoutBufffer[bb]);
		glVertexPointer(4,GL_FLOAT,0,NULL);
		glDrawArrays(GL_POINTS, 0, outPointCount);

		glEndTransformFeedback();

		glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
		while(!succ)
			glGetQueryObjectiv(outputQuery, GL_QUERY_RESULT_AVAILABLE, &succ);
		glGetQueryObjectiv(outputQuery, GL_QUERY_RESULT, &outPointCount);
		succ = 0;

		bb = (bb + 1) % 2;
	}
	glDisableClientState(GL_VERTEX_ARRAY);
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
	glGetError();


	fullscreenQuad = new Quad();
	simpleShader = new Shader("shaders/passthrough.vert", "shaders/simple.frag");

	noiseTexture = new Texture2D();
	noiseTexture->loadFromFile(std::string("../common/images/noise.tga"));

	dataBuffer = new Framebuffer(labyrinthSize, labyrinthSize, 2, true, false);
	generateShader = new Shader("shaders/passthrough.vert", "shaders/generate.frag");

	visualizeTex = new Framebuffer(dataBuffer->getWidth() * 2 + 1,
									dataBuffer->getHeight() * 2 + 1,
									1,
									false,
									false);
	visualizeShader = new Shader("shaders/passthrough.vert", "shaders/visualize.frag");

	shortestPath = new Framebuffer(labyrinthSize, labyrinthSize, 1, false, true);

	glGenBuffers(2,streamoutBufffer);
	glBindBuffer(GL_ARRAY_BUFFER_ARB,streamoutBufffer[0]);
	glBufferData(GL_ARRAY_BUFFER_ARB, 	labyrinthSize*labyrinthSize*4*sizeof(float)*2,0,GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER_ARB,streamoutBufffer[1]);
	glBufferData(GL_ARRAY_BUFFER_ARB, 	labyrinthSize*labyrinthSize*4*sizeof(float)*2,0,GL_DYNAMIC_DRAW);

	searchShader = new Shader("shaders/search.vert", "shaders/search.frag", "shaders/search.geom");
	walkPathShader = new Shader("shaders/search.vert", "shaders/red.frag", "shaders/walkpath.geom");

	glGenQueries(1, &outputQuery);

	glutReshapeWindow(labyrinthSize * windowScale, labyrinthSize * windowScale);
}

void display(){

	glClearColor(0.17f, 0.4f, 0.6f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if(recalcLabyrinth){
		int level = dataBuffer->getLevels() - 1;
		dataBuffer->setRenderTarget(level);
		GLenum ca = GL_COLOR_ATTACHMENT0; glDrawBuffers(1, &ca);
		glClearColor(1,0,1,0); glClear(GL_COLOR_BUFFER_BIT);
		ca = GL_COLOR_ATTACHMENT1; glDrawBuffers(1, &ca);
		glClearColor(0.5,0.5,0.5,0.5); glClear(GL_COLOR_BUFFER_BIT);
		float scale = noiseStart;
		for(level = level - 1; level >= 0; level--){
			dataBuffer->setRenderTarget(level);
			generateShader->enable();
			generateShader->bindUniformInt("level", level);
			generateShader->bindUniformFloat("noiseScale", scale);
			generateShader->bindUniformFloat("noiseSeed", noiseSeed * 16.0);
			generateShader->bindUniformTexture("inputTex1", dataBuffer->getColorBuffer(0),0);
			generateShader->bindUniformTexture("inputTex2", dataBuffer->getColorBuffer(1),1);
			generateShader->bindUniformTexture("noiseTex", noiseTexture->getTextureHandle(),2);
			fullscreenQuad->render(generateShader);
			generateShader->disable();
			scale *= noiseScale;
		}
		dataBuffer->disableRenderTarget();
		recalcLabyrinth = false;
		startPointChanged = true;
	}

	if(startPointChanged)
	{
		endpointChanged = true;
		shortestPath->setRenderTarget();
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT);

		searchShader->enable();
		searchShader->bindUniformTexture("inputTex1", dataBuffer->getColorBuffer(0), 0);
		searchShader->bindUniformTexture("inputTex2", shortestPath->getColorBuffer(0), 1);

		recursion(startX, startY, 0, 5);

		searchShader->disable();
		shortestPath->disableRenderTarget();
		startPointChanged = false;
	}

	if(endpointChanged)
	{
		visualizeTex->setRenderTarget();

		visualizeShader->enable();
		visualizeShader->bindUniformTexture("inputTex", dataBuffer->getColorBuffer(0),0);
		fullscreenQuad->render(visualizeShader);
		visualizeShader->disable();

		walkPathShader->enable();
		walkPathShader->bindUniformTexture("inputTex1", shortestPath->getColorBuffer(0), 0);
		recursion(endX, endY, 0, 5);
		walkPathShader->disable();

		glBegin(GL_POINTS);
		glColor3f(0,0,1);
		glVertex3f((float)(startX * 2 + 1) / (float)(2 * labyrinthSize) * 2.0f - 1.0f,
					(float)(startY  * 2 + 1) / (float)(2 * labyrinthSize) * 2.0f - 1.0f,
					0);
		glColor3f(0,1,0);
		glVertex3f((float)(endX * 2 + 1) / (float)(2 * labyrinthSize) * 2.0f - 1.0f,
					(float)(endY  * 2 + 1) / (float)(2 * labyrinthSize) * 2.0f - 1.0f,
					0);
		glEnd();

		visualizeTex->disableRenderTarget();

		endpointChanged = false;
	}

	glViewport(0,0,windowWidth, windowHeight);
	simpleShader->enable();
	simpleShader->bindUniformTexture("inputTex", visualizeTex->getColorBuffer(0),0);
	fullscreenQuad->render(simpleShader);
	simpleShader->disable();

	glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y){
	switch(key){

	case 27:
		exit(0);
		break;
	case ' ':
		noiseSeed = (float) rand() / (float) RAND_MAX;
		recalcLabyrinth = true;
		break;
	case 'q':
		noiseScale *= 1.01;
		recalcLabyrinth = true;
		std::cout<<"noiseScale: "<<noiseScale<<" noiseScaleStart: "<<noiseStart<<std::endl;
		break;
	case 'a':
		noiseScale *= 0.99;
		recalcLabyrinth = true;
		std::cout<<"noiseScale: "<<noiseScale<<" noiseScaleStart: "<<noiseStart<<std::endl;
		break;
	case 'w':
		noiseStart *= 1.01;
		recalcLabyrinth = true;
		std::cout<<"noiseScale: "<<noiseScale<<" noiseScaleStart: "<<noiseStart<<std::endl;
		break;
	case 's':
		noiseStart *= 0.99;
		recalcLabyrinth = true;
		std::cout<<"noiseScale: "<<noiseScale<<" noiseScaleStart: "<<noiseStart<<std::endl;
		break;

	default:
		std::cout << "Unbinded key: " << (unsigned int)key << std::endl;
	}

	glutPostRedisplay();
}

void reshape(int width, int height){
	windowWidth = width;
	windowHeight = height;
	glViewport(0, 0, width, height);
}

void mouse(int button, int state, int x, int y)
{
	if(GLUT_LEFT_BUTTON == button && GLUT_UP == state)
	{
		startX = x/windowScale;
		startY = y/windowScale;
		startPointChanged = true;
	}
	else if(GLUT_RIGHT_BUTTON == button && GLUT_UP == state)
	{
		endX = x/windowScale;
		endY = y/windowScale;
		endpointChanged = true;
	}

}

int main(int argc, char* argv[]){
	glutInit(&argc, argv);
	glutInitContextVersion (3, 0);
	glutInitContextFlags (GLUT_CORE_PROFILE | GLUT_DEBUG);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(windowWidth, windowHeight);
	glutCreateWindow("GPGPU 6. labor: glsl");

	init();

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);

	glutMainLoop();

	return(0);
}
