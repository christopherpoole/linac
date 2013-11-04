import os

from linac import Linac, Simulation

linac = Linac("machine/example.yaml")
sim = Simulation("example", linac)
sim.show()

sim.set_phasespaces("exitwindow", os.getpid())
sim.set_phasespaces("iso", os.getpid())

sim.beam_on(int(1e4))
sim.start_session()

