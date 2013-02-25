#/usr/bin/python

# License & Copyright
# ===================
#
# Copyright 2012 Christopher M Poole <mail@christopherpoole.net>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os
import sys
import time
import socket

from pprint import pprint
from math import *

import numpy
import scipy
import pyublas

from linac import Simulation
from machines import Elekta

linac = Elekta("machines/Elekta_Precise/Elekta_Precise.yaml") 

energy = float(sys.argv[2])
fwhm = float(sys.argv[3])

name = "%s_%f_%f" % (sys.argv[1], energy, fwhm)

sim = Simulation(name, linac)

sim._update()
sim.show()
"""
sim.phasespace = "chamber" 
tic = time.time()
sim.beam_on(int(1e5), fwhm=fwhm, energy=energy*MeV)
print "t1: ", time.time() - tic


for field in [50, 100, 150, 200, 300, 400]:
    sim.name = name
    sim.source = "chamber"
    sim.name = "%s_%ix%i" % (name, field, field)
    sim.phasespace = "exit_window" 

    sim.config.square_field(field)

    sim.detector_construction.UsePhantom(False)
    tic = time.time()
    sim.beam_on(2**30)
    print "t2: ", time.time() - tic

    sim.detector_construction.UsePhantom(True)
    sim.source = "exit_window"
    sim.phasespace = None
    tic = time.time()
    sim.beam_on(2**30)
    print "t3: ", time.time() - tic
   
    energy_data = sim.detector_construction.GetEnergyHistogram()
    counts_data = sim.detector_construction.GetCountsHistogram()

    numpy.save("output/energy_%s_%ix%i_%s_%i" % (name, field, field, socket.gethostname(), os.getpid()), energy_data) 
    numpy.save("output/counts_%s_%ix%i_%s_%i" % (name, field, field, socket.gethostname(), os.getpid()), counts_data) 

    sim.detector_construction.ZeroHistograms()
"""

