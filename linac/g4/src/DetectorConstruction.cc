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

// GEANT4 //
#include "globals.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4UserLimits.hh"
#include "G4VisAttributes.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4SolidStore.hh"
#include "G4RunManager.hh"



DetectorConstruction::DetectorConstruction()
{
    verbose = 4;

    if (verbose >= 4)
        G4cout << "DetectorConstruction::DetectorConstruction" << G4endl;

    // Setpoints we will interpolate between for
    // our materials ramp.
    hounsfield.push_back(Hounsfield(-1050, "G4_AIR", 0.001 ));
    hounsfield.push_back(Hounsfield(-950,"G4_AIR", 0.044));
    hounsfield.push_back(Hounsfield(-700,"G4_LUNG_ICRP", 0.302));
    hounsfield.push_back(Hounsfield(125,"G4_TISSUE_SOFT_ICRP", 1.101));
    hounsfield.push_back(Hounsfield(2500,"G4_BONE_CORTICAL_ICRP", 2.088));

    world_size = G4ThreeVector(3*m, 3*m, 3*m);

    nist_manager = G4NistManager::Instance();
    world_material = nist_manager->FindOrBuildMaterial("G4_AIR");

    use_phantom = false;
    region = NULL;
    use_ct = false;
    ct_built = false;

    headless = false;

    detector = NULL;
    voxeldata_param = NULL;

    RegisterParallelWorld(new ParallelDetectorConstruction("parallel_world"));
}

DetectorConstruction::~DetectorConstruction()
{
}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    if (verbose >= 4)
        G4cout << "DetectorConstruction::Construct" << G4endl;

    G4SolidStore::GetInstance()->Clean();
    G4LogicalVolumeStore::GetInstance()->Clean();
    G4PhysicalVolumeStore::GetInstance()->Clean();

    G4NistManager* man = G4NistManager::Instance();
    man->SetVerbose(1);
    // Define elements from NIST 
    man->FindOrBuildElement("H");
    G4Element* C  = man->FindOrBuildElement("C");
    G4Element* Si = man->FindOrBuildElement("Si");
    G4Element* Cr = man->FindOrBuildElement("Cr");
    G4Element* Mn = man->FindOrBuildElement("Mn");
    G4Element* Fe = man->FindOrBuildElement("Fe");
    G4Element* Ni = man->FindOrBuildElement("Ni");
    // Define materials not in NIST
    G4double density;
    G4int ncomponents;
    G4double fractionmass;
    StainlessSteel = new G4Material("StainlessSteel", density= 8.06*g/cm3, ncomponents=6);
    StainlessSteel->AddElement(C, fractionmass=0.001);
    StainlessSteel->AddElement(Si, fractionmass=0.007);
    StainlessSteel->AddElement(Cr, fractionmass=0.18);
    StainlessSteel->AddElement(Mn, fractionmass=0.01);
    StainlessSteel->AddElement(Fe, fractionmass=0.712);
    StainlessSteel->AddElement(Ni, fractionmass=0.09);


    world_solid = new G4Box("world_solid", world_size.x()/2., world_size.y()/2., world_size.z()/2.);
    world_logical = new G4LogicalVolume(world_solid, world_material, "world_logical", 0, 0, 0);
    world_physical = new G4PVPlacement(0, G4ThreeVector(), world_logical, 
                                       "world_physical", 0, false, 0);
    if (world_colour.GetAlpha() == 0) {
       world_logical->SetVisAttributes(G4VisAttributes::Invisible);
    } else {
        world_logical->SetVisAttributes(new G4VisAttributes(world_colour));
    }
/*
    if (region) {
        G4cout << "Deleting region." << G4endl;
        cuts = region->GetProductionCuts();
        delete region;
    }
    G4cout << "Added region." << G4endl;
    region = new G4Region("ShortCuts");
    region->SetProductionCuts(cuts);

    phantom_logical->SetRegion(region);
    region->AddRootLogicalVolume(phantom_logical);
*/
    return world_physical;
}


G4VPhysicalVolume* DetectorConstruction::FindVolume(G4String name, G4VPhysicalVolume * mother)
{
    if (verbose >= 4)
        G4cout << "DetectorConstruction::FindVolume" << G4endl;

    if(mother->GetName() == name) {
        return mother;
    }

    int count = mother->GetLogicalVolume()->GetNoDaughters();
    for(int i=0; i<count; i++) {
        G4VPhysicalVolume * daughter = mother->GetLogicalVolume()->GetDaughter(i);
        if(daughter->GetName() == name) {
            return daughter;
        } else {
            G4VPhysicalVolume * volume = FindVolume(name, daughter);

            if(volume){
                return volume;
            }
        }
    }
    return NULL;
}


void DetectorConstruction::SetupPhantom()
{
    if (verbose >= 4)
        G4cout << "DetectorConstruction::SetupPhantom" << G4endl;

    G4Material* water = nist_manager->FindOrBuildMaterial("G4_WATER");

    phantom_solid = new G4Box("phantom_solid", 200*mm, 200*mm, 200*mm);
    phantom_logical = new G4LogicalVolume(phantom_solid, water, "phantom_logical", 0, 0, 0);
    phantom_physical = new G4PVPlacement(0, G4ThreeVector(0, 0, -200*mm), phantom_logical, 
                                       "phantom_physical", world_logical, false, 0);
    phantom_logical->SetVisAttributes(new G4VisAttributes(G4Colour(0, 0.6, 0.9, 1))); 

    if (!this->detector)
        detector = new SensitiveDetector("phantom_detector");

//    detector = new SensitiveDetector("phantom_detector");

    G4SDManager* sd_manager = G4SDManager::GetSDMpointer();
    sd_manager->AddNewDetector(detector);
    phantom_logical->SetSensitiveDetector(detector);
}


void DetectorConstruction::AddMaterial(G4String name, G4double density,
                                        boost::python::object move)
{
    if (verbose >= 4)
        G4cout << "DetectorConstruction::AddMaterial" << G4endl;

    G4cout << "adding material " << name << " with density : " << density << G4endl;
    boost::python::object a = move();
    int b = boost::python::extract<int>(a);
    G4cout << b << G4endl;
}


G4VPhysicalVolume* DetectorConstruction::AddTube(char* name,
        double inner_radius, double outer_radius, double length, 
        G4ThreeVector translation, G4ThreeVector rotation,
        char* material_name, G4Colour colour,
        G4LogicalVolume* mother_logical)
{
    if (verbose >= 4)
        G4cout << "DetectorConstruction::AddTube" << G4endl;

    G4Material* material = nist_manager->FindOrBuildMaterial(material_name);
    
    G4RotationMatrix* rot = new G4RotationMatrix();
    rot->rotateX(rotation.x()*deg);
    rot->rotateY(rotation.y()*deg);
    rot->rotateZ(rotation.z()*deg);
   
    G4Tubs* solid = new G4Tubs(name, inner_radius, outer_radius, length/2., 0, 360*deg);
    G4LogicalVolume* logical = new G4LogicalVolume(solid, material, name, 0, 0, 0);
    logical->SetVisAttributes(new G4VisAttributes(colour)); 

    G4VPhysicalVolume* physical = new G4PVPlacement(rot, translation,
            logical, name, mother_logical, false, 0);

    return physical;
}


G4VPhysicalVolume* DetectorConstruction::AddSlab(char* name,
                                                   double side, double thickness,
                                                   char* material,
                                                   G4ThreeVector translation,
                                                   G4ThreeVector rotation,
                                                   G4Colour colour,
                                                   G4LogicalVolume* mother_logical)
{
    if (verbose >= 4)
        G4cout << "DetectorConstruction::AddSlab" << G4endl;

    G4Material* mat = nist_manager->FindOrBuildMaterial(material);
    
    G4RotationMatrix* rot = new G4RotationMatrix();
    rot->rotateX(rotation.x()*deg);
    rot->rotateY(rotation.y()*deg);
    rot->rotateZ(rotation.z()*deg);
   
    G4Box* solid = new G4Box(name, side/2., side/2., thickness/2.);
    G4LogicalVolume* logical = new G4LogicalVolume(solid, mat, name, 0, 0, 0);
    logical->SetVisAttributes(new G4VisAttributes(colour)); 

    G4VPhysicalVolume* physical = new G4PVPlacement(rot, translation,
                                                    logical, name, mother_logical,
                                                    false, 0);

    return physical;
}


G4VPhysicalVolume* DetectorConstruction::AddSubtractionSlab(char* name,
                                  double inner_side, double outer_side, double thickness,
                                  char* material,
                                  G4ThreeVector translation,
                                  G4ThreeVector rotation,
                                  G4Colour colour,
                                  G4LogicalVolume* mother_logical)
{
    if (verbose >= 4)
        G4cout << "DetectorConstruction::AddSubtractionSlab" << G4endl;

    G4Material* mat = nist_manager->FindOrBuildMaterial(material);
    
    G4RotationMatrix* rot = new G4RotationMatrix();
    rot->rotateX(rotation.x()*deg);
    rot->rotateY(rotation.y()*deg);
    rot->rotateZ(rotation.z()*deg);
    
    std::string inner_name = "inner_";
    std::string outer_name = "outer_";
    
    inner_name += name;
    outer_name += name;
    
    G4Box* inner_solid = new G4Box(inner_name, inner_side/2., inner_side/2., thickness/2. + 1.);
    G4Box* outer_solid = new G4Box(outer_name, outer_side/2., outer_side/2., thickness/2.);
    
    G4SubtractionSolid* solid = new G4SubtractionSolid(name,outer_solid,inner_solid);
    
    G4LogicalVolume* logical = new G4LogicalVolume(solid, mat, name, 0, 0, 0);
    logical->SetVisAttributes(new G4VisAttributes(colour)); 

    G4VPhysicalVolume* physical = new G4PVPlacement(rot, translation,
                                                    logical, name, mother_logical,
                                                    false, 0);

    return physical;
}



void DetectorConstruction::SetupCT()
{
    if (verbose >= 4)
        G4cout << "DetectorConstruction::SetupCT" << G4endl;

    if (!voxeldata_param) {
        voxeldata_param =
            new G4VoxelDataParameterisation<int16_t>(array, materials, world_physical );

        G4RotationMatrix* rotation = new G4RotationMatrix();
        rotation->rotateZ(90*deg);
        rotation->rotateX(-90*deg);

        voxeldata_param->Construct(ct_position, rotation);
        voxeldata_param->SetRounding(25, -1000, 2000);

        std::map<int16_t, G4Colour*> colours;
        for (int i=-2500; i<5000; i++) {
            double gray = (double) (i + 2500) / 7500.;
            double alpha = 1;

            if (i < -500) {
                gray = 0;
                alpha = 0;
            }

            if (gray > 1)
                gray = 1;

            colours[i] = new G4Colour(gray, gray, gray, alpha);
        }
        voxeldata_param->SetColourMap(colours);
        voxeldata_param->SetVisibility(false);

        detector = new SensitiveDetector("ct_detector");

        G4SDManager* sd_manager = G4SDManager::GetSDMpointer();
        sd_manager->AddNewDetector(detector);
        voxeldata_param->GetLogicalVolume()->SetSensitiveDetector(detector);
        
        G4RunManager::GetRunManager()->GeometryHasBeenModified();
    }
}


std::map<int16_t, G4Material*> DetectorConstruction::MakeMaterialsMap(G4int increment)
{
    if (verbose >= 4)
        G4cout << "DetectorConstruction::MakeMaterialsMap" << G4endl;

    // Our materials map or ramp
    std::map<int16_t, G4Material*> ramp;
    
    // Calculate intermediate points in each segment
    for (unsigned int i=0; i <hounsfield.size()-1; i++) { 
        G4double hounsfield_rise = hounsfield[i+1].density - hounsfield[i].density;
        G4double density_run = hounsfield[i+1].value - hounsfield[i].value;
        G4double gradient = hounsfield_rise / density_run;

        // Add each increment in the current segment to the ramp  
        int count = 0;
        for (int hf=hounsfield[i].value; hf<hounsfield[i+1].value; hf+=increment) {
            G4double density = count*increment*gradient + hounsfield[i].density;
            ramp[hf] = MakeNewMaterial(hounsfield[i].material_name, density);            
            count++;
        }
    } 
    // Add the last setpoint to the ramp
    ramp[hounsfield.back().value] = MakeNewMaterial(hounsfield.back().material_name, 
                                                    hounsfield.back().density);
    return ramp;
}


G4Material* DetectorConstruction::MakeNewMaterial(G4String base_material_name, G4double density)
{
    if (verbose >= 4)
        G4cout << "DetectorConstruction::MakeNewMaterial" << G4endl;

    G4NistManager* nist_manager = G4NistManager::Instance();
    G4String new_name = base_material_name + G4UIcommand::ConvertToString(density);

    return nist_manager->BuildMaterialWithNewDensity(new_name, base_material_name, density*g/cm3);
}

