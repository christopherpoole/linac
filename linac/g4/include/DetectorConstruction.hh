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
#include "G4VoxelDataParameterisation.hh"
#include "DicomDataIO.hh"
#include "NumpyDataIO.hh"

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
    G4VPhysicalVolume* FindVolume(G4String name, G4VPhysicalVolume * mother); 
    
    void AddMaterial(G4String name, G4double density, boost::python::object move);

    void SetupPhantom();
    void SetupCADPhantom(char* filename, G4ThreeVector offset);

    G4VPhysicalVolume* AddPhasespace(char* name, double radius, double z_position, char* material, bool kill);
    void RemovePhasespace(char* name);

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
 
    G4VPhysicalVolume* AddCADComponent(char* name, char* filename, char* material,
                    double scale,
                    G4ThreeVector translation,
                    G4ThreeVector rotation,
                    G4Colour colour, G4bool tessellated,
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

    void SetWorldColour(G4Color colour) {
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

    void UseCADPhantom(char* filename, G4ThreeVector offset) {
        phantom_filename = filename;
        phantom_offset = offset;
        use_cad_phantom = true;

        SetupCADPhantom(filename, offset);
    }

    G4ThreeVector GetCTOrigin() {
        return G4ThreeVector(this->data->origin[0],
                this->data->origin[1],
                this->data->origin[2]);
    }

    void SetCTPosition(G4ThreeVector ct_position) {
        this->ct_position = ct_position;
    }

    void UseCT(G4String ct_directory, G4int acquisition_number) {
        this->use_ct = true;
        this->ct_directory = ct_directory;

        DicomDataIO* reader = new DicomDataIO();
        reader->SetAcquisitionNumber(acquisition_number);
        
        this->data = reader->ReadDirectory(this->ct_directory);
        this->array = new G4VoxelArray<int16_t>(this->data);
        this->array->Merge(2, 2, 2);

        G4int increment = 25;
        materials = MakeMaterialsMap(increment);
    }

    void UseArray(G4String filename, G4double x, G4double y, G4double z) {
        this->use_ct = true;

        NumpyDataIO* reader = new NumpyDataIO();
        
        this->data = reader->Read(filename);
        this->array = new G4VoxelArray<int16_t>(this->data);

        std::vector<double> spacing;
        spacing.push_back(x); spacing.push_back(y); spacing.push_back(z);
        //this->array->SetSpacing(spacing);

        G4int increment = 25;
        materials = MakeMaterialsMap(increment);
    };

    void HideCT(G4bool hide) {
        this->use_ct = !hide;
    };

    void CropX(G4int xmin, G4int xmax) {
        this->array->CropX(xmin, xmax);
    };

    void CropY(G4int ymin, G4int ymax) {
        this->array->CropY(ymin, ymax);
    };

    void CropZ(G4int zmin, G4int zmax) {
        this->array->CropZ(zmin, zmax);
    };

    void CropCT(G4int xmin, G4int xmax, G4int ymin, G4int ymax, G4int zmin, G4int zmax) {
        this->array->Crop(xmin, xmax, ymin, ymax, zmin, zmax);
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

  private:
    G4Region* region;
    G4ProductionCuts* cuts;

    G4Box* world_solid;
    G4LogicalVolume* world_logical;
    G4VPhysicalVolume* world_physical;

    G4ThreeVector world_size;
    G4Material* world_material;
    G4Color world_colour;

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
    G4bool use_ct;
    G4bool ct_built;
    
    G4bool use_cad_phantom;
    char* phantom_filename;
    G4ThreeVector phantom_offset;    

    G4String ct_directory;
    G4ThreeVector ct_position;

    G4bool headless;

    G4VoxelData* data;
    G4VoxelArray<int16_t>* array;
    G4VoxelDataParameterisation<int16_t>* voxeldata_param;
    std::map<int16_t, G4Material*> materials;
    std::vector<Hounsfield> hounsfield;

    G4int verbose;
};

#endif

