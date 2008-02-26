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

#ifdef __cplusplus
extern "C"
{
#endif

	
typedef struct 
{
	// the following variables can be changed manually after calling initialize
	float2 step_size;
	int2 cutoff_frequency;
	float3 u_axis;
	float3 v_axis;
	
	bool cull_fully_aliased_terms;
	
	// _-prefix indicates 'private', you can read them, but they should only be modified a vis_config_< do something > function
	bool _automatic_d_image;
	cufftHandle _plan;
	int2 _number_of_samples;
	int _fvr_block_length;
	float _scale;
	float2* _d_image;
	float2* _d_freq_image;
	int _block_length;
	int minimum_number_of_blocks;

} VisConfig;

VisConfig* vis_config_create(bool automatic_d_image, float2 step_size, int2 cutoff_frequency, float3 u_axis, float3 v_axis, int2 number_of_samples, int fvr_block_length, int minimum_number_of_blocks);
VisConfig* vis_config_get_default();
bool vis_config_check(VisConfig* vis_config);
void vis_config_manual_d_image(VisConfig* vis_config, float2* d_image);
void vis_config_change_fvr_block_length(VisConfig* vis_config, int fvr_block_length);
void vis_config_change_number_of_samples(VisConfig* vis_config, bool automatic_d_image, int2 number_of_samples);
void vis_config_clean(VisConfig* vis_config);
void vis_config_destroy(VisConfig* vis_config);
void vis_config_compute_scale(VisConfig* vis_config);
void vis_register_meshless_dataset(MeshlessDataset* meshless_dataset);
void vis_unregister_meshless_dataset(MeshlessDataset* meshless_dataset);
void vis_register_buffer_object(GLuint buffer_object);
void vis_unregister_buffer_object(GLuint buffer_object);
void vis_opengl_fourier_volume_rendering(MeshlessDataset meshless_dataset, VisConfig* vis_config, GLuint registered_buffer_object);
void vis_fourier_volume_rendering(MeshlessDataset meshless_dataset, VisConfig* vis_config);
void vis_copy_to_host(VisConfig* vis_config, float* h_image);

#ifdef __cplusplus
}
#endif

#endif
