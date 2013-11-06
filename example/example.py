import os
import sys

from linac import Linac, Simulation


linac = Linac("machine/example.yaml")

sim = Simulation("example", linac, run_id=os.getpid())
sim.show()

# Control flow
sim.enable_phasespace("exitwindow1")
sim.enable_phasespace("exitwindow2")
sim.beam_on(int(1e4))

sim.disable_phasespace("exitwindow1")
#sim.disable_phasespace("exitwindow2")

sim.enable_phasespace_source("exitwindow2")    # Automatically disables a phasespace, if enabled

for angle in range(0, 360, 5):
    linac.rotate_gantry(angle)
    sim.beam_on(int(1e9))

#sim.start_session()

