#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include<math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "textfile.h"

#include "Vectors.h"
#include "Matrices.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

using namespace std;

// Default window size
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

bool mouse_pressed = false;
int current_x = -1;
int current_y = -1;

enum TransMode
{
	GeoTranslation = 0,
	GeoRotation = 1,
	GeoScaling = 2,
	ViewCenter = 3,
	ViewEye = 4,
	ViewUp = 5,
    Lighting = 6,
    Shininess = 7
};

GLint iLocMVP;
GLuint iLoc_Light_Mode;
int Light_Mode; //0:dir 1:position 2:spot
GLuint iLoc_Shading_Mode;
int Shading_Mode;
GLuint iLoc_view_matrix;
GLuint iLoc_project_matrix;
GLuint iLoc_rotation_matrix;
GLuint iLoc_scaling_matrix;
GLuint iLoc_translation_matrix;

struct iLoc_Light
{
    GLuint d_position;//direction light
    GLuint d_direction;
    GLuint d_diffuse_intensity;
    
    GLuint p_position;//position light
    GLuint p_diffuse_intensity;
    
    GLuint s_position;//spot light
    GLuint s_direction;
    GLuint s_diffuse_intensity;
    GLuint spot_attenuation;
    GLuint spot_exp;//spot
    GLuint cutoff;
    
    GLuint ambient_intensity;
    
    
    
    GLuint point_attenuation;
    GLuint shininess;//specular
}iLoc_Light;

struct Light
{
    Vector4 d_position;//direction light
    Vector4 d_direction;
    Vector4 d_diffuse_intensity;
    Vector4 p_position;//position light
    Vector4 p_diffuse_intensity;
    Vector4 s_position;//spot light
    Vector4 s_direction;
    Vector4 s_diffuse_intensity;
    Vector4 ambient_intensity;
    Vector3 point_attenuation;
    Vector3    spot_attenuation;
    float spot_exp;//spot
    float cutoff;
    float shininess;//specular
}Light;

struct iLoc_PhongMaterial
{
    GLuint Ka;
    GLuint Kd;
    GLuint Ks;
}iLoc_PhongMaterial;

vector<string> filenames; // .obj filename list

struct PhongMaterial
{
    Vector3 Ka;
    Vector3 Kd;
    Vector3 Ks;
    
};

typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	GLuint p_color;
	int vertex_count;
	GLuint p_normal;
	PhongMaterial material;
	int indexCount;
	GLuint m_texture;
} Shape;

struct model
{
	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 rotation = Vector3(0, 0, 0);	// Euler form

	vector<Shape> shapes;
};
vector<model> models;

struct camera
{
	Vector3 position;
	Vector3 center;
	Vector3 up_vector;
};
camera main_camera;

struct project_setting
{
	GLfloat nearClip, farClip;
	GLfloat fovy;
	GLfloat aspect;
	GLfloat left, right, top, bottom;
};
project_setting proj;

enum ProjMode
{
	Orthogonal = 0,
	Perspective = 1,
};
ProjMode cur_proj_mode = Orthogonal;
TransMode cur_trans_mode = GeoTranslation;

Matrix4 rotation_matrix;
Matrix4 scaling_matrix;
Matrix4 translation_matrix;
Matrix4 view_matrix;
Matrix4 project_matrix;

Shape quad;
Shape m_shpae;
int cur_idx = 0; // represent which model should be rendered now


static GLvoid Normalize(GLfloat v[3])
{
	GLfloat l;

	l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] /= l;
	v[1] /= l;
	v[2] /= l;
}

static GLvoid Cross(GLfloat u[3], GLfloat v[3], GLfloat n[3])
{

	n[0] = u[1] * v[2] - u[2] * v[1];
	n[1] = u[2] * v[0] - u[0] * v[2];
	n[2] = u[0] * v[1] - u[1] * v[0];
}


// [TODO] given a translation vector then output a Matrix4 (Translation Matrix)
Matrix4 translate(Vector3 vec)
{
    translation_matrix.set
    (
     1, 0, 0, vec.x,
     0, 1, 0, vec.y,
     0, 0, 1, vec.z,
     0, 0, 0, 1
     );
    
    return translation_matrix;
}

// [TODO] given a scaling vector then output a Matrix4 (Scaling Matrix)
Matrix4 scaling(Vector3 vec)
{
    scaling_matrix.set
    (
     vec.x, 0, 0, 0,
     0, vec.y, 0, 0,
     0, 0, vec.z, 0,
     0, 0, 0, 1
     );
    
    return scaling_matrix;
}


// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
Matrix4 rotateX(GLfloat val)
{
	Matrix4 mat;

    mat = Matrix4
    (
     1, 0, 0, 0,
     0, cosf(val), -sinf(val), 0,
     0, sinf(val), cosf(val), 0,
     0, 0, 0, 1
     );

	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Y (rotate alone axis-Y)
Matrix4 rotateY(GLfloat val)
{
	Matrix4 mat;

    mat = Matrix4
    (
     cosf(val), 0, sinf(val), 0,
     0, 1, 0, 0,
     -sinf(val), 0, cosf(val), 0,
     0, 0, 0, 1
     );

	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Z (rotate alone axis-Z)
Matrix4 rotateZ(GLfloat val)
{
	Matrix4 mat;

    mat = Matrix4
    (
     cosf(val), -sinf(val), 0, 0,
     sinf(val), cosf(val), 0, 0,
     0, 0, 1, 0,
     0, 0, 0, 1
     );

	return mat;
}

Matrix4 rotate(Vector3 vec)
{
	return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}

// [TODO] compute viewing matrix accroding to the setting of main_camera
void setViewingMatrix()
{
    Matrix4 T, R;
    T = Matrix4(
                1, 0, 0, -main_camera.position.x,
                0, 1, 0, -main_camera.position.y,
                0, 0, 1, -main_camera.position.z,
                0, 0, 0, 1
                );
    
    Vector3 forward = main_camera.center - main_camera.position;
    Vector3 up = main_camera.up_vector.normalize();
    
    Vector3 Rz = forward.normalize();
    Vector3 Rx = Rz.cross(up);
    Vector3 Ry = Rx.cross(forward);
    
    R = Matrix4(
                Rx.x, Rx.y, Rx.z, 0,
                Ry.x, Ry.y, Ry.z, 0,
                -Rz.x, -Rz.y, -Rz.z, 0,
                0, 0, 0, 1
                );
    
    view_matrix = R * T;
}

// [TODO] compute orthogonal projection matrix
void setOrthogonal()
{
	cur_proj_mode = Orthogonal;
    project_matrix = Matrix4(
                             2 / (proj.right - proj.left)/proj.aspect, 0, 0, -(proj.left + proj.right) / (proj.right - proj.left)/proj.aspect,
                             0, 2 / (proj.top - proj.bottom), 0, -(proj.top + proj.bottom) / (proj.top - proj.bottom),
                             0, 0, -2 / (proj.farClip - proj.nearClip), -(proj.farClip + proj.nearClip) / (proj.farClip - proj.nearClip),
                             0, 0, 0, 1
                             );
}

// [TODO] compute persepective projection matrix
void setPerspective()
{
	cur_proj_mode = Perspective;
    project_matrix = Matrix4(
                             -(1/tan(proj.fovy/2))/proj.aspect, 0, 0, 0,
                             0, -(1 / tan(proj.fovy / 2)), 0, 0,
                             0, 0, (proj.farClip + proj.nearClip) / (proj.nearClip - proj.farClip), (2*proj.farClip*proj.nearClip/(proj.nearClip-proj.farClip)),
                             0, 0, -1, 0
                             );
}


// Vertex buffers
GLuint VAO, VBO;

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
    proj.aspect = (float)width / height;
    setViewingMatrix();
    if(cur_proj_mode == Orthogonal){
        setOrthogonal();
    }
    if(cur_proj_mode == Perspective){
        setPerspective();
    }
    
    Matrix4 MVP;
    GLfloat mvp[16];
    
    MVP = project_matrix * view_matrix;
    
    mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
    mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
    mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
    mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];   mvp[15] = MVP[15];
    
    // use uniform to send mvp to vertex shader
    glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);
    for(int i = 0; i < models[cur_idx].shapes.size(); i++){
        glBindVertexArray(models[cur_idx].shapes[i].vao);
        glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
    }
}

// Render function for display rendering
void RenderScene(void) {	
	// clear canvas
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	Matrix4 T, R, S;
    T = translate(models[cur_idx].position);
    R = rotate(models[cur_idx].rotation);
    S = scaling(models[cur_idx].scale);

	Matrix4 MVP;
	GLfloat mvp[16];

	MVP = project_matrix * view_matrix * T * R * S;
	// [TODO] row-major ---> column-major

    mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
    mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
    mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
    mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];   mvp[15] = MVP[15];
    /*
    glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);
    glUniform1i(iLocShade, 0);
    glViewport(-WINDOW_WIDTH/2, 0, WINDOW_WIDTH*2, WINDOW_HEIGHT*2);
    for(int i = 0; i < models[cur_idx].shapes.size(); i++){
        glBindVertexArray(models[cur_idx].shapes[i].vao);
        glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
    }
    glUniform1i(iLocShade, 1);
    glViewport(WINDOW_WIDTH/2, 0, WINDOW_WIDTH*2, WINDOW_HEIGHT*2);
    for(int i = 0; i < models[cur_idx].shapes.size(); i++){
        glBindVertexArray(models[cur_idx].shapes[i].vao);
        glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
    }*/
    //------------------------------------------//

    for (int i = 0; i < models[cur_idx].shapes.size(); i++)
     {
     glUniformMatrix4fv(iLocMVP, 1, GL_TRUE, MVP.get());
     glUniformMatrix4fv(iLoc_view_matrix, 1, GL_TRUE, view_matrix.get());
     glUniformMatrix4fv(iLoc_project_matrix, 1, GL_TRUE, project_matrix.get());
     glUniformMatrix4fv(iLoc_rotation_matrix, 1, GL_TRUE, rotation_matrix.get());
     glUniformMatrix4fv(iLoc_scaling_matrix, 1, GL_TRUE, scaling_matrix.get());
     glUniformMatrix4fv(iLoc_translation_matrix, 1, GL_TRUE, translation_matrix.get());
     
     GLfloat Ka[4] = { models[cur_idx].shapes[i].material.Ka[0], models[cur_idx].shapes[i].material.Ka[1],
     models[cur_idx].shapes[i].material.Ka[2], 1 };
     GLfloat Ks[4] = { models[cur_idx].shapes[i].material.Ks[0], models[cur_idx].shapes[i].material.Ks[1],
     models[cur_idx].shapes[i].material.Ks[2], 1 };
     GLfloat Kd[4] = { models[cur_idx].shapes[i].material.Kd[0], models[cur_idx].shapes[i].material.Kd[1],
     models[cur_idx].shapes[i].material.Kd[2], 1 };
     
     glUniform4fv(iLoc_PhongMaterial.Ka, 1, Ka);
     glUniform4fv(iLoc_PhongMaterial.Ks, 1, Ks);
     glUniform4fv(iLoc_PhongMaterial.Kd, 1, Kd);
     
     glUniform4f(iLoc_Light.d_position, Light.d_position.x, Light.d_position.y, Light.d_position.z, Light.d_position.w);
     glUniform4f(iLoc_Light.d_direction, Light.d_direction.x, Light.d_direction.y, Light.d_direction.z, Light.d_direction.w);
     glUniform4f(iLoc_Light.p_position, Light.p_position.x, Light.p_position.y, Light.p_position.z, Light.p_position.w);
     glUniform4f(iLoc_Light.s_position, Light.s_position.x, Light.s_position.y, Light.s_position.z, Light.s_position.w);
     glUniform4f(iLoc_Light.s_direction, Light.s_direction.x, Light.s_direction.y, Light.s_direction.z, Light.s_direction.w);
     glUniform4f(iLoc_Light.ambient_intensity, Light.ambient_intensity.x, Light.ambient_intensity.y, Light.ambient_intensity.z, Light.ambient_intensity.w);
     glUniform4f(iLoc_Light.d_diffuse_intensity, Light.d_diffuse_intensity.x, Light.d_diffuse_intensity.y, Light.d_diffuse_intensity.z, Light.d_diffuse_intensity.w);
     glUniform4f(iLoc_Light.p_diffuse_intensity, Light.p_diffuse_intensity.x, Light.p_diffuse_intensity.y, Light.p_diffuse_intensity.z, Light.p_diffuse_intensity.w);
     glUniform4f(iLoc_Light.s_diffuse_intensity, Light.s_diffuse_intensity.x, Light.s_diffuse_intensity.y, Light.s_diffuse_intensity.z, Light.s_diffuse_intensity.w);
     glUniform3f(iLoc_Light.point_attenuation, Light.point_attenuation.x, Light.point_attenuation.y, Light.point_attenuation.z);
     glUniform3f(iLoc_Light.spot_attenuation, Light.spot_attenuation.x, Light.spot_attenuation.y, Light.spot_attenuation.z);
     
     glUniform1f(iLoc_Light.shininess, Light.shininess);
     glUniform1f(iLoc_Light.spot_exp, Light.spot_exp);
     glUniform1f(iLoc_Light.cutoff, Light.cutoff);
     glUniform1i(iLoc_Light_Mode, Light_Mode);
     glUniform1i(iLoc_Shading_Mode, Shading_Mode);
     
     glBindVertexArray(models[cur_idx].shapes[i].vao);
     glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
     }
    //-----------------------------------------//
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    switch (action) {
        case GLFW_PRESS:
            switch (key) {
                case GLFW_KEY_Z:
                    cur_idx = (cur_idx + models.size() - 1) % models.size();
                    break;
                    
                case GLFW_KEY_X:
                    cur_idx = (cur_idx + 1) % models.size();
                    break;
                    
                case GLFW_KEY_O:
                    setOrthogonal();
                    break;
                    
                case GLFW_KEY_P:
                    setPerspective();
                    break;
                    
                case GLFW_KEY_T:
                    cur_trans_mode = GeoTranslation;
                    break;
                    
                case GLFW_KEY_S:
                    cur_trans_mode = GeoScaling;
                    break;
                    
                case GLFW_KEY_R:
                    cur_trans_mode = GeoRotation;
                    break;
                    
                case GLFW_KEY_E:
                    cur_trans_mode = ViewEye;
                    break;
                    
                case GLFW_KEY_C:
                    cur_trans_mode = ViewCenter;
                    break;
                    
                case GLFW_KEY_U:
                    cur_trans_mode = ViewUp;
                    break;
                    
                case GLFW_KEY_L:
                    Light_Mode = (Light_Mode + 1) % 3;
                    break;
                    
                case GLFW_KEY_K:
                    cur_trans_mode = Lighting;
                    break;
                    
                case GLFW_KEY_J:
                    cur_trans_mode = Shininess;
                    break;
                    
                case GLFW_KEY_ESCAPE:
                    exit(0);
                    break;
                    
                default:
                    break;
            }
            break;
            
        default:
            break;
    }
}


void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // [TODO] scroll up positive, otherwise it would be negtive
    switch (cur_trans_mode) {
        case GeoTranslation:
            models[cur_idx].position.z += yoffset / 2;
            break;
            
        case GeoScaling:
            models[cur_idx].scale.z += yoffset / 2;
            break;
            
        case GeoRotation:
            models[cur_idx].rotation.z += yoffset / 2;
            break;
            
        case ViewEye:
            main_camera.position.z += yoffset / 2;
            setViewingMatrix();
            break;
            
        case ViewCenter:
            main_camera.center.z += yoffset / 2;
            setViewingMatrix();
            break;
            
        case ViewUp:
            cout << yoffset / 2;
            cout << main_camera.up_vector;
            main_camera.up_vector.z += yoffset / 2;
            setViewingMatrix();
            break;
            
        case Lighting:
            if (Light_Mode == 2)
            {
                if (Light.cutoff <= 90 && Light.cutoff >= 0){Light.cutoff += yoffset / 20;}
                else if(Light.cutoff > 90){Light.cutoff = 90;}
                else if (Light.cutoff < 0){Light.cutoff = 0;}
            }
            else if (Light_Mode == 0)
            {
                if (Light.d_diffuse_intensity.x >= 0 && Light.d_diffuse_intensity.y >= 0 && Light.d_diffuse_intensity.z >= 0)
                {
                    Light.d_diffuse_intensity.x += yoffset / 20;
                    Light.d_diffuse_intensity.y += yoffset / 20;
                    Light.d_diffuse_intensity.z += yoffset / 20;
                }
                else if (Light.d_diffuse_intensity.x < 0 && Light.d_diffuse_intensity.y < 0 && Light.d_diffuse_intensity.z < 0)
                {
                    Light.d_diffuse_intensity.x = 0;
                    Light.d_diffuse_intensity.y = 0;
                    Light.d_diffuse_intensity.z = 0;
                }
            }
            else if (Light_Mode == 1)
            {
                if (Light.p_diffuse_intensity.x >= 0  && Light.p_diffuse_intensity.y >= 0 && Light.p_diffuse_intensity.z >= 0)
                {
                    Light.p_diffuse_intensity.x += yoffset / 20;
                    Light.p_diffuse_intensity.y += yoffset / 20;
                    Light.p_diffuse_intensity.z += yoffset / 20;
                }
                else if (Light.p_diffuse_intensity.x < 0 && Light.p_diffuse_intensity.y < 0 && Light.p_diffuse_intensity.z < 0)
                {
                    Light.p_diffuse_intensity.x = 0;
                    Light.p_diffuse_intensity.y = 0;
                    Light.p_diffuse_intensity.z = 0;
                }
            }
            
            break;
        case Shininess:
            printf("shininess: %f", Light.shininess);
            Light.shininess += yoffset;
            break;
        default:
            break;
    }
}


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        mouse_pressed = true;
    }
    else if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE){
        mouse_pressed = false;
    }
		
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    if(mouse_pressed){
        if(current_y == -1 && current_x == -1){
            current_y = ypos;
            current_x = xpos;
        }
        else{
            int diff_x = xpos - current_x;
            int diff_y = ypos - current_y;
            current_x = xpos;
            current_y = ypos;
            if (cur_trans_mode == GeoTranslation)
            {
                models[cur_idx].position.x += diff_x*(1.0 / 400.0);
                models[cur_idx].position.y -= diff_y*(1.0 / 400.0);
            }
            else if (cur_trans_mode == GeoScaling)
            {
                models[cur_idx].scale.x += diff_x*0.025;
                models[cur_idx].scale.y += diff_y*0.025;
            }
            else if (cur_trans_mode == GeoRotation)
            {
                models[cur_idx].rotation.x += 3.14 / 180.0*diff_y*(45.0 / 400.0);
                models[cur_idx].rotation.y += 3.14 / 180.0*diff_x*(45.0 / 400.0);
            }
        }
    }
    else{
        current_x = -1;
        current_y = -1;
    }
}

void setShaders()
{
	GLuint v, f, p;
	char *vs = NULL;
	char *fs = NULL;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead("shader.vs");
	fs = textFileRead("shader.fs");

	glShaderSource(v, 1, (const GLchar**)&vs, NULL);
	glShaderSource(f, 1, (const GLchar**)&fs, NULL);

	free(vs);
	free(fs);

	GLint success;
	char infoLog[1000];
	// compile vertex shader
	glCompileShader(v);
	// check for shader compile errors
	glGetShaderiv(v, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(v, 1000, NULL, infoLog);
		std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// compile fragment shader
	glCompileShader(f);
	// check for shader compile errors
	glGetShaderiv(f, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(f, 1000, NULL, infoLog);
		std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// create program object
	p = glCreateProgram();

	// attach shaders to program object
	glAttachShader(p,f);
	glAttachShader(p,v);

	// link program
	glLinkProgram(p);
	// check for linking errors
	glGetProgramiv(p, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(p, 1000, NULL, infoLog);
		std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
	}

	glDeleteShader(v);
	glDeleteShader(f);

	iLocMVP = glGetUniformLocation(p, "mvp");

    iLocMVP = glGetUniformLocation(p, "mvp");
    iLoc_Light.d_position = glGetUniformLocation(p, "light.d_position");
    iLoc_Light.d_direction = glGetUniformLocation(p, "light.d_direction");
    iLoc_Light.p_position = glGetUniformLocation(p, "light.p_position");
    iLoc_Light.s_position = glGetUniformLocation(p, "light.s_position");
    iLoc_Light.s_direction = glGetUniformLocation(p, "light.s_direction");
    iLoc_Light.ambient_intensity = glGetUniformLocation(p, "light.ambient_intensity");
    iLoc_Light.d_diffuse_intensity = glGetUniformLocation(p, "light.d_diffuse_intensity");
    iLoc_Light.p_diffuse_intensity = glGetUniformLocation(p, "light.p_diffuse_intensity");
    iLoc_Light.s_diffuse_intensity = glGetUniformLocation(p, "light.s_diffuse_intensity");
    iLoc_Light.point_attenuation = glGetUniformLocation(p, "light.point_attenuation");
    iLoc_Light.spot_attenuation = glGetUniformLocation(p, "light.spot_attenuation");
    iLoc_Light.spot_exp = glGetUniformLocation(p, "light.spot_exp");
    iLoc_Light.cutoff = glGetUniformLocation(p, "light.cutoff");
    iLoc_Light.shininess = glGetUniformLocation(p, "light.shininess");
    
    iLoc_Light_Mode = glGetUniformLocation(p, "Light_Mode");
    iLoc_Shading_Mode = glGetUniformLocation(p, "Shading_Mode");
    iLoc_PhongMaterial.Ka = glGetUniformLocation(p, "material.Ka");
    iLoc_PhongMaterial.Kd = glGetUniformLocation(p, "material.Kd");
    iLoc_PhongMaterial.Ks = glGetUniformLocation(p, "material.Ks");
    
    iLoc_view_matrix = glGetUniformLocation(p, "view_matrix");
    iLoc_project_matrix = glGetUniformLocation(p, "project_matrix");
    iLoc_rotation_matrix = glGetUniformLocation(p, "rotation_matrix");
    iLoc_scaling_matrix = glGetUniformLocation(p, "scaling_matrix");
    iLoc_translation_matrix = glGetUniformLocation(p, "translation_matrix");

	if (success)
		glUseProgram(p);
    else
    {
        system("pause");
        exit(123);
    }
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, tinyobj::shape_t* shape)
{
	vector<float> xVector, yVector, zVector;
	float minX = 10000, maxX = -10000, minY = 10000, maxY = -10000, minZ = 10000, maxZ = -10000;

	// find out min and max value of X, Y and Z axis
	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//maxs = max(maxs, attrib->vertices.at(i));
		if (i % 3 == 0)
		{

			xVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minX)
			{
				minX = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxX)
			{
				maxX = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 1)
		{
			yVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minY)
			{
				minY = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxY)
			{
				maxY = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 2)
		{
			zVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minZ)
			{
				minZ = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxZ)
			{
				maxZ = attrib->vertices.at(i);
			}
		}
	}

	float offsetX = (maxX + minX) / 2;
	float offsetY = (maxY + minY) / 2;
	float offsetZ = (maxZ + minZ) / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		if (offsetX != 0 && i % 3 == 0)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetX;
		}
		else if (offsetY != 0 && i % 3 == 1)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetY;
		}
		else if (offsetZ != 0 && i % 3 == 2)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetZ;
		}
	}

	float greatestAxis = maxX - minX;
	float distanceOfYAxis = maxY - minY;
	float distanceOfZAxis = maxZ - minZ;

	if (distanceOfYAxis > greatestAxis)
	{
		greatestAxis = distanceOfYAxis;
	}

	if (distanceOfZAxis > greatestAxis)
	{
		greatestAxis = distanceOfZAxis;
	}

	float scale = greatestAxis / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//std::cout << i << " = " << (double)(attrib.vertices.at(i) / greatestAxis) << std::endl;
		attrib->vertices.at(i) = attrib->vertices.at(i) / scale;
	}
	size_t index_offset = 0;
	for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); f++) {
		int fv = shape->mesh.num_face_vertices[f];

		// Loop over vertices in the face.
		for (size_t v = 0; v < fv; v++) {
			// access to vertex
			tinyobj::index_t idx = shape->mesh.indices[index_offset + v];
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 0]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 1]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 2]);
			// Optional: vertex colors
			colors.push_back(attrib->colors[3 * idx.vertex_index + 0]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 1]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 2]);
			// Optional: vertex normals
			if (idx.normal_index >= 0) {
				normals.push_back(attrib->normals[3 * idx.normal_index + 0]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 1]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 2]);
			}
		}
		index_offset += fv;
	}
}

string GetBaseDir(const string& filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos)
		return filepath.substr(0, filepath.find_last_of("/\\"));
	return "";
}

void LoadModels(string model_path)
{
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	tinyobj::attrib_t attrib;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;
	vector<GLfloat> normals;

	string err;
	string warn;

	string base_dir = GetBaseDir(model_path); // handle .mtl with relative path

#ifdef _WIN32
	base_dir += "\\";
#else
	base_dir += "/";
#endif

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str(), base_dir.c_str());

	if (!warn.empty()) {
		cout << warn << std::endl;
	}

	if (!err.empty()) {
		cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	printf("Load Models Success ! Shapes size %d Material size %d\n", shapes.size(), materials.size());
	model tmp_model;

	vector<PhongMaterial> allMaterial;
	for (int i = 0; i < materials.size(); i++)
	{
		PhongMaterial material;
		material.Ka = Vector3(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
		material.Kd = Vector3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
		material.Ks = Vector3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
		allMaterial.push_back(material);
	}

	for (int i = 0; i < shapes.size(); i++)
	{

		vertices.clear();
		colors.clear();
		normals.clear();
		normalization(&attrib, vertices, colors, normals, &shapes[i]);
		// printf("Vertices size: %d", vertices.size() / 3);

		Shape tmp_shape;
		glGenVertexArrays(1, &tmp_shape.vao);
		glBindVertexArray(tmp_shape.vao);

		glGenBuffers(1, &tmp_shape.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT), &vertices.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		tmp_shape.vertex_count = vertices.size() / 3;

		glGenBuffers(1, &tmp_shape.p_color);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
		glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GL_FLOAT), &colors.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glGenBuffers(1, &tmp_shape.p_normal);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_normal);
		
			glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GL_FLOAT), &normals.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		// not support per face material, use material of first face
		if (allMaterial.size() > 0)
			tmp_shape.material = allMaterial[shapes[i].mesh.material_ids[0]];
		tmp_model.shapes.push_back(tmp_shape);
	}
	shapes.clear();
	materials.clear();
	models.push_back(tmp_model);
}

void initParameter()
{
	proj.left = -1;
	proj.right = 1;
	proj.top = 1;
	proj.bottom = -1;
	proj.nearClip = 0.001;
	proj.farClip = 100.0;
	proj.fovy = 80;
	proj.aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

	main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
	main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);

    Light_Mode = 0;
    Shading_Mode = 1;
    
    Light.d_position = Vector4(1, 1, 1, 0);
    Light.d_direction = Vector4(0, 0, 0, 0);
    Light.d_diffuse_intensity = Vector4(1, 1, 1, 1);
    
    Light.p_position = Vector4(0, 2, 1, 1);
    Light.p_diffuse_intensity = Vector4(1, 1, 1, 1);
    
    Light.s_position = Vector4(0, 0, 2, 1);
    Light.s_direction = Vector4(0, 0, -1, 0);
    Light.s_diffuse_intensity = Vector4(1, 1, 1, 1);
    
    Light.ambient_intensity = Vector4(0.15, 0.15, 0.15, 1);
    Light.point_attenuation = Vector3(0.01, 0.8, 0.1);
    Light.spot_attenuation = Vector3(0.05, 0.3, 0.6);
    Light.spot_exp = 50;
    Light.cutoff = 30*3.14159/180;
    Light.shininess = 64;
    
    //CUR_WINDOW_WIDTH = WINDOW_WIDTH;
    //CUR_WINDOW_HEIGHT = WINDOW_HEIGHT;

    setViewingMatrix();
	setPerspective();	//set default projection matrix as perspective matrix
}

void setupRC()
{
	// setup shaders
	setShaders();
	initParameter();

	// OpenGL States and Values
	glClearColor(0.2, 0.2, 0.2, 1.0);
	vector<string> model_list{ "../NormalModels/bunny5KN.obj", "../NormalModels/dragon10KN.obj", "../NormalModels/lucy25KN.obj", "../NormalModels/teapot4KN.obj", "../NormalModels/dolphinN.obj"};
	// [TODO] Load five model at here
    for(int i=cur_idx; i<5; i++)
        LoadModels(model_list[i]);
}

void glPrintContextInfo(bool printExtension)
{
	cout << "GL_VENDOR = " << (const char*)glGetString(GL_VENDOR) << endl;
	cout << "GL_RENDERER = " << (const char*)glGetString(GL_RENDERER) << endl;
	cout << "GL_VERSION = " << (const char*)glGetString(GL_VERSION) << endl;
	cout << "GL_SHADING_LANGUAGE_VERSION = " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	if (printExtension)
	{
		GLint numExt;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
		cout << "GL_EXTENSIONS =" << endl;
		for (GLint i = 0; i < numExt; i++)
		{
			cout << "\t" << (const char*)glGetStringi(GL_EXTENSIONS, i) << endl;
		}
	}
}


int main(int argc, char **argv)
{
    // initial glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
#endif

    
    // create window
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Student ID HW2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    
    // load OpenGL function pointer
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
	// register glfw callback functions
    glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);

    glfwSetFramebufferSizeCallback(window, ChangeSize);
	glEnable(GL_DEPTH_TEST);
	// Setup render context
	setupRC();

	// main loop
    while (!glfwWindowShouldClose(window))
    {
        // render
        RenderScene();
        
        // swap buffer from back to front
        glfwSwapBuffers(window);
        
        // Poll input event
        glfwPollEvents();
    }
	
	// just for compatibiliy purposes
	return 0;
}
