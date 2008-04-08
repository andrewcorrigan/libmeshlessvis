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

#include "meshless.h"
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>

MeshlessDataset get_simple_meshless_dataset(int number_of_terms, Constraint* h_constraints, float* h_radii, BasisFunctionId basis_function_id)
{
	MeshlessDataset meshless_dataset;
	meshless_dataset.number_of_groups = 1;
	meshless_dataset.groups = new Group[1];
	meshless_dataset.groups[0].basis_function_id = basis_function_id;
	meshless_dataset.groups[0].number_of_terms = number_of_terms;
	meshless_dataset.groups[0].h_constraints = h_constraints;
	meshless_dataset.groups[0].h_radii = h_radii;
	return meshless_dataset;
}

void delete_meshless_dataset(MeshlessDataset meshless_dataset)
{
	for(int i = 0; i != meshless_dataset.number_of_groups; i++)
	{
		delete[] meshless_dataset.groups[i].h_constraints;
		if(meshless_dataset.groups[i].h_radii != 0) delete[] meshless_dataset.groups[i].h_radii;
	}
	delete[] meshless_dataset.groups;
}

void delete_meshless_datasets(MeshlessDataset* meshless_datasets, int number_of_datasets)
{
	for(int i = 0; i != number_of_datasets; i++)
	{
		delete_meshless_dataset(meshless_datasets[i]);
	}
	delete[] meshless_datasets;
}

int get_number_of_terms(MeshlessDataset* meshless_dataset)
{
	int N = 0;
	for(int i = 0; i != meshless_dataset->number_of_groups; i++)
	{
		N += meshless_dataset->groups[i].number_of_terms;
	}
	return N;
}

void shift_meshless_dataset(MeshlessDataset* meshless_dataset, float x, float y, float z)
{
	for(int j = 0; j != meshless_dataset->number_of_groups; j++)
	{
		for(int k = 0; k != meshless_dataset->groups[j].number_of_terms; k++)
		{
			meshless_dataset->groups[j].h_constraints[k].position.x += x;
			meshless_dataset->groups[j].h_constraints[k].position.y += y;
			meshless_dataset->groups[j].h_constraints[k].position.z += z;
		}
	}
}

void shift_meshless_datasets(MeshlessDataset* meshless_datasets, int number_of_datasets, float x, float y, float z)
{
	for(int i = 0; i != number_of_datasets; ++i) shift_meshless_dataset(meshless_datasets+i, x,y,z);
}

void load_meshless_datasets_from_file(const char* filename, MeshlessDataset** meshless_datasets, int* number_of_datasets)
{
	std::ifstream file(filename);

	file >> *number_of_datasets;
	if(*number_of_datasets == 0)
	{
		(*meshless_datasets) = 0;
		return;	
	}

	(*meshless_datasets) = new MeshlessDataset[*number_of_datasets];

	for(int i = 0; i != *number_of_datasets; i++) {
		
		file >> (*meshless_datasets)[i].number_of_groups;
		(*meshless_datasets)[i].groups = new Group[(*meshless_datasets)[i].number_of_groups];
		for(int j = 0; j != (*meshless_datasets)[i].number_of_groups; j++) {
			std::string basis_function_name, ignore_operator;
			file >> (*meshless_datasets)[i].groups[j].number_of_terms;

			file >> basis_function_name;
			if(basis_function_name == "sph")                 (*meshless_datasets)[i].groups[j].basis_function_id = SPH;
			else if(basis_function_name == "gaussian")       (*meshless_datasets)[i].groups[j].basis_function_id = GAUSSIAN;
			else if(basis_function_name == "wendland_d3_c2") (*meshless_datasets)[i].groups[j].basis_function_id = WENDLAND_D3_C2;

			file >>  ignore_operator;
	
			//todo: handle parameters
			int parameters;
			file >> parameters;		//0 if none, 1 if each bf has a parameter, 2 if all are the same
		}
		for(int j = 0; j != (*meshless_datasets)[i].number_of_groups; j++)
		{
			(*meshless_datasets)[i].groups[j].h_constraints = new Constraint[(*meshless_datasets)[i].groups[j].number_of_terms];

			for(int k = 0; k != (*meshless_datasets)[i].groups[j].number_of_terms; k++) 
				file >> (*meshless_datasets)[i].groups[j].h_constraints[k].position.x >> (*meshless_datasets)[i].groups[j].h_constraints[k].position.y >> (*meshless_datasets)[i].groups[j].h_constraints[k].position.z;
		}

		for(int j = 0; j != (*meshless_datasets)[i].number_of_groups; j++)
			for(int k = 0; k != (*meshless_datasets)[i].groups[j].number_of_terms; k++)
				file >> (*meshless_datasets)[i].groups[j].h_constraints[k].weight;

		int has_radii;
		file >> has_radii;
		for(int j = 0; j != (*meshless_datasets)[i].number_of_groups; j++)
		{
			if(has_radii)
			{
				(*meshless_datasets)[i].groups[j].h_radii = new float[(*meshless_datasets)[i].groups[j].number_of_terms];
				for(int k = 0; k != (*meshless_datasets)[i].groups[j].number_of_terms; k++) file >> (*meshless_datasets)[i].groups[j].h_radii[k];
			}
			else
			{
				(*meshless_datasets)[i].groups[j].h_radii = 0;
			}
		}
	}
}

void save_meshless_datasets_to_file(const char* filename, MeshlessDataset* meshless_datasets, int number_of_datasets)
{
	std::ofstream file(filename);

	file << number_of_datasets << std::endl;
	if(number_of_datasets == 0)	return;	

	for(int i = 0; i != number_of_datasets; i++) {

		file << meshless_datasets[i].number_of_groups << std::endl;

		for(int j = 0; j != meshless_datasets[i].number_of_groups; j++) {
			file << meshless_datasets[i].groups[j].number_of_terms << " ";

			//todo: handle parameters
			int parameters = 0;	//0 if none, 1 if each bf has a parameter, 2 if all are the same
			if(meshless_datasets[i].groups[j].basis_function_id == SPH)                 file << "sph";
			else if(meshless_datasets[i].groups[j].basis_function_id == GAUSSIAN)       file << "gaussian";
			else if(meshless_datasets[i].groups[j].basis_function_id == WENDLAND_D3_C2) file << "wendland_d3_c2";

			file << " unused " << parameters << std::endl;
		}
		
		for(int j = 0; j != meshless_datasets[i].number_of_groups; j++)
			for(int k = 0; k != meshless_datasets[i].groups[j].number_of_terms; k++) 
				file << meshless_datasets[i].groups[j].h_constraints[k].position.x << " " << meshless_datasets[i].groups[j].h_constraints[k].position.y << " "  << meshless_datasets[i].groups[j].h_constraints[k].position.z << std::endl;
	
		for(int j = 0; j != meshless_datasets[i].number_of_groups; j++)
			for(int k = 0; k != meshless_datasets[i].groups[j].number_of_terms; k++) 
				file << meshless_datasets[i].groups[j].h_constraints[k].weight << std::endl;

		if(meshless_datasets[0].number_of_groups > 0 && meshless_datasets[0].groups[0].h_radii != 0)
		{
			file << 1 << std::endl;
			for(int j = 0; j != meshless_datasets[i].number_of_groups; j++)
				for(int k = 0; k != meshless_datasets[i].groups[j].number_of_terms; k++)
					file << meshless_datasets[i].groups[j].h_radii[k] << std::endl;
		}
		else file << 0 << std::endl;
	}
}


