import random

import Geant4
from Geant4 import G4ThreeVector, G4RotationMatrix, G4Color, mm, deg, MeV

import g4
import numpy
import pyublas


class Simulation(object):
    def __init__(self, name, config, phsp_dir='.', use_phantom=True):
        self.name = name
        self.run_id = 0

        self.config = config
        self.geometry_modified = False

        self.phsp_dir = phsp_dir
        self._source = None
        self._phasespace = None
        self.phasespaces = []
        self.phasespace_files = []

        self.detector_construction = g4.DetectorConstruction()
        Geant4.gRunManager.SetUserInitialization(self.detector_construction)
        self.detector_construction.UsePhantom(use_phantom)

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

        Geant4.gUImanager.ApplyCommand("/run/verbose 2")
        Geant4.gRunManager.Initialize()

        self.build_geometry()

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
            self.source_file = "%s/%s_%s_%s.phsp" % (self.phsp_dir, s, self.name, self.run_id)
        else:
            self.source_file = None

    def set_source(self, source, run_id=0):
        self.run_id = run_id
        self.source = source

        self.detector_construction.RemovePhasespace(self.source_file)
        z = self.config.phasespaces[source]["z_position"]
        self.primary_generator.SetPhasespaceLimits(-200, 200, -200, 200, z-0.1, z+0.1) 

    @property
    def phasespace(self):
        return self._phasespace

    @phasespace.setter
    def phasespace(self, p):
        print "setting ", p
        self._phasespace = p
        if p is not None:
            # TODO: Specify save phasespace dynamically
            self.phasespace_file = "%s/%s_%s_%s.phsp" % (self.phsp_dir, p, self.name, self.run_id)
        else:
            self.phasespace_file = None

    def set_phasespace(self, phasespace, run_id=0):
        self.run_id = run_id
        self.phasespace = phasespace

    # Hack
    def set_phasespaces(self, phasespace, run_id=0):
        self.set_phasespace(phasespace, run_id)
        self.phasespaces.append(self.phasespace)
        self.phasespace_files.append(self.phasespace_file)

    def set_cuts(self, gamma=1., electron=1.):
        self.physics_list.OverrideCuts(gamma, electron)

    def set_ct(self, directory, acquisition=1):
        self.detector_construction.UseCT(directory, acquisition)

    def set_array(self, filename, x=1., y=1., z=1.):
        self.detector_construction.UseArray(filename, x, y, z)

    def set_ct_position(self, position):
        pos = G4ThreeVector(*position)
        self.detector_construction.SetCTPosition(pos)

    def crop_ct_x(self, xmin, xmax):
        self.detector_construction.CropX(xmin, xmax)

    def crop_ct_y(self, ymin, ymax):
        self.detector_construction.CropY(ymin, ymax)

    def crop_ct_z(self, zmin, zmax):
        self.detector_construction.CropZ(zmin, zmax)

    def get_ct_origin(self):
        return self.detector_construction.GetCTOrigin()

    def hide_ct(self, hide):
        self.detector_construction.HideCT(hide)

    def use_phantom(self, use):
        self.detector_construction.UsePhantom(use)

    def _update(self):
        self.update_geometry()

        self.primary_generator.SetRecyclingNumber(self.config.gun["recycling_number"])       
 
        self.primary_generator.SetPosition(G4ThreeVector(0., 0., 1050.))
        self.primary_generator.SetDirection(G4ThreeVector(0, 0, -1))

    def save_histograms(self, directory, name, runid):
        energy_data = self.detector_construction.GetEnergyHistogram()
        numpy.save("%s/energy_%s_%s_%s" % (directory, self.name, name, runid), energy_data)

        energy2_data = self.detector_construction.GetEnergySqHistogram()
        numpy.save("%s/energy2_%s_%s_%s" % (directory, self.name, name, runid), energy2_data)

        counts_data = self.detector_construction.GetCountsHistogram()
        numpy.save("%s/counts_%s_%s_%s" % (directory, self.name, name, runid), counts_data)

    def zero_histograms(self):
        self.detector_construction.ZeroHistograms()

    def build_materials(self):
        for material in self.config["materials"]:
            self.detector_construction.AddMaterial(material["name"],
                                                    material["density"], cb)

    def build_geometry(self):
        self.detector_construction.ClosePhasespace()

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
        
        if len(self.phasespaces) > 0:
            for phasespace, phasespace_file in zip(self.phasespaces, self.phasespace_files):
                ps = self.config.phasespaces[phasespace]
                self.detector_construction.AddPhasespace(phasespace_file,
                        ps["radius"], ps["z_position"], ps["material"], ps["kill"])
        elif hasattr(self, "phasespace_file") and self.phasespace is not None:
            ps = self.config.phasespaces[self.phasespace]
            self.detector_construction.AddPhasespace(self.phasespace_file,
                    ps["radius"], ps["z_position"], ps["material"], ps["kill"])
        
    def update_geometry(self):
        self.detector_construction.ClosePhasespace()

        self.detector_construction.SetGantryAngle(self.config.head.rotation.y)
         
        for name, params in self.config.vacuum.daughters.iteritems():
            if params.filename != "":
                self.detector_construction.TranslateCADComponent(name, params.translation, True)
                self.detector_construction.RotateCADComponent(name, params.rotation)
            if hasattr(params, "solid"):
                if params.solid == "cylinder":
                    self.detector_construction.UpdateCylinder(name, params.radius,
                            params.thickness, params.material,
                            params.translation, params.rotation) 
                if params.solid == "slab":
                    self.detector_construction.UpdateSlab(name, params.side,
                            params.thickness, params.material,
                            params.translation, params.rotation) 

        for name, params in self.config.head.daughters.iteritems():
            if params.filename != "":
                self.detector_construction.TranslateCADComponent(name, params.translation, False)
                self.detector_construction.RotateCADComponent(name, params.rotation)
            if hasattr(params, "solid"):
                if params.solid == "cylinder":
                    self.detector_construction.UpdateCylinder(name, params.radius,
                            params.thickness, params.material,
                            params.translation, params.rotation) 
                if params.solid == "slab":
                    self.detector_construction.UpdateSlab(name, params.side,
                            params.thickness, params.material,
                            params.translation, params.rotation) 
 
        if len(self.phasespaces) > 0:
            for phasespace, phasespace_file in zip(self.phasespaces, self.phasespace_files):
                ps = self.config.phasespaces[phasespace]
                self.detector_construction.AddPhasespace(phasespace_file,
                        ps["radius"], ps["z_position"], ps["material"], ps["kill"])
        elif hasattr(self, "phasespace_file") and self.phasespace is not None:
            ps = self.config.phasespaces[self.phasespace]
            self.detector_construction.AddPhasespace(self.phasespace_file,
                    ps["radius"], ps["z_position"], ps["material"], ps["kill"])
 
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
        self.detector_construction.ClosePhasespace()

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


