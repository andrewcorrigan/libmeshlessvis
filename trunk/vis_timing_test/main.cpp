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

#include <iostream>
#include <cstdio>

int main(int argc, char** argv)
{
	if(argc != 8)
	{
		std::cout << std::endl << "vis_timing_test number_of_runs number_of_terms cutoff_frequency block_length number_of_samples factor number_of_partial_sums" << std::endl;
		std::cout << "a good value for block length is 256" << std::endl;
		std::cout << "factor should usually be 0, 1 is a special case that should be faster" << std::endl << std::endl;
		return 0;
	}

	int number_of_runs;
	int number_of_terms;
	int2 cutoff_frequency;
	int block_length;
	int factor;
	int2 number_of_samples;
	int number_of_partial_sums;

	std::sscanf(argv[1], "%d", &number_of_runs);
	std::sscanf(argv[2], "%d", &number_of_terms);
	std::sscanf(argv[3], "%d", &cutoff_frequency.x);
	std::sscanf(argv[4], "%d", &block_length);
	std::sscanf(argv[5], "%d", &number_of_samples.x);
	std::sscanf(argv[6], "%d", &factor);
	std::sscanf(argv[7], "%d", &number_of_partial_sums);

	std::cout << "number of runs: " << number_of_runs << std::endl;
	std::cout << "number of terms: " << number_of_terms << std::endl;
	std::cout << "cutoff frequency: " << cutoff_frequency.x << std::endl;
	std::cout << "block length: " <<  block_length << std::endl;
	std::cout << "number of samples: " << number_of_samples.x << std::endl;
	std::cout << "factoring out fourier transform: " << factor << std::endl;
	std::cout << "number of partial sums: " << number_of_partial_sums << std::endl;

	number_of_samples.y = number_of_samples.x;
	cutoff_frequency.y = cutoff_frequency.x;
	
	std::cout << "initializing vis config" << std::endl;
	VisConfig* vis_config = vis_config_create(true, make_float2(1.0, 1.0), cutoff_frequency, make_float3(1.0, 0.0, 0.0), make_float3(0.0, 1.0, 0.0), number_of_samples, block_length, number_of_partial_sums);
	if(!vis_config_check(vis_config)) std::cout << "Warning: Invalid Configuration" << std::endl;
	else std::cout << "Valid Configuration" << std::endl;
	
	std::cout << "initializing data" << std::endl;
	Constraint* h_constraints = new Constraint[number_of_terms];
	float* h_radii = 0;
	if(!factor) h_radii = new float[number_of_terms];
	
	for (int i = 0; i != number_of_terms; ++i) {
		h_constraints[i].weight = 1.0f;
		h_constraints[i].position.x = h_constraints[i].position.y = h_constraints[i].position.z = 0.0f;
		if(h_radii) h_radii[i] = 1e-5f;
	}
	
	MeshlessDataset meshless_dataset = get_simple_meshless_dataset(number_of_terms, h_constraints, h_radii, SPH);
	std::cout << "loading data" << std::endl;
	vis_register_meshless_dataset(vis_config, &meshless_dataset);
	
	std::cout << "running code to measure performance" << std::endl;
	for(unsigned int k = 0; k != number_of_runs; ++k) vis_fourier_volume_rendering(&meshless_dataset, vis_config);
	std::cout << "finished running tests" << std::endl << std::endl;
	
	vis_unregister_meshless_dataset(vis_config, &meshless_dataset);

	vis_config_destroy(vis_config);
	delete_meshless_dataset(meshless_dataset);
	
	return 0;
}
