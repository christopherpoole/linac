import math
from math import *
from itertools import repeat

import yaml

from Geant4 import G4Tubs, G4Box, G4ThreeVector, G4Color
from Geant4 import mm, deg, keV, MeV


# Default values for repeated properties
default_multiples = {
    'translation' : [0, 0, 0],
    'rotation' : [0, 0, 0],
    'colour' : [0, 0, 0, 1],
}

# Register for functions that enable rotation/translation
# of a component within the geomtry (jaws for example).
transformers = {}

def register_transformer(name, f):
    transformers[name] = f


class Solid(object):
    """Creates a G4Solid

    Makes a G4VSolid of type `solid_type` specified by a string.

    Attributes:
        name: The desired name of the solid
        solid_type: The name of the target G4VSolid, G4Tubs, G4Box for example
        solid: An instance of the chosen `solid_type`
    """
    def __init__(self, name, solid_type, **kwargs):
        for key, val in kwargs.iteritems():
            setattr(self, key, val)

        if solid_type == 'cylinder':
            self.solid = G4Tubs(name, 0, self.radius, self.thickness, 0, 360*deg)
        if solid_type == 'box':
            self.solid = G4Box(name, self.side, self.side, self.thickness)


class Volume(object):
    """A CAD tessellated solid placed within the geometry

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
        self._translation = (0, 0, 0)
        self._rotation = (0, 0, 0)
        self._colour = (1, 0, 0, 1)
        self._material = 'G4_AIR'
        self.tessellated = True
       
        for key, val in kwargs.iteritems():
            if hasattr(self, key):
                setattr(self, key, val)

            if key == "cylinder":
                self.solid = "cylinder"
                self.radius = val["radius"]
                self.thickness = val["thickness"]
            if key == "slab":
                self.solid = "slab"
                self.side = val["side"]
                self.thickness = val["thickness"]

    @property
    def translation(self):
        return G4ThreeVector(*self._translation)

    @translation.setter
    def translation(self, value):
        self._translation = value

    @property
    def x_position(self):
        return self._translation[0]

    @x_position.setter
    def x_position(self, value):
        self._translation = (value, self._translation[1], self._translation[2])

    @property
    def y_position(self):
        return self._translation[1]

    @x_position.setter
    def y_position(self, value):
        self._translation = (self._translation[0], value, self._translation[2])

    @property
    def rotation(self):
        #rot = G4RotationMatrix()
        #rot.rotateX(self._rotation[0]*deg)
        #rot.rotateY(self._rotation[1]*deg)
        #rot.rotateZ(self._rotation[2]*deg)
        #return rot
        return G4ThreeVector(*self._rotation)

    @rotation.setter
    def rotation(self, value):
        self._rotation = value

    @property
    def colour(self):
        return G4Color(*self._colour)
    
    @colour.setter
    def colour(self, value):
        self._colour = value

    @property
    def material(self):
#        return Geant4.gNistManager.FindOrBuildMaterial(self._material)
        return self._material
   
    @material.setter
    def material(self, value):
        self._material = value


class Daughter(Volume):
    """A `Volume` within a `Volume`

    Attributes:
        name: The name of the volume
        config; Loaded from **kwargs and used to build the daughter `Volume`
    """
    def __init__(self, name, **kwargs):
        self._config = kwargs
        self.filename = ''
        self.scale = 1

        super(Daughter, self).__init__(name, **kwargs)

        for key, val in kwargs.iteritems():
            if hasattr(self, key):
                setattr(self, key, val)

    @property
    def config(self):
#        return dict(zip(self._config.keys(),
#                        [getattr(self, a) for a in self._config.keys()
#                                          if hasattr(self, a)]))
        d = {}
        for k in self._config.keys():
            if hasattr(self, k):
                d[k] = getattr(self, k)

        return d
   

class Mother(Volume):
    """A `Volume` with `Volume`'s in it

    If a volume is a mother volume, we want to make sure all of its daughters
    are correctly initialised. This is where the bulk of the geometry building
    happens.

    Attributtes:
        name: The name of the volume
        daughters: A list of `Volume`'s within the `Mother`
    """
    def __init__(self, name, **kwargs):
        self.radius = 0
        self.length = 0
 
        super(Mother, self).__init__(name)
        
        for key, val in kwargs.iteritems():
            if hasattr(self, key):
                setattr(self, key, val)

        self.daughters = {}
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
                self.daughters[name] = Daughter(name, **daughter)

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
                args = daughter[m]['args'].copy()
                for k, v in args.iteritems():
                    if v is None:
                        args[k] = daughter[k]            
                r = []
                f = transformers[daughter[m]['function']] 
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
            daughters.append((n, Daughter(n, **d)))
        
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
          
        self.head = Mother('head', **self.config['head'])
        self.vacuum = Mother('vacuum', **self.config['vacuum'])
        self.phasespaces = self.config['phasespaces']

        self.gun = self.config["gun"]

    def rounded_leaf_position(self, leaf_radius, radius_position,
            field_size, iso_position=1000.):
        theta = atan(field_size / iso_position) 
        delta = (leaf_radius / cos(theta)) - leaf_radius

        if abs(field_size) == field_size:
            return (field_size / iso_position * radius_position) + delta
        else:
            return (field_size / iso_position * radius_position) - delta

    def rectangular_field_jaws(self, x1, x2, y1, y2):
        raise NotImplementedError("Definition of a rectangular jaw field must be implemented by the user")

    def rectangular_field_mlc(self, x1, x2, y1, y2):
        raise NotImplementedError("Definition of a rectangular MLC field must be implemented by the user")
 
    def rectangular_field(self, x1, x2, y1, y2):
        self.rectangular_field_jaws(x1, x2, y1, y2)
        self.rectangular_field_mlc(x1, x2, y1, y2)

    def square_field_jaws(self, size, x_offset=0, y_offset=0):
        size = size/2.
        self.rectangular_field_jaws(size + x_offset, (-size) + x_offset, size + y_offset, (-size) + y_offset)

    def square_field_mlc(self, size, x_offset=0, y_offset=0):
        size = size/2.
        self.rectangular_field_mlc(size + x_offset, (-size) + x_offset, size + y_offset, (-size) + y_offset)

    def square_field(self, size, x_offset=0, y_offset=0):
        self.square_field_jaws(size, x_offset, y_offset)
        self.square_field_mlc(size, x_offset, y_offset)

    def arbitary_field_mlc(self, bank1_positions, bank2_positions):
        raise NotImplementedError("Definition of an arbitary MLC field must be implemented by the user")

    def rotate_gantry(self, angle):
        self.head.rotation = (self.head.rotation.x, angle, self.head.rotation.z)
    
    def rotate_collimator(self, angle):
        self.head.rotation = (angle, self.head.rotation.y, self.head.rotation.z)


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


def cb():
    return 123

class Component:
    def __init__(self, physical_volume, config):
        self.physical_volume = physical_volume
        self.config = config

    def move(self, **kwargs):
        self.config["move"](**kwargs)



