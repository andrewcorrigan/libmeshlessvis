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

#include "framebuffer_object.h"
#include "texture.h"
#include <sstream>

void FramebufferObject::initialize(int width, int height) {
	_width = width, _height = height;
	glGenFramebuffersEXT(1, &_framebuffer);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _framebuffer);
	format();
	if(_depth) attach_depth_buffer();
	textures();
	if(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_UNSUPPORTED_EXT) throw "FBO unsupported";
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);	//unbind this fbo
}

FramebufferObject::~FramebufferObject() {
	while(!texture_ids.empty()) { delete_texture(texture_ids.top()); texture_ids.pop();}	
	glDeleteFramebuffersEXT(1, &_framebuffer);
}

void FramebufferObject::attach_depth_buffer() {
	glGenRenderbuffersEXT(1, &_depth_rb);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, _depth_rb);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, _width, _height);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, _depth_rb);
}

void FramebufferObject::bind() {
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _framebuffer);
}

FramebufferObject::Texture FramebufferObject::create_texture(int index) {
	if(glGetError() != GL_NO_ERROR) throw "OpenGL error before creating FBO texture";
	Texture t;
	t.attachment = GL_COLOR_ATTACHMENT0_EXT + index;
	create_texture_2D(t.texture, _target, GL_NEAREST, GL_CLAMP, _internal_format, _external_format, _width, _height, 0);
	texture_ids.push(t.texture);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, t.attachment, _target, t.texture, 0);
	if(glGetError() != GL_NO_ERROR) throw "OpenGL error creating FBO texture";
	return t;
}

void FramebufferObject::set_format(GLenum target, int components, int bits, bool depth) {
	
	_target = target;
	_depth = depth;
	_components = components;

	try {
		if(bits == 8) {
				_internal_format = GL_RGBA;
				_external_format = GL_RGBA;
		}
		else if(bits == 16) {
			switch(components) {
			case 2:
				_internal_format =  GL_LUMINANCE_ALPHA16F_ARB;
				_external_format = GL_LUMINANCE_ALPHA;
				break;
			case 3:
				_internal_format =  GL_RGB16F_ARB;
				_external_format = GL_RGB;
				break;
			case 4:
				_internal_format =  GL_RGBA16F_ARB;
				_external_format = GL_RGBA;
				break;
			default:
				throw;
			}
		}
		else if (bits == 32) {
			switch(components) {
			case 1:
				_internal_format = GL_LUMINANCE32F_ARB;
				_external_format = GL_LUMINANCE;
				break;
			case 2:
				_internal_format =  GL_LUMINANCE_ALPHA32F_ARB;
				_external_format = GL_LUMINANCE_ALPHA;
				break;
			case 3:
				_internal_format =  GL_RGB32F_ARB;
				_external_format = GL_RGB;
				break;
			case 4:
				_internal_format =  GL_RGBA32F_ARB;
				_external_format = GL_RGBA;
				break;
			default:
				throw;
			}
		}
		else {
			throw;
		}
	}
	catch (...) {
		std::stringstream ss; ss << "invalid texture: " << components << "  " << bits << std::endl;
		throw ss.str().c_str();
	}

}
