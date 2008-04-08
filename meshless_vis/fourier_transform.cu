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

#include "fourier_transform.h"
#include <string.h>
#include <math_functions.h>
#include <math_constants.h>

#ifndef CUDART_2PI_F
#define CUDART_2PI_F 6.283185307179586476925286766559f
#endif

inline __device__ float fourier_transform_sph(float r)
{
	if(r >= 0.06f) 
	{
		float m = CUDART_PI_F*r;
		float cos_2_m, sin_2_m;
		__sincosf(2.0f*m, &sin_2_m, &cos_2_m);	// this makes a huge difference
		return ((2.3561944901923448f)/(m*m*m*m*m*m))*(cos_2_m-1.0f)*(cos_2_m+m*sin_2_m-1.0f);		// computing m**6 this way is much faster than pow(m,6)
	}
	return (CUDART_PI_F + r*(-0.007968913156311f + r*(-18.293608272337678f)));		// a quadratic interpolant, note that CUDART_PI_F is the value at zero
}

inline __device__ float fourier_transform_gaussian(float r)
{
	return __expf(CUDART_PI_F*r*r);
}

inline __device__ float fourier_transform_wendland_d3_c2(float r)
{
	if(r >= 0.24f) 
	{
		float m = CUDART_PI_F*r;
		float cos_2_m, sin_2_m;
		__sincosf(2.0f*m, &sin_2_m, &cos_2_m);	// this makes a huge difference
		float m_2 = m*m;
		float m_4 = m_2*m_2;
		return ((CUDART_PI_F*7.5f)/(m_4*m_4))*(4.0f*m_2 - 6.0f + (6.0f-m_2)*cos_2_m+4.5f*m*sin_2_m);
	}
	return 0.299199300341890f + r*(-0.002379178586124f + r*(-0.370530218163545f));
}

inline __device__ float dot(float3 a, float3 b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

template <int block_length, bool is_first_group, BasisFunctionId basis_function_id, bool has_radii>
__global__ void sample_fourier_transform_over_grid(/*int d_number_of_terms, float* d_radii, Constraint* d_constraints,*/Group group, VisConfig vis_config)
{
	extern __shared__ float shared[];
	Constraint* ds_constraints = (Constraint*)shared;
	float* ds_radii;
	if(has_radii) ds_radii = (float*)(ds_constraints + block_length);

	int image_size = 2*vis_config._cutoff_frequency.x*vis_config._cutoff_frequency.y;
	int index = (block_length*blockIdx.x + threadIdx.x);
	int x = index % (2*vis_config._cutoff_frequency.x);
	if(x > vis_config._cutoff_frequency.x) x = x-(2*vis_config._cutoff_frequency.x);
	int y = (index % image_size) / (2*vis_config._cutoff_frequency.x);
	int partial_sum_index = index / image_size;
	int number_of_terms_per_partial_sum = group.d_number_of_terms / vis_config._number_of_partial_sums;
	int first_term = partial_sum_index*number_of_terms_per_partial_sum;
	int last_term = first_term + number_of_terms_per_partial_sum;

	// compute the image space coordinates
	float fu = vis_config.step_size.x*x, fv = vis_config.step_size.y*y;
	
	// map from image space into frequency space
	float3 f_coord = make_float3(fu*vis_config.u_axis.x + fv*vis_config.v_axis.x, fu*vis_config.u_axis.y + fv*vis_config.v_axis.y, fu*vis_config.u_axis.z + fv*vis_config.v_axis.z);

	// we don't use the fast sqrt since it's only computed once
	float r = sqrtf((float)(fu*fu + fv*fv));

	// begin the loop
	float2 sum = make_float2(0.0f, 0.0f);
	float sin_v, cos_v;
	Constraint constraint;
	float r0;
	int thread_index = threadIdx.x;
	for(unsigned int k = first_term; k < last_term; k += block_length) 
	{
		// step 1: stage global memory into shared memory.  this access is coalesced, and should be minimal since there is one 128-bit read and one 32-bit read
		ds_constraints[thread_index] = group.d_constraints[k + thread_index];
		if(has_radii) ds_radii[thread_index] = group.d_radii[k + thread_index];
		__syncthreads();
		
		//step 2: for each point, accumulate the terms in shared memory (there are no bank conflicts here, the same data is broadcast to every thread)		
		for(unsigned int j = 0; j != block_length; j++)
		{
			constraint = ds_constraints[j];
			float term;
			if(has_radii)
			{
				r0 = ds_radii[j];
				if     (basis_function_id == SPH)            term = fourier_transform_sph(r*r0);
				else if(basis_function_id == GAUSSIAN)       term = fourier_transform_gaussian(r*r0);
				else if(basis_function_id == WENDLAND_D3_C2) term = fourier_transform_wendland_d3_c2(r*r0);
			}
			else
			{
				if     (basis_function_id == SPH)            term = fourier_transform_sph(r);
				else if(basis_function_id == GAUSSIAN)       term = fourier_transform_gaussian(r);
				else if(basis_function_id == WENDLAND_D3_C2) term = fourier_transform_wendland_d3_c2(r);
			}
			__sincosf(CUDART_2PI_F*dot(f_coord, constraint.position), &sin_v, &cos_v);	// using this __sincos2f function compared to sin and cos makes a huge difference
			sum.x += constraint.weight*term*cos_v;
			sum.y += constraint.weight*term*sin_v;
		}
		__syncthreads();
	}

	if(is_first_group)
	{
		vis_config._d_freq_image[index] = make_float2(sum.x*vis_config._scale, -sum.y*vis_config._scale);
	}
	else
	{
		float2 prev = vis_config._d_freq_image[index];
		vis_config._d_freq_image[index] = make_float2(prev.x + sum.x*vis_config._scale, prev.y - sum.y*vis_config._scale);
	}
}

template <int block_length, bool is_first_group, BasisFunctionId basis_function_id>
void fourier_transform_level_2(Group* group, VisConfig* vis_config)
{
	dim3 block_size(vis_config->block_length);
	dim3 cutoff_grid((2*vis_config->_cutoff_frequency.x*vis_config->_cutoff_frequency.y*vis_config->_number_of_partial_sums) / vis_config->block_length);	

	if(group->d_radii) sample_fourier_transform_over_grid <block_length, is_first_group, basis_function_id, true>  <<<cutoff_grid, block_size, vis_config->block_length*sizeof(float)*5>>> (*group, *vis_config);
	else               sample_fourier_transform_over_grid <block_length, is_first_group, basis_function_id, false> <<<cutoff_grid, block_size, vis_config->block_length*sizeof(float)*4>>> (*group, *vis_config);
}

template <int block_length, bool is_first_group>
void fourier_transform_level_1(Group* group, VisConfig* vis_config)
{
	if     (group->basis_function_id == SPH)            fourier_transform_level_2 <block_length, is_first_group, SPH>            (group, vis_config);
	else if(group->basis_function_id == GAUSSIAN)       fourier_transform_level_2 <block_length, is_first_group, GAUSSIAN>       (group, vis_config);
	else if(group->basis_function_id == WENDLAND_D3_C2) fourier_transform_level_2 <block_length, is_first_group, WENDLAND_D3_C2> (group, vis_config);
}

void fourier_transform(MeshlessDataset* meshless_dataset, VisConfig* vis_config)
{
	if (meshless_dataset->number_of_groups < 1) return;

	vis_config_compute_scale(vis_config);

	if     (vis_config->block_length == 512) fourier_transform_level_1 <512, true> (meshless_dataset->groups, vis_config);
	else if(vis_config->block_length == 256) fourier_transform_level_1 <256, true> (meshless_dataset->groups, vis_config);
	else if(vis_config->block_length == 128) fourier_transform_level_1 <128, true> (meshless_dataset->groups, vis_config);
	else if(vis_config->block_length ==  64) fourier_transform_level_1 < 64, true> (meshless_dataset->groups, vis_config);
	else if(vis_config->block_length ==  32) fourier_transform_level_1 < 32, true> (meshless_dataset->groups, vis_config);

	for(int i = 1; i < meshless_dataset->number_of_groups; i++)
	{
		if     (vis_config->block_length == 512) fourier_transform_level_1 <512, false> (meshless_dataset->groups+i, vis_config);
		else if(vis_config->block_length == 256) fourier_transform_level_1 <256, false> (meshless_dataset->groups+i, vis_config);
		else if(vis_config->block_length == 128) fourier_transform_level_1 <128, false> (meshless_dataset->groups+i, vis_config);
		else if(vis_config->block_length ==  64) fourier_transform_level_1 < 64, false> (meshless_dataset->groups+i, vis_config);
		else if(vis_config->block_length ==  32) fourier_transform_level_1 < 32, false> (meshless_dataset->groups+i, vis_config);
	}
}
