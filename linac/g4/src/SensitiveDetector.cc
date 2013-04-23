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

#include "SensitiveDetector.hh"
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

#include "boost/python.hpp"
#include "pyublas/numpy.hpp"
#include <vector>


SensitiveDetector::SensitiveDetector(const G4String& name) : G4VSensitiveDetector(name) {

    debug = false;

    x_dim = 501;
    y_dim = x_dim;
    z_dim = 500;

    x_min = 0;
    y_min = 0;
    z_min = 0;

    x_max = x_dim;
    y_max = y_dim;
    z_max = z_dim;

    x_res = 1*mm;
    y_res = x_res;
    z_res = x_res;

    volume = x_res*y_res*z_res;

    detector_construction = (DetectorConstruction*) (G4RunManager::GetRunManager()->GetUserDetectorConstruction());

    npy_intp dims[] = {x_max - x_min, y_max - y_min, z_max - z_min};

    energy_histogram = pyublas::numpy_vector<float> (3, dims);
    std::fill(energy_histogram.begin(), energy_histogram.end(), 0.0);

    energysq_histogram = pyublas::numpy_vector<float> (3, dims);
    std::fill(energysq_histogram.begin(), energysq_histogram.end(), 0.0);

    counts_histogram = pyublas::numpy_vector<float> (3, dims);
    std::fill(counts_histogram.begin(), counts_histogram.end(), 0.0);
}

SensitiveDetector::~SensitiveDetector() {
}

void SensitiveDetector::Initialize(G4HCofThisEvent*) {
}

G4bool SensitiveDetector::ProcessHits(G4Step* aStep, G4TouchableHistory* touchable) {
    const G4Track* aTrack = aStep->GetTrack();
    const G4String name = aTrack->GetDefinition()->GetParticleName();

    G4double energy_deposit = aStep->GetTotalEnergyDeposit();

    if(energy_deposit == 0) {
        if (debug) G4cout << "No enegy to deposit." << G4endl << G4endl;
        return false;
    }

    G4double voxel_mass = aTrack->GetMaterial()->GetDensity() * volume;


    G4ThreeVector position = aTrack->GetPosition();

    int x_index = std::floor((position.x() + (x_dim/2. * x_res)) / x_res);
    int y_index = std::floor((position.y() + (y_dim/2. * y_res)) / y_res);
    //int z_index = std::floor((position.z() + 300*mm + (z_dim/2. * z_res)) / z_res);
    int z_index = std::floor((position.z() + (z_dim/2. * z_res)) / z_res);

    if (debug) {
        G4cout << "Solid name:       " << aTrack->GetVolume()->GetLogicalVolume()->GetName() << G4endl;
        G4cout << "Total energy:     " << aTrack->GetTotalEnergy() << G4endl;
        G4cout << "Enegy to deposit: " << energy_deposit << " MeV" << G4endl;
        G4cout << "Voxel material:   " << aTrack->GetMaterial()->GetName() << G4endl;
        G4cout << "Voxel mass:       " << voxel_mass << G4endl;
        G4cout << "Voxel volume:     " << volume << G4endl;
        G4cout << "Position: "
               << position.x() << " "
               << position.y() << " "
               << position.z() << " " << G4endl;
        G4cout << "Histogram index: "
               << x_index << " "
               << y_index << " "
               << z_index << " " << G4endl;
        if (x_index != std::abs(x_index)) {
            G4cout << "Error: X index" << G4endl;
        }
        if (y_index != std::abs(y_index)) {
            G4cout << "Error: Y index" << G4endl;
        }
        if (z_index != std::abs(z_index)) {
            G4cout << "Error: Z index" << G4endl;
        }
    }


    if (x_index < x_min || x_index >= x_max) {
        if (debug) {G4cout << "Out of bounds: X index" << G4endl;}
        return false;
    }
    if (y_index < y_min || y_index >= y_max) {
        if (debug) {G4cout << "Out of bounds: Y index" << G4endl;}
        return false;
    }
    if (z_index < z_min || z_index >= z_max) {
        if (debug) {G4cout << "Out of bounds: Z index" << G4endl;}
        return false;
    }

    x_index -= x_min;
    y_index -= y_min;
    z_index -= z_min;

    if (debug){
        G4cout << "New index: " << x_index << " " << y_index << " " << z_index << " " << G4endl;
    }
    energy_histogram.sub(x_index, y_index, z_index) += energy_deposit;
    energysq_histogram.sub(x_index, y_index, z_index) += std::pow(energy_deposit, 2.);
    counts_histogram.sub(x_index, y_index, z_index) += aTrack->GetWeight();
    
    if (debug) G4cout << G4endl;

    return true;
}

void SensitiveDetector::EndOfEvent(G4HCofThisEvent*) {
}

void SensitiveDetector::clear() {
}

void SensitiveDetector::PrintAll() {
}
