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


#ifndef DetectorConstruction_H
#define DetectorConstruction_H 1

// USER //
#include "ParallelDetectorConstruction.hh"
#include "StopKillShield.hh"
#include "SensitiveDetector.hh"
#include "Phasespace.hh"

// GEANT4 //
#include "G4VUserDetectorConstruction.hh"

#include "G4Region.hh"
#include "G4ProductionCuts.hh"

#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4SubtractionSolid.hh"
#include "G4UnionSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4AssemblyVolume.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4Colour.hh"
#include "G4SDManager.hh"

#include "G4RotationMatrix.hh"

// BOOST/PYTHON //
#include "boost/python.hpp"
//#include "pyublas/numpy.hpp"



class DetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    DetectorConstruction();
    ~DetectorConstruction();

    G4VPhysicalVolume* Construct();
  
  public:
    G4VPhysicalVolume* FindVolume(G4String name, G4VPhysicalVolume * mother); 
    
    void AddMaterial(G4String name, G4double density, boost::python::object move);

    void SetupPhantom();

    G4VPhysicalVolume* AddTube(char* name,
            double inner_radius, double outer_radius, double length,
            G4ThreeVector translation, G4ThreeVector rotation,
            char* material_name, G4Colour colour,
            G4LogicalVolume* mother_logical);
 
    G4VPhysicalVolume* AddSlab(char* name,
                      double side, double thickness,
                      char* material,
                      G4ThreeVector translation,
                      G4ThreeVector rotation,
                      G4Colour colour,
                      G4LogicalVolume* mother_logical);
                      
    G4VPhysicalVolume* AddSubtractionSlab(char* name,    
                      double inner_side, double outer_side, double thickness,
                      char* material,
                      G4ThreeVector translation,
                      G4ThreeVector rotation,
                      G4Colour colour,
                      G4LogicalVolume* mother_logical);    
    
    void SetupCT();

    std::map<int16_t, G4Material*> MakeMaterialsMap(G4int increment);
    G4Material* MakeNewMaterial(G4String base_material_name, G4double density);

  public:
    void SetWorldSize(G4ThreeVector size) {
        this->world_size = size;
    };

    void SetWorldMaterial(G4String name) {
        this->world_material = nist_manager->FindOrBuildMaterial(name);
    };

    void SetWorldColour(G4Colour colour) {
        this->world_colour = colour;
    };

    void RemoveHead() {
        if (head_physical)
            delete head_physical;
        if (sheild_physical)
            delete sheild_physical;
        if (lid_physical)
            delete lid_physical; 

        headless = true;
    };

    void ClosePhasespace() {
//        phasespace_sensitive_detector->Close();
        for(int i=0; i<phasespaces.size(); i++) {
            (phasespaces[i])->Close();
        }
    
        phasespaces.clear();
    };

    void UsePhantom(G4bool use) {
        use_phantom = use;
    }
    

    pyublas::numpy_vector<float> GetEnergyHistogram() {
        return detector->GetEnergyHistogram();
    }

    pyublas::numpy_vector<float> GetEnergySqHistogram() {
        return detector->GetEnergySqHistogram();
    }

    pyublas::numpy_vector<float> GetCountsHistogram() {
        return detector->GetCountsHistogram();
    }

    void ZeroHistograms() {
        std::fill(detector->energy_histogram.begin(), detector->energy_histogram.end(), 0.0);
        std::fill(detector->energysq_histogram.begin(), detector->energysq_histogram.end(), 0.0);
        std::fill(detector->counts_histogram.begin(), detector->counts_histogram.end(), 0.0);
    }

    void SetVerbosity(G4int verbose) {
        this->verbose = verbose;
    }

    G4LogicalVolume* GetWorld() {
        return world_logical;
    }

    void SetAsStopKillSheild(G4VPhysicalVolume* physical) {
        StopKillSheild* sheild = new StopKillSheild(physical->GetName());

        G4SDManager* sd_manager = G4SDManager::GetSDMpointer();
        sd_manager->AddNewDetector(sheild);
        physical->GetLogicalVolume()->SetSensitiveDetector(sheild);
    }

    G4VPhysicalVolume* AddPhasespace(char* name, double radius, double z_position, bool kill) {
        if (GetNumberOfParallelWorld() == 1) {
            ParallelDetectorConstruction* pw = (ParallelDetectorConstruction*) GetParallelWorld(0);
            return pw->AddPhasespace(name, radius, z_position, kill);
        }

        return 0;
    }

    void RemovePhasespace(char* name) {
        if (GetNumberOfParallelWorld() == 1) {
            ParallelDetectorConstruction* pw = (ParallelDetectorConstruction*) GetParallelWorld(0);
            pw->RemovePhasespace(name);
        }
    }

  private:
    G4Region* region;
    G4ProductionCuts* cuts;

    G4Box* world_solid;
    G4LogicalVolume* world_logical;
    G4VPhysicalVolume* world_physical;

    G4ThreeVector world_size;
    G4Material* world_material;
    G4Colour world_colour;

    G4Box* phantom_solid;
    G4LogicalVolume* phantom_logical;
    G4VPhysicalVolume* phantom_physical;

    SensitiveDetector* detector;
    //Phasespace* phasespace_sensitive_detector;
    std::vector<Phasespace*> phasespaces;

    G4Tubs* head_solid;
    G4LogicalVolume* head_logical;
    G4VPhysicalVolume* head_physical;

    G4Tubs* vacuum_solid;
    G4LogicalVolume* vacuum_logical;
    G4VPhysicalVolume* vacuum_physical;

    G4Tubs* sheild_solid;
    G4LogicalVolume* sheild_logical;
    G4VPhysicalVolume* sheild_physical;

    G4Tubs* lid_solid;
    G4LogicalVolume* lid_logical;
    G4VPhysicalVolume* lid_physical;

    G4NistManager* nist_manager;
    G4Material* StainlessSteel;

    G4ThreeVector head_position;
    G4RotationMatrix* head_rotation;
    G4double head_length;
    G4ThreeVector vacuum_position;
    G4double vacuum_length;

    G4bool use_phantom;
    
    char* phantom_filename;
    G4ThreeVector phantom_offset;    

    G4bool headless;

    std::map<int16_t, G4Material*> materials; // Not sure if I need this... probs not

    G4int verbose;
};

#endif

