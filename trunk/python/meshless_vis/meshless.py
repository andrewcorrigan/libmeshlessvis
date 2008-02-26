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

import ctypes
import numpy.ctypeslib

meshless_vis_debug = False
meshless_vis_emu = False

csub = None

if meshless_vis_debug == False:
	if meshless_vis_emu == False: csub = numpy.ctypeslib.load_library('libmeshless_vis.so', '../lib')
	else:  csub = numpy.ctypeslib.load_library('libmeshless_vis_emu.so', '../lib')
else:
	if meshless_vis_emu == False: csub = numpy.ctypeslib.load_library('libmeshless_vis_D.so', '../lib')
	else:  csub = numpy.ctypeslib.load_library('libmeshless_vis_emu.so', '../lib')

c_float32_p = numpy.ctypeslib.ndpointer(dtype='float32', flags='CONTIGUOUS')
c_int_p = numpy.ctypeslib.ndpointer(dtype='int', flags='CONTIGUOUS')


"""
##################################################
# Do you need to see Group from Python    #
##################################################
typedef struct	
{
	int number_of_terms;
	char* name;
} Group;
*/

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
"""




c_constraint_p = None #todo

csub.get_simple_meshless_dataset.argtypes = [ctypes.c_int, c_constraint_p, c_float32_p, ctypes.c_char_p]

def get_simple_meshless_dataset(number_of_terms, h_constraints, h_radii, name):
	if h_radii.dtype != float32: h_radii = array(h_radii, dtype=float32)

	meshless_dataset = csub.get_simple_meshless_dataset(number_of_terms, h_constraints, h_radii, name)
	return meshless_dataset

c_meshless_dataset = None #todo
csub.delete_meshless_dataset_groups.argtypes = [c_meshless_dataset]
delete_meshless_dataset_groups = csub.delete_meshless_dataset_groups
	
csub.delete_meshless_dataset.argtypes = [c_meshless_dataset]
delete_meshless_dataset = csub.delete_meshless_dataset

c_meshless_dataset_p = None #todo
csub.delete_meshless_datasets.argtypes = [c_meshless_dataset_p, ctypes.c_int]
delete_meshless_datasets = csub.delete_meshless_datasets

csub.get_number_of_terms.argtypes = [c_meshless_dataset_p]
get_number_of_terms = csub.get_number_of_terms

c_meshless_dataset_pp = None #todo

csub.load_meshless_datasets_from_file.argtypes = [ctypes.c_char_p, c_meshless_dataset_pp, c_int_p]
load_meshless_datasets_from_file = csub.load_meshless_datasets_from_file

csub.save_meshless_datasets_to_file.argtypes = [ctypes.c_char_p, c_meshless_dataset_p, ctypes, c_int]
save_meshless_datasets_to_file = csub.save_meshless_datasets_to_file



