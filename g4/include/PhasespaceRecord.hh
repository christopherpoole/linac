//////////////////////////////////////////////////////////////////////////
// License & Copyright
// ===================
// 
// Copyright 2012 Christopher M Poole <mail@christopherpoole.net>
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


#ifndef PHASESPACERECORD_HH
#define PHASESPACERECORD_HH

#include "globals.hh"

#include "G4Step.hh"
#include "G4ThreeVector.hh"

#include "boost/archive/binary_iarchive.hpp"
#include "boost/archive/binary_oarchive.hpp"


class PhasespaceRecord {
  public:
    PhasespaceRecord();
    PhasespaceRecord(G4Step* step);
    virtual ~PhasespaceRecord();

  private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int) {
        ar & position_x;
        ar & position_y;
        ar & position_z;

        ar & momentum_x;
        ar & momentum_y;
        ar & momentum_z;

        ar & kinetic_energy;
        
        ar & weight;

        ar & particle_type;

    } 

  public:
    G4ThreeVector GetPosition();
    G4ThreeVector GetMomentum();
    G4double GetKineticEnergy();
    G4int GetParticleType();
    G4double GetWeight();
    
  public:
    double position_x;
    double position_y;
    double position_z;

    double momentum_x;
    double momentum_y;
    double momentum_z;

    double kinetic_energy;

    double weight;

    int particle_type;

};

#endif /* PHASESPACERECORD_HH */
