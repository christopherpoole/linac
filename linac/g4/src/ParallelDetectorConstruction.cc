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



// USER //
#include "DetectorConstruction.hh"
#include "ParallelDetectorConstruction.hh"

// GEANT4 //
#include "globals.hh"
#include "G4RunManager.hh"
#include "G4SDManager.hh"
#include "G4VisAttributes.hh"
#include "G4Color.hh"


ParallelDetectorConstruction::ParallelDetectorConstruction(G4String name)
    : G4VUserParallelWorld(name)
{
    this->verbose = 4;
}


ParallelDetectorConstruction::~ParallelDetectorConstruction()
{
}


void ParallelDetectorConstruction::Construct()
{
    world_physical = GetWorld();
    world_logical = world_physical->GetLogicalVolume();
    world_logical->SetVisAttributes(G4VisAttributes::Invisible);
}

G4VPhysicalVolume* ParallelDetectorConstruction::AddPhasespace(char* name, double radius, double z_position, bool kill)
{
    if (verbose >= 4)
        G4cout << "DetectorConstruction::AddPhasespace: " << name << G4endl;

    DetectorConstruction* detector = (DetectorConstruction*) G4RunManager::GetRunManager()->GetUserDetectorConstruction();

    G4Box* solid = new G4Box(name, radius, radius, 1*nm);
    G4LogicalVolume* logical = new G4LogicalVolume(solid, 0, name, 0, 0, 0);
    logical->SetVisAttributes(new G4VisAttributes(G4Color(1, 0, 1, 0.5))); 

    G4VPhysicalVolume* physical = new G4PVPlacement(0, G4ThreeVector(0, 0, z_position*mm),
            logical, name, world_logical, false, 0);


    // Active scoring area is 1% smaller than actual plane - avoids navigation errors when point on edge with direction (0,0,0)
    Phasespace* phasespace_sensitive_detector = new Phasespace(name, radius - (radius*0.01));
    phasespace_sensitive_detector->SetKillAtPlane(kill);
    phasespaces[name] = phasespace_sensitive_detector;

    G4SDManager* sensitive_detector_manager = G4SDManager::GetSDMpointer();
    sensitive_detector_manager->AddNewDetector(phasespace_sensitive_detector);
    logical->SetSensitiveDetector(phasespace_sensitive_detector);

    return physical;
}


void ParallelDetectorConstruction::RemovePhasespace(char* name) {
    if (verbose >=4)
        G4cout << "DetectorConstruction::RemovePhasespace" << G4endl;
   
    phasespaces.erase(name);

    DetectorConstruction* detector = (DetectorConstruction*) G4RunManager::GetRunManager()->GetUserDetectorConstruction();

    G4VPhysicalVolume* physical = detector->FindVolume(name, world_physical);
    delete physical;
    G4RunManager::GetRunManager()->GeometryHasBeenModified();
}

