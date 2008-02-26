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

#ifndef MESHLESS_H_
#define MESHLESS_H_

#include <vector_types.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
	int number_of_terms;
	char* name;
} Group;

typedef struct __align__(16)
{
	float3 position;
	float weight;
} Constraint;

typedef struct
{
	Constraint* h_constraints, *d_constraints;
	float*  h_radii,     *d_radii;

	Group* groups;
	int number_of_groups;

} MeshlessDataset;

MeshlessDataset get_simple_meshless_dataset(int number_of_terms, Constraint* h_constraints, float* h_radii, const char* name);

void delete_meshless_dataset_groups(MeshlessDataset meshless_dataset);

void delete_meshless_dataset(MeshlessDataset meshless_dataset);
void delete_meshless_datasets(MeshlessDataset* meshless_datasets, int number_of_datasets);

int get_number_of_terms(MeshlessDataset* meshless_dataset);

void shift_meshless_dataset(MeshlessDataset* meshless_dataset, float x, float y, float z);
void shift_meshless_datasets(MeshlessDataset* meshless_dataset, int number_of_datasets, float x, float y, float z);

void load_meshless_datasets_from_file(const char* filename, MeshlessDataset** meshless_datasets, int* number_of_datasets);
void save_meshless_datasets_to_file(const char* filename, MeshlessDataset* meshless_datasets, int number_of_datasets);

#ifdef __cplusplus
}
#endif

#endif /*MESHLESS_H_*/
