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


#include "globals.hh"

#include "Phasespace.hh"
#include "DetectorConstruction.hh"


#include "G4ProcessType.hh"
#include "G4HCofThisEvent.hh"
#include "G4TouchableHistory.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4SteppingManager.hh"
#include "G4ThreeVector.hh"

#include <fstream>


Phasespace::Phasespace(const G4String& name, G4double radius) : G4VSensitiveDetector(name) {

//    debug = false;
    
    output_file_stream = new std::ofstream(name);
    phasespace_archive = new boost::archive::binary_oarchive(*output_file_stream);
    
    detector_construction = (DetectorConstruction*) (G4RunManager::GetRunManager()->GetUserDetectorConstruction());

    this->radius = radius;
    record_count = 0;
}

Phasespace::~Phasespace() {
    Close();
//    delete output_file_stream;
//    delete phasespace_archive;
}

void Phasespace::Close() {
    output_file_stream->close();
}

void Phasespace::Initialize(G4HCofThisEvent*) {
}

G4bool Phasespace::ProcessHits(G4Step* aStep, G4TouchableHistory* touchable) {
    const G4Track* aTrack = aStep->GetTrack();
    const G4String name = aTrack->GetDefinition()->GetParticleName();

    G4ThreeVector position = aTrack->GetPosition();
    G4ThreeVector direction = aTrack->GetMomentumDirection();

    // Do not record hits that are outside/near the edge of the 'plane'
    if (abs(position.x()) >= radius || abs(position.y()) >= radius)
        return false; 

    PhasespaceRecord record = PhasespaceRecord(aStep);    

    *phasespace_archive << record;

    aStep->GetTrack()->SetTrackStatus(fStopAndKill);

    record_count ++;
//    if (record_count % 1000 == 0)
//        G4cout << "Phasespace record count: " << record_count << G4endl;

    return true;
}

void Phasespace::EndOfEvent(G4HCofThisEvent*) {
}

void Phasespace::clear() {
}

void Phasespace::PrintAll() {
}
