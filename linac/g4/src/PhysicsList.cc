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


// USER//
#include "PhysicsList.hh"

// GEANT4 //
//#include "G4EmStandardPhysics_option1.hh"
//#include "G4EmStandardPhysics_option2.hh"
//#include "G4EmStandardPhysics_option3.hh"
//#include "G4EmDNAPhysics.hh"
//#include "G4EmLivermorePhysics.hh"
//#include "G4EmPenelopePhysics.hh"

#include "G4RegionStore.hh"

#include "BremSplittingProcess.hh"

#include "G4ParticleDefinition.hh"
#include "G4ParticleWithCuts.hh"
#include "G4ProcessManager.hh"
#include "G4ProcessVector.hh"

#include "G4ProductionCuts.hh"
#include "G4ProductionCutsTable.hh"

#include "G4ParticleTypes.hh"

//includes for phsyics processes
#include "G4ComptonScattering.hh"
#include "G4PhotoElectricEffect.hh"
#include "G4GammaConversion.hh"
#include "G4RayleighScattering.hh"

#include "G4eMultipleScattering.hh"

#include "G4eIonisation.hh"
#include "G4eBremsstrahlung.hh"
#include "G4eplusAnnihilation.hh"

// e+
#include "G4eIonisation.hh"
#include "G4PenelopeIonisationModel.hh"
#include "G4eBremsstrahlung.hh"
#include "G4eplusAnnihilation.hh"


//include penelope models

#include "G4PenelopeRayleighModel.hh"
#include "G4PenelopeComptonModel.hh"
#include "G4PenelopePhotoElectricModel.hh"
#include "G4PenelopeGammaConversionModel.hh"
#include "G4PenelopeIonisationModel.hh"
#include "G4PenelopeBremsstrahlungModel.hh"
#include "G4PenelopeAnnihilationModel.hh"

#include "G4LivermoreBremsstrahlungModel.hh"
#include "G4LivermoreIonisationModel.hh"
#include "G4LivermoreGammaConversionModel.hh"
#include "G4LivermoreComptonModel.hh"
#include "G4LivermorePhotoElectricModel.hh"
#include "G4LivermoreRayleighModel.hh"

#include "G4StepLimiter.hh"
#include "G4EmProcessOptions.hh"

#include "G4PhotoNuclearProcess.hh"
#include "G4GammaNuclearReaction.hh"
#include "G4hMultipleScattering.hh"
#include "G4ionIonisation.hh"
#include "G4hIonisation.hh"

#include "G4RayleighScattering.hh"
//#include "G4PEEffectFluoModel.hh"
//#include "G4KleinNishinaModel.hh"


PhysicsList::PhysicsList()
{

    defaultCutValue = 1*cm;

    //RegisterPhysics(new G4EmStandardPhysics_option1());
    //RegisterPhysics(new G4EmStandardPhysics_option2());
    //RegisterPhysics(new G4EmStandardPhysics_option3());
    //RegisterPhysics(new G4EmLivermorePhysics());
    //RegisterPhysics(new G4EmPenelopePhysics());
}

PhysicsList::~PhysicsList()
{;}

void PhysicsList::ConstructParticle()
{
    //G4VModularPhysicsList::ConstructParticle();

    G4Gamma::GammaDefinition();
    G4Electron::ElectronDefinition();
    G4Positron::PositronDefinition();
    G4Proton::ProtonDefinition();//needed for range cuts apparently!!!!!!!!!

}

void PhysicsList::ConstructProcess()
{
    //G4VModularPhysicsList::ConstructProcess();

    AddTransportation();

    theParticleIterator->reset();

    while ( (*theParticleIterator)()){

        G4ParticleDefinition* particle = theParticleIterator->value();
        G4ProcessManager* pmanager = particle->GetProcessManager();
        G4String particleName = particle->GetParticleName();

        if(particleName == "gamma"){
            //G4EmStandardPhysics_option3            
            G4PhotoElectricEffect* pe = new G4PhotoElectricEffect;
            //pe->SetModel(new G4PEEffectFluoModel());
            G4ComptonScattering* cs   = new G4ComptonScattering;
            //cs->SetModel(new G4KleinNishinaModel());
            pmanager->AddDiscreteProcess(pe);
            pmanager->AddDiscreteProcess(cs);
            pmanager->AddDiscreteProcess(new G4GammaConversion);
           // pmanager->AddDiscreteProcess(new G4RayleighScattering);
        } else if (particleName == "e-"){
            //G4EmStandardPhysics_option3
            G4eMultipleScattering* msc = new G4eMultipleScattering();
            //msc->AddEmModel(0, new G4UrbanMscModel93());
            msc->SetStepLimitType(fUseDistanceToBoundary);
            pmanager->AddProcess(msc,                   -1, 1, 1);
            G4eIonisation* eIoni = new G4eIonisation();
            eIoni->SetStepFunction(0.2, 100*um);
            pmanager->AddProcess(eIoni,                 -1, 2, 2);
            G4eBremsstrahlung* bremProcess = new G4eBremsstrahlung();
            BremSplittingProcess* bremSplitting = new BremSplittingProcess();
            bremSplitting->RegisterProcess(bremProcess);
            pmanager->AddProcess(bremSplitting, -1,-3, 3);
            pmanager->AddProcess(new G4StepLimiter, -1, -1, 4);//step limiter required for phantom
        } else if (particleName == "e+"){
            //G4EmStandardPhysics_option3
            G4eMultipleScattering* msc = new G4eMultipleScattering();
            //msc->AddEmModel(0, new G4UrbanMscModel93());
            msc->SetStepLimitType(fUseDistanceToBoundary);
            pmanager->AddProcess(msc,                   -1, 1, 1);
            G4eIonisation* eIoni = new G4eIonisation();
            eIoni->SetStepFunction(0.2, 100*um);
            pmanager->AddProcess(eIoni,                 -1, 2, 2);
            pmanager->AddProcess(new G4eBremsstrahlung, -1,-3, 3);
            pmanager->AddProcess(new G4eplusAnnihilation,0,-1, 4);
        }
    }

    //G4EmStandardPhysics_option3
    // activate range cuts!!! THIS IS A BUG AND SHOULD BE FIXED BY GEANT4 DUDES
    //MACRO COMMANDS DON'T WORK1!!
    G4EmProcessOptions emOptions;
    emOptions.SetApplyCuts(true);

    // Multiple Coulomb scattering
    //???
    emOptions.SetPolarAngleLimit(0.2);

    // Physics tables
    //
    emOptions.SetMinEnergy(100*eV);
    emOptions.SetMaxEnergy(100*MeV);
    //opt.SetDEDXBinning(100);
    //opt.SetLambdaBinning(100);

}


void PhysicsList::SetCuts()
{
    SetCutsWithDefault();
/*
    G4Region* region = G4RegionStore::GetInstance()->GetRegion("ShortCuts");

    G4ProductionCuts* cuts = new G4ProductionCuts();
    cuts->SetProductionCut(0.1*mm,G4ProductionCuts::GetIndex("gamma"));
    cuts->SetProductionCut(0.1*mm,G4ProductionCuts::GetIndex("e-"));
    cuts->SetProductionCut(0.1*mm,G4ProductionCuts::GetIndex("e+"));

    region->SetProductionCuts(cuts);
*/
//    SetCutValue(0.1*mm, "gamma");
//    SetCutValue(0.01*mm, "e-");
//    SetCutValue(0.01*mm, "e+");
}

