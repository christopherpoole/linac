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

#include "StopKillShield.hh"
#include "SensitiveDetector.hh"
#include "Phasespace.hh"

// GEANT4 //
#include "G4VUserDetectorConstruction.hh"

#include "G4Region.hh"
#include "G4ProductionCuts.hh"

#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4AssemblyVolume.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4Colour.hh"

#include "G4RotationMatrix.hh"

// G4VoxelData//
#include "G4VoxelData.hh"
#include "G4VoxelArray.hh"
#include "DicomDataIO.hh"

// BOOST/PYTHON //
#include "boost/python.hpp"
//#include "pyublas/numpy.hpp"

// Simple data structure for setpoints in hounsfiled -> G4Material ramp
class Hounsfield{
  public:
    Hounsfield(int value, G4String material_name, G4double density) {
        this->value = value;
        this->material_name = material_name;
        this->density = density;
    };

  public:
    G4int value;
    G4String material_name;
    G4double density;
};


class DetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    DetectorConstruction();
    ~DetectorConstruction();

    G4VPhysicalVolume* Construct();
  
  public:
    void SetupHead(G4double head_radius, G4double head_length,
                   G4ThreeVector head_position, G4ThreeVector head_rotation,
                    G4double vacuum_radius, G4double vacuum_length, G4ThreeVector vacuum_position);
    void AddMaterial(G4String name, G4double density, boost::python::object move);

    void SetupPhantom();

    G4VPhysicalVolume* AddPhasespace(char* name, double radius, double z_position, char* material);

    G4VPhysicalVolume* AddCylinder(char* name,
                      double radius, double thickness,
                      char* material,
                      G4bool in_vacuum,
                      G4ThreeVector translation,
                      G4ThreeVector rotation,
                      G4Colour colour);

    G4VPhysicalVolume* AddSlab(char* name,
                      double side, double thickness,
                      char* material,
                      G4bool in_vacuum,
                      G4ThreeVector translation,
                      G4ThreeVector rotation,
                      G4Colour colour);

    G4VPhysicalVolume* AddCADComponent(char* name, char* filename, char* material,
                    G4bool in_vacuum, double scale,
                    G4ThreeVector translation,
                    G4ThreeVector rotation,
                    G4Colour colour, G4bool tessellated);

    void SetupCT();

    std::map<int16_t, G4Material*> MakeMaterialsMap(G4int increment);
    G4Material* MakeNewMaterial(G4String base_material_name, G4double density);

    void UsePhantom(G4bool use) {
        use_phantom = use;
    }

    void UseCT(G4String ct_directory, G4ThreeVector ct_position) {
        this->use_ct = true;
        this->ct_directory = ct_directory;
        this->ct_position = ct_position;

        DicomDataIO* reader = new DicomDataIO();
        this->data = reader->ReadDirectory(this->ct_directory);

        // We can peek at the data type with data->type, however at some point
        // we will have to nominate exactly what the type of the data is. For
        // standard DICOM CT as in this example we are using int16's.
        this->array = new G4VoxelArray<int16_t>(this->data);

        // Make a mapping between the data in array and G4Materials
        // at increaments of 25 HU.
        G4int increment = 25;
        materials = MakeMaterialsMap(increment);
    }

    pyublas::numpy_vector<float> GetEnergyHistogram() {
        G4cout << "Getting energy histogram." << G4endl;
        return detector->energy_histogram;
    }

    pyublas::numpy_vector<float> GetCountsHistogram() {
        return detector->counts_histogram;
    }

    void ZeroHistograms() {
        std::fill(detector->energy_histogram.begin(), detector->energy_histogram.end(), 0.0);
        std::fill(detector->counts_histogram.begin(), detector->counts_histogram.end(), 0.0);
    }

  private:
    G4Region* region;
    G4ProductionCuts* cuts;

    G4Box* world_solid;
    G4LogicalVolume* world_logical;
    G4VPhysicalVolume* world_physical;

    G4Box* phantom_solid;
    G4LogicalVolume* phantom_logical;
    G4VPhysicalVolume* phantom_physical;

    SensitiveDetector* detector;

    G4Tubs* head_solid;
    G4LogicalVolume* head_logical;
    G4VPhysicalVolume* head_physical;

    G4Tubs* vacuum_solid;
    G4LogicalVolume* vacuum_logical;
    G4VPhysicalVolume* vacuum_physical;

    G4Tubs* sheild_solid;
    G4LogicalVolume* sheild_logical;
    G4VPhysicalVolume* sheild_physical;

    G4NistManager* nist_manager;
    G4Material* StainlessSteel;

    G4ThreeVector head_position;
    G4RotationMatrix* head_rotation;
    G4double head_length;
    G4ThreeVector vacuum_position;
    G4double vacuum_length;

    G4bool use_phantom;
    G4bool use_ct;
    G4bool ct_built;
    
    G4String ct_directory;
    G4ThreeVector ct_position;

    G4VoxelData* data;
    G4VoxelArray<int16_t>* array;
    std::map<int16_t, G4Material*> materials;
    std::vector<Hounsfield> hounsfield;
};

#endif

