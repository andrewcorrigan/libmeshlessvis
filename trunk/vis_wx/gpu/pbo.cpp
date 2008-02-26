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

#include "pbo.h"
#include <cstdlib>

void create_pbo(GLuint& pbo, GLenum target, GLenum usage, unsigned int size)
{
	glGenBuffers(1, &pbo);
	glBindBuffer(target, pbo);
	void* data = malloc(size);
	glBufferData(target, size, data, usage);
	std::free(data);
	glBindBuffer(target, 0);
}

void delete_pbo(GLuint& pbo)
{
	glDeleteBuffers(1, &pbo);		//opengl silently ignores if 0 and sets to 0 regardless
}
