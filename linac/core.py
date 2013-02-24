import math
import random

from math import *
from itertools import repeat

import yaml

import Geant4
from Geant4 import G4Tubs, G4Box, G4ThreeVector, G4RotationMatrix, G4Color, mm, deg, MeV

import g4

default_multiples = {
    'translation' : [0, 0, 0],
    'rotation' : [0, 0, 0],
    'colour' : [0, 0, 0, 1],
}

transformers = {}

def register_transformer(name, f):
    transformers[name] = f


class Solid(object):
    def __init__(self, name, solid_type, **kwargs):
        for key, val in kwargs.iteritems():
            setattr(self, key, val)

        if solid_type == 'cylinder':
            self.solid = G4Tubs(name, 0, self.radius, self.thickness, 0, 360*deg)
        if solid_type == 'box':
            self.solid = G4Box(name, self.side, self.side, self.thickness)


class Volume(object):
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

                
class Linac:
    def __init__(self, filename):
        self.config = yaml.load(file(filename))
          
        self.head = Mother('head', **self.config['head'])
        self.vacuum = Mother('vacuum', **self.config['vacuum'])
        self.phasespaces = self.config['phasespaces']

        self.gun = self.config["gun"]

    def rounded_leaf_position(self, leaf_radius, radius_position, field_size, iso_position=1000.):
        theta = atan(field_size / iso_position) 
        delta = (leaf_radius / cos(theta)) - leaf_radius

        if abs(field_size) == field_size:
            return (field_size / iso_position * radius_position) + delta
        else:
            return (field_size / iso_position * radius_position) - delta
        
    def rectangular_field_mlc(self, x1, x2, y1, y2):
        for name, daughter in self.head.daughters.iteritems():
            if name.startswith("mlc_1"):
                daughter.x_position = self.rounded_leaf_position(150., 330.8, x1) 
            if name.startswith("mlc_2"):
                daughter.x_position = self.rounded_leaf_position(150., 330.8, x2)
       
    def rectangular_field_jaws(self, x1, x2, y1, y2):
        self.head.daughters["jaw_x_1"].x_position = self.rounded_leaf_position(70., 412., x1)
        self.head.daughters["jaw_x_0"].x_position = self.rounded_leaf_position(70., 412., x2)
        self.head.daughters["jaw_y_0"].y_position = y1 * ((1000. - 569.) / 1000.)
        #self.head.daughters["jaw_y_0"].y_position = y1 * ((1000. - 431.) / 1000.)
        self.head.daughters["jaw_y_1"].y_position = y2 * ((1000. - 569.) / 1000.)
        #self.head.daughters["jaw_y_1"].y_position = y2 * ((1000. - 431.) / 1000.)

    def rectangular_field(self, x1, x2, y1, y2):
        self.rectangular_field_jaws(x1, x2, y1, y2)
        self.rectangular_field_mlc(x1, x2, y1, y2)

    def square_field_jaws(self, size):
        size = size/2.
        self.rectangular_field_jaws(size, -size, size, -size)

    def square_field_mlc(self, size):
        size = size/2.
        self.rectangular_field_mlc(size, -size, size, -size)

    def square_field(self, size):
        self.square_field_jaws(size)
        self.square_field_mlc(size)

    def arbitary_field_mlc(self, bank1_positions, bank2_positions):
        for i in range(len(bank1_positions)):
            self.head.daughters["mlc_1_%i" % i].x_position = self.rounded_leaf_position(150, 330.8, bank1_positions[i])
        for i in range(len(bank2_positions)):
            self.head.daughters["mlc_2_%i" % i].x_position = self.rounded_leaf_position(150, 330.8, bank2_positions[i])

    def rotate_gantry(self, angle):
        self.head.rotation = [0, angle, 0]
    

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


class Simulation(object):
    def __init__(self, name, config):
        self.name = name
        self.config = config
        self.geometry_modified = False

        self._source = None
        self._phasespace = None

        self.detector_construction = g4.DetectorConstruction()
        Geant4.gRunManager.SetUserInitialization(self.detector_construction)

        self.physics_list = g4.PhysicsList()
        Geant4.gRunManager.SetUserInitialization(self.physics_list)

        self.primary_generator = g4.PrimaryGeneratorAction()
        Geant4.gRunManager.SetUserAction(self.primary_generator)

        self.event_action = g4.EventAction()
        Geant4.gRunManager.SetUserAction(self.event_action)

        self.stepping_action = g4.SteppingAction()
        Geant4.gRunManager.SetUserAction(self.stepping_action)

        self.rand_engine= Geant4.Ranlux64Engine()
        Geant4.HepRandom.setTheEngine(self.rand_engine)
        self.seed = random.randint(0, 2**32)
        Geant4.HepRandom.setTheSeed(self.seed)

        Geant4.gRunManager.Initialize()

    #@property
    #def config(self):
    #    return self._config

    #@config.setter
    #def config(self, c):
    #   self._config = c

    @property
    def source(self):
        return self._source

    @source.setter
    def source(self, s):
        self._source = s
        if s is not None:
            # TODO: Specify source phasespace dynamically
            self.source_file = "/media/localscratch/phsp/%s_%s.phsp" % (s, self.name)
        else:
            self.source_file = None

    @property
    def phasespace(self):
        return self._phasespace

    @phasespace.setter
    def phasespace(self, p):
        print "setting ", p
        self._phasespace = p
        if p is not None:
            # TODO: Specify save phasespace dynamically
            self.phasespace_file = "/media/localscratch/phsp/%s_%s.phsp" % (p, self.name)
        else:
            self.phasespace_file = None

    def _update(self):
        Geant4.gGeometryManager.OpenGeometry()

        world = self.detector_construction.Construct()

        #self.build_materials()
        self.build_geometry()

        Geant4.gRunManager.DefineWorldVolume(world)
        Geant4.gRunManager.GeometryHasBeenModified()
        #Geant4.gRunManager.PhysicsHasBeenModified()

        Geant4.gGeometryManager.CloseGeometry()

        self.primary_generator.SetRecyclingNumber(self.config.gun["recycling_number"])       
 
        self.primary_generator.SetPosition(G4ThreeVector(0., 0., 1050.))
        self.primary_generator.SetDirection(G4ThreeVector(0, 0, -1))

    def build_materials(self):
        for material in self.config["materials"]:
            self.detector_construction.AddMaterial(material["name"],
                                                    material["density"], cb)

    def build_geometry(self):
        self.detector_construction.SetupHead( 
            self.config.head.radius,
            self.config.head.length,
            self.config.head.translation,
            self.config.head.rotation,
            self.config.vacuum.radius,
            self.config.vacuum.length,
            self.config.vacuum.translation)
 
        for name, params in self.config.vacuum.daughters.iteritems():
            if params.filename != "":
                self.detector_construction.AddCADComponent(name, params.filename,
                    params.material, True,
                    params.scale, params.translation,
                    params.rotation, params.colour, params.tessellated)
            if hasattr(params, "solid"):
                if params.solid == "cylinder":
                    self.detector_construction.AddCylinder(name, params.radius,
                            params.thickness, params.material,
                        False, params.translation, params.rotation, params.colour) 
                if params.solid == "slab":
                    self.detector_construction.AddSlab(name, params.side,
                            params.thickness, params.material,
                        False, params.translation, params.rotation, params.colour) 

        for name, params in self.config.head.daughters.iteritems():
            if params.filename != "":
                self.detector_construction.AddCADComponent(name, params.filename,
                    params.material, False,
                    params.scale, params.translation,
                    params.rotation, params.colour, params.tessellated)
            if hasattr(params, "solid"):
                if params.solid == "cylinder":
                    self.detector_construction.AddCylinder(name, params.radius,
                            params.thickness, params.material,
                        False, params.translation, params.rotation, params.colour) 
                if params.solid == "slab":
                    self.detector_construction.AddSlab(name, params.side,
                            params.thickness, params.material,
                        False, params.translation, params.rotation, params.colour) 

        if self.phasespace:
            ps = self.config.phasespaces[self.phasespace]
            self.detector_construction.AddPhasespace(self.phasespace_file,
                    ps["radius"], ps["z_position"], ps["material"])

    def beam_on(self, histories, fwhm=2.0*mm, energy=6*MeV):
        self.primary_generator.SetGantryRotation(self.config.head.rotation)

        self._update()
  
        if self.source: 
            self.primary_generator.SetSource(self.source_file)
        else:
            self.primary_generator.SetSource(None)

            self.primary_generator.SetFWHM(fwhm)
            self.primary_generator.SetEnergy(energy)

       
        if self.source is not None:
            self.primary_generator.SetRedistribute(
                    self.config.phasespaces[self.source]["redistribute"])
    
        self.seed = random.randint(0, 2**32)
        Geant4.HepRandom.setTheSeed(self.seed)

        Geant4.gRunManager.BeamOn(int(histories))

    def show(self):
        self._update()
        Geant4.gUImanager.ExecuteMacroFile("macros/vis.mac")  

    def start_session(self): 
        Geant4.StartUISession()

    def add_head_component(self, name, component):
        self.config.head.daughters[name] = {
            'filename' : component.filename,
            'material' : component.material,
            'scale' : component.scale,
            'translation' : component.translation,
            'rotation' : component.rotation,
            'colour' : component.colour
        }

    def add_vacuum_component(self, name, component):
        self.config.vacuum.daughters[name] = {
            'filename' : component.filename,
            'material' : component.material,
            'scale' : component.scale,
            'translation' : component.translation,
            'rotation' : component.rotation,
            'colour' : component.colour
        }


