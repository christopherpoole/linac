//////////////////////////////////////////////////////////////////////////
// License & Copyright
// ===================
// 
// Copyright 2013 Christopher M Poole <mail@christopherpoole.net>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////

// Standard Template Library //
#include <vector>
#include <iostream>

// linac //
#include "PhasespaceRecord.hh"

// boost::python //
#include "boost/python.hpp"
#include "boost/python/converter/builtin_converters.hpp"
#include "boost/python/suite/indexing/vector_indexing_suite.hpp"

// PyUBLAS //
#include "pyublas/numpy.hpp"

// boost::serialization //
#include "boost/archive/binary_iarchive.hpp"
#include "boost/archive/binary_oarchive.hpp"


using namespace boost::python;


class PhasespaceInspector {
  public:
    PhasespaceInspector() {
    };

    void Read(std::string filename) {
        input_file_stream = new std::ifstream(filename.c_str(), std::ios::binary);
        phasespace_archive = new boost::archive::binary_iarchive(*input_file_stream);

        PhasespaceRecord phasespace_record;

        while(1) {
            phasespace_record = PhasespaceRecord();
            try {
                *phasespace_archive >> phasespace_record;
                this->energy.push_back((float)phasespace_record.GetKineticEnergy());
            } catch (...) {
                std::cout << "Reached end of archive. Aborting." << std::endl;
                break;
            }   
        }
    };

    pyublas::numpy_vector<float> GetEnergy() {

        pyublas::numpy_vector<float> energy(this->energy.size());
        std::copy(this->energy.begin(), this->energy.end(), energy.begin());
        return energy; 
    };

  private:
    std::ifstream* input_file_stream;
    boost::archive::binary_iarchive* phasespace_archive;
       
    std::vector<float> energy;
};


BOOST_PYTHON_MODULE(libphsp_inspector) {
    class_<PhasespaceInspector, PhasespaceInspector*>("PhasespaceInspector")
        .def("Read", &PhasespaceInspector::Read)
        .add_property("energy", &PhasespaceInspector::GetEnergy)
    ; 
}

