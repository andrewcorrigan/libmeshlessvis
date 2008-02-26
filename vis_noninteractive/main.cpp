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

#include <fstream>
#include <iostream>
#include <sstream>

#include <Magick++.h> 


void save_rgb_float_image_to_file(const char* file_name, float*& rgb_image, int width, int height)
{
	Magick::Image image;
	image.read(width, height, "RGB", Magick::FloatPixel, rgb_image);
	image.flip();
	image.write(file_name);
}

int main(int argc, char** argv)
{
	MeshlessDataset* meshless_datasets;
	int number_of_meshless_datasets;
	std::string file_name("../data/cartwheel.sph");
	std::string name_prefix("image_");
	if(argc > 1) file_name = argv[1];
	if(argc > 2) name_prefix = argv[2];
	load_meshless_datasets_from_file(file_name.c_str(), &meshless_datasets, &number_of_meshless_datasets);
	shift_meshless_datasets(meshless_datasets, number_of_meshless_datasets, -5.0f,-5.0f,-5.0f);
	VisConfig* vis_config = vis_config_get_default();


	float* h_image = new float[vis_config->_number_of_samples.x*vis_config->_number_of_samples.y];
	float* rgb_image = new float[vis_config->_number_of_samples.x*vis_config->_number_of_samples.y*3];
	for(unsigned int k = 0; k != number_of_meshless_datasets; ++k)
	{
		std::string name = name_prefix;
		if(k < 10) name += "0";
		if(k < 100) name += "0";
		std::stringstream k_ss;
		k_ss << k;
		name += k_ss.str();
		name += ".jpg";
		std::cout << name << std::endl;

		MeshlessDataset meshless_dataset = meshless_datasets[k];
		vis_register_meshless_dataset(&meshless_dataset);
		vis_fourier_volume_rendering(meshless_dataset, vis_config);
		vis_unregister_meshless_dataset(&meshless_dataset);
		vis_copy_to_host(vis_config, h_image);

		
		{
			// compute the maximum intensity
			float mmax = h_image[0];
			for(unsigned int i = 0; i != vis_config->_number_of_samples.x * vis_config->_number_of_samples.y; ++i)
			{
				if(h_image[i] > mmax) mmax = h_image[i];
			}
			//normalize with respect to the maximum intensity
			for(unsigned int i = 0; i != vis_config->_number_of_samples.x * vis_config->_number_of_samples.y; ++i)
			{
				if(h_image[i] < 0) h_image[i] = 0;
				rgb_image[3*i] = rgb_image[3*i+1] = rgb_image[3*i+2] = h_image[i]/mmax;
			}
			save_rgb_float_image_to_file(name.c_str(), rgb_image, vis_config->_number_of_samples.x, vis_config->_number_of_samples.y); 
		}
	}

	delete[] rgb_image;
	delete[] h_image;
	vis_config_destroy(vis_config);

	delete_meshless_datasets(meshless_datasets, number_of_meshless_datasets);
	return 0;
}
