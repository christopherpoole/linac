import sys
import pyublas

from phsp_inspector import PhasespaceInspector


if __name__ == "__main__":
    filename = sys.argv[1]
    
    inspector = PhasespaceInspector()
    inspector.Read(filename)

    inspector.energy

