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

#include "texture.h"

void create_texture_1D(
	GLuint& texture,
	GLint filter,
	GLint wrap,
	GLint internal_format,
	GLenum external_format,
	int length,
	const GLvoid* pixels
	) {

	const GLenum target = GL_TEXTURE_1D;

	if(glGetError() != GL_NO_ERROR) throw "OpenGL error before loading texture";

	glGenTextures(1, &texture);
	glBindTexture(target, texture);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap);
	glTexImage1D(target, 0, internal_format, length, 0, external_format, GL_FLOAT, pixels);
	
	if(glGetError() != GL_NO_ERROR) throw "OpenGL error after loading texture";
}

void create_texture_2D(
	GLuint& texture,
	GLenum target,
	GLint filter,
	GLint wrap,
	GLint internal_format,
	GLenum external_format,
	int width, int height,
	const GLvoid* pixels
	) {

	if(target != GL_TEXTURE_2D && target != GL_TEXTURE_RECTANGLE_ARB) throw "invalid texture target";
	if(glGetError() != GL_NO_ERROR) throw "OpenGL error before loading texture";

	glGenTextures(1, &texture);
	glBindTexture(target, texture);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap);
	glTexImage2D(target, 0, internal_format, width, height, 0, external_format, GL_FLOAT, pixels);
	
	if(glGetError() != GL_NO_ERROR) throw "OpenGL error after loading texture";
}

void create_texture_3D(
	GLuint& texture,
	GLint filter,
	GLint wrap,
	GLint internal_format,
	GLenum external_format,
	int width, int height, int length,
	const GLvoid* pixels
	) {

	const GLenum target = GL_TEXTURE_3D;

	if(glGetError() != GL_NO_ERROR) throw "OpenGL error before loading texture";


	glGenTextures(1, &texture);
	glBindTexture(target, texture);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap);
	glTexParameteri(target, GL_TEXTURE_WRAP_R, wrap);
	glTexImage3D(target, 0, internal_format, width, height, length, 0, external_format, GL_FLOAT, pixels);
	
	if(glGetError() != GL_NO_ERROR) throw "OpenGL error before loading texture";
}

void delete_texture(GLuint texture) {
	glDeleteTextures(1, &texture);	//opengl silently ignores if 0 and sets to 0 regardless
}
