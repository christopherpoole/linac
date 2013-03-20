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
#include "PrimaryGeneratorAction.hh"

// GEANT4 //
#include "globals.hh"
#include "Randomize.hh"

#include "G4RunManager.hh"
#include "G4Event.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"


PrimaryGeneratorAction::PrimaryGeneratorAction()
{
    phasespace_particle_gun = new G4ParticleGun();
    particle_gun = new G4GeneralParticleSource();

    G4ParticleTable* particle_table = G4ParticleTable::GetParticleTable();
    G4ParticleDefinition* particle = particle_table->FindParticle("e-");

    electron = particle_table->FindParticle("e-");
    gamma = particle_table->FindParticle("gamma");
    positron = particle_table->FindParticle("e+");
 
    particle_gun->GetCurrentSource()->SetParticleDefinition(particle);
    particle_gun->GetCurrentSource()->GetPosDist()->SetPosDisType("Beam");
    particle_gun->GetCurrentSource()->GetPosDist()->SetBeamSigmaInR(1*mm);
    particle_gun->GetCurrentSource()->GetEneDist()->SetMonoEnergy(6*MeV);
 
//    particle_gun->SetParticleDefinition(particle);
//    particle_gun->SetParticlePosition(G4ThreeVector(0., 0., 1005.));
//    particle_gun->SetParticleEnergy(6.*MeV);
//    particle_gun->SetParticleMomentumDirection(G4ThreeVector(0.,0.,-1.));

    rotation = G4ThreeVector();

    recycling_number = 1;
    redistribute = false;
    phasespace_record_repeat = 0;
    count = 0;
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    if (from_phasespace) {
        GeneratePhasespacePrimaries(event);
    } else {
        particle_gun->GeneratePrimaryVertex(event);
    }
}

void PrimaryGeneratorAction::GeneratePhasespacePrimaries(G4Event* event)
{
    if (phasespace_record_repeat == 0) {

        phasespace_record = PhasespaceRecord();
        try {
            *phasespace_archive >> phasespace_record;
        } catch (...) {
            G4cout << "ABORTING RUN" << G4endl;
            G4cout << "Particles in phasespace: " << count << G4endl;

            phasespace_record_repeat = 0;
            count = 0;

            G4RunManager* run_manager = G4RunManager::GetRunManager();
            run_manager->AbortRun(); 
        }
  
        G4ParticleDefinition* particle = gamma; 

        if (phasespace_record.GetParticleType() == -1) {
            particle = electron;
        } else if (phasespace_record.GetParticleType() == 1) {
            particle = positron;
        } 

        phasespace_particle_gun->SetParticleDefinition(particle);
        phasespace_particle_gun->SetParticleEnergy(phasespace_record.GetKineticEnergy());
    }

    phasespace_record_repeat++;
    count++;

    if (phasespace_record_repeat  > recycling_number) {
        phasespace_record_repeat = 0;
    }

    G4ThreeVector pos = G4ThreeVector(phasespace_record.GetPosition());
    G4ThreeVector mom = G4ThreeVector(phasespace_record.GetMomentum());

    if (redistribute) {
        G4double angle = G4UniformRand() * 360.0*deg;

        pos.rotateZ(angle);
        mom.rotateZ(angle);
    }
    
    // gantry rotation correction
    pos.rotateX(-rotation.y()*deg);
    mom.rotateX(-rotation.y()*deg);

    phasespace_particle_gun->SetParticlePosition(pos);
    phasespace_particle_gun->SetParticleMomentumDirection(mom);

    phasespace_particle_gun->GeneratePrimaryVertex(event);
    event->GetPrimaryVertex()->SetWeight(phasespace_record.GetWeight());
}

