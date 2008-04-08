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
#include "meshless_vis.h"

#ifdef WIN32
#include <omp.h>	//necessary for me to compile using visual studio
#endif

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <GL/glew.h>
#include <fftw3.h>

#include "fourier_transform.h"

inline void complex_assign(fftwf_complex& l, const fftwf_complex& r) { l[0] = r[0], l[1] = r[1]; }
inline void complex_accumulate(fftwf_complex& l, const fftwf_complex& r) { l[0] += r[0], l[1] += r[1]; }

VisConfig* vis_config_create(bool automatic_d_image, float2 step_size, int2 cutoff_frequency, float3 u_axis, float3 v_axis, int2 number_of_samples, int block_length, int number_of_partial_sums)
{
	VisConfig* vis_config = new VisConfig;
	vis_config->step_size = step_size;
	vis_config->u_axis = u_axis;
	vis_config->v_axis = v_axis;
	vis_config->cull_fully_aliased_terms = false;
	vis_config->_number_of_samples = number_of_samples;
	vis_config->_cutoff_frequency = cutoff_frequency;
	vis_config->block_length = block_length;
	vis_config->_number_of_partial_sums = 1; // until there are on the order of 128 cores in CPUs this optimization is pointless

	vis_config->_d_freq_image = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex)*2*vis_config->_cutoff_frequency.x*vis_config->_cutoff_frequency.y*vis_config->_number_of_partial_sums);
	vis_config->_d_freq_image_arranged = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex)*vis_config->_number_of_samples.x*(vis_config->_number_of_samples.y/2+1));

	vis_config->_automatic_d_image = true; // the CPU code only works this way
	if(vis_config->_automatic_d_image) vis_config->_d_image = (float*)fftwf_malloc(sizeof(float)*vis_config->_number_of_samples.x*vis_config->_number_of_samples.y);

	char* str_num_threads = getenv ("OMP_NUM_THREADS");
	int num_threads = 1;
	if(str_num_threads != 0)
	{
		std::stringstream(std::string(str_num_threads)) >> num_threads;
		std::cout << "Using " << num_threads << " threads" << std::endl;
	}
	fftwf_init_threads();
	fftwf_plan_with_nthreads(num_threads);
	vis_config->_plan = fftwf_plan_dft_c2r_2d(vis_config->_number_of_samples.x, vis_config->_number_of_samples.y, vis_config->_d_freq_image_arranged, vis_config->_d_image, FFTW_ESTIMATE);
	
	return vis_config;
}

VisConfig* vis_config_get_default()
{
	float2 step_size = make_float2(0.1f, 0.1f);
	int2 cutoff_frequency = make_int2(64,64);
	float3 u_axis = make_float3(1.0f, 0.0f, 0.0f);
	float3 v_axis = make_float3(0.0f, 1.0f, 0.0f);
	int2 number_of_samples = make_int2(512,512);
	int block_length = 256;
	int number_of_partial_sums = 1;
	VisConfig* vis_config = vis_config_create(true, step_size, cutoff_frequency, u_axis, v_axis, number_of_samples, block_length, number_of_partial_sums);
	return vis_config;
}

bool vis_config_check(VisConfig* vis_config)
{

	if(2*vis_config->_cutoff_frequency.x > vis_config->_number_of_samples.x) return false;
	if(2*vis_config->_cutoff_frequency.y > vis_config->_number_of_samples.y) return false;
	if(vis_config->_number_of_partial_sums != 1) return false;

	return true;
}

void vis_config_manual_d_image(VisConfig* vis_config, float* d_image)
{
	vis_config->_d_image = d_image;
}

void vis_config_change_number_of_samples(VisConfig* vis_config, int2 number_of_samples)
{
	vis_config->_number_of_samples = number_of_samples;

	if(vis_config->_automatic_d_image)
	{
		fftwf_free(vis_config->_d_image);
		vis_config->_d_image = (float*)fftwf_malloc(sizeof(float)*vis_config->_number_of_samples.x*vis_config->_number_of_samples.y);
	}
	
	fftwf_free(vis_config->_d_freq_image_arranged);
	vis_config->_d_freq_image_arranged = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex)*vis_config->_number_of_samples.x*(vis_config->_number_of_samples.y/2+1));
	
	fftwf_destroy_plan(vis_config->_plan);
	vis_config->_plan = fftwf_plan_dft_c2r_2d(vis_config->_number_of_samples.x, vis_config->_number_of_samples.y, vis_config->_d_freq_image_arranged, vis_config->_d_image, FFTW_ESTIMATE);
}

void vis_config_change_cutoff_frequency(VisConfig* vis_config, int2 cutoff_frequency)
{
	vis_config->_cutoff_frequency = cutoff_frequency;
	fftwf_free(vis_config->_d_freq_image);
	vis_config->_d_freq_image = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex)*2*vis_config->_cutoff_frequency.x*vis_config->_cutoff_frequency.y*vis_config->_number_of_partial_sums);
}

void vis_config_change_number_of_partial_sums(VisConfig* vis_config, int number_of_partial_sums)
{
	vis_config->_number_of_partial_sums = number_of_partial_sums;

	fftwf_free(vis_config->_d_freq_image);
	vis_config->_d_freq_image = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex)*2*vis_config->_cutoff_frequency.x*vis_config->_cutoff_frequency.y*vis_config->_number_of_partial_sums);
}

void vis_config_destroy(VisConfig* vis_config)
{
	if(vis_config->_automatic_d_image) fftwf_free(vis_config->_d_image);
	fftwf_free(vis_config->_d_freq_image_arranged);
	fftwf_free(vis_config->_d_freq_image);
	fftwf_destroy_plan(vis_config->_plan);
	fftwf_cleanup_threads();
	delete vis_config;
}

void vis_config_compute_scale(VisConfig* vis_config)
{
	// scale the unnormalized FFT by our discrete step size
	vis_config->_scale = vis_config->step_size.x * vis_config->step_size.y;
}

template <typename T>
T* load_into_device(T* h_data, int N, int integer_multiple_of, int& rounded_N)
{
	T* d_data;
	if(h_data == 0)
	{
		d_data = 0;
	}
	else
	{
		int k = (N / integer_multiple_of) + std::min(1, N%integer_multiple_of);
		rounded_N = integer_multiple_of*k;
		d_data = new T[rounded_N];
		memset((void*)d_data, 0, sizeof(T)*rounded_N);
		memcpy(d_data, h_data, sizeof(T)*N);
	}
	return d_data;
}

void vis_register_meshless_dataset(VisConfig* vis_config, MeshlessDataset* meshless_dataset)
{
	for(int j = 0; j != meshless_dataset->number_of_groups; j++)
	{
		meshless_dataset->groups[j].d_constraints = load_into_device(meshless_dataset->groups[j].h_constraints, meshless_dataset->groups[j].number_of_terms, vis_config->_number_of_partial_sums*vis_config->block_length, meshless_dataset->groups[j].d_number_of_terms);
		meshless_dataset->groups[j].d_radii = load_into_device(meshless_dataset->groups[j].h_radii, meshless_dataset->groups[j].number_of_terms, vis_config->_number_of_partial_sums*vis_config->block_length, meshless_dataset->groups[j].d_number_of_terms);
	}
}

void vis_unregister_meshless_dataset(VisConfig* vis_config, MeshlessDataset* meshless_dataset)
{
	for(int j = 0; j != meshless_dataset->number_of_groups; j++)
	{
		free(meshless_dataset->groups[j].d_constraints);
		free(meshless_dataset->groups[j].d_radii);
	}
}


void reduce_partial_sums(VisConfig vis_config)
{
	// this optimization is pointless for the CPU until there are something like 128 cores on a CPU

	#if 0
	int index, size, i;
	fftwf_complex partial_sum;
	int chunk = vis_config.block_length;
	size = 2*vis_config._cutoff_frequency.x*vis_config._cutoff_frequency.y;

	#pragma omp parallel default(shared) private(index, partial_sum)
	{
		#pragma omp for schedule(dynamic,chunk) nowait
		for(index = 0; index < size; index++)
		{
			complex_assign(partial_sum, vis_config._d_freq_image[index]);
			for(i = 1; i < vis_config._number_of_partial_sums; i++)
			{
				complex_accumulate(partial_sum, vis_config._d_freq_image[index+i*size]);
			}
			complex_assign(vis_config._d_freq_image[index], partial_sum);
		}
	}
	#endif
}

void arrange_samples(VisConfig vis_config)
{
	int index, size, x, y, index_x;
	int chunk = vis_config.block_length;
	size = 2*vis_config._cutoff_frequency.x*vis_config._cutoff_frequency.y;

	#pragma omp parallel default(shared) private(index, x, y, index_x)
	{
		#pragma omp for schedule(dynamic,chunk) nowait
		for(index = 0; index < size; index++)
		{
			x = index % (2*vis_config._cutoff_frequency.x);
			y = index / (2*vis_config._cutoff_frequency.x);
			index_x = x;
			if(x > vis_config._cutoff_frequency.x) 
			{
				x = x-(2*vis_config._cutoff_frequency.x);
				index_x = (x+vis_config._number_of_samples.x);
			}
			if(x != vis_config._cutoff_frequency.x)
			{
				complex_assign(vis_config._d_freq_image_arranged[index_x*(vis_config._number_of_samples.y/2+1)+y], vis_config._d_freq_image[index]);
			}
		}
	}
}

void vis_opengl_fourier_volume_rendering(MeshlessDataset* meshless_dataset, VisConfig* vis_config, GLuint registered_buffer_object)
{
	vis_fourier_volume_rendering(meshless_dataset, vis_config);

	// now copy the CPU-side image to the buffer object
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, registered_buffer_object);
	float* mapped_buffer_object = (float*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	for(int i = 0; i < vis_config->_number_of_samples.x*vis_config->_number_of_samples.y; i++)
	{
		mapped_buffer_object[i] = vis_config->_d_image[i];
	}
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
}

void vis_fourier_volume_rendering(MeshlessDataset* meshless_dataset, VisConfig* vis_config)
{
	memset((void*)vis_config->_d_freq_image_arranged, 0, sizeof(float2)*vis_config->_number_of_samples.x*(vis_config->_number_of_samples.y/2+1));
	fourier_transform(meshless_dataset, vis_config);

	if(vis_config->_number_of_partial_sums > 1)
	{
		reduce_partial_sums(*vis_config);
	}
	arrange_samples(*vis_config);

	fftwf_execute(vis_config->_plan);
}

void vis_copy_to_host(VisConfig* vis_config, float* h_image)
{
	memcpy(h_image, vis_config->_d_image, sizeof(float)*vis_config->_number_of_samples.x*vis_config->_number_of_samples.y);
}
