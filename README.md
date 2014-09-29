# GEANT4 Clinical Linear Accelerator Simulation 

A generic platform for clinical linear accelerator simulation using GEANT4. 

# Dependencies
* GEANT4 (4.9.6)
* CADMesh (with [tetgen1.4.3](http://opensees.berkeley.edu/WebSVN/filedetails.php?repname=OpenSees&path=%2Ftrunk%2FOTHER%2Ftetgen1.4.3%2Ftetgen.h) included)
* G4VoxelData
* Python, numpy, yaml (`sudo apt-get install python python-numpy python-yaml`)
* PyUblas ([install guide](http://documen.tician.de/pyublas/installing.html))

# Installation
To compile the Python module:

    $> cd g4/
    $> mkdir build
    $> cd build/
    $> cmake ..
    $> make

