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
#ifndef BREMSPLITTINGPROCESS_HH
#define BREMSPLITTINGPROCESS_HH 1

#include "G4WrapperProcess.hh"

class BremSplittingProcessMessenger;

class BremSplittingProcess : public G4WrapperProcess {
  
public:

  BremSplittingProcess();	
  
  virtual ~BremSplittingProcess();	
  
  // Override PostStepDoIt  method
  G4VParticleChange* PostStepDoIt(const G4Track& track, const G4Step& step);
  
  // Modifiers
   void SetNSplit(G4int);
  static void SetIsActive(G4bool);

  // Accessors
  static G4bool GetIsActive();
   G4int GetNSplit();
  static G4int GetNSecondaries();


private:
  
  // Data members
//  static G4int fNSplit;
   G4int fNSplit;
  static G4int fNSecondaries;
  static G4bool fActive;

  BremSplittingProcessMessenger* bremMessenger;

};

#endif
