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

#ifndef GLSL_PROGRAMh
#define GLSL_PROGRAMh

#include <GL/glew.h>
#include <string>

struct GLSL_Program {
	//create a constructor which calls initialize() at the end
	virtual ~GLSL_Program();
	static GLuint load_shader(const char* file_name, GLenum type);	//feel free to use these outside the class
	static std::string load_shader_source(const char* file_name);
	static GLuint load_shader_from_source(std::string source, GLenum type);	
	void use();
	
	static void uniform_tex_rect(int i, GLuint tex, GLuint param);
	static void uniform_tex_1d(int i, GLuint tex, GLuint param);
	static void uniform_tex_2d(int i, GLuint tex, GLuint param);
	static void uniform_tex_3d(int i, GLuint tex, GLuint param);
	GLuint program;	// keep program and variables public
	
protected:
	void initialize();
	static void uniform_tex(int i, GLuint tex, GLuint param, GLenum target);

	virtual void attach_shaders() = 0;
	virtual void set_parameters() = 0;

	int locate_uniform(const char* name);

	void attach_shader(GLuint shader);
	void attach_shader(const char* file_name, GLenum type);
	void attach_fragment_shader(const char* file_name);
	void attach_vertex_shader(const char* file_name);
	
	void attach_shader_from_source(std::string source, GLenum type);
	void attach_fragment_shader_from_source(std::string source);
	void attach_vertex_shader_from_source(std::string source);
};

#endif
