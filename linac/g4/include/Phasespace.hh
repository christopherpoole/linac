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


#ifndef _PHASESPACE_HH
#define	_PHASESPACE_HH

#include "PhasespaceRecord.hh"

#include "G4VSensitiveDetector.hh"
#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

//#include "boost/python.hpp"
//#include "pyublas/numpy.hpp"

#include "boost/archive/binary_iarchive.hpp"
#include "boost/archive/binary_oarchive.hpp"

#include <fstream>


class G4Step;
class G4TouchableHistory;
class G4HCofThisEvent;
class DetectorConstruction;

class Phasespace : public G4VSensitiveDetector {
  public:
    Phasespace(const G4String& name, G4double radius);
    virtual ~Phasespace();

    void Close();

    void Initialize(G4HCofThisEvent*);
    G4bool ProcessHits(G4Step*, G4TouchableHistory*);
    void EndOfEvent(G4HCofThisEvent*);
    void clear();
    void PrintAll();

    DetectorConstruction* detector_construction;

    std::ofstream* output_file_stream;
    boost::archive::binary_oarchive* phasespace_archive;

    G4double radius;
    G4int record_count;
};


#endif	/* _PHASESPACE_HH */

