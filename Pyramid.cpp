#include <iostream>
#include <fstream>
#include <climits>
#include <math.h> 
#include <GL/freeglut.h>
#include "loadBMP.h"

using namespace std;

#define NUM_OBJECTS 2
#define NUM_ANIMATIONS 11
#define M_PI 3.14159265358979323846
#define GLOBAL_TIMER 10
#define ANIMATION_TIMER 100


//--Globals ---------------------------------------------------------------
struct object3d {
	float* x, *y, *z;		//vertex coordinate arrays
	int* t1,* t2,* t3;		//triangles
	int nvrt, ntri;			//total number of vertices and triangles
};

struct animations {
	float pos_x = 0;
	float pos_y = 0;
	float pos_z = 0;
	float rot_x = 0;
	float rot_y = 0;
	float rot_z = 0;
	float scale_x = 1;
	float scale_y = 1;
	float scale_z = 1;
};

struct vector3d {
	float x, y, z;
};


//1: Open wall of pyramid
//2: Closed wall of pyramid
struct object3d object_list[NUM_OBJECTS];

//0: Teapot
//1: Fluid
//2: Altar left
//3: Altar right
//4: Teeth left
//5: Teeth right
//6-10: JUICY
struct animations animations_list[NUM_ANIMATIONS];


//Angle around the y axis the camera is pointing.
int angle = 6;	

float cam_pos_x = 77;
float cam_pos_y = 15;
float cam_pos_z = -196;

float rotation_speed = 1;
float move_speed = 2;
float look_distance = 1;

uint8_t ctrl_pressed = 0;
uint8_t draw_fluid = 0;
int rotation_timer = 0;

GLuint texId[10];
const char* textureNames[10] = { "brick.bmp", "sand.bmp", "fire.bmp", "left.bmp",  "front.bmp", "right.bmp", "back.bmp", "up.bmp", "down.bmp", "sandworm.bmp"};

//returns n mod 360 like % should do.
float angle_wraparound(float n) {
	if (n > 0) {
		return n - 360;
	}
	else {
		return n + 360;
	}
}


//===============Functions taken from labs.=======================================
//-- Loads mesh data in OFF format    -------------------------------------
void loadMeshFile(const char* fname, int index)  
{
	ifstream fp_in;
	int num, ne;

	fp_in.open(fname, ios::in);
	if(!fp_in.is_open())
	{
		cout << "Error opening mesh file" << endl;
		exit(1);
	}

	fp_in.ignore(INT_MAX, '\n');				//ignore first line
	fp_in >> object_list[index].nvrt >> object_list[index].ntri >> ne;			    // read number of vertices, polygons, edges

	object_list[index].x = new float[object_list[index].nvrt];                        //create arrays
	object_list[index].y = new float[object_list[index].nvrt];
	object_list[index].z = new float[object_list[index].nvrt];

	object_list[index].t1 = new int[object_list[index].ntri];
	object_list[index].t2 = new int[object_list[index].ntri];
	object_list[index].t3 = new int[object_list[index].ntri];

	for(int i=0; i < object_list[index].nvrt; i++)                         //read vertex list 
		fp_in >> object_list[index].x[i] >> object_list[index].y[i] >> object_list[index].z[i];

	for(int i=0; i < object_list[index].ntri; i++)                         //read polygon list 
	{
		fp_in >> num >> object_list[index].t1[i] >> object_list[index].t2[i] >> object_list[index].t3[i];
		if(num != 3)
		{
			cout << "ERROR: Polygon with index " << i  << " is not a triangle." << endl;  //not a triangle!!
			exit(1);
		}
	}

	fp_in.close();
	cout << " File successfully read." << endl;
}

//--Function to compute the normal vector of a triangle with index tindx ----------
void normal(int tindx, int index)
{
	float x1 = object_list[index].x[object_list[index].t1[tindx]], x2 = object_list[index].x[object_list[index].t2[tindx]], x3 = object_list[index].x[object_list[index].t3[tindx]];
	float y1 = object_list[index].y[object_list[index].t1[tindx]], y2 = object_list[index].y[object_list[index].t2[tindx]], y3 = object_list[index].y[object_list[index].t3[tindx]];
	float z1 = object_list[index].z[object_list[index].t1[tindx]], z2 = object_list[index].z[object_list[index].t2[tindx]], z3 = object_list[index].z[object_list[index].t3[tindx]];
	float nx, ny, nz;
	nx = y1*(z2-z3) + y2*(z3-z1) + y3*(z1-z2);
	ny = z1*(x2-x3) + z2*(x3-x1) + z3*(x1-x2);
	nz = x1*(y2-y3) + x2*(y3-y1) + x3*(y1-y2);
	glNormal3f(nx, ny, nz);
}

//--------draws the mesh model at the provided index----------------------------
void drawObject(int index)
{
	//Construct the object model here using triangles read from OFF file
	glBegin(GL_TRIANGLES);
	for (int tindx = 0; tindx < object_list[index].ntri; tindx++)
	{
		normal(tindx, index);
		glVertex3d(object_list[index].x[object_list[index].t1[tindx]], object_list[index].y[object_list[index].t1[tindx]], object_list[index].z[object_list[index].t1[tindx]]);
		glVertex3d(object_list[index].x[object_list[index].t2[tindx]], object_list[index].y[object_list[index].t2[tindx]], object_list[index].z[object_list[index].t2[tindx]]);
		glVertex3d(object_list[index].x[object_list[index].t3[tindx]], object_list[index].y[object_list[index].t3[tindx]], object_list[index].z[object_list[index].t3[tindx]]);
	}
	glEnd();
}

//==============================================================

float degreesToRads(int theta) {
	return theta * (M_PI / 180);
}

void drawFloor()
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texId[1]);

	int squareSize = 10;

	glBegin(GL_QUADS);
	for (int i = -1000; i < 1000; i = i+squareSize)
	{
		for (int j = -1000; j < 1000; j = j+ squareSize)
		{
			glTexCoord2f(0., 0.);
			glVertex3f(i, -0.001, j);
			glTexCoord2f(0., 1.);
			glVertex3f(i, -0.001, j + squareSize);
			glTexCoord2f(1., 1.);
			glVertex3f(i + squareSize, -0.001, j + squareSize);
			glTexCoord2f(1., 0.);
			glVertex3f(i + squareSize, -0.001, j);
		}
	}
	glEnd();
	
	glDisable(GL_TEXTURE_2D);
}

void drawSkybox() {
	//Box size
	int s = 300;
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texId[5]);
	glBegin(GL_QUADS);
		glTexCoord2f(0., 0.);
		glVertex3f(-s, -s, s);
		glTexCoord2f(1., 0.);
		glVertex3f(-s, -s, -s);
		glTexCoord2f(1., 1.);
		glVertex3f(-s, s, -s);
		glTexCoord2f(0., 1.);
		glVertex3f(-s, s, s);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, texId[6]);
	glBegin(GL_QUADS);
	glTexCoord2f(0., 0.);
	glVertex3f(s, -s, s);
	glTexCoord2f(1., 0.);
	glVertex3f(-s, -s, s);
	glTexCoord2f(1., 1.);
	glVertex3f(-s, s, s);
	glTexCoord2f(0., 1.);
	glVertex3f(s, s, s);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, texId[3]);
	glBegin(GL_QUADS);
	glTexCoord2f(0., 0.);
	glVertex3f(s, -s, -s);
	glTexCoord2f(1., 0.);
	glVertex3f(s, -s, s);
	glTexCoord2f(1., 1.);
	glVertex3f(s, s, s);
	glTexCoord2f(0., 1.);
	glVertex3f(s, s, -s);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, texId[4]);
	glBegin(GL_QUADS);
	glTexCoord2f(0., 0.);
	glVertex3f(-s, -s, -s);
	glTexCoord2f(1., 0.);
	glVertex3f(s, -s, -s);
	glTexCoord2f(1., 1.);
	glVertex3f(s, s, -s);
	glTexCoord2f(0., 1.);
	glVertex3f(-s, s, -s);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, texId[7]);
	glBegin(GL_QUADS);
	glTexCoord2f(0., 0.);
	glVertex3f(-s, s, s);
	glTexCoord2f(1., 0.);
	glVertex3f(-s, s, -s);
	glTexCoord2f(1., 1.);
	glVertex3f(s, s, -s);
	glTexCoord2f(0., 1.);
	glVertex3f(s, s, s);
	glEnd();

	glDisable(GL_TEXTURE_2D);
}

void drawPyramid()
{
	glColor3f(1, (1.0 / 255.0) * 236.0, (1.0 / 255.0) * 153.0);
	//Wall1
	glPushMatrix();
		glScalef(100, 100, 100);
		drawObject(0);
	glPopMatrix();

	//Wall2
	glPushMatrix();
		glTranslatef(-50, 0, 150);
		glRotatef(90, 0, 1, 0);
		glScalef(100, 100, 100);
		drawObject(1);
	glPopMatrix();

	//Wall3
	glPushMatrix();
		glTranslatef(150, 0, 50);
		glRotatef(-90, 0, 1, 0);
		glScalef(100, 100, 100);
		drawObject(1);
	glPopMatrix();

	//Wall4
	glPushMatrix();
		glTranslatef(100, 0, 200);
		glRotatef(180, 0, 1, 0);
		glScalef(100, 100, 100);
		drawObject(1);
	glPopMatrix();
}

void drawPillars() {

	glEnable(GL_TEXTURE_2D);
	//Pillars
	glBindTexture(GL_TEXTURE_2D, texId[0]);
	GLUquadric* q = gluNewQuadric();
	gluQuadricDrawStyle(q, GLU_FILL);
	gluQuadricNormals(q, GLU_SMOOTH);
	gluQuadricTexture(q, GL_TRUE);

	//Front right
	glPushMatrix();
		
		glTranslatef(-25, 0, 25);
		glRotatef(-90, 1, 0, 0);
		gluCylinder(q, 15, 15, 85, 10, 10);
	glPopMatrix();

	//Front left
	glPushMatrix();
		glTranslatef(125, 0, 25);
		glRotatef(-90, 1, 0, 0);
		gluCylinder(q, 15, 15, 65, 10, 10);
	glPopMatrix();


	//Back right
	glPushMatrix();
		glTranslatef(-25, 0, 175);
		glRotatef(-90, 1, 0, 0);
		gluCylinder(q, 15, 15, 105, 10, 10);
	glPopMatrix();

	//Back left
	glPushMatrix();
		glTranslatef(125, 0, 175);
		glRotatef(-90, 1, 0, 0);
		gluCylinder(q, 15, 15, 125, 10, 10);
	glPopMatrix();

	//Bridges
	glDisable(GL_TEXTURE_2D);
	glColor3f((1.0 / 255.0) * 138.0, (1.0 / 255.0) * 117.0, (1.0 / 255.0) * 74.0);
	//FL to FR
	glPushMatrix();
		glPushMatrix();
		glTranslatef(50, 60, 25);
		glRotatef(90 - 7.595, 0, 0, 1);
		glScalef(5, 151.32, 10);
		glutSolidCube(1);
	glPopMatrix();

	//FR to BR
	glPushMatrix();
		glPushMatrix();
		glTranslatef(-25, 80, 100);
		glRotatef(90, 0, 1, 0);
		glRotatef(90 - 7.595, 0, 0, 1);
		glScalef(5, 151.32, 10);
		glutSolidCube(1);
	glPopMatrix();

	//BR to BL
	glPushMatrix();
		glPushMatrix();
		glTranslatef(50, 100, 175);
		glRotatef(180, 0, 1, 0);
		glRotatef(90 - 7.595, 0, 0, 1);
		glScalef(5, 151.32, 10);
		glutSolidCube(1);
	glPopMatrix();

}

void drawFire(int offset) {
		glEnable(GL_TEXTURE_2D);
		//Pillars
		glBindTexture(GL_TEXTURE_2D, texId[2]);
		GLUquadric* q = gluNewQuadric();
		gluQuadricDrawStyle(q, GLU_FILL);
		gluQuadricNormals(q, GLU_SMOOTH);
		gluQuadricTexture(q, GL_TRUE);

	glPushMatrix();
		glTranslatef(-25, 85, 25);
		glRotatef(-90, 1, 0, 0);
		glScalef(1, 1, 1 * (sin(degreesToRads(rotation_timer + offset)) + 1.5));
		gluCylinder(q, 7, 0, 10, 10, 10);
	glPopMatrix();

	glPushMatrix();
		glTranslatef(-25, 85, 30);
		glRotatef(-90, 1, 0, 0);
		glScalef(1, 1, 1 * (sin(degreesToRads(rotation_timer + offset + 60)) + 1.5));
		gluCylinder(q, 7, 0, 5, 10, 10);
	glPopMatrix();

	glPushMatrix();
		glTranslatef(-30, 85, 20);
		glRotatef(-90, 1, 0, 0);
		glScalef(1, 1, 1 * (sin(degreesToRads(rotation_timer + offset + 125)) + 1.5));
		gluCylinder(q, 7, 0, 5, 10, 10);
	glPopMatrix();

	glPushMatrix();
		glTranslatef(-20, 85, 20);
		glRotatef(-90, 1, 0, 0);
		glScalef(1, 1, 1 * (sin(degreesToRads(rotation_timer + offset + 254)) + 1.5));
		gluCylinder(q, 7, 0, 5, 10, 10);
	glPopMatrix();

	glDisable(GL_TEXTURE_2D);
}

void drawOrb()
{
	glColor3f((1.0 / 255.0) * 205.0, (1.0 / 255.0) * 134.0, (1.0 / 255.0) * 207.0);
	glPushMatrix();
		glTranslatef(50, 140, 100);
		glRotatef(rotation_timer, 0, 1, 1);
		glScalef(20, 20, 20);
		glutSolidDodecahedron();
	glPopMatrix();

	glColor3f((1.0 / 255.0) * 39.0, (1.0 / 255.0) * 44.0, (1.0 / 255.0) * 105.0);
	glPushMatrix();
		glTranslatef(50, 140, 100);
		glRotatef(rotation_timer, 0, 0, 1);
		glRotatef(90, 1, 0, 0);
		glutSolidTorus(4, 45, 10, 10);
	glPopMatrix();

	glColor3f((1.0 / 255.0) * 38.0, (1.0 / 255.0) * 47.0, (1.0 / 255.0) * 112.0);
	glPushMatrix();
		glTranslatef(50, 140, 100);
		glRotatef(90, 1, 0, 0);
		glRotatef(35, 0, 1, 0);
		glRotatef(rotation_timer, 1, 0, 0);
		glutSolidTorus(4, 50, 10, 10);
	glPopMatrix();
}

void drawJ() {
	glColor3f((1.0 / 255.0) * 218.0, (1.0 / 255.0) * 96.0, (1.0 / 255.0) * 224.0);
	glPushMatrix();
			
		glTranslatef(50 + animations_list[6].pos_x, -25 + animations_list[6].pos_y, 100 + animations_list[6].pos_z);
		glRotatef(animations_list[6].rot_y, 0, 1, 0);
		
		glPushMatrix();
			glTranslatef(0, 0, 0);
			glScalef(1, 1, 3);
			glutSolidCube(5);
		glPopMatrix();

		glPushMatrix();
			glTranslatef(0, 5, 5);
			glScalef(1, 1, 1);
			glutSolidCube(5);
		glPopMatrix();

		glPushMatrix();
			glTranslatef(0, 7.5, -5);
			glScalef(1, 2, 1);
			glutSolidCube(5);
		glPopMatrix();

		glPushMatrix();
			glTranslatef(0, 15, -5);
			glScalef(1, 1, 3);
			glutSolidCube(5);
			glPopMatrix();
	glPopMatrix();
}

void drawU() {
	glColor3f((1.0 / 255.0) * 255.0, (1.0 / 255.0) * 247.0, (1.0 / 255.0) * 52.0);
	glPushMatrix();

		glTranslatef(50 + animations_list[7].pos_x, -47 + animations_list[7].pos_y, 100 + animations_list[7].pos_z);
		glRotatef(animations_list[7].rot_y, 0, 1, 0);

		glPushMatrix();
			glTranslatef(0, 0, 0);
			glScalef(1, 1, 3);
			glutSolidCube(5);
		glPopMatrix();

		glPushMatrix();
			glTranslatef(0, 10, -5);
			glScalef(1, 3, 1);
			glutSolidCube(5);
		glPopMatrix();

		glPushMatrix();
			glTranslatef(0, 10, 5);
			glScalef(1, 3, 1);
			glutSolidCube(5);
		glPopMatrix();

	glPopMatrix();
}

void drawI() {
	glColor3f((1.0 / 255.0) * 218.0, (1.0 / 255.0) * 96.0, (1.0 / 255.0) * 224.0);
	glPushMatrix();
		glTranslatef(50 + animations_list[8].pos_x, -69 + animations_list[8].pos_y, 100 + animations_list[8].pos_z);
		glRotatef(animations_list[8].rot_y, 0, 1, 0);

		glPushMatrix();
			glTranslatef(0, 0, 0);
			glScalef(1, 1, 3);
			glutSolidCube(5);
		glPopMatrix();

		glPushMatrix();
			glTranslatef(0, 7.5, 0);
			glScalef(1, 2, 1);
			glutSolidCube(5);
		glPopMatrix();

		glPushMatrix();
			glTranslatef(0, 15, 0);
			glScalef(1, 1, 3);
			glutSolidCube(5);
		glPopMatrix();

	glPopMatrix();
}

void drawC() {
	glColor3f((1.0 / 255.0) * 255.0, (1.0 / 255.0) * 247.0, (1.0 / 255.0) * 52.0);
	glPushMatrix();
		glTranslatef(50 + animations_list[9].pos_x, -91 + animations_list[9].pos_y, 100 + animations_list[9].pos_z);
		glRotatef(animations_list[9].rot_y, 0, 1, 0);

		glPushMatrix();
			glTranslatef(0, 0, 0);
			glScalef(1, 1, 3);
			glutSolidCube(5);
		glPopMatrix();

		glPushMatrix();
			glTranslatef(0, 7.5, 5);
			glScalef(1, 2, 1);
			glutSolidCube(5);
		glPopMatrix();

		glPushMatrix();
			glTranslatef(0, 15, 0);
			glScalef(1, 1, 3);
			glutSolidCube(5);
		glPopMatrix();

	glPopMatrix();
}

void drawY() {
	glColor3f((1.0 / 255.0) * 218.0, (1.0 / 255.0) * 96.0, (1.0 / 255.0) * 224.0);
	glPushMatrix();
		glTranslatef(50 + animations_list[10].pos_x, -113 + animations_list[10].pos_y, 100 + animations_list[10].pos_z);
		glRotatef(animations_list[10].rot_y, 0, 1, 0);

		glPushMatrix();
			glTranslatef(0, 0, 0);
			glScalef(1, 3, 1);
			glutSolidCube(5);
		glPopMatrix();

		glPushMatrix();
			glTranslatef(0, 7.5, 5);
			glScalef(1, 2, 1);
			glutSolidCube(5);
		glPopMatrix();

		glPushMatrix();
			glTranslatef(0, 7.5, -5);
			glScalef(1, 2, 1);
			glutSolidCube(5);
		glPopMatrix();

	glPopMatrix();
}

void drawTeapot() {
	//teapot
	glPushMatrix();
		glTranslatef(50, 0, 100);
		if (ctrl_pressed == 0) {
			glTranslatef(0, 25 + (sin(degreesToRads(rotation_timer)) * 3), 0);
		}
		else {

			glScalef(1 * animations_list[0].scale_x, 1 * animations_list[0].scale_y, 1 * animations_list[0].scale_z);

			glTranslatef(0 + animations_list[0].pos_x, 25 + animations_list[0].pos_y, 0 + animations_list[0].pos_z);

			glRotatef(animations_list[0].rot_x, 1, 0, 0);
			glRotatef(animations_list[0].rot_y, 0, 1, 0);
			glRotatef(animations_list[0].rot_z, 0, 0, 1);



		}
		glutSolidTeapot(5);
	glPopMatrix();
}

void drawAltar() {
	glPushMatrix();
		glTranslatef(50, 0, 100);
		
		//altar
		glPushMatrix();
			glTranslatef(-10 + animations_list[2].pos_x, 7.5 + animations_list[2].pos_y, 0 + +animations_list[2].pos_z);
			glScalef(20 * animations_list[2].scale_x, 15 * animations_list[2].scale_y, 20 * +animations_list[2].scale_z);
			glutSolidCube(1);
		glPopMatrix();

		glPushMatrix();
			glTranslatef(10 + animations_list[3].pos_x, 7.5 + animations_list[3].pos_y, 0 + animations_list[3].pos_z);
			glScalef(20 * animations_list[3].scale_x, 15 * animations_list[3].scale_y, 20 * animations_list[3].scale_z);
			glutSolidCube(1);
		glPopMatrix();

		glColor3f(1, 1, 1);
		//teeth
		glPushMatrix();
			glRotatef(animations_list[4].rot_x, 1, 0, 0);
			glRotatef(animations_list[4].rot_y, 0, 1, 0);
			glRotatef(animations_list[4].rot_z, 0, 0, 1);
			glTranslatef(-7.5 + animations_list[4].pos_x, 7.5 + animations_list[4].pos_y, 0 + animations_list[4].pos_z);
			glScalef(15 * animations_list[4].scale_x, 13 * animations_list[4].scale_y, 15 * animations_list[4].scale_z);
			glutSolidCube(1);
		glPopMatrix();

		glPushMatrix();
			glRotatef(animations_list[5].rot_x, 1, 0, 0);
			glRotatef(animations_list[5].rot_y, 0, 1, 0);
			glRotatef(animations_list[5].rot_z, 0, 0, 1);
			glTranslatef(7.5 + animations_list[5].pos_x, 7.5 + animations_list[5].pos_y, 0 + animations_list[5].pos_z);
			glScalef(15 * animations_list[5].scale_x, 13 * animations_list[5].scale_y, 15 * animations_list[5].scale_z);
			glutSolidCube(1);
		glPopMatrix();


		float shadowMat[16] = { 85,0,0,0, -50,0,-180,-1,0,0,85,0,  0,0,0,85 };
		glDisable(GL_LIGHTING);
		glPushMatrix();
			glTranslatef(-50, 0, -100);
			glMultMatrixf(shadowMat);
			/* Object Transformations */
			glColor4f(0.2, 0.2, 0.2, 1.0);
			drawTeapot();
		glPopMatrix();
		glEnable(GL_LIGHTING);

		drawTeapot();

		//Gaping maw

		int s = 10;

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texId[9]);
		glBegin(GL_QUADS);
			glTexCoord2f(0., 0.);
			glVertex3f(s + 50, 0, -s + 100);
			glTexCoord2f(1., 0.);
			glVertex3f(-s + 50, 0, -s + 100);
			glTexCoord2f(1., 1.);
			glVertex3f(-s + 50, 0, s + 100);
			glTexCoord2f(0., 1.);
			glVertex3f(s + 50, 0, s + 100);
		glEnd();

		glDisable(GL_TEXTURE_2D);
		
	glPopMatrix();
}

void drawFluid() {
	if (draw_fluid != 0) {
		glPushMatrix();
			
			
			glTranslatef(46.5 + animations_list[1].pos_x,
				21 + animations_list[0].pos_y + animations_list[1].pos_y,
				100 + animations_list[1].pos_z);
			glRotatef(90, 1, 0, 0);
			glScalef(1 * animations_list[1].scale_x, 1 * animations_list[1].scale_y, 1 * animations_list[1].scale_z);
			glutSolidCylinder(1, .1, 5, 5);
		glPopMatrix();
	}
}

//Creates a vector in the angle theta with the intensity of scalar.
vector3d generateMovementVector(int theta, int scalar) {
	float x;
	float y;
	float z;

	float theta_rads = degreesToRads(theta);

	if (theta <= 90) {
		z = cos(theta_rads);
		x = -cos(degreesToRads(90) - theta_rads);
	}
	else if (theta <= 180) {
		x = -cos(theta_rads - degreesToRads(90));
		z = -cos(degreesToRads(180) - theta_rads);
	}
	else if (theta <= 270) {
		z = -cos(theta_rads - degreesToRads(180));
		x = cos(degreesToRads(270) - theta_rads);
	}
	else {
		x = cos(theta_rads - degreesToRads(270));
		z = cos(degreesToRads(360) - theta_rads);
	}

	struct vector3d vector;
	vector.x = x * scalar;
	vector.y = 0;
	vector.z = z * scalar;
	return vector;
}

//Main timer
void myTimer(int value) {
	rotation_timer = (rotation_timer + 1) % 360;
	glutPostRedisplay();
	glutTimerFunc(GLOBAL_TIMER, myTimer, value);
}

int phase_timer = 0;
int teapot_gravity_timer = 0;
float altar_rotation_timer = 0;

//Timer for main animation sequence
void animationTimer(int phase) {
	phase_timer += 1;
	int phase_length = -1;

	//Opening of altar, raising of teapot
	if (phase == 0) {
		phase_length = 100;
		animations_list[0].pos_x -= .1;
		animations_list[0].pos_y += .1;
		animations_list[0].rot_z -= .5;
		animations_list[0].rot_z = angle_wraparound(animations_list[0].rot_z);

		animations_list[2].pos_x -= .1;
		animations_list[3].pos_x += .1;
		animations_list[4].pos_x -= .08;
		animations_list[5].pos_x += .08;
	}

	//Pouring of tea
	if (phase == 1) {
		phase_length = 400;
		draw_fluid = 1;
		if (phase_timer < phase_length / 2) {
			animations_list[1].scale_z += 2;
		}
		else {
			animations_list[1].pos_y -= .3;
		}
		
		
	}
	//Eating of teapot
	if (phase == 2) {
		
		draw_fluid = 0;
		phase_length = 300;
		animations_list[0].pos_y += -0.0001 * (teapot_gravity_timer * teapot_gravity_timer) + .2;
		animations_list[0].pos_x += 0.1;
		animations_list[0].rot_x += 3;

		if (phase_timer > 100) {

			animations_list[2].pos_x += (sin(7 * degreesToRads(altar_rotation_timer)));
			animations_list[3].pos_x -= (sin(7 * degreesToRads(altar_rotation_timer)));
			animations_list[4].pos_x += (sin(7 * degreesToRads(altar_rotation_timer)));
			animations_list[5].pos_x -= (sin(7 * degreesToRads(altar_rotation_timer)));

			altar_rotation_timer = angle_wraparound(altar_rotation_timer + 1);
		}
		teapot_gravity_timer += 1;
	}

	//Belching of juicy
	if (phase == 3) {

		for (int i = 6; i < 11; i++) {
			if (i % 2 == 0) {
				animations_list[i].rot_y -= 1;
			}
			else {
				animations_list[i].rot_y += 1;
			}

			animations_list[i].pos_y += 0.15;
			
		}
	}

	if (phase_timer == phase_length) {
		glutTimerFunc(GLOBAL_TIMER, animationTimer, phase + 1);
		phase_timer = 0;
		altar_rotation_timer = 0;
	}
	else {
		glutTimerFunc(GLOBAL_TIMER, animationTimer, phase);
	}
	
}

void display()  
{
	  //light's position

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	float light_pos[4] = {60, 300, -200, 1};
	float white[3] = { 1, 1, 1 };
	float gray[3] = { 0.3, 0.3, 0.3 };

	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
	glLightfv(GL_LIGHT0, GL_AMBIENT, gray);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
	glLightfv(GL_LIGHT0, GL_SPECULAR, white);

	struct vector3d vector = generateMovementVector(angle, look_distance);
	
	gluLookAt(cam_pos_x, cam_pos_y, cam_pos_z, cam_pos_x + vector.x, cam_pos_y + vector.y, cam_pos_z + vector.z, 0, 1, 0);
	glTranslatef(cam_pos_x, cam_pos_y, cam_pos_z);
    //glRotatef(angle, 0.0, 1.0, 0.0);		//rotate the whole scene
	glTranslatef(-cam_pos_x, -cam_pos_y, -cam_pos_z);

	drawFloor();

	drawPyramid();
	drawPillars();
	drawOrb();
	
	drawAltar();
	drawFluid();
	drawJ();
	drawU();
	drawI();
	drawC();
	drawY();
	drawFire(0);
	glPushMatrix();
		glTranslatef(150, -20, 0);
		drawFire(45);
	glPopMatrix();

	glPushMatrix();
		glTranslatef(0, 20, 150);
		drawFire(45);
	glPopMatrix();

	glPushMatrix();
		glTranslatef(150, 40, 150);
		drawFire(235);
	glPopMatrix();

	drawSkybox();

	glFlush();
}

void initialize()
{
    loadMeshFile("PyramidWallOpen.off", 0);				
	loadMeshFile("PyramidWall.off", 1);
	glClearColor(119.0f / 255.0f, 207.0f / 255.0f, 242.0f / 255.0f, 1.0f);

	glEnable(GL_LIGHTING);		
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
 	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glEnable(GL_TEXTURE_2D);

	glGenTextures(10, texId);
	
	for (int i = 0; i < 10; i++) {
		glBindTexture(GL_TEXTURE_2D, texId[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		loadBMP(textureNames[i]);

	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, 1, 1, 1000); 
	
}

void special(int key, int x, int y)
{
	struct vector3d vector;
	if (key == GLUT_KEY_LEFT) {
		angle -= rotation_speed;
		if (angle < 0) {
			angle = 359;
		}
	}
	else if (key == GLUT_KEY_RIGHT) {
		angle += rotation_speed;
		angle = angle % 360;
	}
	else if (key == GLUT_KEY_UP) {
		vector = generateMovementVector(angle, move_speed);
		cam_pos_x += vector.x;
		cam_pos_z += vector.z;
	}
	else if (key == GLUT_KEY_DOWN) {
		vector = generateMovementVector(angle, move_speed);
		cam_pos_x -= vector.x;
		cam_pos_z -= vector.z;
	}
    else if(key == GLUT_KEY_CTRL_L){
		cout << "x ";
		cout << cam_pos_x;
		cout << "\n";
		cout << "y ";
		cout << cam_pos_y;
		cout << "\n";
		cout << "z ";
		cout << cam_pos_z;
		cout << "\n";
		cout << "angle ";
		cout << angle;
		cout << "\n";
		cout << "timer ";
		cout << rotation_timer;
		cout << "\n";
		cout << "---------------------------\n";

		if (ctrl_pressed != 1) {
			animations_list[0].pos_y = sin(degreesToRads(rotation_timer)) * 3;
			glutTimerFunc(GLOBAL_TIMER, animationTimer, 0);
			ctrl_pressed = 1;
		}
		
    }
   
}

int main(int argc, char** argv)
{
   glutInit(&argc, argv);
   glutInitDisplayMode (GLUT_SINGLE | GLUT_DEPTH);
   glutInitWindowSize (800, 800); 
   glutInitWindowPosition (10, 10);
   glutCreateWindow ("Museum?");
   initialize();
   glutTimerFunc(GLOBAL_TIMER, myTimer, 1);
   glutDisplayFunc(display);
   glutSpecialFunc(special); 
   glutMainLoop();
   return 0;
}
