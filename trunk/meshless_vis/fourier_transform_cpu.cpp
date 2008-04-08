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

#define _LIBMESHLESSVIS_USE_CPU
#include "fourier_transform_cpu.h"
#include <cmath>
#include <iostream>

#ifndef PI_F
#define PI_F  3.141592653589793238462643383279f
#endif

#ifndef _2PI_F
#define _2PI_F 6.283185307179586476925286766559f
#endif

inline void complex_assign(fftwf_complex& l, const float real, const float imag) { l[0] = real, l[1] = imag; }
inline void complex_accumulate(fftwf_complex& l, const float real, const float imag) { l[0] += real, l[1] += imag; }

inline float fourier_transform_sph(float r)
{
	if(r >= 0.06f) 
	{
		float m = PI_F*r;
		float _2_m = 2.0f*m;
		float cos_2_m = std::cos(_2_m), sin_2_m = std::sin(_2_m);
		return ((2.3561944901923448f)/(m*m*m*m*m*m))*(cos_2_m-1.0f)*(cos_2_m+m*sin_2_m-1.0f);		// computing m**6 this way is much faster than pow(m,6)
	}
	return (PI_F + r*(-0.007968913156311f + r*(-18.293608272337678f)));		// a quadratic interpolant, note that PI_F is the value at zero
}

inline float fourier_transform_gaussian(float r)
{
	return std::exp(PI_F*r*r);
}

inline float fourier_transform_wendland_d3_c2(float r)
{
	if(r >= 0.24f) 
	{
		float m = PI_F*r;
		float _2_m = 2.0f*m;
		float cos_2_m = std::cos(_2_m), sin_2_m = std::sin(_2_m);
		float m_2 = m*m;
		float m_4 = m_2*m_2;
		return ((PI_F*7.5f)/(m_4*m_4))*(4.0f*m_2 - 6.0f + (6.0f-m_2)*cos_2_m+4.5f*m*sin_2_m);
	}
	return 0.299199300341890f + r*(-0.002379178586124f + r*(-0.370530218163545f));
}

inline float dot(float3 a, float3 b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

template <int block_length, bool is_first_group, BasisFunctionId basis_function_id, bool has_radii>
void sample_fourier_transform_over_grid(int d_number_of_terms, float* d_radii, Constraint* d_constraints, VisConfig vis_config)
{
	int index, k, x, y;
	float fu, fv, r, r0, v, term;
	float3 f_coord;
	Constraint constraint;
	float2 sum;
	int image_size = 2*vis_config._cutoff_frequency.x*vis_config._cutoff_frequency.y;

	#pragma omp parallel default(shared) private(index, k, x, y, fu, fv, r, r0, v, term, f_coord, constraint, sum)
	{
		#pragma omp for schedule(dynamic,block_length) nowait
		for(index = 0; index < image_size; index++)
		{
			x = index % (2*vis_config._cutoff_frequency.x);
			if(x > vis_config._cutoff_frequency.x) x = x-(2*vis_config._cutoff_frequency.x);
			y = (index % image_size) / (2*vis_config._cutoff_frequency.x);
	
			// compute the image space coordinates
			fu = vis_config.step_size.x*x, fv = vis_config.step_size.y*y;
			
			// map from image space into frequency space
			f_coord = make_float3(fu*vis_config.u_axis.x + fv*vis_config.v_axis.x, fu*vis_config.u_axis.y + fv*vis_config.v_axis.y, fu*vis_config.u_axis.z + fv*vis_config.v_axis.z);
		
			// we don't use the fast sqrt since it's only computed once
			r = sqrtf((float)(fu*fu + fv*fv));
		
			// begin the loop
			sum = make_float2(0.0f, 0.0f);
			for(k = 0; k < d_number_of_terms; k++) 
			{
				constraint = d_constraints[k];
				if(has_radii)
				{
					r0 = d_radii[k];
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
				v = _2PI_F*dot(f_coord, constraint.position);
				sum.x += constraint.weight*term*std::cos(v);
				sum.y += constraint.weight*term*std::sin(v);
			}			
			
			if(is_first_group)
			{
				 complex_assign(vis_config._d_freq_image[index], sum.x*vis_config._scale, -sum.y*vis_config._scale);
			}
			else
			{
				complex_accumulate(vis_config._d_freq_image[index], sum.x*vis_config._scale, -sum.y*vis_config._scale);
			}
		}
	}
}

template <int block_length, bool is_first_group, BasisFunctionId basis_function_id>
void fourier_transform_level_2(Group* group, VisConfig* vis_config)
{
	if(group->d_radii) sample_fourier_transform_over_grid <block_length, is_first_group, basis_function_id, true>  (group->d_number_of_terms, group->d_radii, group->d_constraints, *vis_config);
	else               sample_fourier_transform_over_grid <block_length, is_first_group, basis_function_id, false> (group->d_number_of_terms, group->d_radii, group->d_constraints, *vis_config);
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
