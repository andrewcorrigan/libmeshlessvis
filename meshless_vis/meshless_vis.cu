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
#include <cudpp/cudpp.h>
#include <cuda_gl_interop.h>
#include <stdio.h>
#include <math.h>

#include "fourier_transform.h"



__device__ __host__ float dot(float3 a, float3 b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

__device__ __host__ unsigned int point_in_box(float2 point, float4 bounding_box)
{
	return (point.x >= bounding_box.x && point.y >= bounding_box.y && point.x <= bounding_box.z && point.y <= bounding_box.w);
}

__global__ void compute_mask(Group group, unsigned int* mask, float4 bounding_box, float3 u_axis, float3 v_axis)
{
	int index = (blockDim.x*blockIdx.x + threadIdx.x);
	float3 center = group.d_constraints[index].position;
	float radius = group.d_radii[index];
	
	float2 icenter = make_float2(dot(center, u_axis),dot(center, v_axis));

	float2 corner1 = make_float2(icenter.x+radius, icenter.y+radius);
	float2 corner2 = make_float2(icenter.x+radius, icenter.y-radius);
	float2 corner3 = make_float2(icenter.x-radius, icenter.y-radius);
	float2 corner4 = make_float2(icenter.x-radius, icenter.y+radius);	
	
	mask[index] = index < group.number_of_terms && (point_in_box(corner1, bounding_box) && point_in_box(corner2, bounding_box) && point_in_box(corner3, bounding_box) && point_in_box(corner4, bounding_box));
}

template<typename T>
__global__ void compact(T* in, T* out, unsigned int* mask, unsigned int* indices)
{
	int index = (blockDim.x*blockIdx.x + threadIdx.x);
	if(mask[index]) out[index] = in[indices[index]];
}

void cull_fully_aliased_terms(MeshlessDataset* meshless_dataset, VisConfig* vis_config)
{
	int max_number_of_terms = meshless_dataset->groups[0].d_number_of_terms;
	for(int k = 1; k < meshless_dataset->number_of_groups; k++) { if(max_number_of_terms < meshless_dataset->groups[k].number_of_terms) max_number_of_terms = meshless_dataset->groups[k].d_number_of_terms; }
	
	unsigned int* mask, *indices;
	CUDA_SAFE_CALL(cudaMalloc((void**)&mask, sizeof(unsigned int)*max_number_of_terms));
	CUDA_SAFE_CALL(cudaMalloc((void**)&indices, sizeof(unsigned int)*max_number_of_terms));

	unsigned int* cpu_mask1 = (unsigned int*)malloc(sizeof(unsigned int)*max_number_of_terms);
	unsigned int* cpu_mask2 = (unsigned int*)malloc(sizeof(unsigned int)*max_number_of_terms);

	Constraint* compacted_constraints; float* compacted_radii;
	CUDA_SAFE_CALL(cudaMalloc((void**)&compacted_constraints, sizeof(Constraint)*max_number_of_terms));
	CUDA_SAFE_CALL(cudaMalloc((void**)&compacted_radii, sizeof(float)*max_number_of_terms));

	CUDPPScanConfig config;
    config.direction = CUDPP_SCAN_FORWARD;
    config.exclusivity = CUDPP_SCAN_EXCLUSIVE;
    config.maxNumElements = max_number_of_terms;
    config.maxNumRows = 1;
    config.datatype = CUDPP_UINT;
    config.op = CUDPP_ADD;
    
    cudppInitializeScan(&config);

	dim3 block_length(vis_config->block_length);
	float w = 1.0f/vis_config->step_size.x, h = 1.0f/vis_config->step_size.y;
	float4 bounding_box = make_float4(0, 0, w/2, h/2);
	for(int k = 0; k < meshless_dataset->number_of_groups; k++)
	{
		Group group = meshless_dataset->groups[k];

		// Step 1: Compute the mask
		dim3 grid(group.d_number_of_terms / vis_config->block_length); if(group.d_number_of_terms%vis_config->block_length > 0) printf("error\n");
		compute_mask<<<grid, block_length>>>(group, mask, bounding_box, vis_config->u_axis, vis_config->v_axis);
		
		// Step 2: Compute the indices
		cudppScan(indices, mask, group.number_of_terms, &config);
		
		// Step 3: Map data
		CUDA_SAFE_CALL(cudaMemset(compacted_radii, 0, group.d_number_of_terms*sizeof(float)));
		compact<<<grid, block_length>>>(group.d_radii, compacted_radii, mask, indices);
		CUDA_SAFE_CALL(cudaMemcpy(group.d_constraints, compacted_constraints, group.d_number_of_terms*sizeof(float), cudaMemcpyDeviceToDevice));

		CUDA_SAFE_CALL(cudaMemset(compacted_constraints, 0, group.d_number_of_terms*sizeof(Constraint)));
		compact<<<grid, block_length>>>(group.d_constraints, compacted_constraints, mask, indices);
		CUDA_SAFE_CALL(cudaMemcpy(group.d_constraints, compacted_constraints, group.d_number_of_terms*sizeof(Constraint), cudaMemcpyDeviceToDevice));
	}

	cudppFinalizeScan(&config);
	
	free(cpu_mask1);
	free(cpu_mask2);
		
	CUDA_SAFE_CALL(cudaFree(mask));
	CUDA_SAFE_CALL(cudaFree(indices));
	CUDA_SAFE_CALL(cudaFree(compacted_constraints));
	CUDA_SAFE_CALL(cudaFree(compacted_radii));

}

VisConfig* vis_config_create(bool automatic_d_image, float2 step_size, int2 cutoff_frequency, float3 u_axis, float3 v_axis, int2 number_of_samples, int block_length, int number_of_partial_sums)
{
	VisConfig* vis_config = (VisConfig*)malloc(sizeof(VisConfig));
	vis_config->step_size = step_size;
	vis_config->u_axis = u_axis;
	vis_config->v_axis = v_axis;
	vis_config->cull_fully_aliased_terms = false;
	vis_config->_number_of_samples = number_of_samples;
	vis_config->_cutoff_frequency = cutoff_frequency;
	vis_config->block_length = block_length;
	vis_config->_number_of_partial_sums = number_of_partial_sums;

	CUDA_SAFE_CALL(cudaMalloc((void**)&vis_config->_d_freq_image, sizeof(float2)*2*vis_config->_cutoff_frequency.x*vis_config->_cutoff_frequency.y*vis_config->_number_of_partial_sums));
	CUDA_SAFE_CALL(cudaMalloc((void**)&vis_config->_d_freq_image_arranged, sizeof(float2)*vis_config->_number_of_samples.x*(vis_config->_number_of_samples.y/2+1)));

	vis_config->_automatic_d_image = automatic_d_image;
	if(vis_config->_automatic_d_image) CUDA_SAFE_CALL(cudaMalloc((void**)&vis_config->_d_image, sizeof(float)*vis_config->_number_of_samples.x*vis_config->_number_of_samples.y));
	CUFFT_SAFE_CALL(cufftPlan2d(&vis_config->_plan, vis_config->_number_of_samples.x, vis_config->_number_of_samples.y, CUFFT_C2R));
	
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
	
	return vis_config;
}

VisConfig* vis_config_get_default()
{
	float2 step_size = make_float2(0.1f, 0.1f);
	int2 cutoff_frequency = make_int2(128,128);
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
	cudaDeviceProp prop;
	int dev;
	CUDA_SAFE_CALL(cudaGetDevice(&dev));
	CUDA_SAFE_CALL(cudaGetDeviceProperties(&prop, dev));

	if(vis_config->block_length < 64) return false;	/* Minimum specified by Appendix A.1 of the NVIDIA CUDA 1.1 Programming Guide */
	if(vis_config->block_length < prop.warpSize) return false;	/* We want a fully populated warp */
	if(vis_config->block_length > prop.maxThreadsPerBlock) return false;
	if(vis_config->block_length > prop.maxThreadsDim[0]) return false;
	if((2*vis_config->_cutoff_frequency.x*vis_config->_cutoff_frequency.y)%vis_config->block_length) return false;
	
	if(2*vis_config->_cutoff_frequency.x > vis_config->_number_of_samples.x) return false;
	if(2*vis_config->_cutoff_frequency.y > vis_config->_number_of_samples.y) return false;
	
	if(vis_config->_number_of_partial_sums < 1) return false;

	return true;
}

void vis_config_change_number_of_samples(VisConfig* vis_config, int2 number_of_samples)
{
	vis_config->_number_of_samples = number_of_samples;

	if(vis_config->_automatic_d_image) 
	{
		CUDA_SAFE_CALL(cudaFree(vis_config->_d_image));
		CUDA_SAFE_CALL(cudaMalloc((void**)&vis_config->_d_image, sizeof(float)*vis_config->_number_of_samples.x*vis_config->_number_of_samples.y));
	}

	CUDA_SAFE_CALL(cudaFree(vis_config->_d_freq_image_arranged));
	CUDA_SAFE_CALL(cudaMalloc((void**)&vis_config->_d_freq_image_arranged, sizeof(float2)*vis_config->_number_of_samples.x*(vis_config->_number_of_samples.y/2+1)));
	
	CUFFT_SAFE_CALL(cufftDestroy(vis_config->_plan));
	CUFFT_SAFE_CALL(cufftPlan2d(&vis_config->_plan, vis_config->_number_of_samples.x, vis_config->_number_of_samples.y, CUFFT_C2R));
}

void vis_config_change_cutoff_frequency(VisConfig* vis_config, int2 cutoff_frequency)
{
	vis_config->_cutoff_frequency = cutoff_frequency;
	CUDA_SAFE_CALL(cudaFree(vis_config->_d_freq_image));
	CUDA_SAFE_CALL(cudaMalloc((void**)&vis_config->_d_freq_image, sizeof(float2)*2*vis_config->_cutoff_frequency.x*vis_config->_cutoff_frequency.y*vis_config->_number_of_partial_sums));
}

void vis_config_change_number_of_partial_sums(VisConfig* vis_config, int number_of_partial_sums)
{
	vis_config->_number_of_partial_sums = number_of_partial_sums;

	CUDA_SAFE_CALL(cudaFree(vis_config->_d_freq_image));
	CUDA_SAFE_CALL(cudaMalloc((void**)&vis_config->_d_freq_image, sizeof(float2)*2*vis_config->_cutoff_frequency.x*vis_config->_cutoff_frequency.y*vis_config->_number_of_partial_sums));
}

void vis_config_manual_d_image(VisConfig* vis_config, float* d_image)
{
	vis_config->_d_image = d_image;
}

void vis_config_destroy(VisConfig* vis_config)
{
	if(vis_config->_automatic_d_image) CUDA_SAFE_CALL(cudaFree(vis_config->_d_image));
	CUDA_SAFE_CALL(cudaFree(vis_config->_d_freq_image_arranged));
	CUDA_SAFE_CALL(cudaFree(vis_config->_d_freq_image));
	CUFFT_SAFE_CALL(cufftDestroy(vis_config->_plan));
	free(vis_config);
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
		int k = (N / integer_multiple_of) + min(1, N%integer_multiple_of);
		rounded_N = integer_multiple_of*k;
		CUDA_SAFE_CALL(cudaMalloc((void**)&d_data, sizeof(T)*rounded_N));
		CUDA_SAFE_CALL(cudaMemset((void*)d_data, 0, sizeof(T)*rounded_N));
		CUDA_SAFE_CALL(cudaMemcpy(d_data, h_data, sizeof(T)*N, cudaMemcpyHostToDevice));
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
		CUDA_SAFE_CALL(cudaFree(meshless_dataset->groups[j].d_constraints));
		CUDA_SAFE_CALL(cudaFree(meshless_dataset->groups[j].d_radii));
	}
}

__global__ void reduce_partial_sums(VisConfig vis_config)
{
	// TODO: replace this kernel with a call to cudppMultiScan (if possible)
	
	int index = (blockDim.x*blockIdx.x + threadIdx.x);
	int size = 2*vis_config._cutoff_frequency.x*vis_config._cutoff_frequency.y;
	
	float2 partial_sum = vis_config._d_freq_image[index];
	float2 term;
	for(int i = 1; i < vis_config._number_of_partial_sums; i++)
	{
		term = vis_config._d_freq_image[index+i*size];
		partial_sum = make_float2(partial_sum.x+term.x, partial_sum.y+term.y);
	}
	vis_config._d_freq_image[index] = partial_sum;
}

__global__ void arrange_samples(VisConfig vis_config)
{
	
	int index = (blockDim.x*blockIdx.x + threadIdx.x);
	int x = index % (2*vis_config._cutoff_frequency.x);
	if(x == vis_config._cutoff_frequency.x) return;
	int y = index / (2*vis_config._cutoff_frequency.x);
	int index_x = x;
	if(x > vis_config._cutoff_frequency.x) 
	{
		x = x-(2*vis_config._cutoff_frequency.x);
		index_x = (x+vis_config._number_of_samples.x);
	}
	vis_config._d_freq_image_arranged[index_x*(vis_config._number_of_samples.y/2+1)+y] = vis_config._d_freq_image[index];
}

void vis_fourier_volume_rendering(MeshlessDataset* meshless_dataset, VisConfig* vis_config)
{
//	cull_fully_aliased_terms(meshless_dataset, vis_config);
	
	CUDA_SAFE_CALL(cudaMemset((void*)vis_config->_d_freq_image_arranged, 0, sizeof(float2)*vis_config->_number_of_samples.x*(vis_config->_number_of_samples.y/2+1)));
	fourier_transform(meshless_dataset, vis_config); CUT_CHECK_ERROR("fourier_transform failed");

	dim3 block_size(vis_config->block_length);
	dim3 cutoff_grid(2*vis_config->_cutoff_frequency.x*vis_config->_cutoff_frequency.y / vis_config->block_length);	
	if(vis_config->_number_of_partial_sums > 1)
	{
		reduce_partial_sums<<<cutoff_grid, block_size>>>(*vis_config); CUT_CHECK_ERROR("reduce_partial_sums failed");
	}
	arrange_samples<<<cutoff_grid, block_size>>>(*vis_config); CUT_CHECK_ERROR("arrange_samples failed");

	CUFFT_SAFE_CALL(cufftExecC2R(vis_config->_plan, (cufftComplex*)vis_config->_d_freq_image_arranged, (cufftReal*)vis_config->_d_image));
}

void vis_copy_to_host(VisConfig* vis_config, float* h_image)
{
	CUDA_SAFE_CALL(cudaMemcpy(h_image, vis_config->_d_image, sizeof(float)*vis_config->_number_of_samples.x*vis_config->_number_of_samples.y, cudaMemcpyDeviceToHost));
}

void vis_opengl_fourier_volume_rendering(MeshlessDataset* meshless_dataset, VisConfig* vis_config, GLuint buffer_object)
{
	CUDA_SAFE_CALL(cudaGLRegisterBufferObject(buffer_object));
	CUDA_SAFE_CALL(cudaGLMapBufferObject( (void**)&vis_config->_d_image, buffer_object));
	vis_fourier_volume_rendering(meshless_dataset,vis_config);	
	CUDA_SAFE_CALL(cudaGLUnmapBufferObject(buffer_object));
	CUDA_SAFE_CALL(cudaGLUnregisterBufferObject(buffer_object));
}
