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


#ifndef PrimaryGeneratorAction_h
#define PrimaryGeneratorAction_h 1

#include "PhasespaceRecord.hh"

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4GeneralParticleSource.hh"
#include "G4ParticleGun.hh"

#include "boost/archive/binary_iarchive.hpp"
#include "boost/archive/binary_oarchive.hpp"

class G4GeneralParticleSource;
class G4Event;

/*
class PhasespaceRecordStream {
  public:
    PhasespaceRecordStream(char* filename) {//: input_file_stream(filename) , phasespace_archive(input_file_stream) {
        input_file_stream = new std::ifstream(filename);
        phasespace_archive = new boost::archive::binary_iarchive(*input_file_stream);
        
        current_record = new PhasespaceRecord();
    };

   ~PhasespaceRecordStream() {
        delete input_file_stream;
        delete phasespace_archive;
        delete current_record;
    };

    PhasespaceRecord* GetNextRecord() {
        G4cout << "reading record... ";
        *phasespace_archive >> current_record; 
        G4cout << "done." << G4endl;

        return current_record;
    }

  public:
    PhasespaceRecord* current_record;

  private:
    std::ifstream* input_file_stream;
    boost::archive::binary_iarchive* phasespace_archive;
};
*/

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
    public:
        PrimaryGeneratorAction();
        ~PrimaryGeneratorAction();

        void SetPosition(G4ThreeVector position) {
            particle_gun->GetCurrentSource()->GetPosDist()->SetCentreCoords(position);
        };

        void SetDirection(G4ThreeVector direction) {
            particle_gun->GetCurrentSource()->GetAngDist()->
                SetParticleMomentumDirection(direction);
        };

        void SetEnergy(G4double energy) {
            particle_gun->GetCurrentSource()->GetEneDist()->SetMonoEnergy(energy);
        };

        void SetFWHM(G4double fwhm) {
            particle_gun->GetCurrentSource()->GetPosDist()->SetBeamSigmaInR(fwhm); 
        };

        void SetRecyclingNumber(G4int number) {
            recycling_number = number;
        }

        void SetRedistribute(G4bool flag) {
            redistribute = flag;
            G4cout << "setting redistribute flag to " << redistribute << G4endl;
        }   

        void SetGantryRotation(G4ThreeVector rotation) {
            G4cout << "Setting gantry rotation: " << rotation << G4endl;
            this->rotation = rotation;
        };

        void SetSource(char* phasespace) {
            if (phasespace == NULL) {
                G4cout << "Not using phasespace file as particle source, running from GPS" << G4endl;
                from_phasespace = false;
            } else {
                from_phasespace = true;
                G4cout << "Using phasespace file as particle source: " << phasespace << G4endl;
                
                input_file_stream = new std::ifstream(phasespace);
                phasespace_archive = new boost::archive::binary_iarchive(*input_file_stream);

                particle_gun->GetCurrentSource()->GetPosDist()->SetPosDisType("Point");
                phasespace_record_repeat = -1;
            }
        }
    
    public:
        void GeneratePrimaries(G4Event* event);
        void GeneratePhasespacePrimaries(G4Event* event);
        
    private:
        G4ParticleGun* phasespace_particle_gun;
        G4GeneralParticleSource* particle_gun;

        G4ThreeVector rotation;

        G4bool from_phasespace;

        // Not in the constructor, so we need pointers.
        std::ifstream* input_file_stream;
        boost::archive::binary_iarchive* phasespace_archive;
    
        G4ParticleDefinition* electron;
        G4ParticleDefinition* gamma;
        G4ParticleDefinition* positron;

        G4int recycling_number;
        G4bool redistribute;

        PhasespaceRecord phasespace_record;
        G4int phasespace_record_repeat;

        G4int count;
};

#endif

