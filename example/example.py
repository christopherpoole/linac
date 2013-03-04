from linac import Linac, Simulation

linac = Linac("machine/example.yaml")
sim = Simulation("example", linac)

sim.show()

