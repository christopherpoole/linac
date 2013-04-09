import sys

import pyublas
import numpy
import pylab

from phsp_inspector import PhasespaceInspector


if __name__ == "__main__":
    filename = sys.argv[1]
    
    inspector = PhasespaceInspector()
    inspector.Read(filename)

    pylab.hist(inspector.energy, bins=numpy.arange(0, 6, 0.1))
    pylab.show()

