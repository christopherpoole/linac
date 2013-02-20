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

// THIRD PARTY //
#include "CADMesh.hh"
//#include "G4VoxelData.hh"

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
    nist_manager = G4NistManager::Instance();
    use_phantom = false;
    region = NULL;
}

DetectorConstruction::~DetectorConstruction()
{
}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
//    G4SolidStore::GetInstance()->Clean();
//    G4LogicalVolumeStore::GetInstance()->Clean();
//    G4PhysicalVolumeStore::GetInstance()->Clean();

    G4NistManager* nist_manager = G4NistManager::Instance();
    G4Material* air = nist_manager->FindOrBuildMaterial("G4_AIR");

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


    world_solid = new G4Box("world_solid", 2*m, 2*m, 2*m);
    world_logical = new G4LogicalVolume(world_solid, air, "world_logical", 0, 0, 0);
    world_physical = new G4PVPlacement(0, G4ThreeVector(), world_logical, 
                                       "world_physical", 0, false, 0);
    world_logical->SetVisAttributes(G4VisAttributes::Invisible); 

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

void DetectorConstruction::SetupHead(G4double head_radius,
                                     G4double head_length,
                                     G4ThreeVector head_position,
                                     G4ThreeVector head_rotation,
                                     G4double vacuum_radius,
                                     G4double vacuum_length,
                                     G4ThreeVector vacuum_position)
{
    this->head_length = head_length;
    this->head_position = G4ThreeVector(head_position);
    head_position += G4ThreeVector(0, 0, head_length/2.);

    head_position.rotate(0, head_rotation.y()*deg, 0);
    
    this->head_rotation = new G4RotationMatrix();
    //this->head_rotation->rotateZ(head_rotation.z()*deg);
    this->head_rotation->rotateX(head_rotation.y()*deg);
    //this->head_rotation->rotateX(head_rotation.x()*deg);

    G4Material* head_material = nist_manager->FindOrBuildMaterial("G4_AIR");
    head_solid = new G4Tubs("head_solid", 0*mm, head_radius*mm, head_length/2.*mm,
                            0*deg, 360*deg);
    head_logical = new G4LogicalVolume(head_solid, head_material, "head_logical", 0, 0, 0);
    head_physical = new G4PVPlacement(this->head_rotation, head_position, head_logical, "head_physical",
                                      world_logical, false, 0);
//    head_logical->SetVisAttributes(G4VisAttributes::Invisible); 
    
    this->vacuum_length = vacuum_length;
    this->vacuum_position = vacuum_position;
    vacuum_position -= (this->head_position + G4ThreeVector(0, 0, head_length/2.));
    vacuum_position += G4ThreeVector(0, 0, vacuum_length/2.);
//    vacuum_position.rotate(head_rotation.x()*deg, head_rotation.y()*deg, head_rotation.z()*deg);
    
    G4Material* vacuum_material = nist_manager->FindOrBuildMaterial("G4_Galactic");

    vacuum_solid = new G4Tubs("vacuum_solid", 0*mm, vacuum_radius*mm, vacuum_length/2.*mm,
                              0*deg, 360*deg);
    vacuum_logical = new G4LogicalVolume(vacuum_solid, vacuum_material, "vacuum_logical",
                                         0, 0, 0);
    vacuum_physical = new G4PVPlacement(0, vacuum_position, vacuum_logical, "vacuum_physical",
                                        head_logical, false, 0);
//    vacuum_logical->SetVisAttributes(G4VisAttributes::Invisible); 

    sheild_solid = new G4Tubs("sheild_solid", head_radius*mm,
                               (head_radius+1)*mm, head_length/2.*mm, 0*deg, 360*deg);
    sheild_logical = new G4LogicalVolume(sheild_solid, head_material, "sheild_logical",
                                         0, 0, 0);
    sheild_physical = new G4PVPlacement(this->head_rotation, head_position, sheild_logical, "sheild_physical",
                                        world_logical, false, 0);
//    sheild_logical->SetVisAttributes(G4VisAttributes::Invisible); 

    if (use_phantom) {
        SetupPhantom();
    }

	StopKillSheild* sheild_sensitive_detector = new StopKillSheild("sheild");
    G4SDManager* sensitive_detector_manager = G4SDManager::GetSDMpointer();
    sensitive_detector_manager->AddNewDetector(sheild_sensitive_detector);
    sheild_logical->SetSensitiveDetector(sheild_sensitive_detector);
}


void DetectorConstruction::SetupPhantom() {
    G4Material* water = nist_manager->FindOrBuildMaterial("G4_WATER");

    phantom_solid = new G4Box("phantom_solid", 200*mm, 200*mm, 200*mm);
    phantom_logical = new G4LogicalVolume(phantom_solid, water, "phantom_logical", 0, 0, 0);
    phantom_physical = new G4PVPlacement(0, G4ThreeVector(0, 0, 0), phantom_logical, 
                                       "phantom_physical", world_logical, false, 0);
//    phantom_logical->SetVisAttributes(new G4VisAttributes(G4Colour(0, 0.6, 0.9, 1))); 

    detector = new SensitiveDetector("phantom_detector");
    G4SDManager* sd_manager = G4SDManager::GetSDMpointer();
    sd_manager->AddNewDetector(detector);
    phantom_logical->SetSensitiveDetector(detector);
}


G4VPhysicalVolume* DetectorConstruction::AddPhasespace(char* name, double radius, double z_position, char* material)
{
//    G4Material* phasespace_material = nist_manager->FindOrBuildMaterial(material);
    
//    G4Box* phasespace_solid = new G4Box(name, radius, radius, 1*um);
//    G4LogicalVolume* phasespace_logical = new G4LogicalVolume(phasespace_solid, phasespace_material, name, 0, 0, 0);
//    G4VPhysicalVolume* phasespace_physical = new G4PVPlacement(0, G4ThreeVector(0, 0, z_position), 
//                                                               phasespace_logical, name, head_logical, false, 0);
//    phasespace_logical->SetVisAttributes(G4VisAttributes::Invisible); 

    G4VPhysicalVolume* phasespace_physical = AddSlab(name, radius, 1*nm, material, false, G4ThreeVector(0, 0, z_position),
                                                     G4ThreeVector(), G4Colour(1, 0, 1, 0.5));

    // Active scoring area is 1% smaller than actual plane - avoids navigation errors when point on edge with direction (0,0,0)
	Phasespace* phasespace_sensitive_detector = new Phasespace(name, radius - (radius*0.01));
    G4SDManager* sensitive_detector_manager = G4SDManager::GetSDMpointer();
    sensitive_detector_manager->AddNewDetector(phasespace_sensitive_detector);
    phasespace_physical->GetLogicalVolume()->SetSensitiveDetector(phasespace_sensitive_detector);

    return phasespace_physical;
}

void DetectorConstruction::AddMaterial(G4String name, G4double density,
                                        boost::python::object move)
{
    G4cout << "adding material " << name << " with density : " << density << G4endl;
    boost::python::object a = move();
    int b = boost::python::extract<int>(a);
    G4cout << b << G4endl;
}


G4VPhysicalVolume* DetectorConstruction::AddCylinder(char* name,
                                                   double radius, double thickness,
                                                   char* material,
                                                   G4bool in_vacuum,
                                                   G4ThreeVector translation,
                                                   G4ThreeVector rotation,
                                                   G4Colour colour)
{
    G4Material* mat = nist_manager->FindOrBuildMaterial(material);
    
    G4LogicalVolume* mother_logical;
    if (in_vacuum == true) {
        G4cout << "added volume to vacuum" << G4endl;
        
        translation -= vacuum_position;
        translation -= G4ThreeVector(0, 0, vacuum_length/2.);
        mother_logical = vacuum_logical;
    } else {
        translation -= head_position;
        translation -= G4ThreeVector(0, 0, head_length/2.);
        mother_logical = head_logical;
    }

    G4RotationMatrix* rot = new G4RotationMatrix();
    rot->rotateX(rotation.x()*deg);
    rot->rotateY(rotation.y()*deg);
    rot->rotateZ(rotation.z()*deg);
   
    G4cout << name << G4endl;
 
    G4Tubs* solid = new G4Tubs(name, 0, radius, thickness/2., 0, 360*deg);
    G4LogicalVolume* logical = new G4LogicalVolume(solid, mat, name, 0, 0, 0);
    logical->SetVisAttributes(new G4VisAttributes(colour)); 

    G4VPhysicalVolume* physical = new G4PVPlacement(rot, translation,
                                                    logical, name, mother_logical,
                                                    false, 0);

    return physical;
}

G4VPhysicalVolume* DetectorConstruction::AddSlab(char* name,
                                                   double side, double thickness,
                                                   char* material,
                                                   G4bool in_vacuum,
                                                   G4ThreeVector translation,
                                                   G4ThreeVector rotation,
                                                   G4Colour colour)
{
    G4Material* mat = nist_manager->FindOrBuildMaterial(material);
    
    G4LogicalVolume* mother_logical;
    if (in_vacuum == true) {
        G4cout << "added volume to vacuum" << G4endl;
        
        translation -= vacuum_position;
        translation -= G4ThreeVector(0, 0, vacuum_length/2.);
        mother_logical = vacuum_logical;
    } else {
        translation -= head_position;
        translation -= G4ThreeVector(0, 0, head_length/2.);
        mother_logical = head_logical;
    }

    G4RotationMatrix* rot = new G4RotationMatrix();
    rot->rotateX(rotation.x()*deg);
    rot->rotateY(rotation.y()*deg);
    rot->rotateZ(rotation.z()*deg);
   
    G4cout << name << G4endl;
 
    G4Box* solid = new G4Box(name, side/2., side/2., thickness/2.);
    G4LogicalVolume* logical = new G4LogicalVolume(solid, mat, name, 0, 0, 0);
    logical->SetVisAttributes(new G4VisAttributes(colour)); 

    G4VPhysicalVolume* physical = new G4PVPlacement(rot, translation,
                                                    logical, name, mother_logical,
                                                    false, 0);

    return physical;
}



G4VPhysicalVolume* DetectorConstruction::AddCADComponent(char* name,
                                                   char* filename,
                                                   char* material,
                                                   G4bool in_vacuum,
                                                   double scale,
                                                   G4ThreeVector translation,
                                                   G4ThreeVector rotation,
                                                   G4Colour colour,
                                                   G4bool tessellated)
{
    G4Material* mat = nist_manager->FindOrBuildMaterial(material);

    G4LogicalVolume* mother_logical;
    if (in_vacuum == true) {
        G4cout << "added volume to vacuum" << G4endl;
        
        translation -= vacuum_position;
        translation -= G4ThreeVector(0, 0, vacuum_length/2.);
        mother_logical = vacuum_logical;
    } else {
        translation -= head_position;
        translation -= G4ThreeVector(0, 0, head_length/2.);
        mother_logical = head_logical;
    }

    G4RotationMatrix* rot = new G4RotationMatrix();
    rot->rotateX(rotation.x()*deg);
    rot->rotateY(rotation.y()*deg);
    rot->rotateZ(rotation.z()*deg);

    if (tessellated) {
        CADMesh* mesh = new CADMesh(filename, "STL", scale, G4ThreeVector(), false);
        G4VSolid* solid = mesh->TessellatedMesh();
        G4LogicalVolume* logical = new G4LogicalVolume(solid, mat, name, 0, 0, 0);
        logical->SetVisAttributes(new G4VisAttributes(colour)); 

        G4VPhysicalVolume* physical = new G4PVPlacement(rot, translation,
                                                        logical, name, mother_logical,
                                                        false, 0);
    } else {
        CADMesh * mesh = new CADMesh(filename, "STL", mat);
        G4AssemblyVolume* assembly = mesh->TetrahedralMesh();
        G4Translate3D trans(translation.x(), translation.y(), translation.z());
        G4Transform3D rotation = G4Rotate3D(*rot);
        G4Transform3D transform = trans*rotation;
        assembly->MakeImprint(mother_logical, transform, 0, 0);
    }

    G4RunManager::GetRunManager()->GeometryHasBeenModified();
    return NULL;
}

