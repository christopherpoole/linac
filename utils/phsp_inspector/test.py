import sys

import pyublas
import numpy
import pylab

from phsp_inspector import PhasespaceInspector


if __name__ == "__main__":
    filenames = sys.argv[1:]
    
    for f in filenames:
        inspector = PhasespaceInspector()
        inspector.Read(f)

        pylab.hist(inspector.energy, bins=numpy.arange(0, 6, 0.1), alpha=1./len(filenames))
    pylab.show()

