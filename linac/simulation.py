import random

import Geant4
from Geant4 import G4ThreeVector, G4RotationMatrix, G4Color, mm, deg, MeV

import g4


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

