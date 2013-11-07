import math
import inspect 

from math import *
from itertools import repeat

import yaml

from Geant4 import G4Tubs, G4Box, G4ThreeVector, G4RotationMatrix, G4Color
from Geant4 import mm, deg, keV, MeV


# Default values for repeated properties
default_multiples = {
    'translation' : [0, 0, 0],
    'rotation' : [0, 0, 0],
    'colour' : [1, 1, 1, 1],
}

# Register for functions that enable rotation/translation
# of a component within the geometry (jaws for example).
transformers = {}

def register_transformer(name, f):
    transformers[name] = f

## MLC transformers ##

def mlc_diverge(i, interval=None, position=None, shift=0, z_rotation=0, centre=0, repeat=0):
    offset = -(interval * repeat / 2. - interval + shift)
    divergance = math.atan((i*interval + offset + centre)/(1000 - position))
    return (-divergance/deg, 0, z_rotation)

def mlc_interleave(i, interval=None, position=None, shift=0):
    offset = -(interval * 20 / 2. - interval + shift)
    return (10, i*interval + offset, position)

def mlc_arc(i, interval=None, position=None, shift=0, repeat=0):
    offset = -(interval * repeat / 2. - interval + shift)
    return (0, i*interval + offset, 1000 - math.sqrt((1000 - position)**2 - (i*interval + offset)**2))

register_transformer('mlc_diverge', mlc_diverge)
register_transformer('mlc_interleave', mlc_interleave)
register_transformer('mlc_arc', mlc_arc)

## Basic repeated geometry transformers ##

def repeat_x(i, interval=None, origin=None):
    return (origin[0] + i*interval, origin[1], origin[2])

def repeat_y(i, interval=None, origin=None):
    return (origin[0], origin[1] + i*interval, origin[2])

def repeat_z(i, interval=None, origin=None):
    return (origin[0], origin[1], origin[2] + i*interval)

register_transformer('repeat_x', repeat_x)
register_transformer('repeat_y', repeat_y)
register_transformer('repeat_z', repeat_z)


class Volume(object):
    """A solid placed within the geometry

    A solid can be any GEANT4 primitive.
    
    CAD Files:
    Using CADMesh a CAD file is loaded into the geometry as a G4TessellatedSolid,
    the user can specify if the volume is left as a normal tessellated solid or
    loaded as a tetrahedral mesh (for fast/er navigation).

    Attributes:
        name: The desired name of the solid
        translation: G4ThreeVector position of the solid within its mother volume
        rotation: G4ThreeVector rotation of the solid about its center
        colour: G4Colour as displayed by the VisManager
        material: The name of the target G4Material (it has to already exist)
        tessellated: If tetrahedralisation if not performed, otherwise
    """
    def __init__(self, name, **kwargs):
        self.name = name

        self.config = kwargs
        self.filename = ''
        self.scale = 1
        self.translation = (0, 0, 0)
        self.rotation = (0, 0, 0)
        self._rotation_matrix = None
        self.colour = (1, 0, 0, 1)
        self.material = 'G4_AIR'
        self.tessellated = True
       
        for key, val in kwargs.iteritems():
            if hasattr(self, key):
                setattr(self, key, val)

            if key == "cylinder":
                self.solid = "cylinder"
                self.radius = val["radius"]
                self.length = val["length"]
            if key == "slab":
                self.solid = "slab"
                self.side = val["side"]
                self.thickness = val["thickness"]
            if key == "cube":
                self.solid = "slab"
                self.side = val["side"]
                self.thickness = val["side"]

        self.daughters = {}
        if kwargs.has_key("daughters"):
            self._init_daughters(**kwargs)

    ## Getters for rotation/translation as G4* objects ##

    @property
    def translation_vector(self):
        """Return the current translations as a G4ThreeVector
        """
        return G4ThreeVector(*self.translation)

    @property
    def rotation_vector(self):
        """Return the x-, y-, z-direction rotations as a G4ThreeVector
        """
        return G4ThreeVector(self.rotation[0], self.rotation[1], self.rotation[2])

    @property
    def rotation_matrix(self):
        """Return the x-, y-, z-direction rotations as a G4RotationMatrix
        """
        self._rotation_matrix = G4RotationMatrix()
        self._rotation_matrix.rotateX(self.rotation[0]*deg)
        self._rotation_matrix.rotateY(self.rotation[1]*deg)
        self._rotation_matrix.rotateZ(self.rotation[2]*deg)
        return self._rotation_matrix

    @property
    def color(self):
        """Return the colour as a G4Color
        """
        return G4Color(*self.colour)

    ## Initialisers ##

    def _init_daughters(self, **kwargs):
        """If a `Volume` has daugther volumes, iterativley initialise them and
        unpack repeats of the same volume if required.
        """
        for name, daughter in kwargs['daughters'].iteritems():
            if daughter.has_key('inherit'):
                d = kwargs['daughters'][daughter['inherit']].copy()
                d.update(daughter)
                del d['inherit']
                daughter = d
                
            try:
                repeats = daughter['repeat']
            except KeyError:
                repeats = 1

            if repeats > 1:
                daughters = self._unpack_repeats(name, daughter, repeats)
                self.daughters.update(daughters)
            else:
                self.daughters[name] = Volume(name, **daughter)

    def _unpack_repeats(self, name, daughter, repeats):
        """Construct individual Volumes for those specified as repeats

        The geometry configuration does not require all instances of volumes to be
        explicity defined, repated volumes can be specified with lists provided for
        the parameters that vary.

        Args:
            name: The base name of the volume
            daughter: The instance of the base volume
            repeasts: The number of copies to make
        """
        
        multiples = {}
        for m, default in default_multiples.iteritems():
            if not daughter.has_key(m):
                multiples[m] = list(repeat(default, repeats))
                continue

            if isinstance(daughter[m], dict):
                # with functions/transformers
                f = transformers[daughter[m]['function']] 

                spec = inspect.getargspec(f)
                argnames = spec.args[-len(spec.defaults):]

                args = {}
                for k in argnames:
                    args[k] = daughter[k]            

                r = []
                for i in range(repeats):
                    r.append(f(i, **args))
                multiples[m] = r
                
            elif isinstance(daughter[m], list):
                if isinstance(daughter[m][0], list):
                   multiples[m] = daughter[m] 
                else:
                    multiples[m] = list(repeat(daughter[m], repeats))
            else:
                # something is wrong
                pass

        daughters = []
        for i in range(repeats):
            d = daughter.copy()
            for k, v in multiples.iteritems():
                d[k] = multiples[k][i]
            n = "%s_%i" % (name, i)
            daughters.append((n, Volume(n, **d)))
        
        return daughters 


class Linac(object):
    """The Python object representation of the geometry configuration

    Attributes:
        config: configuration yaml loaded as a dict
        head: Treatment head Mother volume
        vacuum: Vacuum parts Mother volume
        phasespaces: All phasespace files used or created
        gun: The particle gun configuration
    """
    def __init__(self, filename):
        self.config = yaml.load(file(filename))
          
        self.world = Volume('world', **self.config['world'])
        self.phasespaces = self.config['phasespaces']

        self.gun = self.config["gun"]

    def rounded_leaf_position(self, leaf_radius, radius_position,
            field_size, iso_position=1000.):
        """Calculator for positioning a leaf/jaw with a rounded end.

        Attributes:
            leaf_radius: the radius on the end of the leaf
            radius_position: the position of the center of the leaf radius
            field_size: target field opening
            iso_position: distance from the source to the iso center
        """
        theta = atan(field_size / iso_position) 
        delta = (leaf_radius / cos(theta)) - leaf_radius

        if abs(field_size) == field_size:
            return (field_size / iso_position * radius_position) + delta
        else:
            return (field_size / iso_position * radius_position) - delta

    ## Define rectangular fields ##

    def rectangular_field_jaws(self, x1, x2, y1, y2):
        raise NotImplementedError("Definition of a rectangular jaw field must be implemented by the user")

    def rectangular_field_mlc(self, x1, x2, y1, y2):
        raise NotImplementedError("Definition of a rectangular MLC field must be implemented by the user")
 
    def rectangular_field(self, x1, x2, y1, y2):
        """Convenience function for calling the user defined jaw/MLC rectangular
        field setters both together.
        """
        self.rectangular_field_jaws(x1, x2, y1, y2)
        self.rectangular_field_mlc(x1, x2, y1, y2)

    ## Define square fields ##

    def square_field_jaws(self, size, x_offset=0, y_offset=0):
        """Set a square field for the jaw only.
        """
        size = size/2.
        self.rectangular_field_jaws(size + x_offset, (-size) + x_offset, size + y_offset, (-size) + y_offset)

    def square_field_mlc(self, size, x_offset=0, y_offset=0):
        """Set a square field for the MLC only.
        """
        size = size/2.
        self.rectangular_field_mlc(size + x_offset, (-size) + x_offset, size + y_offset, (-size) + y_offset)

    def square_field(self, size, x_offset=0, y_offset=0):
        """Convenience function for setting a square field given the field size length.
        """
        self.square_field_jaws(size, x_offset, y_offset)
        self.square_field_mlc(size, x_offset, y_offset)
    
    ## Define arbitrary fields ##

    def arbitary_field_mlc(self, bank1_positions, bank2_positions):
        raise NotImplementedError("Definition of an arbitary MLC field must be implemented by the user")

    ## Move the gantry/collimator ##

    def rotate_gantry(self, angle):
        """Rotate the gantry to the nominated angular position. This is NOT
        and incremental rotation.
        """
        offset_angle = (self.world.daughters['head'].rotation_vector.y - angle)*deg

        self.world.daughters['head'].rotation = (self.world.daughters['head'].rotation_vector.x,
                                                 angle,
                                                 self.world.daughters['head'].rotation_vector.z)
   
        t = self.world.daughters['head'].translation_vector
        t = t.rotateY(offset_angle)
        self.world.daughters['head'].translation = (t.x, t.y, t.z)
 
    def rotate_collimator(self, angle):
        """Rotation the collimator to the nominated angular position.
        """
        self.world.daughters['head'].rotation = (angle,
                                                 self.world.daughters['head'].rotation_vector.y,
                                                 self.world.daughters['head'].rotation_vector.z)

