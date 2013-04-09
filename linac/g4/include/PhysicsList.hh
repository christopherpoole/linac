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


#ifndef PhysicsList_h
#define PhysicsList_h 1

// GEANT4 //
#include "globals.hh"
#include "G4VModularPhysicsList.hh"
#include "G4VUserPhysicsList.hh"
#include "G4RunManager.hh"
#include "G4ProductionCutsTable.hh"


//class PhysicsList: public G4VUserPhysicsList
class PhysicsList: public G4VModularPhysicsList
{
    public:
        PhysicsList();
        ~PhysicsList();

    protected:
        void ConstructParticle();
        void ConstructProcess();
        void SetCuts();

  public:
    void OverrideCuts(double gamma_cuts, double e_cuts){
        this->gamma_cuts = gamma_cuts;
        this->e_cuts = e_cuts;

        SetCuts();
        G4RunManager::GetRunManager()->GeometryHasBeenModified();
    };

  private:
    double gamma_cuts;
    double e_cuts;
};

#endif

