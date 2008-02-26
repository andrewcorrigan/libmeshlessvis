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

#ifndef TEXTUREh
#define TEXTUREh

#include <GL/glew.h>

void create_texture_1D(
	GLuint& texture,
	GLint filter,
	GLint wrap,
	GLint internal_format,
	GLenum external_format,
	int length,
	const GLvoid* pixels
	);

void create_texture_2D (
	GLuint& texture,
	GLenum target,
	GLint filter,
	GLint wrap,
	GLint internal_format,
	GLenum external_format,
	int width, int height,
	const GLvoid* pixels
	);

void create_texture_3D(
	GLuint& texture,
	GLint filter,
	GLint wrap,
	GLint internal_format,
	GLenum external_format,
	int width, int height, int length,
	const GLvoid* pixels
	);


void delete_texture(GLuint texture);

#endif
