/*
libMeshlessVis
Copyright (C) 2008 Andrew Corrigan

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "glsl_program.h"
#include <GL/glew.h>
#include <string>
#include <fstream>

GLSL_Program::~GLSL_Program() {
	glDeleteProgram(program);
}

void GLSL_Program::use() {
	glUseProgram(program);
}

void GLSL_Program::uniform_tex(int i, GLuint tex, GLuint param, GLenum target)
{
	glActiveTexture(GL_TEXTURE0+i);
	glBindTexture(target, tex);
	glUniform1i(param, i);
}

void GLSL_Program::uniform_tex_rect(int i, GLuint tex, GLuint param) {
	uniform_tex(i, tex, param, GL_TEXTURE_RECTANGLE_ARB);
}

void GLSL_Program::uniform_tex_1d(int i, GLuint tex, GLuint param) {
	uniform_tex(i, tex, param, GL_TEXTURE_1D);
}

void GLSL_Program::uniform_tex_2d(int i, GLuint tex, GLuint param) {
	uniform_tex(i, tex, param, GL_TEXTURE_2D);
}

void GLSL_Program::uniform_tex_3d(int i, GLuint tex, GLuint param) {
	uniform_tex(i, tex, param, GL_TEXTURE_3D);
}

GLuint GLSL_Program::load_shader(const char* file_name, GLenum type) {
	
	return GLSL_Program::load_shader_from_source(GLSL_Program::load_shader_source(file_name), type);
}

std::string GLSL_Program::load_shader_source(const char* file_name) {
	std::ifstream file(file_name, std::ios::in | std::ios::binary);
	file.seekg (0, std::ios::end);
	int size = file.tellg();
	file.seekg(0, std::ios::beg);
	char* buffer = new char [size+1];
	file.read(buffer, size);
	buffer[size] = 0;
	std::string source(buffer);
	delete[] buffer;
	return source;
}

GLuint GLSL_Program::load_shader_from_source(std::string source, GLenum type) {
	//make sure it's ok to proceed
	if(type == 0) throw "invalid shader type";
	if(glGetError() != GL_NO_ERROR) throw "OpenGL error before loading shader";

	//create the shader
	GLuint shader = glCreateShader(type);
	if(shader == 0) throw "Could not create shader";

	//compile the shader source code into the shader
	const char* cstr = source.c_str();
	glShaderSource(shader, 1, &cstr, 0);
		
	glCompileShader(shader);

	//make sure everything went ok
	GLint compiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if(compiled == GL_FALSE) {
		static char log[2048];
		glGetShaderInfoLog(shader, 2048, 0, log);
		throw log;
	}
	
	if(glGetError() != GL_NO_ERROR) throw "OpenGL error after loading shader";
	
	return shader;
}

void GLSL_Program::initialize() {
	program = glCreateProgram();
	attach_shaders();
	glLinkProgram(program);
	set_parameters();
	if(glGetError() != GL_NO_ERROR) throw "OpenGL after initializing shader";
}

void GLSL_Program::attach_shader(GLuint shader) {
	glAttachShader(program, shader);
	
	//tell shader to be deleted once it is no longer attached to any programs
	glDeleteShader(shader);	
}

void GLSL_Program::attach_shader(const char* file_name, GLenum type) {
	attach_shader(load_shader(file_name, type));
}

void GLSL_Program::attach_fragment_shader(const char* file_name) {
	attach_shader(load_shader(file_name, GL_FRAGMENT_SHADER));
}

void GLSL_Program::attach_vertex_shader(const char* file_name) {
	attach_shader(load_shader(file_name, GL_VERTEX_SHADER));
}

void GLSL_Program::attach_shader_from_source(std::string source, GLenum type) {
	attach_shader(load_shader_from_source(source, type));
}

void GLSL_Program::attach_fragment_shader_from_source(std::string source) {
	attach_shader(load_shader_from_source(source, GL_FRAGMENT_SHADER));
}

void GLSL_Program::attach_vertex_shader_from_source(std::string source) {
	attach_shader(load_shader_from_source(source, GL_VERTEX_SHADER));
}

int GLSL_Program::locate_uniform(const char* name) {
	GLint loc = glGetUniformLocation(program, name); 
	return loc;
}
