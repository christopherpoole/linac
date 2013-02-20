//
// ********************************************************************
// * DISCLAIMER                                                       *
// *                                                                  *
// * The following disclaimer summarizes all the specific disclaimers *
// * of contributors to this software. The specific disclaimers,which *
// * govern, are listed with their locations in:                      *
// *   http://cern.ch/geant4/license                                  *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.                                                             *
// *                                                                  *
// * This  code  implementation is the  intellectual property  of the *
// * GEANT4 collaboration.                                            *
// * By copying,  distributing  or modifying the Program (or any work *
// * based  on  the Program)  you indicate  your  acceptance of  this *
// * statement, and all its terms.                                    *
// ********************************************************************
//
//
// Jane Tinslay, March 2006
//
#include "BremSplittingProcess.hh"
//#include "BremSplittingProcessMessenger.hh"//used for UI command to change splitting number

#include "G4Track.hh"
#include "G4VParticleChange.hh"
#include <assert.h>
#include <vector>

//#include "BremSplittingProcessMessenger.hh"

// Initialise static data
//G4int BremSplittingProcess::fNSplit = 10;
G4int BremSplittingProcess::fNSecondaries = 10;

G4bool BremSplittingProcess::fActive = true;

BremSplittingProcess::BremSplittingProcess() {

    fNSplit = 10;//unless specified by UI command, will be 10 (ie no splitting

//    bremMessenger = new BremSplittingProcessMessenger(this);//instantiate messenger class for UI commands

}

BremSplittingProcess::~BremSplittingProcess() {

    if(bremMessenger)
        delete bremMessenger;

}

G4VParticleChange* BremSplittingProcess::PostStepDoIt(const G4Track& track, const G4Step& step)
{
  // Just do regular processing if brem splitting is not activated
  G4VParticleChange* particleChange(0);

  if (!fActive) {
    particleChange = pRegProcess->PostStepDoIt(track, step);
    assert (0 != particleChange);

    fNSecondaries += particleChange->GetNumberOfSecondaries();

    return particleChange;
  }
  
  // Do brem splitting
  assert (fNSplit > 0);

  G4int i(0);
  G4double weight = track.GetWeight()/fNSplit;
  
  // Secondary store
  std::vector<G4Track*> secondaries;
  secondaries.reserve(fNSplit);
    
  // Loop over PostStepDoIt method to generate multiple secondaries.
  for (i=0; i<fNSplit; i++) {    
    particleChange = pRegProcess->PostStepDoIt(track, step);

    assert (0 != particleChange);
    particleChange->SetVerboseLevel(0);

    G4int j(0);

    for (j=0; j<particleChange->GetNumberOfSecondaries(); j++) {
      secondaries.push_back(new G4Track(*(particleChange->GetSecondary(j))));
    }
  }	

  // Configure particleChange to handle multiple secondaries. Other 
  // data is unchanged
  particleChange->SetNumberOfSecondaries(secondaries.size());
  particleChange->SetSecondaryWeightByProcess(true);

  // Add all secondaries 
  std::vector<G4Track*>::iterator iter = secondaries.begin();

  while (iter != secondaries.end()) {
    G4Track* myTrack = *iter;
    myTrack->SetWeight(weight);

    // particleChange takes ownership
    particleChange->AddSecondary(myTrack); 
    
    iter++;
  }

  fNSecondaries += secondaries.size();

  return particleChange;
}

void BremSplittingProcess::SetNSplit(G4int nSplit) 
{
  fNSplit = nSplit;
}

void BremSplittingProcess::SetIsActive(G4bool active) 
{
  fActive = active;
}

G4bool BremSplittingProcess::GetIsActive() 
{
  return fActive;
}

G4int BremSplittingProcess::GetNSplit() 
{
  return fNSplit;
}

G4int BremSplittingProcess::GetNSecondaries() 
{
  return fNSecondaries;
}
