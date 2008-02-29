#!/usr/bin/python

"""
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
"""

# this script generates fourier_transform.cu and fourier_transform.h


fourier_transform = {}

class FourierTransform:
	def __init__(self, scaling_prefix, source):
		self.scaling_prefix = scaling_prefix
		self.source = source

#  !!!!! BEGIN: Add new Fourier transforms here
fourier_transform['sph'] = FourierTransform(scaling_prefix=False, source=\
"""
	if(r >= 0.06f) 
	{
		float m = CUDART_PI_F*r;
		float cos_2_m, sin_2_m;
		__sincosf(2.0f*m, &sin_2_m, &cos_2_m);	// this makes a huge difference
		return ((2.3561944901923448f)/(m*m*m*m*m*m))*(cos_2_m-1.0f)*(cos_2_m+m*sin_2_m-1.0f);		// computing m**6 this way is much faster than pow(m,6)
	}
	return (CUDART_PI_F + r*(-0.007968913156311f + r*(-18.293608272337678f)));		// a quadratic interpolant, note that CUDART_PI_F is the value at zero
""")

fourier_transform['wendland_d3_c2'] = FourierTransform(scaling_prefix=True, source=\
"""
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
""")

fourier_transform['gaussian'] = FourierTransform(scaling_prefix=True, source=\
"""
	return __expf(CUDART_PI_F*r*r);
""")

#  !!!!! END: Add new Fourier transforms here





source_file = open('fourier_transform.cu', 'w')
header_file = open('fourier_transform.h', 'w')

warning = \
"""\
//
//Warning: This file was automatically generated by fourier_transform_generate.py
//         Do not edit it!
//
"""

source_file.write(warning + \
"""
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
#include <math_constants.h>

#ifndef CUDART_2PI_F
#define CUDART_2PI_F 6.283185307179586476925286766559f
#endif

__device__ float dot(float3 a, float3 b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

""" \
)

header_file.write(warning + \
"""

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

#ifndef FOURIER_TRANSFORM_H_
#define FOURIER_TRANSFORM_H_

#include "meshless_vis.h"
#include "meshless.h"

""" \
)

header_file.write('void fourier_transform(MeshlessDataset meshless_dataset, VisConfig* vis_config);\n\n')

source_file.write(\
"""
void fourier_transform(MeshlessDataset meshless_dataset, VisConfig* vis_config)
{
	vis_config_compute_scale(vis_config);

	dim3 block_size_fvr(vis_config->_fvr_block_length);
	dim3 cutoff_grid((2*vis_config->cutoff_frequency.x*vis_config->cutoff_frequency.y) / vis_config->_fvr_block_length);	
	for(int i = 0, first_term = 0; i != meshless_dataset.number_of_groups; ++i)
	{
""")
function_names = fourier_transform.keys()
def write_call(function_name, prefix='', suffix=''):
	source_file.write('\t\t'+prefix+'if(strcmp(meshless_dataset.groups[i].name, "' + function_name + '")==0)\n\t\t{\n')
	source_file.write('\t\t\tif(meshless_dataset.d_radii) fourier_transform_'+function_name+'_half_domain<<<cutoff_grid, block_size_fvr, vis_config->_fvr_block_length*sizeof(float)*5>>>(first_term, meshless_dataset.groups[i].number_of_terms, meshless_dataset, *vis_config);\n')
	source_file.write('\t\t\telse fourier_transform_'+function_name+'_half_domain_no_radii<<<cutoff_grid, block_size_fvr, vis_config->_fvr_block_length*sizeof(float)*4>>>(first_term, meshless_dataset.groups[i].number_of_terms, meshless_dataset, *vis_config);\n')
	source_file.write('\t\t}\n')

write_call(function_names[0])
for function_name in fourier_transform.keys()[1:]:
	write_call(function_name, prefix='else ')

source_file.write(\
"""
		first_term += meshless_dataset.groups[i].number_of_terms;
	}
}

""")

# this is the ft_meshless_data evaluation code which is duplicated once for each basis function
for function_name in fourier_transform.keys():

	signature = '__device__ float fourier_transform_' + function_name + '(float r)'
	header_file.write(signature + ';\n')
	source_file.write(signature + '\n{' + fourier_transform[function_name].source + '}\n\n')

	for suffix, has_radii in zip(['', '_no_radii'], [True, False]):
		signature = '__global__ void fourier_transform_' + function_name + '_half_domain' + suffix + '(int first_term, int number_of_terms, MeshlessDataset meshless_dataset, VisConfig vis_config)'
		header_file.write(signature + ';\n')
	
		source_file.write(signature)
		
		source_file.write(\
"""
{
	extern __shared__ float shared[];
	int block_length = vis_config._fvr_block_length;
	Constraint* ds_constraints = (Constraint*)shared;
""")
		if has_radii: source_file.write('\tfloat* ds_radii = (float*)(ds_constraints + block_length);\n')
		source_file.write(\
"""
	// in order to understand how x, index_x, and index are computed we consider a concrete 1D example: Nx = 8, cutoff.x = 3
	// our goal then is to sample over [-2 -1 0 1 2] and store the results in the format [0 1 2 x x -2 -1]
	// for better parallelism we also sample at 3 but do not store the computed sample
	
	int global_index = (blockDim.x*blockIdx.x + threadIdx.x);

	//x will be 0 1 2 3 4 5
	int x = global_index % (2*vis_config.cutoff_frequency.x);
	int y = global_index / (2*vis_config.cutoff_frequency.x);

	// if x <= cutoff_frequency, the index into the array and the actual position are just as given above
	int index_x = x;


	// the higher freqencies [4 5] are the negative frequencies
	// so this requires using periodicity 
	if(x > vis_config.cutoff_frequency.x) 
	{
		// 4 > 3 so,  4-6 = -2,   and 5>3 so 5-6=-1
		//x will be [0 1 2 3 -2 -1]
		x = x-(2*vis_config.cutoff_frequency.x);

		// the negative values then have to go all the way to the right side of the array, since cutoff is less than N
		// this means instead of [0 1 2 3 -2 -1 x x]
		// the indexes should be [0 1 2 3 x x -2 -1]
		index_x = (x+vis_config._number_of_samples.x);
	}
	int index = index_x*(vis_config._number_of_samples.y/2 + 1) + y ;

	// compute the image space coordinates
	float fu = vis_config.step_size.x*x, fv = vis_config.step_size.y*y;
	
	// map from image space into frequency space
	float3 f_coord = make_float3(fu*vis_config.u_axis.x + fv*vis_config.v_axis.x, fu*vis_config.u_axis.y + fv*vis_config.v_axis.y, fu*vis_config.u_axis.z + fv*vis_config.v_axis.z);

	// we don't use the fast sqrt since it's only computed once
	float r = sqrtf((float)(fu*fu + fv*fv));

	// begin the loop
	float2 sum = make_float2(0.0f, 0.0f);
	float sin_v, cos_v;
	Constraint constraint;""")
		if has_radii: source_file.write('\n\tfloat r0;')
		source_file.write(\
"""
	int thread_index = threadIdx.x;
	for(unsigned int k = first_term; k < number_of_terms; ) 
	{
		// step 1: stage global memory into shared memory.  this access is coalesced, and should be minimal since there is one 128-bit read""")
		if has_radii: source_file.write(' and one 32-bit read')
		source_file.write(\
"""
		if(k + thread_index < number_of_terms)
		{
			ds_constraints[thread_index] = meshless_dataset.d_constraints[k + thread_index];
""")

		if has_radii: source_file.write('\t\t\tds_radii[thread_index] = meshless_dataset.d_radii[k + thread_index];\n')
		source_file.write(\
"""		}		
		__syncthreads();
		
		//step 2: for each point, accumulate the terms in shared memory (there are no bank conflicts here, the same data is broadcast to every thread)
		
		for(int j = 0; j != block_length && k < number_of_terms; ++j, ++k)
		{
			constraint = ds_constraints[j];""")

		if has_radii: source_file.write('\n\t\t\tr0 = ds_radii[j];')
		source_file.write('\n\t\t\tfloat term = constraint.weight*')
		if has_radii and fourier_transform[function_name].scaling_prefix: source_file.write('r0*r0*r0*')
		source_file.write('fourier_transform_' + function_name + '(r')		
		if has_radii: source_file.write('*r0')
		source_file.write(""");
			__sincosf(CUDART_2PI_F*dot(f_coord, constraint.position), &sin_v, &cos_v);	// using this __sincos2f function compared to sin and cos makes a huge difference
			sum.x += term*cos_v;
			sum.y += term*sin_v;
		}
		__syncthreads();
	}

	if(x == vis_config.cutoff_frequency.x) return;

	float2 prev = vis_config._d_freq_image[index];
	vis_config._d_freq_image[index] = make_float2(prev.x + sum.x*vis_config._scale, prev.y - sum.y*vis_config._scale);
}			

""")
	header_file.write('\n')
header_file.write('\n#endif /*FOURIER_TRANSFORM_H_*/\n')
