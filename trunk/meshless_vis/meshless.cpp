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

MeshlessDataset get_simple_meshless_dataset(int number_of_terms, Constraint* h_constraints, float* h_radii, const char* name)
{
	MeshlessDataset meshless_dataset;
	meshless_dataset.h_constraints = h_constraints;
	meshless_dataset.h_radii = h_radii;
	meshless_dataset.number_of_groups = 1;
	meshless_dataset.groups = new Group[1];
	meshless_dataset.groups[0].name = new char[strlen(name) + 1];
	strcpy(meshless_dataset.groups[0].name, name);
	meshless_dataset.groups[0].number_of_terms = number_of_terms;
	return meshless_dataset;
}

void delete_meshless_dataset_groups(MeshlessDataset meshless_dataset)
{
	for(int i = 0; i != meshless_dataset.number_of_groups; i++)
	{
		delete[] meshless_dataset.groups[i].name;
	}
	delete[] meshless_dataset.groups;
}

void delete_meshless_dataset(MeshlessDataset meshless_dataset)
{
	delete[] meshless_dataset.h_constraints;
	if(meshless_dataset.h_radii != 0) delete[] meshless_dataset.h_radii;

	delete_meshless_dataset_groups(meshless_dataset);
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
	int N = get_number_of_terms(meshless_dataset);
	for(int i = 0; i != N; i++)
	{
		meshless_dataset->h_constraints[i].position.x += x;
		meshless_dataset->h_constraints[i].position.y += y;
		meshless_dataset->h_constraints[i].position.z += z;
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
		int number_of_terms = 0;

		file >> (*meshless_datasets)[i].number_of_groups;
		(*meshless_datasets)[i].groups = new Group[(*meshless_datasets)[i].number_of_groups];
		for(int j = 0; j != (*meshless_datasets)[i].number_of_groups; j++) {
			std::string name, ignore_operator;
			file >> (*meshless_datasets)[i].groups[j].number_of_terms;
			number_of_terms += (*meshless_datasets)[i].groups[j].number_of_terms;

			file >> name;
			(*meshless_datasets)[i].groups[j].name = new char[name.length() + 1];
			strcpy((*meshless_datasets)[i].groups[j].name, name.c_str());

			file >>  ignore_operator;
	
			//todo: handle parameters
			int parameters;
			file >> parameters;		//0 if none, 1 if each bf has a parameter, 2 if all are the same
		}
		(*meshless_datasets)[i].h_constraints = new Constraint[number_of_terms];
		for(int j = 0; j != number_of_terms; j++) 
			file >> (*meshless_datasets)[i].h_constraints[j].position.x >> (*meshless_datasets)[i].h_constraints[j].position.y >> (*meshless_datasets)[i].h_constraints[j].position.z;

		for(int j = 0; j != number_of_terms; j++) file >> (*meshless_datasets)[i].h_constraints[j].weight;

		int has_radii;
		file >> has_radii;
		if(has_radii)
		{
			(*meshless_datasets)[i].h_radii = new float[number_of_terms];
			for(int j = 0; j != number_of_terms; j++) file >> (*meshless_datasets)[i].h_radii[j];
		}
		else {
			(*meshless_datasets)[i].h_radii = 0;	
		}
	}
}

void save_meshless_datasets_to_file(const char* filename, MeshlessDataset* meshless_datasets, int number_of_datasets)
{
	std::ofstream file(filename);

	file << number_of_datasets << std::endl;
	if(number_of_datasets == 0)
	{
		return;	
	}


	for(int i = 0; i != number_of_datasets; i++) {
		int number_of_terms = 0;

		file << meshless_datasets[i].number_of_groups << std::endl;

		for(int j = 0; j != meshless_datasets[i].number_of_groups; j++) {
			file << meshless_datasets[i].groups[j].number_of_terms << " ";
			number_of_terms += meshless_datasets[i].groups[j].number_of_terms;

			//todo: handle parameters
			int parameters = 0;	//0 if none, 1 if each bf has a parameter, 2 if all are the same

			file << meshless_datasets[i].groups[j].name << " operatornotusedanymore " << parameters << std::endl;

		}
			
		for(int j = 0; j != number_of_terms; j++) 
			file << meshless_datasets[i].h_constraints[j].position.x << " " << meshless_datasets[i].h_constraints[j].position.y << " "  << meshless_datasets[i].h_constraints[j].position.z << std::endl;
		
		for(int j = 0; j != number_of_terms; j++) file << meshless_datasets[i].h_constraints[j].weight << std::endl;

		if(meshless_datasets[i].h_radii != 0) {
			file << 1 << std::endl;
			for(int j = 0; j != number_of_terms; j++) file << meshless_datasets[i].h_radii[j] << std::endl;
		}
		else file << 0 << std::endl;
	}
}


