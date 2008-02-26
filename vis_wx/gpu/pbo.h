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

#ifndef PBO_H_
#define PBO_H_

#include <GL/glew.h>

void create_pbo(GLuint& pbo, GLenum target, GLenum usage, unsigned int size);
//typically:
//target = GL_PIXEL_UNPACK_BUFFER
//usage = GL_DYNAMIC_COPY
//if you want a pixel buffer to store N GLfloats then size = N*sizeof(GLfloat)

void delete_pbo(GLuint& pbo);


#endif /*PBO_H_*/
