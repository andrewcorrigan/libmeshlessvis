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

#ifndef FRAMEBUFFER_OBJECTh
#define FRAMEBUFFER_OBJECTh

#include <GL/glew.h>
#include <stack>

struct FramebufferObject {
	
	virtual ~FramebufferObject();

	void bind();

	GLuint _framebuffer, _depth_rb;	// keep framebuffer and textures public
	GLint _internal_format;
	GLenum _target, _external_format;
	int _width, _height, _components;
	bool _depth;

	
	struct Texture { GLuint texture; GLenum attachment; };

protected:
	void initialize(int w, int h);
	void attach_depth_buffer();
	Texture create_texture(int index);
	static bool const DEPTH = true, NODEPTH = false;
	void set_format(GLenum target, int components, int bits, bool depth = NODEPTH);
	virtual void format() = 0;
	virtual void textures() = 0;

private:
	std::stack<GLuint> texture_ids;	//only for cleanup
};

#endif
