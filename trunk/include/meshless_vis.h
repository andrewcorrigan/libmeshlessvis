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

#ifndef MESHLESS_VISh
#define MESHLESS_VISh

#include "meshless.h"
#include <vector_functions.h>
#include <builtin_types.h>
#include <cufft.h>
#include <GL/glew.h>

#ifdef _LIBMESHLESSVIS_USE_CPU
#include <fftw3.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

	
typedef struct 
{
	// variables prefixed with an underscore are private and should only be modified a vis_config_< do something > function
	
	float2 step_size;
	int2 _cutoff_frequency;
	float3 u_axis;
	float3 v_axis;
	
	bool cull_fully_aliased_terms;
	
	
	bool _automatic_d_image;

#ifdef _LIBMESHLESSVIS_USE_CPU
	fftwf_plan _plan;
#else
	cufftHandle _plan;
#endif

	int2 _number_of_samples;
	int block_length;
	float _scale;
	float* _d_image;

#ifdef _LIBMESHLESSVIS_USE_CPU
	fftwf_complex* _d_freq_image;
#else
	float2* _d_freq_image;
#endif

	int _number_of_partial_sums;

#ifdef _LIBMESHLESSVIS_USE_CPU
	fftwf_complex* _d_freq_image_arranged;
#else
	float2* _d_freq_image_arranged;
#endif

	
} VisConfig;

VisConfig* vis_config_create(bool automatic_d_image, float2 step_size, int2 cutoff_frequency, float3 u_axis, float3 v_axis, int2 number_of_samples, int block_length, int number_of_partial_sums);
VisConfig* vis_config_get_default();
bool vis_config_check(VisConfig* vis_config);
void vis_config_manual_d_image(VisConfig* vis_config, float2* d_image);
void vis_config_change_number_of_samples(VisConfig* vis_config, int2 number_of_samples);
void vis_config_change_cutoff_frequency(VisConfig* vis_config, int2 cutoff_frequency);
void vis_config_change_number_of_partial_sums(VisConfig* vis_config, int number_of_partial_sums);
void vis_config_destroy(VisConfig* vis_config);
void vis_config_compute_scale(VisConfig* vis_config);
void vis_register_meshless_dataset(VisConfig* vis_config, MeshlessDataset* meshless_dataset);
void vis_unregister_meshless_dataset(VisConfig* vis_config, MeshlessDataset* meshless_dataset);
void vis_opengl_fourier_volume_rendering(MeshlessDataset* meshless_dataset, VisConfig* vis_config, GLuint buffer_object);
void vis_fourier_volume_rendering(MeshlessDataset* meshless_dataset, VisConfig* vis_config);
void vis_copy_to_host(VisConfig* vis_config, float* h_image);

#ifdef __cplusplus
}
#endif

#endif
