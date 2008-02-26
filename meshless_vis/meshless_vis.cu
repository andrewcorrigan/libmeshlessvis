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

#include "meshless_vis.h"

#include <GL/glew.h>
#include <cufft.h>
#include <cutil.h>
#include <cuda_gl_interop.h>
#include <stdio.h>
#include <math.h>

#include "fourier_transform.h"



__device__ float dot(float3 a, float3 b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

__device__ bool point_in_box(float2 point, float4 bounding_box)
{
	return (point.x >= bounding_box.x && point.y >= bounding_box.y && point.x <= bounding_box.z && point.y <= bounding_box.w);
}

__global__ void mask_fully_aliased_terms(int number_of_terms, MeshlessDataset meshless_dataset, bool* mask, float4 bounding_box, float3 u_axis, float3 v_axis)
{
	int index = (blockDim.x*blockIdx.x + threadIdx.x);
	if(index > number_of_terms) return;
	
	float3 center = meshless_dataset.d_constraints[index].position;
	float radius = meshless_dataset.d_radii[index];
	
	float2 corner1 = make_float2(dot(center, u_axis)+radius,dot(center, v_axis)+radius);
	float2 corner2 = make_float2(corner1.x, corner1.y-2*radius);
	float2 corner3 = make_float2(corner1.x-2*radius, corner1.y);
	float2 corner4 = make_float2(corner3.x, corner2.y);

	mask[index] = (point_in_box(corner1, bounding_box) && point_in_box(corner2, bounding_box) && point_in_box(corner3, bounding_box) && point_in_box(corner4, bounding_box));
}


void cull_fully_aliased_points(MeshlessDataset* meshless_dataset, VisConfig* vis_config)
{
	if(vis_config->cull_fully_aliased_terms)
	{
		dim3 block_size(vis_config->_block_length);
		int number_of_terms = get_number_of_terms(meshless_dataset);
		dim3 grid((number_of_terms/block_size.x)+(number_of_terms%block_size.x > 0)?1:0);
		//float px = 1.0f/vis_config->step_size.x, py = 1.0f/vis_config->step_size.y;
		float4 bounding_box = make_float4(0.0f, 0.0f, 1.0f/vis_config->step_size.x, 1.0f/vis_config->step_size.y);
		bool* mask;
	
		CUDA_SAFE_CALL(cudaMalloc((void**)&mask, sizeof(bool)*number_of_terms));
				
		mask_fully_aliased_terms<<<grid,block_size>>>(number_of_terms, *meshless_dataset, mask, bounding_box, vis_config->u_axis, vis_config->v_axis);

		/*
		Constraint* culled_constraints;
		float* culled_radii;
		unsigned int number_not_culled;
		CUDPPScanConfig* config;
		cudppInitializeScan(config);
		cudppCompact((void*)culled_constraints, mask, meshless_data->d_constraints, number_of_terms, config, &number_not_culled);
		cudppCompact((void*)culled_radii, mask, meshless_data->d_radii, number_of_terms, config, &number_not_culled);
		cudppFinalizeScan(config);
		*/
		
		CUDA_SAFE_CALL(cudaFree(mask));
	}
}

VisConfig* vis_config_create(bool automatic_d_image, float2 step_size, int2 cutoff_frequency, float3 u_axis, float3 v_axis, int2 number_of_samples, int fvr_block_length, int minimum_number_of_blocks)
{
	VisConfig* vis_config = (VisConfig*)malloc(sizeof(VisConfig));
	vis_config->step_size = step_size;
	vis_config->u_axis = u_axis;
	vis_config->v_axis = v_axis;
	vis_config->cutoff_frequency = cutoff_frequency;
	vis_config->cull_fully_aliased_terms = false;
	vis_config_change_fvr_block_length(vis_config, fvr_block_length);
	vis_config->_d_image = 0;	//need this otherwise it will try to delete whatever random location d_image currently points to
	vis_config_change_number_of_samples(vis_config, automatic_d_image, number_of_samples);
	vis_config->minimum_number_of_blocks = minimum_number_of_blocks;
	
	
	cudaDeviceProp prop;
	int dev;
	
	CUDA_SAFE_CALL(cudaGetDevice(&dev));
	CUDA_SAFE_CALL(cudaGetDeviceProperties(&prop, dev));
	
	printf("Name:                    %s\n", prop.name);
	printf("Global Memory:           %d\n", prop.totalGlobalMem);
	printf("Shared Memory per Block: %d\n", prop.sharedMemPerBlock);
	printf("Register per Block:      %d\n", prop.regsPerBlock);
	printf("Warp Size:               %d\n", prop.warpSize);
	printf("Memory Pitch:            %d\n", prop.memPitch);
	printf("Max Threads Per Block:   %d\n", prop.maxThreadsPerBlock);
	printf("Max Threads Dimension:   %d %d %d\n", prop.maxThreadsDim[0], prop.maxThreadsDim[1], prop.maxThreadsDim[2]);
	printf("Max Grid Size:           %d %d %d\n", prop.maxGridSize[0], prop.maxGridSize[1], prop.maxGridSize[2]);
	printf("Constant Memory:         %d\n", prop.totalConstMem);
	printf("Version:                 %d.%d\n", prop.major, prop.minor);
	printf("Clock Rate:              %d\n", prop.clockRate);
	printf("Texture Alignment:       %d\n", prop.textureAlignment);
	
	if(prop.major==1 && prop.minor < 1)
	{
		printf("Warning: Your card does not have compute capability 1.1\n");
	}
	
	if(prop.maxThreadsPerBlock != prop.maxThreadsDim[0]) printf("libMeshlessVis WARNING: Assumption made that maxThreadsPerBlock == maxThreadsDim[0]\n");
	vis_config->_block_length = prop.maxThreadsDim[0];
	
	return vis_config;
}

VisConfig* vis_config_get_default()
{
	float2 step_size = make_float2(0.1f, 0.1f);
	int2 cutoff_frequency = make_int2(128,128);
	float3 u_axis = make_float3(1.0f, 0.0f, 0.0f);
	float3 v_axis = make_float3(0.0f, 1.0f, 0.0f);
	int2 number_of_samples = make_int2(512,512);
	int fvr_block_length = 256;
	int minimum_number_of_blocks = 0;
	VisConfig* vis_config = vis_config_create(true, step_size, cutoff_frequency, u_axis, v_axis, number_of_samples, fvr_block_length, minimum_number_of_blocks);
	return vis_config;
}

bool vis_config_check(VisConfig* vis_config)
{
	cudaDeviceProp prop;
	int dev;
	CUDA_SAFE_CALL(cudaGetDevice(&dev));
	CUDA_SAFE_CALL(cudaGetDeviceProperties(&prop, dev));
	
	if(vis_config->_fvr_block_length < 64) return false;	/* Minimum specified by Appendix A.1 of the NVIDIA CUDA 1.1 Programming Guide */
	if(vis_config->_fvr_block_length < prop.warpSize) return false;	/* We want a fully populated warp */
	if(vis_config->_block_length < 64) return false;	/* Minimum specified by Appendix A.1 of the NVIDIA CUDA 1.1 Programming Guide */
	if(vis_config->_block_length < prop.warpSize) return false;	/* We want a fully populated warp */

	if(vis_config->_fvr_block_length > prop.maxThreadsPerBlock) return false;
	if(vis_config->_fvr_block_length > prop.maxThreadsDim[0]) return false;
	if((2*vis_config->cutoff_frequency.x*vis_config->cutoff_frequency.y)%vis_config->_fvr_block_length) return false;
		
	if(vis_config->_block_length > prop.maxThreadsPerBlock) return false;
	if(vis_config->_block_length > prop.maxThreadsDim[0]) return false;	
	if((vis_config->_number_of_samples.x*vis_config->_number_of_samples.y)%vis_config->_block_length) return false;

	return true;
}

void vis_config_change_fvr_block_length(VisConfig* vis_config, int fvr_block_length)
{
	vis_config->_fvr_block_length = fvr_block_length;
}

void vis_config_change_number_of_samples(VisConfig* vis_config, bool automatic_d_image, int2 number_of_samples)
{
	vis_config_clean(vis_config);
	vis_config->_number_of_samples = number_of_samples;
	vis_config->_automatic_d_image = automatic_d_image;
	if(automatic_d_image) CUDA_SAFE_CALL(cudaMalloc((void**)&vis_config->_d_image, sizeof(float2)*vis_config->_number_of_samples.x*vis_config->_number_of_samples.y));
	CUDA_SAFE_CALL(cudaMalloc((void**)&vis_config->_d_freq_image, sizeof(float2)*vis_config->_number_of_samples.x*vis_config->_number_of_samples.y));
	// Previously freq_image was not a power of two size in order to minimize memory usage, so a thread block size of 1x1 was used when scaling the image
	// Forsaking the reduced memory cost, by storing the power of two sized image, more optimal thread block sizes can be in the zero and scale kernels, improving performance in certain cases
	cufftPlan2d(&vis_config->_plan, vis_config->_number_of_samples.x, vis_config->_number_of_samples.y, CUFFT_C2R);
}

void vis_config_manual_d_image(VisConfig* vis_config, float2* d_image)
{
	vis_config->_d_image = d_image;
}

void vis_config_clean(VisConfig* vis_config)
{
	if(vis_config->_d_image != 0)
	{
		if(vis_config->_automatic_d_image) CUDA_SAFE_CALL(cudaFree(vis_config->_d_image));
		CUDA_SAFE_CALL(cudaFree(vis_config->_d_freq_image));
		cufftDestroy(vis_config->_plan);		
	}
}

void vis_config_destroy(VisConfig* vis_config)
{
	vis_config_clean(vis_config);
	free(vis_config);
}

void vis_config_compute_scale(VisConfig* vis_config)
{
	// scale the unnormalized FFT by our discrete step size
	vis_config->_scale = vis_config->step_size.x * vis_config->step_size.y;
}

template <typename T>
T* load_into_device(T* h_data, int N)
{
	T* d_data;
	if(h_data == 0)
	{
		d_data = 0;
	}
	else
	{
		CUDA_SAFE_CALL(cudaMalloc((void**)&d_data, sizeof(T)*N));
		CUDA_SAFE_CALL(cudaMemcpy(d_data, h_data, sizeof(T)*N, cudaMemcpyHostToDevice));
	}
	return d_data;
}

void vis_register_meshless_dataset(MeshlessDataset* meshless_dataset)
{
	int number_of_terms = get_number_of_terms(meshless_dataset);
	meshless_dataset->d_constraints = load_into_device(meshless_dataset->h_constraints, number_of_terms);
	meshless_dataset->d_radii = load_into_device(meshless_dataset->h_radii, number_of_terms);
}

void vis_unregister_meshless_dataset(MeshlessDataset* meshless_dataset)
{
	CUDA_SAFE_CALL(cudaFree(meshless_dataset->d_constraints));
	CUDA_SAFE_CALL(cudaFree(meshless_dataset->d_radii));
}

void vis_fourier_volume_rendering(MeshlessDataset meshless_dataset, VisConfig* vis_config)
{

	// Initialize the image's Fourier transform to zero since a lowpass filter will be used
	cudaMemset((void*)vis_config->_d_freq_image, 0, sizeof(float2)*vis_config->_number_of_samples.x*vis_config->_number_of_samples.y);

	// Sample the image's Fourier transform
	fourier_transform(meshless_dataset, vis_config);

	// Compute, using a Complex-To-Real FFT, the inverse Fourier transform of the Fourier transform of the image. 	
	cufftExecC2R(vis_config->_plan, (cufftComplex*)vis_config->_d_freq_image, (cufftReal*)vis_config->_d_image);
}

void vis_copy_to_host(VisConfig* vis_config, float* h_image)
{
	CUDA_SAFE_CALL(cudaMemcpy(h_image, vis_config->_d_image, sizeof(float)*vis_config->_number_of_samples.x*vis_config->_number_of_samples.y, cudaMemcpyDeviceToHost));
}

void vis_register_buffer_object(GLuint buffer_object) { CUDA_SAFE_CALL(cudaGLRegisterBufferObject(buffer_object)); }
void vis_unregister_buffer_object(GLuint buffer_object) { CUDA_SAFE_CALL(cudaGLUnregisterBufferObject(buffer_object)); }
void vis_opengl_fourier_volume_rendering(MeshlessDataset meshless_dataset, VisConfig* vis_config, GLuint registered_buffer_object)
{
	CUDA_SAFE_CALL(cudaGLMapBufferObject( (void**)&vis_config->_d_image, registered_buffer_object));
	vis_fourier_volume_rendering(meshless_dataset,vis_config);	
	CUDA_SAFE_CALL(cudaGLUnmapBufferObject(registered_buffer_object));
}
