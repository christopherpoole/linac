# Current problem

At the moment there is a problem with asymmetry when "applicator like" GEANT4 shapes are entered into the beam. A simplification of the geometry that replicated the issue is given [here](https://github.com/SimonBiggs/linac/blob/master/machine/asymmetricProblem.yaml). An overview of the distributions scored as a result of this geometry can be seen [here](http://nbviewer.ipython.org/github/SimonBiggs/linac/blob/master/figures.ipynb).


# The aim
My aim is to have a ready working easy to use non-real linear accelerator with minimal dependencies. I will use it as a frame upon which to set up a true linac which I will be unable to release. Hopefully others can make use of this non-real set up, especially for education purposes.

Eventually I will write a beginner friendly installation guide assuming someone has just produced a fresh install of Ubuntu 14.04 LTS inside virtual box.


# Details 
The real hard work given here has been done by Christopher Poole, his more powerful linac platform can be found [here](https://github.com/christopherpoole/linac).

Both CADMesh and G4VoxelData have been removed from the original code. They are no longer required in order to install.

An example of the notebook code running the simulation can be viewed [here](http://nbviewer.ipython.org/github/SimonBiggs/linac/blob/master/main.ipynb).


# Dependencies
 * GEANT4 with python environment installed -- will provide details soon
 * Python with pylab packages and yaml
 * Boost and PyUblas -- difficult to install will provide details soon
