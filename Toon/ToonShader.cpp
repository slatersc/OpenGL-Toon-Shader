/*
    Chris Sadler
    Toon Shader Environment

    OpenGL tools include:
    - glew
    - glfw v3.04

    Built with Codeblocks 13.12 using MinGW compiler
    on Windows 10  - Jan 15, 2015

    This environment is lit with a simple toon/cel shader.
    The default object is the Utah Teapot which is rotating
    around its y axis. The light source can be moved with the
    arrow keys to traverse world coordinates x,y. while the numpad
    keys of + (ADDITION) and - (SUBTRACTION) traverse the z axis.

    Next goals into and through the next project(s) will be:
    - improving controls for moving around the environment scene
    - easier loading of models into the scene,
    - shader list options per object.
    - multiple light setups
    - light visualization
    - dynamic camera options
    - camera visualizations
    - animation recording options
*/



#define GLFW_INCLUDE_GLU
#include <GL/glew.h>
#include <GL/glfw3.h>

//#pragma comment (lib, "glew32.lib")

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string.h>
#include <string>


using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

string PATH = "C:/Code/Toon/";
string SHADER = "toon";
string OBJ = "voxel.obj";

float rot_x = 0.0;
float rot_y = 0.0;
float rot_z = 0.0;


class Triangle{
  public:
      double         X[3];
      double         Y[3];
      double         Z[3];
};

class XYZPoints{
  public:
      float         x;
      float         y;
      float         z;
};

class UVPoints{
  public:
      float         u;
      float         v;
};

class Object{
  public:
	vector <XYZPoints> vertices;  // vertices
	vector <UVPoints> uvs; // vertex texture coordinates
	vector <XYZPoints> normals; // vertex normals


	bool loadOBJ(const char * path){
        printf("Loading OBJ file %s...\n", path);

        vector<unsigned int> vertexIndices, uvIndices, normalIndices;
        vector <XYZPoints> temp_vertices;
        vector <UVPoints> temp_uvs;
        vector <XYZPoints> temp_normals;

        FILE * file = fopen(path, "r");
        if( file == NULL ){
            printf(".obj file cannot open\n");
            getchar();
            return false;
        }

        while( 1 ){

            char lineHeader[128];
            // read the first word of the line
            int res = fscanf(file, "%s", lineHeader);
            if (res == EOF)
                break; // EOF = End Of File. Quit the loop.

            // else : parse lineHeader

            if ( strcmp( lineHeader, "v" ) == 0 ){
                XYZPoints vertex;
                fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z );
                temp_vertices.push_back(vertex);
            }else if ( strcmp( lineHeader, "vt" ) == 0 ){
                UVPoints uv;
                fscanf(file, "%f %f\n", &uv.u, &uv.v );
                //uv.v = -uv.v; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
                temp_uvs.push_back(uv);
            }else if ( strcmp( lineHeader, "vn" ) == 0 ){
                XYZPoints normal;
                fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
                temp_normals.push_back(normal);
            }else if ( strcmp( lineHeader, "f" ) == 0 ){
                string vertex1, vertex2, vertex3;
                unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
                int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2] );
                if (matches != 9){
                    printf("File can't be read by our simple parser :-( Try exporting with other options\n");
                    return false;
                }
                vertexIndices.push_back(vertexIndex[0]);
                vertexIndices.push_back(vertexIndex[1]);
                vertexIndices.push_back(vertexIndex[2]);
                uvIndices    .push_back(uvIndex[0]);
                uvIndices    .push_back(uvIndex[1]);
                uvIndices    .push_back(uvIndex[2]);
                normalIndices.push_back(normalIndex[0]);
                normalIndices.push_back(normalIndex[1]);
                normalIndices.push_back(normalIndex[2]);
            }else{
                // Probably a comment, eat up the rest of the line
                char stupidBuffer[1000];
                fgets(stupidBuffer, 1000, file);
            }

        }

        // For each vertex of each triangle
        for( unsigned int i=0; i<vertexIndices.size(); i++ ){

            // Get the indices of its attributes
            unsigned int vertexIndex = vertexIndices[i];
            unsigned int uvIndex = uvIndices[i];
            unsigned int normalIndex = normalIndices[i];

            // Get the attributes thanks to the index
            XYZPoints vertex = temp_vertices[ vertexIndex-1 ];
            UVPoints uv = temp_uvs[ uvIndex-1 ];
            XYZPoints normal = temp_normals[ normalIndex-1 ];

            // Put the attributes in buffers
            vertices.push_back(vertex);
            uvs     .push_back(uv);
            normals .push_back(normal);

        }

	return true;
	}

// switch texture loads to SOIL
GLuint loadTGA_glfw(const char * imagepath){

        // Create one OpenGL texture
        GLuint textureID;
        glGenTextures(1, &textureID);

        // "Bind" the newly created texture : all future texture functions will modify this texture
        glBindTexture(GL_TEXTURE_2D, textureID);

        // Read the file, call glTexImage2D with the right parameters
        //glfwLoadTexture2D(imagepath, 0);
        // not supported in glfw anyone. switch to soil for next version

        // Nice trilinear filtering.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Return the ID of the texture we just created
        return textureID;
    }



};

class Scene{
  public:
    GLuint texture;
	vector <Object> objs;

	Scene(){
	};

	void loadObjects(void){
        //load objects
        Object obj_temp;

        string str = PATH + OBJ;
        char * objPath = new char[str.size() + 1];
        std::copy(str.begin(), str.end(), objPath);
        objPath[str.size()] = '\0'; // don't forget the terminating 0
		obj_temp.loadOBJ(objPath);

		delete[] objPath;

		objs.push_back(obj_temp);

		printf("loadObjects\n");
	}




	void drawVoxel(int x, int y, int z){

        float s = 0.01;// size of voxel cubed




	    float sx = s*x;
		float sy = s*y;
		float sz = s*z;
		float sxs = s*x+s;
		float sys = s*y+s;
		float szs = s*z+s;

        static GLuint vox_list = 0;

        if (!vox_list)
        {
            // Start recording displaylist
            vox_list = glGenLists(1);
            glNewList(vox_list, GL_COMPILE_AND_EXECUTE);

	        glBegin(GL_QUADS);
        	glVertex3f(sx, sy, szs);  glVertex3f(sxs, sy, szs);  glVertex3f(sxs, sy, sz);  glVertex3f(sx, sy, sz);

        	glVertex3f(sx, sy, sz);  glVertex3f(sxs, sy, sz);  glVertex3f(sxs, sys, sz);  glVertex3f(sx, sys, sz);

        	glVertex3f(sx, sys, szs);  glVertex3f(sx, sy, szs);  glVertex3f(sx, sy, sz);  glVertex3f(sx, sys, sz);

        	glVertex3f(sx, sys, szs);  glVertex3f(sxs, sys, szs);  glVertex3f(sxs, sy, szs);  glVertex3f(sx, sy, szs);

        	glVertex3f(sx, sys, sz);  glVertex3f(sxs, sys, sz);  glVertex3f(sxs, sys, szs);  glVertex3f(sx, sys, szs);

        	glVertex3f(sxs, sy, szs);  glVertex3f(sxs, sys, szs);  glVertex3f(sxs, sys, sz);  glVertex3f(sxs, sy, sz);
    		glEnd();

                // Stop recording displaylist
            glEndList();
        }
        else
        {
            // Playback displaylist
            glCallList(vox_list);
        }

	}






	void drawObject(void){


        int k;
        static GLuint obj_list = 0;

        if (!obj_list)
        {
            // Start recording displaylist
            obj_list = glGenLists(1);
            glNewList(obj_list, GL_COMPILE_AND_EXECUTE);

            //glBindTexture(GL_TEXTURE_2D, texture);
            //glEnable(GL_TEXTURE_2D);

            glBegin(GL_TRIANGLES);
            k=0;
            for(unsigned int i=0; i<objs.size(); i++){
                for(unsigned int j=0; j<objs[i].vertices.size(); j++){
                    if(objs[i].normals.size()>0)
                        glNormal3f(objs[i].normals[j].x,	objs[i].normals[j].y,	objs[i].normals[j].z);
                    //if(objs[i].uvs.size()>0)
                        //glTexCoord2f(objs[i].uvs[k].u,	objs[i].uvs[k].v);
                    glVertex3f(objs[i].vertices[j].x,objs[i].vertices[j].y,	objs[i].vertices[j].z);
                    k++;
                }
            }
                glEnd();

                // Stop recording displaylist
            glEndList();
        }
        else
        {
            // Playback displaylist
            glCallList(obj_list);
        }
	}

};

Scene scene; // global scene object for other functions

static void error_callback(int error, const char* description){
    fputs(description, stderr);
}


//========================================================================
// Handle key strokes
//========================================================================
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if (action != GLFW_PRESS)
        return;

    switch (key)
    {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GL_TRUE);
            break;
        case GLFW_KEY_S:
            glShadeModel(GL_SMOOTH);
            break;
        case GLFW_KEY_F:
            glShadeModel(GL_FLAT);
            break;
        case GLFW_KEY_LEFT:
            rot_x += 2;
            break;
        case GLFW_KEY_RIGHT:
            rot_x -= 2;
            break;
        case GLFW_KEY_UP:
            rot_y += 2;
            break;
        case GLFW_KEY_DOWN:
            rot_y -= 2;
            break;
        case GLFW_KEY_KP_SUBTRACT:
            rot_z += 2;
            break;
        case GLFW_KEY_KP_ADD:
            rot_z -= 2;
            break;
        default:
            break;
    }
}

static void drawScene(void){
    // object materials
    const GLfloat model_diffuse[4]  = {255.f, 0.f, 0.f, 1.0f};
    const GLfloat model_specular[4] = {0.8f, 0.8f, 0.8f, 1.0f};
    const GLfloat model_shininess   = 20.0f;

    glPushMatrix();
    // Set model color (used for orthogonal views, lighting disabled)
    glColor4fv(model_diffuse);

    // Set model material (used for perspective view, lighting enabled)
    glMaterialfv(GL_FRONT, GL_DIFFUSE, model_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, model_specular);
    glMaterialf(GL_FRONT, GL_SHININESS, model_shininess);

    // Draw object
    scene.drawObject();




    // draw voxels boxes
      //scene.drawVoxel();

    glPopMatrix();
}


static void setup_lights(void){

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    const GLfloat light_position[4] = {rot_x + 0.0f, rot_y + 3.0f, rot_z + (-8.0f), 1.0f};
    const GLfloat light_diffuse[4]  = {1.0f, 1.0f, 100.0f, 1.0f};
    const GLfloat light_specular[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    const GLfloat light_ambient[4]  = {0.2f, 0.2f, 0.2f, 1.0f};

    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

}


static void setup_camera(double width, double height){

        // setup aspect ratio
        double aspect = (height > 0) ? (double) width / (double) height : 1.0;

        // For perspective view, use solid rendering
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // Setup perspective projection matrix
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity(); // reset transformations
        gluPerspective(80.0f, aspect, 1.0f, 80.0f);
        glViewport(0, 0, width, height);
        glScissor(0, 0, width, height);

        // go back into modelview mode
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(0.0f, 1.5f, -2.0f,     // Eye-position
                  0.0f, 0.5f, 0.0f,     // View-point
                  0.0f, 1.0f, 0.0f);   // Up-vector

}

char *textFileRead(char *fn) {

    FILE *fp;
    char *content = NULL;

    int count=0;

    if (fn != NULL) {
        fp = fopen(fn,"rt");

        if (fp != NULL) {

      fseek(fp, 0, SEEK_END);
      count = ftell(fp);
      rewind(fp);

            if (count > 0) {
                content = (char *)malloc(sizeof(char) * (count+1));
                count = fread(content,sizeof(char),count,fp);
                content[count] = '\0';
            }
            fclose(fp);
        }
    }

    //printf("Reading text file\n");
    return content;
}


GLuint setShader(string path, string name) {

    string str = path + name + ".vert";
    char * vshade = new char[str.size() + 1];
    std::copy(str.begin(), str.end(), vshade);
    vshade[str.size()] = '\0'; // don't forget the terminating 0
    const char* vertex_shader = textFileRead(vshade);
    delete[] vshade;

    str = path + name + ".frag";
    char * fshade = new char[str.size() + 1];
    std::copy(str.begin(), str.end(), fshade);
    fshade[str.size()] = '\0'; // don't forget the terminating 0
    const char* fragment_shader = textFileRead(fshade);
    delete[] fshade;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource (vs, 1, &vertex_shader, NULL);
    glCompileShader (vs);

    GLuint fs = glCreateShader (GL_FRAGMENT_SHADER);
    glShaderSource (fs, 1, &fragment_shader, NULL);
    glCompileShader (fs);

    GLuint shader_programme = glCreateProgram ();

    glAttachShader (shader_programme, vs);
    glAttachShader (shader_programme, fs);


    glLinkProgram (shader_programme);

    return shader_programme;
}


int main(void)
{
    scene.loadObjects(); // load objects

    GLFWwindow* window;

    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        exit(EXIT_FAILURE);

    window = glfwCreateWindow(800, 800, "Simple Toon Shader", NULL, NULL); // create window
    if (!window){
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);

    GLenum err = glewInit();
    if (err != GLEW_OK)
      exit(1); // or handle the error in a nicer way
    if (!GLEW_VERSION_2_1)  // check that the machine supports the 2.1 API.
      exit(1); // or handle the error in a nicer way

    GLuint shader_programme = setShader(PATH, SHADER); // load shaders

    while (!glfwWindowShouldClose(window))
    {
        int width, height;
        glfwSetKeyCallback(window, key_callback); // Key Controls
        glfwGetFramebufferSize(window, &width, &height);

        glClearColor(0.4f, 0.4f, 0.4f, 1.f); // Clear screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_SCISSOR_TEST); // Enable scissor test
        glEnable(GL_DEPTH_TEST);  // Enable depth test
        glEnable(GL_CULL_FACE); // enable GL face culling - faster
        glCullFace(GL_BACK); // no not draw the faces behind the object
        glDepthFunc(GL_LEQUAL);


        setup_camera(width, height);
        setup_lights();

        glRotatef((float) glfwGetTime() * 5, 0.f, 1.f, 0.f);


        drawScene();
/*
        // test outline
        glUseProgram (shader_programme);
        drawScene();
        glUseProgram (0);
        glCullFace(GL_FRONT);
        glColor3f(0,0,0);
        glScalef(1.02,1.02,1.02);
        drawScene();
        glCullFace(GL_BACK);
        glScalef(1,1,1);
        // end test outline
*/

        glDisable(GL_LIGHTING); // Disable lighting
        glDisable(GL_DEPTH_TEST); // Disable depth test
        glDisable(GL_SCISSOR_TEST); // Disable scissor test

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
