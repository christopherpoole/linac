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


    x_dim = 101; // Chosen so that central axis has voxel column
    y_dim = 101;
    z_dim = 100;

    x_min = -75.75 * mm;
    y_min = -75.75 * mm;
    z_min = -50 * mm;

    x_max = 75.75 * mm;
    y_max = 75.75 * mm;
    z_max = 0 * mm;
    
    x_res = (x_max - x_min) / (x_dim);
    y_res = (y_max - y_min) / (y_dim);
    z_res = (z_max - z_min) / (z_dim);

    volume = x_res*y_res*z_res;

    detector_construction = (DetectorConstruction*) (G4RunManager::GetRunManager()->GetUserDetectorConstruction());

    npy_intp dims[] = {x_dim, y_dim,  z_dim};

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

/*
G4bool SensitiveDetector::ProcessHits(G4Step* aStep, G4TouchableHistory* touchable) {

    return false;
}*/


G4bool SensitiveDetector::ProcessHits(G4Step* aStep, G4TouchableHistory* touchable) {
    const G4Track* aTrack = aStep->GetTrack();
    const G4String name = aTrack->GetDefinition()->GetParticleName();

    G4double energy_deposit = aStep->GetTotalEnergyDeposit();

    if(energy_deposit == 0) {
        return false;
    }

    G4double voxel_mass = aTrack->GetMaterial()->GetDensity() * volume;
    G4ThreeVector position = aTrack->GetPosition();
    
    
    if (position.x() < x_min || position.x() >= x_max) {
        return false;
    }
    
    if (position.y() < y_min || position.y() >= y_max) {
        return false;
    }
    
    if (position.z() < z_min || position.z() >= z_max) {
        return false;
    }

    int x_index = std::floor((position.x() - x_min) / x_res);
    int y_index = std::floor((position.y() - y_min) / y_res);
    int z_index = std::floor((position.z() - z_min) / z_res);
    
    /*
    Hacky debugging
    
    if (x_index > 100 || x_index < 0) {
        energy_histogram.sub(1, 0, 0) += position.x();
    }
        
    if (y_index > 100 || y_index < 0) {
        energy_histogram.sub(0, 1, 0) += position.y();
    }
        
    if (z_index > 4999 || z_index < 0) {
        energy_histogram.sub(0, 0, 1) += position.z();
    }*/
    
    energy_histogram.sub(x_index, y_index, z_index) += energy_deposit/voxel_mass;
    energysq_histogram.sub(x_index, y_index, z_index) += std::pow(energy_deposit, 2.);
    counts_histogram.sub(x_index, y_index, z_index) += aTrack->GetWeight();
    
    return true;
}


void SensitiveDetector::EndOfEvent(G4HCofThisEvent*) {
}

void SensitiveDetector::clear() {
}

void SensitiveDetector::PrintAll() {
}
