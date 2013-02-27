# License & Copyright
# ===================
#
# Copyright 2012 Christopher M Poole <mail@christopherpoole.net>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
project(g4)

find_package(Geant4 REQUIRED ui_all vis_all)
find_package(PythonLibs REQUIRED)
find_package(Boost REQUIRED COMPONENTS python serialization iostreams)

include(${Geant4_USE_FILE})
include_directories(${PYTHON_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(/usr/local/include)

# CADMesh
find_package(cadmesh)

# G4VoxelData with DICIOM
find_package(G4VoxelData)

# User code
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

SET(LIBRARY_OUTPUT_PATH "..")
add_library(g4 "SHARED" libg4.cc ${sources} ${headers})

target_link_libraries(g4 ${cadmesh_LIBRARIES})
target_link_libraries(g4 ${Geant4_LIBRARIES})
target_link_libraries(g4 ${Boost_LIBRARIES})
target_link_libraries(g4 ${Python_LIBRARIES})
target_link_libraries(g4 ${G4VOXELDATA_DICOM_LIBRARIES})

