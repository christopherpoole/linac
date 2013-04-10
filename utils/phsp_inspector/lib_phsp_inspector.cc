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
                this->energy.push_back(phasespace_record.GetKineticEnergy());
                this->weight.push_back(phasespace_record.GetWeight());
                this->direction_x.push_back(phasespace_record.GetMomentum().x());
                this->direction_y.push_back(phasespace_record.GetMomentum().y());
                this->direction_z.push_back(phasespace_record.GetMomentum().z());
                this->position.push_back(phasespace_record.GetPosition());
                this->particle_type.push_back(phasespace_record.GetParticleType());
            } catch (...) {
                std::cout << "Reached end of archive. Aborting." << std::endl;
                break;
            }   
        }
    };

    pyublas::numpy_vector<float> GetEnergy() {
        return Get<float>(this->energy); 
    };

    pyublas::numpy_vector<float> GetWeight() {
        return Get<float>(this->weight); 
    };

    pyublas::numpy_vector<float> GetDirectionX() {
        return Get<float>(this->direction_x); 
    };

    pyublas::numpy_vector<float> GetDirectionY() {
        return Get<float>(this->direction_y); 
    };

    pyublas::numpy_vector<float> GetDirectionZ() {
        return Get<float>(this->direction_z); 
    };

  private:
    template <typename T>
    pyublas::numpy_vector<T> Get(std::vector<T> target) {
        pyublas::numpy_vector<T> return_target(target.size());
        std::copy(target.begin(), target.end(), return_target.begin());
        return return_target; 
    };
  
  private: 
    std::ifstream* input_file_stream;
    boost::archive::binary_iarchive* phasespace_archive;
       
    std::vector<float> energy;
    std::vector<float> weight;
    std::vector<float> direction_x;
    std::vector<float> direction_y;
    std::vector<float> direction_z;
    std::vector<G4ThreeVector> position;
    std::vector<int> particle_type;
};


BOOST_PYTHON_MODULE(libphsp_inspector) {
    class_<PhasespaceInspector, PhasespaceInspector*>("PhasespaceInspector")
        .def("Read", &PhasespaceInspector::Read)
        .add_property("energy", &PhasespaceInspector::GetEnergy)
        .add_property("weight", &PhasespaceInspector::GetWeight)
        .add_property("direction_x", &PhasespaceInspector::GetDirectionX)
        .add_property("direction_y", &PhasespaceInspector::GetDirectionY)
        .add_property("direction_z", &PhasespaceInspector::GetDirectionZ)
    ; 
}

