# License & Copyright
# ===================
#
# Copyright 2013 Christopher M Poole <mail@christopherpoole.net>
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
project(phsp_inspector)

find_package(Geant4 REQUIRED ui_all vis_all)
find_package(PythonLibs REQUIRED)
find_package(Boost REQUIRED COMPONENTS python serialization iostreams)

include(${Geant4_USE_FILE})
include_directories(${PYTHON_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(/usr/local/include)

# PhasespaceRecord.*
include_directories(../../linac/g4/include)
file(GLOB sources ../../linac/g4/src/PhasespaceRecord.cc)
file(GLOB headers ../../linac/g4/include/PhasespaceRecord.hh)


SET(LIBRARY_OUTPUT_PATH "../phsp_inspector")
add_library(phsp_inspector "SHARED" lib_phsp_inspector.cc ${sources} ${headers})

target_link_libraries(phsp_inspector ${Geant4_LIBRARIES})
target_link_libraries(phsp_inspector ${Boost_LIBRARIES})
target_link_libraries(phsp_inspector ${Python_LIBRARIES})

