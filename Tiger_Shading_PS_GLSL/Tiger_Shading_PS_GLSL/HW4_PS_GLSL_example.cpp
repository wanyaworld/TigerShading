#pragma warning(disable : 4996)

#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include "Shaders/LoadShaders.h"
#include "My_Shading.h"
float prv_angle = 0.0f;
GLuint h_ShaderProgram_simple, h_ShaderProgram_PS; // handles to shader programs

												   // for simple shaders
GLint loc_ModelViewProjectionMatrix_simple, loc_primitive_color;
GLint loc_funeffect;
int loc_change;
float car_dist = 0.0f;
float angle = 0.0f;
int tmp_idx = 0, prv_idx = 0, prv_prv_idx = 0;
int camera_mode = 0;
int highlight_mode = 0;
int fun_effect = 0;
int blending_flag = 0;
GLint loc_u_flag_blending, loc_u_fragment_alpha;
float rectangle_alpha = 0.0f;

int wheel_flag;
void set_up_scene_lights(void);

// for Phone Shading shaders
#define NUMBER_OF_LIGHT_SUPPORTED 4
GLint loc_global_ambient_color;
loc_light_Parameters loc_light[NUMBER_OF_LIGHT_SUPPORTED];
loc_Material_Parameters loc_material;
GLint loc_ModelViewProjectionMatrix_PS, loc_ModelViewMatrix_PS, loc_ModelViewMatrixInvTrans_PS;

#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, lookAt, perspective, etc.
#include <glm/gtc/matrix_inverse.hpp> // inverseTranspose, etc.
glm::mat4 ModelViewProjectionMatrix, ModelViewMatrix;
glm::mat3 ModelViewMatrixInvTrans;
glm::mat4 ViewMatrix, ProjectionMatrix;
glm::mat4 ModelMatrix_CAR_BODY, ModelMatrix_CAR_WHEEL, ModelMatrix_CAR_NUT, ModelMatrix_CAR_DRIVER, ModelMatrix_CAR_DRIVER2;
glm::mat4 ModelMatrix_CAR_BACKMIRROR;
glm::mat4 ModelMatrix_CAR_BODY_to_DRIVER; // computed only once in initialize_camera()
glm::mat4 ModelMatrix_CAR_PANNEL;
glm::mat4 DriverViewMatrix, BackMirrorViewMatrix;
glm::vec4 wheel0, wheel1;
glm::vec4 zero = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

#define TO_RADIAN 0.01745329252f  
#define TO_DEGREE 57.295779513f
#define BUFFER_OFFSET(offset) ((GLvoid *) (offset))

#define LOC_VERTEX 0
#define LOC_NORMAL 1

Material_Parameters material_cow;
Material_Parameters material_cat;
Material_Parameters material_barel;
Material_Parameters material_box;

#define N_OBJECTS 8
#define N_GEOMETRY_OBJECTS 6

#define GEOM_OBJ_ID_CAR_BODY 0
#define GEOM_OBJ_ID_CAR_WHEEL 1
#define GEOM_OBJ_ID_CAR_NUT 2
#define GEOM_OBJ_ID_COW 3
#define GEOM_OBJ_ID_TEAPOT 4
#define GEOM_OBJ_ID_BOX 5
#define OBJECT_COW 6
#define OBJECT_CAT 7

#define NORMAL_CAMERA 0
#define DRIVER_CAMERA 1
#define BACKMIRROR_CAMERA 2

GLuint object_VBO[N_OBJECTS], object_VAO[N_OBJECTS];
int object_n_triangles[N_OBJECTS];
GLfloat *object_vertices[N_OBJECTS];

// for car
GLuint geom_obj_VBO[N_GEOMETRY_OBJECTS];
GLuint geom_obj_VAO[N_GEOMETRY_OBJECTS];

int geom_obj_n_triangles[N_GEOMETRY_OBJECTS];
GLfloat *geom_obj_vertices[N_GEOMETRY_OBJECTS];


// codes for the 'general' triangular-mesh object
enum class _GEOM_OBJ_TYPE {
	GEOM_OBJ_TYPE_V = 0, GEOM_OBJ_TYPE_VN, GEOM_OBJ_TYPE_VNT
};
//typedef enum _GEOM_OBJ_TYPE { GEOM_OBJ_TYPE_V = 0, GEOM_OBJ_TYPE_VN, GEOM_OBJ_TYPE_VNT } GEOM_OBJ_TYPE;
// GEOM_OBJ_TYPE_V: (x, y, z)
// GEOM_OBJ_TYPE_VN: (x, y, z, nx, ny, nz)
// GEOM_OBJ_TYPE_VNT: (x, y, z, nx, ny, nz, s, t)
int GEOM_OBJ_ELEMENTS_PER_VERTEX[3] = { 3, 6, 8 };

int read_geometry_file(GLfloat **object, char *filename, _GEOM_OBJ_TYPE geom_obj_type) {
	int i, n_triangles;
	float *flt_ptr;
	FILE *fp;

	fprintf(stdout, "Reading geometry from the geometry file %s...\n", filename);
	int ret = fopen_s(&fp, filename, "r");
	if (ret) {
		fprintf(stderr, "Cannot open the geometry file %s ...", filename);
		return -1;
	}

	fscanf_s(fp, "%d", &n_triangles);
	*object = (float *)malloc(3 * n_triangles*GEOM_OBJ_ELEMENTS_PER_VERTEX[(int)geom_obj_type] * sizeof(float));
	if (*object == NULL) {
		fprintf(stderr, "Cannot allocate memory for the geometry file %s ...", filename);
		return -1;
	}

	flt_ptr = *object;
	for (i = 0; i < 3 * n_triangles * GEOM_OBJ_ELEMENTS_PER_VERTEX[(int)geom_obj_type]; i++)
		fscanf_s(fp, "%f", flt_ptr++);
	fclose(fp);

	fprintf(stdout, "Read %d primitives successfully.\n\n", n_triangles);

	return n_triangles;
}

void draw_object(int object_ID) { 
	glFrontFace(GL_CW);
	glBindVertexArray(object_VAO[object_ID]);

	glDrawArrays(GL_TRIANGLES, 0, 3 * object_n_triangles[object_ID]);

	glBindVertexArray(0);
}

void prepare_geom_obj(int geom_obj_ID, char *filename, _GEOM_OBJ_TYPE geom_obj_type) {
	int n_bytes_per_vertex;

	n_bytes_per_vertex = GEOM_OBJ_ELEMENTS_PER_VERTEX[(int)geom_obj_type] * sizeof(float);
	geom_obj_n_triangles[geom_obj_ID] = read_geometry_file(&geom_obj_vertices[geom_obj_ID], filename, geom_obj_type);

	// Initialize vertex array object.
	glGenVertexArrays(1, &geom_obj_VAO[geom_obj_ID]);
	glBindVertexArray(geom_obj_VAO[geom_obj_ID]);
	glGenBuffers(1, &geom_obj_VBO[geom_obj_ID]);
	glBindBuffer(GL_ARRAY_BUFFER, geom_obj_VBO[geom_obj_ID]);
	glBufferData(GL_ARRAY_BUFFER, 3 * geom_obj_n_triangles[geom_obj_ID] * n_bytes_per_vertex,
		geom_obj_vertices[geom_obj_ID], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	if (geom_obj_type >= _GEOM_OBJ_TYPE::GEOM_OBJ_TYPE_VN) {
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
	}
	if (geom_obj_type >= _GEOM_OBJ_TYPE::GEOM_OBJ_TYPE_VNT) {
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
		glEnableVertexAttribArray(2);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	free(geom_obj_vertices[geom_obj_ID]);
}


int read_triangular_mesh(GLfloat **object, int bytes_per_primitive, char *filename) {
	int n_triangles;
	FILE *fp;

	fprintf(stdout, "Reading geometry from the geometry file %s...\n", filename);
	int ret = fopen_s(&fp, filename, "rb");
	if (ret) {
		fprintf(stderr, "Cannot open the object file %s ...", filename);
		return -1;
	}
	fread(&n_triangles, sizeof(int), 1, fp);
	*object = (float *)malloc(n_triangles*bytes_per_primitive);
	if (*object == NULL) {
		fprintf(stderr, "Cannot allocate memory for the geometry file %s ...", filename);
		return -1;
	}

	fread(*object, bytes_per_primitive, n_triangles, fp);
	fprintf(stdout, "Read %d primitives successfully.\n\n", n_triangles);
	fclose(fp);

	return n_triangles;
}
void set_up_object(int object_ID, char *filename, int n_bytes_per_vertex) {
	object_n_triangles[object_ID] = read_triangular_mesh(&object_vertices[object_ID],
		3 * n_bytes_per_vertex, filename);
	// Error checking is needed here.

	// Initialize vertex buffer object.
	glGenBuffers(1, &object_VBO[object_ID]);

	glBindBuffer(GL_ARRAY_BUFFER, object_VBO[object_ID]);
	glBufferData(GL_ARRAY_BUFFER, object_n_triangles[object_ID] * 3 * n_bytes_per_vertex,
		object_vertices[object_ID], GL_STATIC_DRAW);

	// As the geometry data exists now in graphics memory, ...
	free(object_vertices[object_ID]);

	// Initialize vertex array object.
	glGenVertexArrays(1, &object_VAO[object_ID]);
	glBindVertexArray(object_VAO[object_ID]);

	glBindBuffer(GL_ARRAY_BUFFER, object_VBO[object_ID]);
	//glVertexAttribPointer(INDEX_VERTEX_POSITION, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	//glEnableVertexAttribArray(INDEX_VERTEX_POSITION);

	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);


	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}


// lights in scene
Light_Parameters light[NUMBER_OF_LIGHT_SUPPORTED];


// car and path object
GLuint car_path_VBO, car_path_VAO;
int n_car_path_vertices;
GLfloat *car_path_vertices;

void read_car_path(char *path_filename) {
	FILE *fp;
	int i;
	float *tmp_ptr;

	if (fopen_s(&fp, path_filename, "r")) {
		fprintf(stderr, "Error: cannot open the car path file %s...\n", path_filename);
		exit(-1);
	}
	fscanf_s(fp, "%d", &n_car_path_vertices);
	tmp_ptr = car_path_vertices = (GLfloat *)malloc(n_car_path_vertices*sizeof(GLfloat) * 3);
	for (i = 0; i < n_car_path_vertices; i++) {
		fscanf_s(fp, "%fp", tmp_ptr);
		*tmp_ptr *= 20.0f; tmp_ptr++;
		fscanf_s(fp, "%fp", tmp_ptr);
		*tmp_ptr *= 20.0f; tmp_ptr++;
		fscanf_s(fp, "%fp", tmp_ptr);
		*tmp_ptr *= 20.0f; tmp_ptr++;
		//	fscanf_s(fp, "%f", tmp_ptr++);
		//	fscanf_s(fp, "%f", tmp_ptr++);
		//	fscanf_s(fp, "%f", tmp_ptr++);
	}
	fclose(fp);
	fprintf(stdout, "* The number of points in the car path is %d.\n", n_car_path_vertices);
}

void prepare_car(void) {
	read_car_path("Data/car_path.txt");
	// initialize vertex buffer object
	glGenBuffers(1, &car_path_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, car_path_VBO);
	glBufferData(GL_ARRAY_BUFFER, n_car_path_vertices*sizeof(GLfloat) * 3, car_path_vertices, GL_STATIC_DRAW);

	// initialize vertex array object
	glGenVertexArrays(1, &car_path_VAO);
	glBindVertexArray(car_path_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, car_path_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
}

void draw_car_path(void) {
	glUniform3f(loc_primitive_color, 1.0f, 1.0f, 0.0f);
	glBindVertexArray(car_path_VAO);
	glDrawArrays(GL_LINE_STRIP, 0, n_car_path_vertices);
	glBindVertexArray(0);
}

// for tiger animation
int cur_frame_tiger = 0;
float rotation_angle_tiger = 0.0f;

int cur_frame_cow = 0;
float rotation_angle_cow = 0.0f;

int cur_frame_cat = 0;
float rotation_angle_cat = 0.0f;

int cur_frame_barel = 0;
float rotation_angle_barel = 0.0f;

int cur_frame_box = 0;
float rotation_angle_box = 0.0f;

// axes object
GLuint axes_VBO, axes_VAO;
GLfloat axes_vertices[6][3] = {
	{ 0.0f, 0.0f, 0.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 0.0f },{ 0.0f, 1.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f }
};
GLfloat axes_color[3][3] = { { 1.0f, 0.0f, 0.0f },{ 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } };

void prepare_axes(void) { // draw coordinate axes
						  // initialize vertex buffer object
	glGenBuffers(1, &axes_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, axes_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(axes_vertices), &axes_vertices[0][0], GL_STATIC_DRAW);

	// initialize vertex array object
	glGenVertexArrays(1, &axes_VAO);
	glBindVertexArray(axes_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, axes_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_axes(void) {
	// assume ShaderProgram_simple is used
	glBindVertexArray(axes_VAO);
	glUniform3fv(loc_primitive_color, 1, axes_color[0]);
	glDrawArrays(GL_LINES, 0, 2);
	glUniform3fv(loc_primitive_color, 1, axes_color[1]);
	glDrawArrays(GL_LINES, 2, 2);
	glUniform3fv(loc_primitive_color, 1, axes_color[2]);
	glDrawArrays(GL_LINES, 4, 2);
	glBindVertexArray(0);
}

// floor object
GLuint rectangle_VBO, rectangle_VAO;
GLfloat rectangle_vertices[12][3] = {  // vertices enumerated counterclockwise
	{ 0.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f },
	{ 1.0f, 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 0.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f },
	{ 1.0f, 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f }
};

Material_Parameters material_floor;

void prepare_floor(void) { // Draw coordinate axes.
						   // Initialize vertex buffer object.
	glGenBuffers(1, &rectangle_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, rectangle_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rectangle_vertices), &rectangle_vertices[0][0], GL_STATIC_DRAW);

	// Initialize vertex array object.
	glGenVertexArrays(1, &rectangle_VAO);
	glBindVertexArray(rectangle_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, rectangle_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	material_floor.ambient_color[0] = 0.0f;
	material_floor.ambient_color[1] = 0.05f;
	material_floor.ambient_color[2] = 0.0f;
	material_floor.ambient_color[3] = 1.0f;

	material_floor.diffuse_color[0] = 0.4f;
	material_floor.diffuse_color[1] = 0.5f;
	material_floor.diffuse_color[2] = 0.4f;
	material_floor.diffuse_color[3] = 1.0f;

	material_floor.specular_color[0] = 0.04f;
	material_floor.specular_color[1] = 0.7f;
	material_floor.specular_color[2] = 0.04f;
	material_floor.specular_color[3] = 1.0f;

	material_floor.specular_exponent = 2.5f;

	material_floor.emissive_color[0] = 0.0f;
	material_floor.emissive_color[1] = 0.0f;
	material_floor.emissive_color[2] = 0.0f;
	material_floor.emissive_color[3] = 1.0f;
}

void set_material_floor(void) {
	// assume ShaderProgram_PS is used
	glUniform4fv(loc_material.ambient_color, 1, material_floor.ambient_color);
	glUniform4fv(loc_material.diffuse_color, 1, material_floor.diffuse_color);
	glUniform4fv(loc_material.specular_color, 1, material_floor.specular_color);
	glUniform1f(loc_material.specular_exponent, material_floor.specular_exponent);
	glUniform4fv(loc_material.emissive_color, 1, material_floor.emissive_color);
}

void draw_floor(void) {
	glFrontFace(GL_CCW);

	glBindVertexArray(rectangle_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

// tiger object
#define N_TIGER_FRAMES 12
GLuint tiger_VBO, tiger_VAO;
int tiger_n_triangles[N_TIGER_FRAMES];
int tiger_vertex_offset[N_TIGER_FRAMES];
GLfloat *tiger_vertices[N_TIGER_FRAMES];

Material_Parameters material_tiger;

//  barel object
#define N_BAREL_FRAMES 1
GLuint barel_VBO, barel_VAO;
int barel_n_triangles[N_BAREL_FRAMES];
int barel_vertex_offset[N_BAREL_FRAMES];
GLfloat *barel_vertices[N_BAREL_FRAMES];

//  box object
#define N_BOX_FRAMES 1
GLuint box_VBO, box_VAO;
int box_n_triangles[N_BOX_FRAMES];
int box_vertex_offset[N_BOX_FRAMES];
GLfloat *box_vertices[N_BOX_FRAMES];

int read_geometry(GLfloat **object, int bytes_per_primitive, char *filename) {
	int n_triangles;
	FILE *fp;

	// fprintf(stdout, "Reading geometry from the geometry file %s...\n", filename);
	int ret = fopen_s(&fp, filename, "rb");
	if (ret) {
		fprintf(stderr, "Cannot open the object file %s ...", filename);
		return -1;
	}
	fread(&n_triangles, sizeof(int), 1, fp);
	*object = (float *)malloc(n_triangles*bytes_per_primitive);
	if (*object == NULL) {
		fprintf(stderr, "Cannot allocate memory for the geometry file %s ...", filename);
		return -1;
	}

	fread(*object, bytes_per_primitive, n_triangles, fp);
	// fprintf(stdout, "Read %d primitives successfully.\n\n", n_triangles);
	fclose(fp);

	return n_triangles;
}

void prepare_box(void) { // vertices enumerated clockwise
	int i, n_bytes_per_vertex, n_bytes_per_triangle, box_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	for (i = 0; i < N_BOX_FRAMES; i++) {
		sprintf(filename, "Data/box_triangles_v.geom");
		box_n_triangles[i] = read_geometry(&box_vertices[i], n_bytes_per_triangle, filename);
		// assume all geometry files are effective
		box_n_total_triangles += box_n_triangles[i];

		if (i == 0)
			box_vertex_offset[i] = 0;
		else
			box_vertex_offset[i] = box_vertex_offset[i - 1] + 3 * box_n_triangles[i - 1];
	}

	// initialize vertex buffer object
	glGenBuffers(1, &box_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, box_VBO);
	glBufferData(GL_ARRAY_BUFFER, box_n_total_triangles*n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

	for (i = 0; i < N_BOX_FRAMES; i++)
		glBufferSubData(GL_ARRAY_BUFFER, box_vertex_offset[i] * n_bytes_per_vertex,
			box_n_triangles[i] * n_bytes_per_triangle, box_vertices[i]);

	// as the geometry data exists now in graphics memory, ...
	for (i = 0; i < N_BOX_FRAMES; i++)
		free(box_vertices[i]);

	// initialize vertex array object
	glGenVertexArrays(1, &box_VAO);
	glBindVertexArray(box_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, box_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	material_box.ambient_color[0] = 0.24725f;
	material_box.ambient_color[1] = 0.1995f;
	material_box.ambient_color[2] = 0.0745f;
	material_box.ambient_color[3] = 1.0f;

	material_box.diffuse_color[0] = 0.75164f;
	material_box.diffuse_color[1] = 0.60648f;
	material_box.diffuse_color[2] = 0.22648f;
	material_box.diffuse_color[3] = 1.0f;

	material_box.specular_color[0] = 0.628281f;
	material_box.specular_color[1] = 0.555802f;
	material_box.specular_color[2] = 0.366065f;
	material_box.specular_color[3] = 1.0f;

	material_box.specular_exponent = 51.2f;

	material_box.emissive_color[0] = 0.1f;
	material_box.emissive_color[1] = 0.1f;
	material_box.emissive_color[2] = 0.0f;
	material_box.emissive_color[3] = 1.0f;
}

void prepare_barel(void) { // vertices enumerated clockwise
	int i, n_bytes_per_vertex, n_bytes_per_triangle, barel_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	for (i = 0; i < N_BAREL_FRAMES; i++) {
		sprintf(filename, "Data/Barel_triangles_vnt.geom");
		barel_n_triangles[i] = read_geometry(&barel_vertices[i], n_bytes_per_triangle, filename);
		// assume all geometry files are effective
		barel_n_total_triangles += barel_n_triangles[i];

		if (i == 0)
			barel_vertex_offset[i] = 0;
		else
			barel_vertex_offset[i] = barel_vertex_offset[i - 1] + 3 * barel_n_triangles[i - 1];
	}

	// initialize vertex buffer object
	glGenBuffers(1, &barel_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, barel_VBO);
	glBufferData(GL_ARRAY_BUFFER, barel_n_total_triangles*n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

	for (i = 0; i < N_BAREL_FRAMES; i++)
		glBufferSubData(GL_ARRAY_BUFFER, barel_vertex_offset[i] * n_bytes_per_vertex,
			barel_n_triangles[i] * n_bytes_per_triangle, barel_vertices[i]);

	// as the geometry data exists now in graphics memory, ...
	for (i = 0; i < N_BAREL_FRAMES; i++)
		free(barel_vertices[i]);

	// initialize vertex array object
	glGenVertexArrays(1, &barel_VAO);
	glBindVertexArray(barel_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, barel_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	material_barel.ambient_color[0] = 0.4725f;
	material_barel.ambient_color[1] = 0.995f;
	material_barel.ambient_color[2] = 0.0745f;
	material_barel.ambient_color[3] = 1.0f;

	material_barel.diffuse_color[0] = 0.5164f;
	material_barel.diffuse_color[1] = 0.60648f;
	material_barel.diffuse_color[2] = 0.648f;
	material_barel.diffuse_color[3] = 1.0f;

	material_barel.specular_color[0] = 0.28281f;
	material_barel.specular_color[1] = 0.55802f;
	material_barel.specular_color[2] = 0.66065f;
	material_barel.specular_color[3] = 1.0f;

	material_barel.specular_exponent = 51.2f;

	material_barel.emissive_color[0] = 0.1f;
	material_barel.emissive_color[1] = 0.1f;
	material_barel.emissive_color[2] = 0.0f;
	material_barel.emissive_color[3] = 1.0f;
}

void prepare_tiger(void) { // vertices enumerated clockwise
	int i, n_bytes_per_vertex, n_bytes_per_triangle, tiger_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	for (i = 0; i < N_TIGER_FRAMES; i++) {
		sprintf(filename, "Data/Tiger_%d%d_triangles_vnt.geom", i / 10, i % 10);
		tiger_n_triangles[i] = read_geometry(&tiger_vertices[i], n_bytes_per_triangle, filename);
		// assume all geometry files are effective
		tiger_n_total_triangles += tiger_n_triangles[i];

		if (i == 0)
			tiger_vertex_offset[i] = 0;
		else
			tiger_vertex_offset[i] = tiger_vertex_offset[i - 1] + 3 * tiger_n_triangles[i - 1];
	}

	// initialize vertex buffer object
	glGenBuffers(1, &tiger_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, tiger_VBO);
	glBufferData(GL_ARRAY_BUFFER, tiger_n_total_triangles*n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

	for (i = 0; i < N_TIGER_FRAMES; i++)
		glBufferSubData(GL_ARRAY_BUFFER, tiger_vertex_offset[i] * n_bytes_per_vertex,
			tiger_n_triangles[i] * n_bytes_per_triangle, tiger_vertices[i]);

	// as the geometry data exists now in graphics memory, ...
	for (i = 0; i < N_TIGER_FRAMES; i++)
		free(tiger_vertices[i]);

	// initialize vertex array object
	glGenVertexArrays(1, &tiger_VAO);
	glBindVertexArray(tiger_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, tiger_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	material_tiger.ambient_color[0] = 0.94725f;
	material_tiger.ambient_color[1] = 0.5995f;
	material_tiger.ambient_color[2] = 0.6745f;
	material_tiger.ambient_color[3] = 1.0f;

	material_tiger.diffuse_color[0] = 0.164f;
	material_tiger.diffuse_color[1] = 0.60648f;
	material_tiger.diffuse_color[2] = 0.22648f;
	material_tiger.diffuse_color[3] = 1.0f;

	material_tiger.specular_color[0] = 0.628281f;
	material_tiger.specular_color[1] = 0.255802f;
	material_tiger.specular_color[2] = 0.366065f;
	material_tiger.specular_color[3] = 1.0f;

	material_tiger.specular_exponent = 51.2f;

	material_tiger.emissive_color[0] = 0.1f;
	material_tiger.emissive_color[1] = 0.1f;
	material_tiger.emissive_color[2] = 0.0f;
	material_tiger.emissive_color[3] = 1.0f;
}

void set_material_box(void) {

	glUniform4fv(loc_material.ambient_color, 1, material_box.ambient_color);
	glUniform4fv(loc_material.diffuse_color, 1, material_box.diffuse_color);
	glUniform4fv(loc_material.specular_color, 1, material_box.specular_color);
	glUniform1f(loc_material.specular_exponent, material_box.specular_exponent);
	glUniform4fv(loc_material.emissive_color, 1, material_box.emissive_color);
}
void set_material_barel(void) {

	glUniform4fv(loc_material.ambient_color, 1, material_barel.ambient_color);
	glUniform4fv(loc_material.diffuse_color, 1, material_barel.diffuse_color);
	glUniform4fv(loc_material.specular_color, 1, material_barel.specular_color);
	glUniform1f(loc_material.specular_exponent, material_barel.specular_exponent);
	glUniform4fv(loc_material.emissive_color, 1, material_barel.emissive_color);
}

void set_material_tiger(void) {
	// assume ShaderProgram_PS is used
	glUniform4fv(loc_material.ambient_color, 1, material_tiger.ambient_color);
	glUniform4fv(loc_material.diffuse_color, 1, material_tiger.diffuse_color);
	glUniform4fv(loc_material.specular_color, 1, material_tiger.specular_color);
	glUniform1f(loc_material.specular_exponent, material_tiger.specular_exponent);
	glUniform4fv(loc_material.emissive_color, 1, material_tiger.emissive_color);
}

void draw_box(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(box_VAO);
	glDrawArrays(GL_TRIANGLES, box_vertex_offset[cur_frame_box], 3 * box_n_triangles[cur_frame_box]);
	glBindVertexArray(0);
}

void draw_barel(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(barel_VAO);
	glDrawArrays(GL_TRIANGLES, barel_vertex_offset[cur_frame_barel], 3 * barel_n_triangles[cur_frame_barel]);
	glBindVertexArray(0);
}

void draw_tiger(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(tiger_VAO);
	glDrawArrays(GL_TRIANGLES, tiger_vertex_offset[cur_frame_tiger], 3 * tiger_n_triangles[cur_frame_tiger]);
	glBindVertexArray(0);
}

void draw_geom_obj(int geom_obj_ID) {
	glBindVertexArray(geom_obj_VAO[geom_obj_ID]);
	glDrawArrays(GL_TRIANGLES, 0, 3 * geom_obj_n_triangles[geom_obj_ID]);
	glBindVertexArray(0);
}

#define rad 1.7f
#define ww 1.0f
void draw_wheel_and_nut() {
	// angle is used in Hierarchical_Car_Correct later
	int i;

	if (tmp_idx > 2000)
		glUniform3f(loc_primitive_color, 0.400f, 0.208f, 0.520f);
	else if (!wheel_flag )
		glUniform3f(loc_primitive_color, 0.000f, 0.808f, 0.820f); // color name: DarkTurquoise
	else 
		glUniform3f(loc_primitive_color, 0.400f, 0.208f, 0.520f);
	draw_geom_obj(GEOM_OBJ_ID_CAR_WHEEL); // draw wheel

	for (i = 0; i < 5; i++) {
		ModelMatrix_CAR_NUT = glm::rotate(ModelMatrix_CAR_WHEEL, TO_RADIAN*72.0f*i, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix_CAR_NUT = glm::translate(ModelMatrix_CAR_NUT, glm::vec3(rad - 0.5f, 0.0f, ww));
		ModelViewProjectionMatrix = ProjectionMatrix * ViewMatrix * ModelMatrix_CAR_NUT;
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix_simple, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);

		glUniform3f(loc_primitive_color, 0.690f, 0.769f, 0.871f); // color name: LightSteelBlue
		draw_geom_obj(GEOM_OBJ_ID_CAR_NUT); // draw i-th nut
	}
}


void draw_car_dummy(void) {

	glUniform3f(loc_primitive_color, 0.498f, 1.000f, 0.831f); // color name: Aquamarine
	draw_geom_obj(GEOM_OBJ_ID_CAR_BODY); // draw body

	glLineWidth(1.0f);
	draw_axes(); // draw MC axes of body
	glLineWidth(1.0f);

	ModelMatrix_CAR_DRIVER = glm::translate(ModelMatrix_CAR_BODY, glm::vec3(-3.0f, 0.5f, 3.5f));
	ModelMatrix_CAR_DRIVER2 = glm::translate(ModelMatrix_CAR_BODY, glm::vec3(-3.0f, 0.5f, 0.5f));
	ModelMatrix_CAR_DRIVER = glm::rotate(ModelMatrix_CAR_DRIVER, TO_RADIAN*90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelMatrix_CAR_DRIVER2 = glm::rotate(ModelMatrix_CAR_DRIVER2, TO_RADIAN*90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	//ModelMatrix_CAR_DRIVER = glm::scale(ModelMatrix_CAR_DRIVER, glm::vec3(10.0f, 10.0f, 10.0f));
	//ModelMatrix_CAR_DRIVER2 = glm::scale(ModelMatrix_CAR_DRIVER, glm::vec3(10.0f, 10.0f, 10.0f));


	DriverViewMatrix = glm::inverse(ModelMatrix_CAR_DRIVER);
	ModelViewMatrix = ViewMatrix * ModelMatrix_CAR_DRIVER;
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_simple, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glLineWidth(3.0f);
	draw_axes(); // draw camera frame at driver seat
	glLineWidth(1.0f);

	ModelMatrix_CAR_BACKMIRROR = glm::translate(ModelMatrix_CAR_BODY, glm::vec3(-3.5, -0.35f, 6.0f));
	ModelMatrix_CAR_PANNEL = glm::translate(ModelMatrix_CAR_BODY, glm::vec3(-3.5f, -0.35f, 6.0f));
	ModelMatrix_CAR_BACKMIRROR = glm::rotate(ModelMatrix_CAR_BACKMIRROR, TO_RADIAN*(270.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ModelMatrix_CAR_PANNEL = glm::rotate(ModelMatrix_CAR_PANNEL, TO_RADIAN*(270.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ModelMatrix_CAR_PANNEL = glm::translate(ModelMatrix_CAR_PANNEL, glm::vec3(0.0f, -1.5f, -3.5f));
	ModelMatrix_CAR_PANNEL = glm::scale(ModelMatrix_CAR_PANNEL, glm::vec3(4.0f, 4.0f, 4.0f));
	ModelMatrix_CAR_BACKMIRROR = glm::scale(ModelMatrix_CAR_BACKMIRROR, glm::vec3(2.0f, 2.0f, 2.0f));




	BackMirrorViewMatrix = glm::inverse(ModelMatrix_CAR_BACKMIRROR);
	ModelViewMatrix = ViewMatrix * ModelMatrix_CAR_BACKMIRROR;
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_simple, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glLineWidth(3.0f);
	draw_axes(); // draw camera BACKMIRROR at driver seat
	glLineWidth(1.0f);



	if( blending_flag )
		glEnable(GL_BLEND);
	glUseProgram(h_ShaderProgram_PS);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUniform1i(loc_u_flag_blending, 1);
	glUniform1f(loc_u_fragment_alpha, rectangle_alpha);

	ModelViewMatrix = ViewMatrix * ModelMatrix_CAR_PANNEL;
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_PS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_PS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_PS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	glLineWidth(3.0f);
	if( camera_mode == BACKMIRROR_CAMERA )
		draw_floor(); // draw camera BACKMIRROR at driver seat
	glUniform1i(loc_u_flag_blending, 0);
	if (blending_flag)
		glDisable(GL_BLEND);
	glutPostRedisplay();
	glLineWidth(1.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glUseProgram(h_ShaderProgram_simple);




	//float wheel_angle = (car_dist / rad);
	float wheel_angle = (float)tmp_idx / 80.0f;
	//printf("%lf\n", wheel_angle);
	//wheel_angle = (int)wheel_angle - ( (int)wh
	glm::vec3 a = glm::vec3(car_path_vertices[tmp_idx] - car_path_vertices[tmp_idx + 3], car_path_vertices[tmp_idx + 1] - car_path_vertices[tmp_idx + 4], car_path_vertices[tmp_idx + 2] - car_path_vertices[tmp_idx + 5]);
	glm::vec3 b = glm::vec3(car_path_vertices[tmp_idx + 39] - car_path_vertices[tmp_idx + 3 + 39], car_path_vertices[tmp_idx + 1 + 39] - car_path_vertices[tmp_idx + 4 + 39], car_path_vertices[tmp_idx + 2 + 39] - car_path_vertices[tmp_idx + 5 + 39]);
	float wheel_sheer_angle = acos(glm::dot(glm::normalize(a), glm::normalize(b)));
	wheel_sheer_angle *= 5.0f;
	if (glm::dot(glm::cross(a, b), glm::vec3(0.0f, 1.0f, 0.0f)) < 0.0f) {
		wheel_flag = 1;
		wheel_sheer_angle = -wheel_sheer_angle;
	}
	else
		wheel_flag = 0;
	if (tmp_idx + 39 > 2240)
		wheel_sheer_angle = 0.0f;

	ModelMatrix_CAR_WHEEL = glm::translate(ModelMatrix_CAR_BODY, glm::vec3(-3.9f, -3.5f, 4.5f));
	ModelMatrix_CAR_WHEEL = glm::rotate(ModelMatrix_CAR_WHEEL, wheel_sheer_angle, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelMatrix_CAR_WHEEL = glm::rotate(ModelMatrix_CAR_WHEEL, wheel_angle, glm::vec3(0.0f, 0.0f, 1.0f));
	wheel0 = ModelMatrix_CAR_WHEEL * zero;
	//DriverViewMatrix = glm::inverse(ModelMatrix_CAR_WHEEL);
	ModelViewMatrix = ViewMatrix * ModelMatrix_CAR_WHEEL;
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_simple, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_wheel_and_nut();  // draw wheel 0

	ModelMatrix_CAR_WHEEL = glm::translate(ModelMatrix_CAR_BODY, glm::vec3(3.9f, -3.5f, 4.5f));
	//ModelMatrix_CAR_WHEEL = glm::rotate(ModelMatrix_CAR_WHEEL, wheel_sheer_angle, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelMatrix_CAR_WHEEL = glm::rotate(ModelMatrix_CAR_WHEEL, wheel_angle, glm::vec3(0.0f, 0.0f, 1.0f));
	wheel1 = ModelMatrix_CAR_WHEEL * zero;
	ModelViewProjectionMatrix = ProjectionMatrix * ViewMatrix * ModelMatrix_CAR_WHEEL;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_simple, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_wheel_and_nut();  // draw wheel 1

	ModelMatrix_CAR_WHEEL = glm::translate(ModelMatrix_CAR_BODY, glm::vec3(-3.9f, -3.5f, -4.5f));
	ModelMatrix_CAR_WHEEL = glm::rotate(ModelMatrix_CAR_WHEEL, wheel_sheer_angle, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelMatrix_CAR_WHEEL = glm::rotate(ModelMatrix_CAR_WHEEL, wheel_angle, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelMatrix_CAR_WHEEL = glm::scale(ModelMatrix_CAR_WHEEL, glm::vec3(1.0f, 1.0f, -1.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ViewMatrix * ModelMatrix_CAR_WHEEL;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_simple, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_wheel_and_nut();  // draw wheel 2

	ModelMatrix_CAR_WHEEL = glm::translate(ModelMatrix_CAR_BODY, glm::vec3(3.9f, -3.5f, -4.5f));
	//ModelMatrix_CAR_WHEEL = glm::rotate(ModelMatrix_CAR_WHEEL, wheel_sheer_angle, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelMatrix_CAR_WHEEL = glm::rotate(ModelMatrix_CAR_WHEEL, wheel_angle, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelMatrix_CAR_WHEEL = glm::scale(ModelMatrix_CAR_WHEEL, glm::vec3(1.0f, 1.0f, -1.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ViewMatrix * ModelMatrix_CAR_WHEEL;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_simple, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_wheel_and_nut();  // draw wheel 3
}

// callbacks

void set_highlight() {
	glm::vec4 high1 = ModelMatrix_CAR_DRIVER * zero;
	glm::vec4 high2 = ModelMatrix_CAR_DRIVER2 * zero;
	/*glm::vec3 high1_dir = glm::vec3(car_path_vertices[tmp_idx] - car_path_vertices[prv_idx],
	car_path_vertices[tmp_idx+1] - car_path_vertices[prv_idx+1],
	car_path_vertices[tmp_idx+2] - car_path_vertices[prv_idx+2]);*/
	glm::vec3 high1_dir = glm::vec3(wheel0.x - wheel1.x, wheel0.y - wheel1.y, wheel0.z - wheel1.z);
	/*	if( tmp_idx <= 9 )
	high1_dir = glm::vec3(car_path_vertices[3] - car_path_vertices[0],
	car_path_vertices[4] - car_path_vertices[1],
	car_path_vertices[5] - car_path_vertices[2]);*/
	//	if( tmp_idx < prv_idx )
	//	high1_dir = -high1_dir;

	light[3].light_on = 1;
	light[3].position[0] = high1.x; light[3].position[1] = high1.y; // spot light position in WC
	light[3].position[2] = high1.z; light[3].position[3] = high1.w;

	light[3].ambient_color[0] = 0.2f; light[3].ambient_color[1] = 0.2f;
	light[3].ambient_color[2] = 0.2f; light[3].ambient_color[3] = 1.0f;

	light[3].diffuse_color[0] = 0.82f; light[3].diffuse_color[1] = 0.82f;
	light[3].diffuse_color[2] = 0.82f; light[3].diffuse_color[3] = 1.0f;

	light[3].specular_color[0] = 0.82f; light[3].specular_color[1] = 0.82f;
	light[3].specular_color[2] = 0.82f; light[3].specular_color[3] = 1.0f;

	light[3].spot_direction[0] = high1_dir.x; //light[3].spot_direction[1] = -high1_dir.y; // spot light direction in WC
	light[3].spot_direction[2] = high1_dir.z;
	light[3].spot_cutoff_angle = 15.0f;
	light[3].spot_exponent = 15.0f;

	light[2].light_on = 1;
	light[2].position[0] = high2.x; light[2].position[1] = high2.y; // spot light position in WC
	light[2].position[2] = high2.z; light[2].position[3] = high2.w;

	light[2].ambient_color[0] = 0.2f; light[2].ambient_color[1] = 0.2f;
	light[2].ambient_color[2] = 0.2f; light[2].ambient_color[3] = 1.0f;

	light[2].diffuse_color[0] = 0.82f; light[2].diffuse_color[1] = 0.82f;
	light[2].diffuse_color[2] = 0.82f; light[2].diffuse_color[3] = 1.0f;

	light[2].specular_color[0] = 0.82f; light[2].specular_color[1] = 0.82f;
	light[2].specular_color[2] = 0.82f; light[2].specular_color[3] = 1.0f;

	light[2].spot_direction[0] = high1_dir.x; //light[2].spot_direction[1] = -high1_dir.y; // spot light direction in WC
	light[2].spot_direction[2] = high1_dir.z;
	light[2].spot_cutoff_angle = 15.0f;
	light[2].spot_exponent = 15.0f;


	glUseProgram(h_ShaderProgram_PS);
	glUniform1i(loc_light[0].light_on, light[0].light_on);
	glUniform4fv(loc_light[0].position, 1, light[0].position);
	glUniform4fv(loc_light[0].ambient_color, 1, light[0].ambient_color);
	glUniform4fv(loc_light[0].diffuse_color, 1, light[0].diffuse_color);
	glUniform4fv(loc_light[0].specular_color, 1, light[0].specular_color);

	glUniform1i(loc_light[3].light_on, light[3].light_on);
	// need to supply position in EC for shading
	glm::vec4 position_EC = ViewMatrix * glm::vec4(light[3].position[0], light[3].position[1],
		light[3].position[2], light[3].position[3]);
	glUniform4fv(loc_light[3].position, 1, &position_EC[0]);
	glUniform4fv(loc_light[3].ambient_color, 1, light[3].ambient_color);
	glUniform4fv(loc_light[3].diffuse_color, 1, light[3].diffuse_color);
	glUniform4fv(loc_light[3].specular_color, 1, light[3].specular_color);
	// need to supply direction in EC for shading in this example shader
	// note that the viewing transform is a rigid body transform
	// thus transpose(inverse(mat3(ViewMatrix)) = mat3(ViewMatrix)
	glm::vec3 direction_EC = glm::mat3(ViewMatrix) * glm::vec3(light[3].spot_direction[0], light[3].spot_direction[1],
		light[3].spot_direction[2]);
	glUniform3fv(loc_light[3].spot_direction, 1, &direction_EC[0]);
	glUniform1f(loc_light[3].spot_cutoff_angle, light[3].spot_cutoff_angle);
	glUniform1f(loc_light[3].spot_exponent, light[3].spot_exponent);

	glUniform1i(loc_light[2].light_on, light[2].light_on);
	// need to supply position in EC for shading
	glm::vec4 position_EC2 = ViewMatrix * glm::vec4(light[2].position[0], light[2].position[1],
		light[2].position[2], light[2].position[3]);
	glUniform4fv(loc_light[2].position, 1, &position_EC2[0]);
	glUniform4fv(loc_light[2].ambient_color, 1, light[2].ambient_color);
	glUniform4fv(loc_light[2].diffuse_color, 1, light[2].diffuse_color);
	glUniform4fv(loc_light[2].specular_color, 1, light[2].specular_color);
	// need to supply direction in EC for shading in this example shader
	// note that the viewing transform is a rigid body transform
	// thus transpose(inverse(mat3(ViewMatrix)) = mat3(ViewMatrix)
	glm::vec3 direction_EC2 = glm::mat3(ViewMatrix) * glm::vec3(light[2].spot_direction[0], light[2].spot_direction[1],
		light[2].spot_direction[2]);
	glUniform3fv(loc_light[2].spot_direction, 1, &direction_EC2[0]);
	glUniform1f(loc_light[2].spot_cutoff_angle, light[2].spot_cutoff_angle);
	glUniform1f(loc_light[2].spot_exponent, light[2].spot_exponent);

	glUseProgram(0);
}

void display(void) {

	glm::mat4 tiger_axis, tiger_real;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (camera_mode == DRIVER_CAMERA) {
		ViewMatrix = DriverViewMatrix;
		glViewport(0, 0, 800, 800);
		blending_flag = 0;
		rectangle_alpha = 0.0f;
	}
	else if (camera_mode == NORMAL_CAMERA) {
		ViewMatrix = glm::lookAt(glm::vec3(1500.0f, 1500.0f, 1500.0f), glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f));
		glViewport(0, 0, 800, 800);
		blending_flag = 0;
		rectangle_alpha = 0.0f;
	}
	else if (camera_mode == BACKMIRROR_CAMERA) {
		ViewMatrix = BackMirrorViewMatrix;
		glViewport(200, 200, 400, 400);
		blending_flag = 1;
		rectangle_alpha = 0.5f;
	}
	if (highlight_mode)
		set_highlight();
	else
		set_up_scene_lights();

	/* ------------------------------------------------------------------------------------------------------- */

	glUseProgram(h_ShaderProgram_simple);
	ModelViewMatrix = ViewMatrix;
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_simple, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glLineWidth(2.0f);
	draw_car_path();
	glLineWidth(1.0f);

	ModelViewMatrix = glm::scale(ViewMatrix, glm::vec3(50.0f, 50.0f, 50.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_simple, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glLineWidth(2.0f);
	draw_axes();
	glLineWidth(1.0f);

	glUseProgram(h_ShaderProgram_PS);
	set_material_floor();
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-750.0f, 0.0f, 750.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(1500.0f, 1500.0f, 1500.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f*TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_PS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_PS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_PS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_floor();

	/* ------------------------------------------------------------------------------------------------------- */

	set_material_tiger();
	ModelViewMatrix = ViewMatrix;
	ModelViewMatrix = glm::translate(ModelViewMatrix, glm::vec3(0.0f, 50 * cos(rotation_angle_tiger * 5), 0.0f));


	ModelViewMatrix = glm::translate(ModelViewMatrix, glm::vec3(car_path_vertices[1800],
		car_path_vertices[1801], car_path_vertices[1802]));

	ModelViewMatrix = glm::rotate(ModelViewMatrix, -90 * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, 180 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_PS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_PS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_PS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_tiger();

	/* ------------------------------------------------------------------------------------------------------- */

	set_material_barel();
	tiger_axis = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	tiger_axis = glm::rotate(tiger_axis, rotation_angle_tiger, glm::vec3(0.0f, 1.0f, 0.0f));
	tiger_axis = glm::translate(tiger_axis, glm::vec3(200.0f, 0.0f, 200.0f));
	tiger_axis = glm::rotate(tiger_axis, -90 * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));

	tiger_real = tiger_axis;
	tiger_real = glm::rotate(tiger_real, rotation_angle_tiger, glm::vec3(0.0f, 0.0f, 1.0f));
	tiger_real = glm::translate(tiger_real, glm::vec3(100.0f, 0.0f, 100.0f));

	ModelViewMatrix = ViewMatrix * tiger_real;
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(40.0f, 40.0f, 40.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_PS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_PS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_PS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_barel();

	/* ------------------------------------------------------------------------------------------------------- */

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glUseProgram(h_ShaderProgram_simple);

	if (tmp_idx == prv_idx)
		prv_idx = prv_prv_idx;
	glm::vec3 a = glm::vec3((float)car_path_vertices[3] - (float)car_path_vertices[0],
		(float)car_path_vertices[4] - (float)car_path_vertices[1],
		(float)car_path_vertices[5] - (float)car_path_vertices[2]);
	glm::vec3 b;

	b = glm::vec3((float)car_path_vertices[tmp_idx] - (float)car_path_vertices[prv_idx],
		(float)car_path_vertices[tmp_idx + 1] - (float)car_path_vertices[prv_idx + 1],
		(float)car_path_vertices[tmp_idx + 2] - (float)car_path_vertices[prv_idx + 2]);
	if (tmp_idx < prv_idx)
		b = -b;
	car_dist = glm::length(glm::vec3(car_path_vertices[tmp_idx] - car_path_vertices[0], car_path_vertices[tmp_idx + 1] - car_path_vertices[1], car_path_vertices[tmp_idx + 2] - car_path_vertices[2]));

	if (tmp_idx > 5)
		angle = acos(glm::dot(glm::normalize(a), glm::normalize(b)));

	ModelMatrix_CAR_BODY = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 130.0f, 0.0f));
	ModelMatrix_CAR_BODY = glm::translate(ModelMatrix_CAR_BODY, glm::vec3(car_path_vertices[tmp_idx], car_path_vertices[tmp_idx + 1], car_path_vertices[tmp_idx + 2]));
	ModelMatrix_CAR_BODY = glm::rotate(ModelMatrix_CAR_BODY, (90 + 0) * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelMatrix_CAR_BODY = glm::rotate(ModelMatrix_CAR_BODY, angle, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelMatrix_CAR_BODY = glm::scale(ModelMatrix_CAR_BODY, glm::vec3(20.0f, 20.0f, 20.0f));
	ModelViewMatrix = ViewMatrix * ModelMatrix_CAR_BODY;
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_simple, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_car_dummy();

	/* ------------------------------------------------------------------------------------------------------- */

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glUseProgram(h_ShaderProgram_PS);
	glUseProgram(0);

	glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
	static int flag_cull_face = 0;
	static int flag_polygon_mode = 1;

	if ((key >= '0') && (key <= '0' + NUMBER_OF_LIGHT_SUPPORTED - 1)) {
		int light_ID = (int)(key - '0');

		glUseProgram(h_ShaderProgram_PS);
		light[light_ID].light_on = 1 - light[light_ID].light_on;
		glUniform1i(loc_light[light_ID].light_on, light[light_ID].light_on);
		glUseProgram(0);

		glutPostRedisplay();
		return;
	}

	switch (key) {
	case 27: // ESC key
		glutLeaveMainLoop(); // Incur destuction callback for cleanups
		break;
	case 'c':
		flag_cull_face = (flag_cull_face + 1) % 3;
		switch (flag_cull_face) {
		case 0:
			glDisable(GL_CULL_FACE);
			glutPostRedisplay();
			break;
		case 1: // cull back faces;
			glCullFace(GL_BACK);
			glEnable(GL_CULL_FACE);
			glutPostRedisplay();
			break;
		case 2: // cull front faces;
			glCullFace(GL_FRONT);
			glEnable(GL_CULL_FACE);
			glutPostRedisplay();
			break;
		}
		break;
	case 'p':
		flag_polygon_mode = 1 - flag_polygon_mode;
		if (flag_polygon_mode)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glutPostRedisplay();
		break;
	case 'd':
		camera_mode = DRIVER_CAMERA;
		break;
	case 'w':
		camera_mode = NORMAL_CAMERA;
		break;
	case 'b':
		camera_mode = BACKMIRROR_CAMERA;
		break;
	case 'h':
		highlight_mode = 1 - highlight_mode;
		break;
	case 'f':
		fun_effect = 1 - fun_effect;

		glUseProgram(h_ShaderProgram_PS);
		glUniform1i(loc_funeffect, fun_effect);
		glUseProgram(0);
		glutPostRedisplay();
		break;
	}

}

int prv_x = 0;
void motion(int x, int y) {
	int delx;
	int t;
	if (!loc_change) return;

	delx = (prv_x - x);
	//printf("%d %d %d \n", prv_x, x, delx);
	prv_prv_idx = prv_idx;
	prv_idx = tmp_idx;
	t = tmp_idx + (int)delx - ((int)delx) % 3;

	if (t >= 1 && t <= 750 * 3)
		tmp_idx = t;
	prv_x = x;


	glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
	if ((button == GLUT_RIGHT_BUTTON)) {
		if (state == GLUT_DOWN) {
			loc_change = 1;
			prv_x = x;
			glutPostRedisplay();

		}
		else if (state == GLUT_UP) {
			loc_change = 0; glutPostRedisplay();
		}
	}
	if ((button == GLUT_LEFT_BUTTON)) {
		if (state == GLUT_DOWN) {
			printf("x is %d\n", x);
		}
	}
}
void reshape(int width, int height) {
	float aspect_ratio;
	glViewport(0, 0, width, height);

	aspect_ratio = (float)width / height;
	ProjectionMatrix = glm::perspective(45.0f*TO_RADIAN, aspect_ratio, 1.0f, 10000.0f);

	glutPostRedisplay();
}

void timer_scene(int timestamp_scene) {
	cur_frame_tiger = timestamp_scene % N_TIGER_FRAMES;
	rotation_angle_tiger = (timestamp_scene % 360)*TO_RADIAN;
	glutPostRedisplay();
	glutTimerFunc(10, timer_scene, (timestamp_scene + 1) % INT_MAX);
}

void cleanup(void) {
	glDeleteVertexArrays(1, &axes_VAO);
	glDeleteBuffers(1, &axes_VBO);

	glDeleteVertexArrays(1, &tiger_VAO);
	glDeleteBuffers(1, &tiger_VBO);
}

void register_callbacks(void) {
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutReshapeFunc(reshape);
	glutTimerFunc(100, timer_scene, 0);
	glutCloseFunc(cleanup);
}

void prepare_shader_program(void) {
	int i;
	char string[256];
	ShaderInfo shader_info_simple[3] = {
		{ GL_VERTEX_SHADER, "Shaders/simple.vert" },
		{ GL_FRAGMENT_SHADER, "Shaders/simple.frag" },
		{ GL_NONE, NULL }
	};
	ShaderInfo shader_info_PS[3] = {
		{ GL_VERTEX_SHADER, "Shaders/Phong.vert" },
		{ GL_FRAGMENT_SHADER, "Shaders/Phong.frag" },
		{ GL_NONE, NULL }
	};

	h_ShaderProgram_simple = LoadShaders(shader_info_simple);
	loc_primitive_color = glGetUniformLocation(h_ShaderProgram_simple, "u_primitive_color");
	loc_ModelViewProjectionMatrix_simple = glGetUniformLocation(h_ShaderProgram_simple, "u_ModelViewProjectionMatrix");

	h_ShaderProgram_PS = LoadShaders(shader_info_PS);
	loc_ModelViewProjectionMatrix_PS = glGetUniformLocation(h_ShaderProgram_PS, "u_ModelViewProjectionMatrix");
	loc_ModelViewMatrix_PS = glGetUniformLocation(h_ShaderProgram_PS, "u_ModelViewMatrix");
	loc_ModelViewMatrixInvTrans_PS = glGetUniformLocation(h_ShaderProgram_PS, "u_ModelViewMatrixInvTrans");

	loc_global_ambient_color = glGetUniformLocation(h_ShaderProgram_PS, "u_global_ambient_color");
	for (i = 0; i < NUMBER_OF_LIGHT_SUPPORTED; i++) {
		sprintf(string, "u_light[%d].light_on", i);
		loc_light[i].light_on = glGetUniformLocation(h_ShaderProgram_PS, string);
		sprintf(string, "u_light[%d].position", i);
		loc_light[i].position = glGetUniformLocation(h_ShaderProgram_PS, string);
		sprintf(string, "u_light[%d].ambient_color", i);
		loc_light[i].ambient_color = glGetUniformLocation(h_ShaderProgram_PS, string);
		sprintf(string, "u_light[%d].diffuse_color", i);
		loc_light[i].diffuse_color = glGetUniformLocation(h_ShaderProgram_PS, string);
		sprintf(string, "u_light[%d].specular_color", i);
		loc_light[i].specular_color = glGetUniformLocation(h_ShaderProgram_PS, string);
		sprintf(string, "u_light[%d].spot_direction", i);
		loc_light[i].spot_direction = glGetUniformLocation(h_ShaderProgram_PS, string);
		sprintf(string, "u_light[%d].spot_exponent", i);
		loc_light[i].spot_exponent = glGetUniformLocation(h_ShaderProgram_PS, string);
		sprintf(string, "u_light[%d].spot_cutoff_angle", i);
		loc_light[i].spot_cutoff_angle = glGetUniformLocation(h_ShaderProgram_PS, string);
		sprintf(string, "u_light[%d].light_attenuation_factors", i);
		loc_light[i].light_attenuation_factors = glGetUniformLocation(h_ShaderProgram_PS, string);
	}

	loc_material.ambient_color = glGetUniformLocation(h_ShaderProgram_PS, "u_material.ambient_color");
	loc_material.diffuse_color = glGetUniformLocation(h_ShaderProgram_PS, "u_material.diffuse_color");
	loc_material.specular_color = glGetUniformLocation(h_ShaderProgram_PS, "u_material.specular_color");
	loc_material.emissive_color = glGetUniformLocation(h_ShaderProgram_PS, "u_material.emissive_color");
	loc_material.specular_exponent = glGetUniformLocation(h_ShaderProgram_PS, "u_material.specular_exponent");
	loc_funeffect = glGetUniformLocation(h_ShaderProgram_PS, "u_flag_fun_effect");
	loc_u_flag_blending = glGetUniformLocation(h_ShaderProgram_PS, "u_flag_blending");
	loc_u_fragment_alpha = glGetUniformLocation(h_ShaderProgram_PS, "u_fragment_alpha");
}

void initialize_lights_and_material(void) { // follow OpenGL conventions for initialization
	int i;

	glUseProgram(h_ShaderProgram_PS);

	glUniform4f(loc_global_ambient_color, 0.2f, 0.2f, 0.2f, 1.0f);
	for (i = 0; i < NUMBER_OF_LIGHT_SUPPORTED; i++) {
		glUniform1i(loc_light[i].light_on, 0); // turn off all lights initially
		glUniform4f(loc_light[i].position, 0.0f, 0.0f, 1.0f, 0.0f);
		glUniform4f(loc_light[i].ambient_color, 0.0f, 0.0f, 0.0f, 1.0f);
		if (i == 0) {
			glUniform4f(loc_light[i].diffuse_color, 1.0f, 1.0f, 1.0f, 1.0f);
			glUniform4f(loc_light[i].specular_color, 1.0f, 1.0f, 1.0f, 1.0f);
		}
		else {
			glUniform4f(loc_light[i].diffuse_color, 0.0f, 0.0f, 0.0f, 1.0f);
			glUniform4f(loc_light[i].specular_color, 0.0f, 0.0f, 0.0f, 1.0f);
		}
		glUniform3f(loc_light[i].spot_direction, 0.0f, 0.0f, -1.0f);
		glUniform1f(loc_light[i].spot_exponent, 0.0f); // [0.0, 128.0]
		glUniform1f(loc_light[i].spot_cutoff_angle, 180.0f); // [0.0, 90.0] or 180.0 (180.0 for no spot light effect)
		glUniform4f(loc_light[i].light_attenuation_factors, 1.0f, 0.0f, 0.0f, 0.0f); // .w != 0.0f for no ligth attenuation
	}

	glUniform4f(loc_material.ambient_color, 0.2f, 0.2f, 0.2f, 1.0f);
	glUniform4f(loc_material.diffuse_color, 0.8f, 0.8f, 0.8f, 1.0f);
	glUniform4f(loc_material.specular_color, 0.0f, 0.0f, 0.0f, 1.0f);
	glUniform4f(loc_material.emissive_color, 0.0f, 0.0f, 0.0f, 1.0f);
	glUniform1f(loc_material.specular_exponent, 0.0f); // [0.0, 128.0]

	glUseProgram(0);
}

void initialize_OpenGL(void) {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	ViewMatrix = glm::lookAt(glm::vec3(1500.0f, 1500.0f, 1500.0f), glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));

	initialize_lights_and_material();
}

void set_up_scene_lights(void) {
	// point_light_EC: use light 0
	light[0].light_on = 1;
	light[0].position[0] = 0.0f; light[0].position[1] = 10.0f; 	// point light position in EC
	light[0].position[2] = 0.0f; light[0].position[3] = 1.0f;

	light[0].ambient_color[0] = 0.3f; light[0].ambient_color[1] = 0.3f;
	light[0].ambient_color[2] = 0.3f; light[0].ambient_color[3] = 1.0f;

	light[0].diffuse_color[0] = 0.7f; light[0].diffuse_color[1] = 0.7f;
	light[0].diffuse_color[2] = 0.7f; light[0].diffuse_color[3] = 1.0f;

	light[0].specular_color[0] = 0.9f; light[0].specular_color[1] = 0.9f;
	light[0].specular_color[2] = 0.9f; light[0].specular_color[3] = 1.0f;

	// spot_light_WC: use light 1
	light[1].light_on = 1;
	light[1].position[0] = -200.0f; light[1].position[1] = 500.0f; // spot light position in WC
	light[1].position[2] = -200.0f; light[1].position[3] = 1.0f;

	light[1].ambient_color[0] = 0.2f; light[1].ambient_color[1] = 0.2f;
	light[1].ambient_color[2] = 0.2f; light[1].ambient_color[3] = 1.0f;

	light[1].diffuse_color[0] = 0.82f; light[1].diffuse_color[1] = 0.82f;
	light[1].diffuse_color[2] = 0.82f; light[1].diffuse_color[3] = 1.0f;

	light[1].specular_color[0] = 0.82f; light[1].specular_color[1] = 0.82f;
	light[1].specular_color[2] = 0.82f; light[1].specular_color[3] = 1.0f;

	light[1].spot_direction[0] = 0.0f; light[1].spot_direction[1] = -1.0f; // spot light direction in WC
	light[1].spot_direction[2] = 0.0f;
	light[1].spot_cutoff_angle = 20.0f;
	light[1].spot_exponent = 27.0f;




	// spot_light_WC: use light 1
	light[2].light_on = 0;
	// spot_light_WC: use light 1
	light[3].light_on = 0;



	glUseProgram(h_ShaderProgram_PS);
	glUniform1i(loc_light[0].light_on, light[0].light_on);
	glUniform4fv(loc_light[0].position, 1, light[0].position);
	glUniform4fv(loc_light[0].ambient_color, 1, light[0].ambient_color);
	glUniform4fv(loc_light[0].diffuse_color, 1, light[0].diffuse_color);
	glUniform4fv(loc_light[0].specular_color, 1, light[0].specular_color);

	glUniform1i(loc_light[1].light_on, light[1].light_on);
	// need to supply position in EC for shading
	glm::vec4 position_EC = ViewMatrix * glm::vec4(light[1].position[0], light[1].position[1],
		light[1].position[2], light[1].position[3]);
	glUniform4fv(loc_light[1].position, 1, &position_EC[0]);
	glUniform4fv(loc_light[1].ambient_color, 1, light[1].ambient_color);
	glUniform4fv(loc_light[1].diffuse_color, 1, light[1].diffuse_color);
	glUniform4fv(loc_light[1].specular_color, 1, light[1].specular_color);
	// need to supply direction in EC for shading in this example shader
	// note that the viewing transform is a rigid body transform
	// thus transpose(inverse(mat3(ViewMatrix)) = mat3(ViewMatrix)
	glm::vec3 direction_EC = glm::mat3(ViewMatrix) * glm::vec3(light[1].spot_direction[0], light[1].spot_direction[1],
		light[1].spot_direction[2]);
	glUniform3fv(loc_light[1].spot_direction, 1, &direction_EC[0]);
	glUniform1f(loc_light[1].spot_cutoff_angle, light[1].spot_cutoff_angle);
	glUniform1f(loc_light[1].spot_exponent, light[1].spot_exponent);

	glUniform1i(loc_light[2].light_on, light[2].light_on);
	// need to supply position in EC for shading
	glm::vec4 position_EC3 = ViewMatrix * glm::vec4(light[2].position[0], light[2].position[1],
		light[2].position[2], light[2].position[3]);
	glUniform4fv(loc_light[2].position, 1, &position_EC3[0]);
	glUniform4fv(loc_light[2].ambient_color, 1, light[2].ambient_color);
	glUniform4fv(loc_light[2].diffuse_color, 1, light[2].diffuse_color);
	glUniform4fv(loc_light[2].specular_color, 1, light[2].specular_color);
	// need to supply direction in EC for shading in this example shader
	// note that the viewing transform is a rigid body transform
	// thus transpose(inverse(mat3(ViewMatrix)) = mat3(ViewMatrix)
	glm::vec3 direction_EC3 = glm::mat3(ViewMatrix) * glm::vec3(light[2].spot_direction[0], light[2].spot_direction[1],
		light[2].spot_direction[2]);
	glUniform3fv(loc_light[2].spot_direction, 1, &direction_EC3[0]);
	glUniform1f(loc_light[2].spot_cutoff_angle, light[2].spot_cutoff_angle);
	glUniform1f(loc_light[2].spot_exponent, light[2].spot_exponent);


	glUniform1i(loc_light[3].light_on, light[3].light_on);
	// need to supply position in EC for shading
	glm::vec4 position_EC2 = ViewMatrix * glm::vec4(light[3].position[0], light[3].position[1],
		light[3].position[2], light[3].position[3]);
	glUniform4fv(loc_light[3].position, 1, &position_EC2[0]);
	glUniform4fv(loc_light[3].ambient_color, 1, light[3].ambient_color);
	glUniform4fv(loc_light[3].diffuse_color, 1, light[3].diffuse_color);
	glUniform4fv(loc_light[3].specular_color, 1, light[3].specular_color);
	// need to supply direction in EC for shading in this example shader
	// note that the viewing transform is a rigid body transform
	// thus transpose(inverse(mat3(ViewMatrix)) = mat3(ViewMatrix)
	glm::vec3 direction_EC2 = glm::mat3(ViewMatrix) * glm::vec3(light[3].spot_direction[0], light[3].spot_direction[1],
		light[3].spot_direction[2]);
	glUniform3fv(loc_light[3].spot_direction, 1, &direction_EC2[0]);
	glUniform1f(loc_light[3].spot_cutoff_angle, light[3].spot_cutoff_angle);
	glUniform1f(loc_light[3].spot_exponent, light[3].spot_exponent);





	glUseProgram(0);
}

void prepare_scene(void) {
	prepare_axes();
	prepare_geom_obj(GEOM_OBJ_ID_CAR_BODY, "Data/car_body_triangles_v.txt", _GEOM_OBJ_TYPE::GEOM_OBJ_TYPE_V);
	prepare_geom_obj(GEOM_OBJ_ID_CAR_WHEEL, "Data/car_wheel_triangles_v.txt", _GEOM_OBJ_TYPE::GEOM_OBJ_TYPE_V);
	prepare_geom_obj(GEOM_OBJ_ID_CAR_NUT, "Data/car_nut_triangles_v.txt", _GEOM_OBJ_TYPE::GEOM_OBJ_TYPE_V);
	prepare_geom_obj(GEOM_OBJ_ID_COW, "Data/cow_triangles_v.txt", _GEOM_OBJ_TYPE::GEOM_OBJ_TYPE_V);
	prepare_geom_obj(GEOM_OBJ_ID_TEAPOT, "Data/teapot_triangles_v.txt", _GEOM_OBJ_TYPE::GEOM_OBJ_TYPE_V);
	prepare_geom_obj(GEOM_OBJ_ID_BOX, "Data/box_triangles_v.txt", _GEOM_OBJ_TYPE::GEOM_OBJ_TYPE_V);
	prepare_floor();
	prepare_tiger();
	prepare_car();
	prepare_barel();
	prepare_box();
	set_up_scene_lights();
}

void initialize_renderer(void) {
	register_callbacks();
	prepare_shader_program();
	initialize_OpenGL();
	prepare_scene();
}

void initialize_glew(void) {
	GLenum error;

	glewExperimental = GL_TRUE;

	error = glewInit();
	if (error != GLEW_OK) {
		fprintf(stderr, "Error: %s\n", glewGetErrorString(error));
		exit(-1);
	}
	fprintf(stdout, "*********************************************************\n");
	fprintf(stdout, " - GLEW version supported: %s\n", glewGetString(GLEW_VERSION));
	fprintf(stdout, " - OpenGL renderer: %s\n", glGetString(GL_RENDERER));
	fprintf(stdout, " - OpenGL version supported: %s\n", glGetString(GL_VERSION));
	fprintf(stdout, "*********************************************************\n\n");
}

void greetings(char *program_name, char messages[][256], int n_message_lines) {
	fprintf(stdout, "**************************************************************\n\n");
	fprintf(stdout, "  PROGRAM NAME: %s\n\n", program_name);
	fprintf(stdout, "    This program was coded for CSE4170 students\n");
	fprintf(stdout, "      of Dept. of Comp. Sci. & Eng., Sogang University.\n\n");

	for (int i = 0; i < n_message_lines; i++)
		fprintf(stdout, "%s\n", messages[i]);
	fprintf(stdout, "\n**************************************************************\n\n");

	initialize_glew();
}

#define N_MESSAGE_LINES 1
void main(int argc, char *argv[]) {
	// Phong Shading
	char program_name[64] = "OpenGL Rendering Test";
	char messages[N_MESSAGE_LINES][256] = { "    - Keys used: '0', '1', 'c', 'p', 'ESC'" };

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
	glutInitWindowSize(800, 800);
	glutInitContextVersion(4, 0);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutCreateWindow(program_name);

	greetings(program_name, messages, N_MESSAGE_LINES);
	initialize_renderer();

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutMainLoop();
}
