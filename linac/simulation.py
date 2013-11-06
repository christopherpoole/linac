import random

import Geant4
from Geant4 import G4ThreeVector, G4RotationMatrix, G4Color, mm, deg, MeV

import g4
import numpy
import pyublas


class Simulation(object):
    def __init__(self, name, config, phsp_dir='.', run_id=0):
        self.name = name
        self.run_id = run_id

        self.config = config
        self.geometry_modified = False

        self.geometry = {}

        self.phsp_dir = phsp_dir

        self.source = None
        self.phasespace = None

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

        self.build_geometry()
    
    ## Primary generator ##

    def reset_source(self):
        self.primary_generator.Reset()

    @property
    def source_file(self):
        if self.source is not None:
            return "%s/%s_%s_%s.phsp" % (self.phsp_dir, self.source, self.name, self.run_id)
        else:
            return None 

    ## Physics ##

    def set_cuts(self, gamma=1., electron=1.):
        self.physics_list.OverrideCuts(gamma, electron)

    ## Voxelised phantom data ##

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

    ## Scoring ##

    def save_histograms(self, directory, name, runid):
        energy_data = self.detector_construction.GetEnergyHistogram()
        numpy.save("%s/energy_%s_%s_%s" % (directory, self.name, name, runid), energy_data)

        energy2_data = self.detector_construction.GetEnergySqHistogram()
        numpy.save("%s/energy2_%s_%s_%s" % (directory, self.name, name, runid), energy2_data)

        counts_data = self.detector_construction.GetCountsHistogram()
        numpy.save("%s/counts_%s_%s_%s" % (directory, self.name, name, runid), counts_data)

    def zero_histograms(self):
        self.detector_construction.ZeroHistograms()

    ## Visualisation/Session ##

    def show(self):
        Geant4.gUImanager.ExecuteMacroFile("macros/vis.mac")  

    def start_session(self): 
        Geant4.StartUISession()

    ## Geometry ##

    def build_materials(self):
        for material in self.config["materials"]:
            self.detector_construction.AddMaterial(material["name"], material["density"], cb)

    def build_geometry(self):
        def build(volume, mother): 
            for name, params in volume.daughters.iteritems():
                if params.filename != "":
                    physical = self.detector_construction.AddCADComponent(name, params.filename,
                        params.material, params.scale, params.translation, params.rotation,
                        params.colour, params.tessellated, mother)
                if hasattr(params, "solid"):
                    if params.solid == "cylinder":
                        physical = self.detector_construction.AddCylinder(name, params.radius,
                                params.length, params.material, params.translation,
                                params.rotation, params.colour, mother) 
                    if params.solid == "slab":
                        physical = self.detector_construction.AddSlab(name, params.side,
                                params.thickness, params.material, params.translation,
                                params.rotation, params.colour, mother) 

                self.geometry[name] = physical               
 
                build(params, physical.GetLogicalVolume())

        mother = self.detector_construction.GetWorld()
        build(self.config.world, mother)
 
        self.build_phasespaces()       

    def update_geometry(self):
        def update(volume):
            for name, params in volume.daughters.iteritems():
                physical = self.geometry[name]

                physical.SetRotation(params.rotation_matrix)
                physical.SetTranslation(params.translation)

                update(volume.daughters[name])
        
        update(self.config.world)

        self.build_phasespaces() 

    ## Phasespace files ##
    @property
    def phasespace_file(self):
        if self.phasespace is not None:
            return "%s/%s_%s_%s.phsp" % (self.phsp_dir, self.phasespace, self.name, self.run_id)
        else:
            return None

    def close_phasespace(self):
        self.detector_construction.RemovePhasespace(self.phasespace_file)
   
    def build_phasespaces(self):     
        if self.phasespace is not None:
            ps = self.config.phasespaces[self.phasespace]
            self.detector_construction.AddPhasespace(self.phasespace_file,
                    ps["radius"], ps["z_position"], ps["material"], ps["kill"])

    ## Run ##
 
    def beam_on(self, histories, fwhm=2.0*mm, energy=6*MeV):
        self.update_geometry()

        if self.source is not None: 
            self.primary_generator.SetSource(self.source_file)

            z = self.config.phasespaces[self.source]["z_position"]
            self.primary_generator.SetPhasespaceLimits(-200, 200, -200, 200, z-0.1, z+0.1) 
            self.primary_generator.SetRedistribute(self.config.phasespaces[self.source]["redistribute"])
            self.primary_generator.SetRecyclingNumber(self.config.gun["recycling_number"])       
        else:
            self.primary_generator.SetSource(None)

            self.primary_generator.SetFWHM(fwhm)
            self.primary_generator.SetEnergy(energy)
            self.primary_generator.SetPosition(G4ThreeVector(0., 0., 1050.))
            self.primary_generator.SetDirection(G4ThreeVector(0, 0, -1))

        Geant4.gRunManager.BeamOn(int(histories))


