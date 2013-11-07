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

// CADMesh //
#include "CADMesh.hh"

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
#include "G4SDManager.hh"


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
    use_cad_phantom = false;
    region = NULL;
    use_ct = false;
    ct_built = false;

    headless = false;

    detector = NULL;
    voxeldata_param = NULL;
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
    world_logical->SetVisAttributes(new G4VisAttributes(world_colour));
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
    phantom_physical = new G4PVPlacement(0, G4ThreeVector(0, 0, -150*mm), phantom_logical, 
                                       "phantom_physical", world_logical, false, 0);
//    phantom_logical->SetVisAttributes(new G4VisAttributes(G4Colour(0, 0.6, 0.9, 1))); 

    if (!this->detector)
        detector = new SensitiveDetector("phantom_detector");

    G4SDManager* sd_manager = G4SDManager::GetSDMpointer();
    sd_manager->AddNewDetector(detector);
    phantom_logical->SetSensitiveDetector(detector);
}

void DetectorConstruction::SetupCADPhantom(char* filename, G4ThreeVector offset)
{
    if (verbose >= 4)
        G4cout << "DetectorConstruction::SetupCADPhantom" << G4endl;

    G4Material* water = nist_manager->FindOrBuildMaterial("G4_WATER");

    G4RotationMatrix* rot = new G4RotationMatrix();
    rot->rotateZ(90*deg);
    rot->rotateX(-90*deg);

    CADMesh* mesh = new CADMesh(filename, (char*) "STL", 1, offset, false);
    G4VSolid* solid = mesh->TessellatedMesh();
    G4LogicalVolume* logical = new G4LogicalVolume(solid, water, filename, 0, 0, 0);

    G4VPhysicalVolume* physical = new G4PVPlacement(rot, G4ThreeVector(),
                                                    logical, filename, world_logical,
                                                    false, 0);

    if (!this->detector)
        detector = new SensitiveDetector("phantom_detector");

    G4SDManager* sd_manager = G4SDManager::GetSDMpointer();
    sd_manager->AddNewDetector(detector);
    logical->SetSensitiveDetector(detector);

    G4RunManager::GetRunManager()->GeometryHasBeenModified();
}

G4VPhysicalVolume* DetectorConstruction::AddPhasespace(char* name, double radius, double z_position, char* material, bool kill)
{
    if (verbose >= 4)
        G4cout << "DetectorConstruction::AddPhasespace: " << name << G4endl;

//    G4Material* phasespace_material = nist_manager->FindOrBuildMaterial(material);
    
//    G4Box* phasespace_solid = new G4Box(name, radius, radius, 1*um);
//    G4LogicalVolume* phasespace_logical = new G4LogicalVolume(phasespace_solid, phasespace_material, name, 0, 0, 0);
//    G4VPhysicalVolume* phasespace_physical = new G4PVPlacement(0, G4ThreeVector(0, 0, z_position), 
//                                                               phasespace_logical, name, head_logical, false, 0);
//    phasespace_logical->SetVisAttributes(G4VisAttributes::Invisible); 

    G4VPhysicalVolume* phasespace_physical = AddSlab(name, radius, 1*nm, material, G4ThreeVector(0, 0, z_position),
                                                     G4ThreeVector(), G4Colour(1, 0, 1, 0.5), world_logical);

    // Active scoring area is 1% smaller than actual plane - avoids navigation errors when point on edge with direction (0,0,0)
    Phasespace* phasespace_sensitive_detector = new Phasespace(name, radius - (radius*0.01));
    phasespace_sensitive_detector->SetKillAtPlane(kill);
    this->phasespaces.push_back(phasespace_sensitive_detector);

    G4SDManager* sensitive_detector_manager = G4SDManager::GetSDMpointer();
    sensitive_detector_manager->AddNewDetector(phasespace_sensitive_detector);
    phasespace_physical->GetLogicalVolume()->SetSensitiveDetector(phasespace_sensitive_detector);

    return phasespace_physical;
}


void DetectorConstruction::RemovePhasespace(char* name) {
    if (verbose >=4)
        G4cout << "DetectorConstruction::RemovePhasespace" << G4endl;

    G4VPhysicalVolume* physical = FindVolume(name, world_physical);
    delete physical;
    G4RunManager::GetRunManager()->GeometryHasBeenModified();
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


G4VPhysicalVolume* DetectorConstruction::AddCylinder(char* name,
                                                   double radius, double thickness,
                                                   char* material,
                                                   G4ThreeVector translation,
                                                   G4ThreeVector rotation,
                                                   G4Colour colour,
                                                   G4LogicalVolume* mother_logical)
{
    if (verbose >= 4)
        G4cout << "DetectorConstruction::AddCylinder" << G4endl;

    G4Material* mat = nist_manager->FindOrBuildMaterial(material);
    
    G4RotationMatrix* rot = new G4RotationMatrix();
    rot->rotateX(rotation.x()*deg);
    rot->rotateY(rotation.y()*deg);
    rot->rotateZ(rotation.z()*deg);
   
    G4Tubs* solid = new G4Tubs(name, 0, radius, thickness/2., 0, 360*deg);
    G4LogicalVolume* logical = new G4LogicalVolume(solid, mat, name, 0, 0, 0);
    logical->SetVisAttributes(new G4VisAttributes(colour)); 

    G4VPhysicalVolume* physical = new G4PVPlacement(rot, translation,
                                                    logical, name, mother_logical,
                                                    false, 0);

    return physical;
}


void DetectorConstruction::UpdateCylinder(char* name,
                      double radius, double thickness,
                      char* material,
                      G4ThreeVector translation,
                      G4ThreeVector rotation)
{
    if (verbose >= 4)
        G4cout << "DetectorConstruction::UpdateCylinder" << G4endl;
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


void DetectorConstruction::UpdateSlab(char* name,
                      double side, double thickness,
                      char* material,
                      G4ThreeVector translation,
                      G4ThreeVector rotation)
{
    if (verbose >= 4)
        G4cout << "DetectorConstruction::UpdateSlab" << G4endl;
}

G4VPhysicalVolume* DetectorConstruction::AddCADComponent(char* name,
                                                   char* filename,
                                                   char* material,
                                                   double scale,
                                                   G4ThreeVector translation,
                                                   G4ThreeVector rotation,
                                                   G4Colour colour,
                                                   G4bool tessellated,
                                                   G4LogicalVolume* mother_logical)
{
    if (verbose >= 4)
        G4cout << "DetectorConstruction::AddCADComponent" << G4endl;

    G4Material* mat = nist_manager->FindOrBuildMaterial(material);

    G4RotationMatrix* rot = new G4RotationMatrix();
    rot->rotateX(rotation.x()*deg);
    rot->rotateY(rotation.y()*deg);
    rot->rotateZ(rotation.z()*deg);

    if (tessellated) {
        CADMesh* mesh = new CADMesh(filename, (char*) "STL", scale, G4ThreeVector(), false);
        G4VSolid* solid = mesh->TessellatedMesh();
        G4LogicalVolume* logical = new G4LogicalVolume(solid, mat, name, 0, 0, 0);
        logical->SetVisAttributes(new G4VisAttributes(colour)); 

        G4VPhysicalVolume* physical = new G4PVPlacement(rot, translation,
                                                        logical, name, mother_logical,
                                                        false, 0);
        return physical;

    } else {
        CADMesh * mesh = new CADMesh(filename, (char*) "PLY", mat);
        G4AssemblyVolume* assembly = mesh->TetrahedralMesh();
        G4Translate3D trans(translation.x(), translation.y(), translation.z());
        G4Transform3D rotation = G4Rotate3D(*rot);
        G4Transform3D transform = trans*rotation;
        assembly->MakeImprint(mother_logical, transform, 0, 0);
    }

    G4RunManager::GetRunManager()->GeometryHasBeenModified();
    return NULL;
}


void DetectorConstruction::TranslateCADComponent(char* name, G4ThreeVector translation, G4bool in_vacuum)
{
    if (verbose >=4)
        G4cout << "DetectorConstruction::TranslateCADComponent" << G4endl;

    if (in_vacuum == true) {
        translation -= vacuum_position;
        translation -= G4ThreeVector(0, 0, vacuum_length/2.);
    } else {
        translation -= head_position;
        translation -= G4ThreeVector(0, 0, head_length/2.);
    }

    G4VPhysicalVolume* physical = FindVolume(name, head_physical);
    physical->SetTranslation(translation);
    
    G4RunManager::GetRunManager()->GeometryHasBeenModified();
}


void DetectorConstruction::RotateCADComponent(char* name, G4ThreeVector rotation)
{
    if (verbose >= 4)
        G4cout << "DetectorConstruction::RotateCADComponent" << G4endl;

    G4VPhysicalVolume* physical = FindVolume(name, head_physical);

    G4RotationMatrix* rot = new G4RotationMatrix();
    rot->rotateX(rotation.x()*deg);
    rot->rotateY(rotation.y()*deg);
    rot->rotateZ(rotation.z()*deg);
    physical->SetRotation(rot);
    
    G4RunManager::GetRunManager()->GeometryHasBeenModified();
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

