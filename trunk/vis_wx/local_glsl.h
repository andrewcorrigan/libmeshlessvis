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

#ifndef LOCAL_GLSLh
#define LOCAL_GLSLh

#include "gpu/glsl_program.h"
#include "gpu/texture.h"
#include <sstream>
#include <string>

const std::string color_mapping_code =
"#if COLOR == 1\n"
"\n"
"	const int N = 5;\n"
"	float intensities[N] = {0.0, 0.475, 0.694, 0.745, 1.0};\n"
"	vec3 colors[N] = {vec3(0.0,0.0,0.0), vec3(0.686, 0.004, 0.0), vec3(1.0,0.42,0.0), vec3(1.0,0.518,0.0), vec3(1.0,1.0,1.0)};\n"
"\n"
"#elif COLOR == 2\n"
"\n"
"	const int N = 5;\n"
"	float intensities[N] = {0.0, 0.25,0.5,0.75,1.0};\n"
"	vec3 colors[N] = {vec3(0,0,1), vec3(0,1,1), vec3(0,1,0), vec3(1,1,0), vec3(1,0,0)};\n"
"\n"
"#elif  COLOR == 3\n"
"\n"
"	const int N = 5;\n"
"	float intensities[N] = {0.0, 0.475, 0.694, 0.745, 1.0};\n"
"	const vec3 colors[N] = {	vec3(0), vec3(0.0,0.004,0.686), vec3(0.0,0.42,1.0), vec3(0.0,0.518,1.0), vec3(1,1,1) };\n"
"\n"
"#endif\n"
"\n"
"vec3 intensity_to_color(float intensity)\n"
"{\n"
"	vec3 color = colors[N-1];\n"
"	for(int i = 1; i != N; ++i)\n"
"	{\n"
"		if(intensity <= intensities[i]) {\n"
"			color = ((intensity-intensities[i-1])*colors[i] + (intensities[i] - intensity)*colors[i-1]) / (intensities[i] - intensities[i-1]);\n"
"			break;\n"
"		}\n"
"	}\n"
"	return color;\n"
"}\n"
"\n\n";


const std::string display_image_code =
"#define LOG_SCALE\n"
"\n"
"uniform sampler2D image;\n"
"\n"
"uniform float intensity_min;\n"
"uniform float intensity_max;\n"
"\n"
"vec3 intensity_to_color(float intensity);\n"
"\n"
"void main() {\n"
"	float a = texture2D(image, gl_TexCoord[0].st).x;\n"
"\n"
"	a = max(0.0f, (a - intensity_min) / (intensity_max - intensity_min));\n"
"\n"
"#ifdef LOG_SCALE\n"
"	float eps = 1e-3;\n"
"	float alpha = 0.01f;	\n"
"	a = (-log(eps) + log(alpha*a+eps)) / (log(alpha+eps)-log(eps));\n"
"#endif\n"
"\n"
"	gl_FragColor = vec4(intensity_to_color(a), 1.0);\n"
"}\n"
"\n\n";

struct GLSL_DisplayImage : public GLSL_Program {
	GLSL_DisplayImage(int cm) : color_mapping(cm) { initialize(); }
	GLint image, intensity_max, intensity_min;
	int color_mapping;
protected:
	void attach_shaders() { 
		std::ostringstream oss; oss << color_mapping;
		attach_fragment_shader_from_source(std::string("#define COLOR ") + oss.str() + "\n" + display_image_code + color_mapping_code);
	}
	void set_parameters() {
		image = locate_uniform("image");
		intensity_max = locate_uniform("intensity_max");
		intensity_min = locate_uniform("intensity_min");
	}
};

const std::string color_bar_code = 
"vec3 intensity_to_color(float intensity);\n"
""
"void main() {\n"
"	gl_FragColor = vec4(intensity_to_color(gl_TexCoord[0].s), 1);\n"
"}\n"
"\n\n";

struct GLSL_ColorBar : public GLSL_Program {
	GLSL_ColorBar(int cm) : color_mapping(cm) { initialize(); }
	int color_mapping;
protected:
	void attach_shaders() { 
		std::ostringstream oss; oss << color_mapping;
		attach_fragment_shader_from_source(std::string("#define COLOR ") + oss.str() + "\n" + color_bar_code + color_mapping_code);
	}
	void set_parameters() {}
};

#endif
