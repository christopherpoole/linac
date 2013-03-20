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


#ifndef _SensitiveDETECTOR_HH
#define	_SensitiveDETECTOR_HH


#include "G4VSensitiveDetector.hh"
#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

#include "boost/python.hpp"
#include "pyublas/numpy.hpp"

#include <vector>

class G4Step;
class G4TouchableHistory;
class G4HCofThisEvent;
class DetectorConstruction;

class SensitiveDetector : public G4VSensitiveDetector {
public:
    SensitiveDetector(const G4String& name);
    virtual ~SensitiveDetector();

    void Initialize(G4HCofThisEvent*);
    G4bool ProcessHits(G4Step*, G4TouchableHistory*);
    void EndOfEvent(G4HCofThisEvent*);
    void clear();
    void PrintAll();

    DetectorConstruction* detector_construction;

    void SetDimensions(G4int x, G4int y, G4int z) {
        x_dim = x;
        y_dim = y;
        z_dim = z;
    };

    void SetMinimumCutoff(G4int x, G4int y, G4int z) {
        x_min = x;
        y_min = y;
        z_min = z;
    };

    void SetMaximumCutoff(G4int x, G4int y, G4int z) {
        x_max = x;
        y_max = y;
        z_max = z;
    };

    void SetResolution(G4float x, G4float y, G4float z) {
        x_res = x;
        y_res = y;
        z_res = z;

        volume = x * y * z;
    };

public:

    pyublas::numpy_vector<float> energy_histogram;
    pyublas::numpy_vector<float> counts_histogram;

    //G4double voxel_mass;
    G4double volume;
    G4bool debug;

    G4int x_dim;
    G4int y_dim;
    G4int z_dim;
    G4int x_min;
    G4int y_min;
    G4int z_min;
    G4int x_max;
    G4int y_max;
    G4int z_max;

    G4float x_res;
    G4float y_res;
    G4float z_res;

};


#endif	/* _SensitiveDETECTOR_HH */

