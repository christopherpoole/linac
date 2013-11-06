import os

from linac import Linac, Simulation

linac = Linac("machine/example.yaml")
sim = Simulation("example", linac, run_id=os.getpid())
sim.show()

sim.phasespace = "exitwindow"
sim.beam_on(int(1e4))

sim.close_phasespace()

sim.source = "exitwindow"
sim.phasespace = "iso"
sim.beam_on(int(1e9))

sim.close_phasespace()

sim.source = "iso"
sim.phasespace = None 
sim.beam_on(int(1e9))

#sim.start_session()

