
//Some Windows Headers (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>

#include "maths_funcs.h" //Anton's math class
#include "teapot.h" // teapot mesh
#include <string> 
#include <fstream>
#include <iostream>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#define ASSIGNMENT_2

// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;
GLuint shaderProgramID_minnaert;
GLuint shaderProgramID_phong;
GLuint shaderProgramID_toon;

unsigned int teapot_vao = 0;
int width = 800.0;
int height = 600.0;
GLuint loc1;
GLuint loc2;
mat4 view, persp_proj;
float rotate_y = 0.0f;

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS
glm::mat4 translateX = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));

std::string readShaderSource(const std::string& fileName)
{
	std::ifstream file(fileName.c_str()); 
	if(file.fail()) {
		cout << "error loading shader called " << fileName;
		exit (1); 
	} 
	
	std::stringstream stream;
	stream << file.rdbuf();
	file.close();

	return stream.str();
}

static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
    GLuint ShaderObj = glCreateShader(ShaderType);

    if (ShaderObj == 0) {
        fprintf(stderr, "Error creating shader type %d\n", ShaderType);
        exit(0);
    }
	std::string outShader = readShaderSource(pShaderText);
	const char* pShaderSource = outShader.c_str();

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
    glCompileShader(ShaderObj);
    GLint success;
	// check for shader related errors using glGetShaderiv
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
        exit(1);
    }
	// Attach the compiled shader object to the program object
    glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders(const char* pVertexShader, const char* pFragmentShader)
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
    GLuint shaderProgramID = glCreateProgram();
    if (shaderProgramID == 0) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }

	// Create two shader objects, one for the vertex, and one for the fragment shader
    AddShader(shaderProgramID, pVertexShader, GL_VERTEX_SHADER);
    AddShader(shaderProgramID, pFragmentShader, GL_FRAGMENT_SHADER);

    GLint Success = 0;
    GLchar ErrorLog[1024] = { 0 };


	// After compiling all shader objects and attaching them to the program, we can finally link it
    glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
        exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
    glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
        exit(1);
    }
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
    glUseProgram(shaderProgramID);

	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS

void generateObjectBufferTeapot () {
	
	GLuint vp_vbo = 0;
	glGenBuffers (1, &vp_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vp_vbo);
	glBufferData (GL_ARRAY_BUFFER, 3 * teapot_vertex_count * sizeof (float), teapot_vertex_points, GL_STATIC_DRAW);

	GLuint vn_vbo = 0;
	glGenBuffers (1, &vn_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vn_vbo);
	glBufferData (GL_ARRAY_BUFFER, 3 * teapot_vertex_count * sizeof (float), teapot_normals, GL_STATIC_DRAW);
	glGenVertexArrays (1, &teapot_vao);
	glBindVertexArray (teapot_vao);

	loc1 = glGetAttribLocation(shaderProgramID_minnaert, "vertex_position");
	loc2 = glGetAttribLocation(shaderProgramID_minnaert, "vertex_normal");
	glEnableVertexAttribArray (loc1);
	glBindBuffer (GL_ARRAY_BUFFER, vp_vbo);
	glVertexAttribPointer (loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray (loc2);
	glBindBuffer (GL_ARRAY_BUFFER, vn_vbo);
	glVertexAttribPointer (loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}
#pragma endregion VBO_FUNCTIONS


#ifdef ASSIGNMENT_1
GLuint toonTexture;

void display(){

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable (GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc (GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor (0.5f, 0.5f, 0.5f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_1D, toonTexture);

	//Here is where the code for the viewport lab will go, to get you started I have drawn a t-pot in the bottom left
	//The model1 transform rotates the object by 45 degrees, the view transform sets the camera at -40 on the z-axis, and the perspective projection is setup using Antons method

	// root
	glUseProgram(shaderProgramID_minnaert);
	int matrix_location = glGetUniformLocation(shaderProgramID_minnaert, "model");
	int view_mat_location = glGetUniformLocation(shaderProgramID_minnaert, "view");
	int proj_mat_location = glGetUniformLocation(shaderProgramID_minnaert, "proj");
	vec3 eye_position = vec3(0.0, 0.0, -100.0);
	view = translate (identity_mat4 (), eye_position);
	persp_proj = perspective(45.0, (float)width/(float)height, 0.1, 300.0);
	mat4 local1 = identity_mat4();
	local1 = rotate_y_deg(local1, rotate_y);

	mat4 global1 = identity_mat4();
	glUniformMatrix4fv (proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv (view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv (matrix_location, 1, GL_FALSE, local1.m);
	glDrawArrays (GL_TRIANGLES, 0, teapot_vertex_count);

	// left
	glUseProgram(shaderProgramID_phong);
	matrix_location = glGetUniformLocation(shaderProgramID_phong, "model");
	view_mat_location = glGetUniformLocation(shaderProgramID_phong, "view");
	proj_mat_location = glGetUniformLocation(shaderProgramID_phong, "proj");
	mat4 local2 = identity_mat4();
	local2 = rotate_y_deg(local2, rotate_y);
	local2 = translate(local2, vec3(-40.0, 0.0, 0.0));
	mat4 global2 = global1 * local2;
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global2.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	// right
	glUseProgram(shaderProgramID_toon);
	matrix_location = glGetUniformLocation(shaderProgramID_toon, "model");
	view_mat_location = glGetUniformLocation(shaderProgramID_toon, "view");
	proj_mat_location = glGetUniformLocation(shaderProgramID_toon, "proj");
	mat4 local3 = identity_mat4();
	local3 = rotate_y_deg(local3, rotate_y);
	local3 = translate(local3, vec3(40.0, 0.0, 0.0));
	mat4 global3 = global1 * local3;
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global3.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	// top-right
    glutSwapBuffers();
}

void updateScene() {	

		// Wait until at least 16ms passed since start of last frame (Effectively caps framerate at ~60fps)
	static double  last_time = 0;
	double  curr_time = timeGetTime();
	float  delta = (curr_time - last_time) * 0.001f;
	if (delta > 0.03f)
	{
		rotate_y += 5.0f;
		last_time = curr_time;
	}

	// Draw the next frame
	glutPostRedisplay();
}

void CreateToonOneDTexture()
{
	static const GLubyte toonTexData[] = 
	{
		0x44, 0x00, 0x00, 0x00,
		0x88, 0x00, 0x00, 0x00,
		0xCC, 0x00, 0x00, 0x00,
		0xFF, 0x00, 0x00, 0x00
	};

	glGenTextures(1, &toonTexture);
	glBindTexture(GL_TEXTURE_1D, toonTexture);
	glTexStorage1D(GL_TEXTURE_1D, 1, GL_RGB8, sizeof(toonTexData) / 4);
	glTexSubImage1D(GL_TEXTURE_1D, 0, 0, sizeof(toonTexData) / 4,
		GL_RGBA, GL_UNSIGNED_BYTE, toonTexData);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
}

void init()
{
	CreateToonOneDTexture();

	// Create 3 vertices that make up a triangle that fits on the viewport 
	GLfloat vertices[] = {-1.0f, -1.0f, 0.0f, 1.0,
			1.0f, -1.0f, 0.0f, 1.0, 
			0.0f, 1.0f, 0.0f, 1.0};
	// Create a color array that identfies the colors of each vertex (format R, G, B, A)
	GLfloat colors[] = {0.0f, 1.0f, 0.0f, 1.0f,
			1.0f, 0.0f, 0.0f, 1.0f,
			0.0f, 0.0f, 1.0f, 1.0f};
	// Set up the shaders
	shaderProgramID_minnaert = CompileShaders("../ThreeDEditor/src/shaders/minnaertLightingVertexShader.txt",
		"../ThreeDEditor/src/shaders/minnaertLightingFragmentShader.txt");

	shaderProgramID_phong = CompileShaders("../ThreeDEditor/src/shaders/phongLightingVertexShader.txt",
		"../ThreeDEditor/src/shaders/phongLightingFragmentShader.txt");

	shaderProgramID_toon = CompileShaders("../ThreeDEditor/src/shaders/toonLightingVertexShader.txt",
		"../ThreeDEditor/src/shaders/toonLightingFragmentShader.txt");

	// load teapot mesh into a vertex buffer array
	generateObjectBufferTeapot ();
	
}

#endif

#ifdef ASSIGNMENT_2

void display() {

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Here is where the code for the viewport lab will go, to get you started I have drawn a t-pot in the bottom left
	//The model1 transform rotates the object by 45 degrees, the view transform sets the camera at -40 on the z-axis, and the perspective projection is setup using Antons method

	// root
	glUseProgram(shaderProgramID_minnaert);
	int matrix_location = glGetUniformLocation(shaderProgramID_minnaert, "model");
	int view_mat_location = glGetUniformLocation(shaderProgramID_minnaert, "view");
	int proj_mat_location = glGetUniformLocation(shaderProgramID_minnaert, "proj");
	vec3 eye_position = vec3(0.0, 0.0, -100.0);
	view = translate(identity_mat4(), eye_position);
	persp_proj = perspective(45.0, (float)width / (float)height, 0.1, 300.0);
	mat4 local1 = identity_mat4();
	local1 = rotate_y_deg(local1, rotate_y);

	mat4 global1 = identity_mat4();
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, local1.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	// left
	glUseProgram(shaderProgramID_phong);
	matrix_location = glGetUniformLocation(shaderProgramID_phong, "model");
	view_mat_location = glGetUniformLocation(shaderProgramID_phong, "view");
	proj_mat_location = glGetUniformLocation(shaderProgramID_phong, "proj");
	mat4 local2 = identity_mat4();
	local2 = rotate_y_deg(local2, rotate_y);
	local2 = translate(local2, vec3(-40.0, 0.0, 0.0));
	mat4 global2 = global1 * local2;
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global2.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	// right
	glUseProgram(shaderProgramID_toon);
	matrix_location = glGetUniformLocation(shaderProgramID_toon, "model");
	view_mat_location = glGetUniformLocation(shaderProgramID_toon, "view");
	proj_mat_location = glGetUniformLocation(shaderProgramID_toon, "proj");
	mat4 local3 = identity_mat4();
	local3 = rotate_y_deg(local3, rotate_y);
	local3 = translate(local3, vec3(40.0, 0.0, 0.0));
	mat4 global3 = global1 * local3;
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global3.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	// top-right
	glutSwapBuffers();
}

void updateScene() {

	// Wait until at least 16ms passed since start of last frame (Effectively caps framerate at ~60fps)
	static double  last_time = 0;
	double  curr_time = timeGetTime();
	float  delta = (curr_time - last_time) * 0.001f;
	if (delta > 0.03f)
	{
		rotate_y += 5.0f;
		last_time = curr_time;
	}

	// Draw the next frame
	glutPostRedisplay();
}

void init()
{

	// Create 3 vertices that make up a triangle that fits on the viewport 
	GLfloat vertices[] = { -1.0f, -1.0f, 0.0f, 1.0,
		1.0f, -1.0f, 0.0f, 1.0,
		0.0f, 1.0f, 0.0f, 1.0 };
	// Create a color array that identfies the colors of each vertex (format R, G, B, A)
	GLfloat colors[] = { 0.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f };
	// Set up the shaders
	shaderProgramID_minnaert = CompileShaders("../ThreeDEditor/src/shaders/minnaertLightingVertexShader.txt",
		"../ThreeDEditor/src/shaders/minnaertLightingFragmentShader.txt");

	shaderProgramID_phong = CompileShaders("../ThreeDEditor/src/shaders/phongLightingVertexShader.txt",
		"../ThreeDEditor/src/shaders/phongLightingFragmentShader.txt");

	shaderProgramID_toon = CompileShaders("../ThreeDEditor/src/shaders/toonLightingVertexShader.txt",
		"../ThreeDEditor/src/shaders/toonLightingFragmentShader.txt");

	// load teapot mesh into a vertex buffer array
	generateObjectBufferTeapot();

}

#endif

int main(int argc, char** argv){

	// Set up the window
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(width, height);
    glutCreateWindow("Viewport Teapots");
	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);

	 // A call to glewInit() must be done after glut is initialized!
	glewExperimental = GL_TRUE; //for non-lab machines, this line gives better modern GL support
    GLenum res = glewInit();
	// Check for any errors
    if (res != GLEW_OK) {
      fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
      return 1;
    }
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
    return 0;
}











