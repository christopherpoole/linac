#!/bin/bash

cd /media/nfsroot/software/cern/geant4.9.6/bin
source geant4.sh
cd ~

export G4LEDATA=/media/nfsroot/software/cern/geant4.9.6/data/G4EMLOW6.32
export PYTHONPATH=$PYTHONPATH:/media/nfsroot/software/cern/geant4.9.6/python/lib

# Other
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib

cd /media/nfsroot/software/linac

python -B main.py $PBS_ARRAYID $energy $fwhm $field

