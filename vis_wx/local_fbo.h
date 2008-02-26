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

#ifndef LOCAL_FBOh
#define LOCAL_FBOh

#include "gpu/framebuffer_object.h"

struct FBO_Projection : public FramebufferObject {
	FBO_Projection(int N1, int N2) { initialize(N1, N2); }
	Texture tex;
protected:
	void format() { FramebufferObject::set_format(GL_TEXTURE_2D, 4, 8); }
	void textures() { tex = create_texture(0); }
};

#endif
