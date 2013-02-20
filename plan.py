import os
import sys
import socket
import time

import numpy
import dicom

from linac import Linac
from main import Simulation

import pyublas


if __name__ == "__main__":

    name = "plan_test"

    linac = Linac("machines/Elekta_Precise/Elekta_Precise.yaml")
    sim = Simulation(name, linac)

#    sim.show()
    sim._update()

    sim.phasespace = "chamber"
    sim.beam_on(int(1e4), fwhm=4, energy=6)

    plan = dicom.read_file(sys.argv[1])
    beams = plan.BeamSequence
    
    for i, beam in enumerate(beams):

        sim.name = name
        sim.source = "chamber"

        sim.name = "plan_test_%i" % i
        sim.phasespace = "exit_window"

        xjaw = beam.ControlPointSequence[0].BeamLimitingDevicePositionSequence[0].LeafJawPositions
        yjaw = beam.ControlPointSequence[0].BeamLimitingDevicePositionSequence[1].LeafJawPositions

        sim.config.rectangular_field_jaws(xjaw[1], xjaw[0],  yjaw[1], yjaw[0])

        try:
            mlc = beam.ControlPointSequence[0].BeamLimitingDevicePositionSequence[2].LeafJawPositions
            sim.config.arbitary_field_mlc(numpy.array(mlc[40:]), numpy.array(mlc[:40]))
        except IndexError:
            sim.config.arbitary_field_mlc(numpy.ones(40)*xjaw[1], numpy.ones(40)*xjaw[0])

        ssd = beam.ControlPointSequence[0].SourceToSurfaceDistance
        isox = beam.ControlPointSequence[0].IsocenterPosition[0]
        isoy = beam.ControlPointSequence[0].IsocenterPosition[1]
        isoz = beam.ControlPointSequence[0].IsocenterPosition[2]
        ang = beam.ControlPointSequence[0].GantryAngle

        sim.detector_construction.UsePhantom(False)
        sim.config.rotate_gantry(0)
        print "beam on: %i" % i
        sim.beam_on(2**30)

        sim.config.rotate_gantry(ang)
        sim.detector_construction.UsePhantom(True)
        #sim.detector_construction.ZeroHistograms()

        sim.source = "exit_window"
        sim.phasespace = None
        sim.beam_on(2**30)

        energy_data = sim.detector_construction.GetEnergyHistogram()
        counts_data = sim.detector_construction.GetCountsHistogram()

        numpy.save("output/energy_%s_%s_%i_%i" % (name, socket.gethostname(), os.getpid(), i), energy_data)
        numpy.save("output/counts_%s_%s_%i_%i" % (name, socket.gethostname(), os.getpid(), i), counts_data)

