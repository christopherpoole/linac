# Standard Library
import random

# GEANT4
import Geant4
from Geant4 import G4ThreeVector, G4RotationMatrix, G4Color, mm, deg, MeV

# External
import numpy
import pyublas

# User
import g4


class Simulation(object):
    """The base GEANT4 application

    The simulation proper is initialised here, along with the geometry described
    by the world `Volume` and each daughter `Volume` within.
    """
    def __init__(self, name, config, phsp_dir='.', run_id=0):
        self.name = name
        self.run_id = run_id

        self.config = config
        self.geometry_modified = False

        self.geometry = {}

        self.phsp_dir = phsp_dir

        self.source = None
        self.phasespaces = []

        self.detector_construction = g4.DetectorConstruction()

        side = self.config.world.side*mm
        self.detector_construction.SetWorldSize(G4ThreeVector(side, side, side))
        self.detector_construction.SetWorldMaterial(self.config.world.material)
        self.detector_construction.SetWorldColour(self.config.world.color)

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
        """Reset the primary generator action to its default state.
        
        Especially in the case of using a phasespace file as the source,
        resetting it has the effect of replaying it again from the beginning
        on the next run.
        """
        self.primary_generator.Reset()

    def enable_phasespace_source(self, name):
        """Set the primary generator action source to draw from an existing phasespace file. If
        the nominated phasespace file is open for recoding, it will be disabled automatically.
        """
        if name in self.phasespaces:
            self.disable_phasespace(name)

        self.source = name

    def disable_phasespace_source(self):
        """Remove any phasespace set as the primary generator action source, defaults to
        using a general particle source (GPS).
        """
        self.source = None

    ## Physics ##

    def set_cuts(self, gamma=1., electron=1.):
        """Set or override the default cuts for gammas and electrons through all parts of
        the simulation geometry.
        """
        self.physics_list.OverrideCuts(gamma, electron)

    ## Voxelised phantom data ##

    def set_ct(self, directory, acquisition=1):
        """Nominate a DICOM directory as acquisition to load as voxelised geometry.
        """
        self.detector_construction.UseCT(directory, acquisition)

    def set_array(self, filename, x=1., y=1., z=1.):
        """Use a `numpy` array as voxelised geometry.
        """
        self.detector_construction.UseArray(filename, x, y, z)

    def set_ct_position(self, position):
        """Coerce the loaded voxel data to the specified origin.
        """
        pos = G4ThreeVector(*position)
        self.detector_construction.SetCTPosition(pos)

    def crop_ct_x(self, xmin, xmax):
        """Crop the voxel data in the x-direction.
        """
        self.detector_construction.CropX(xmin, xmax)

    def crop_ct_y(self, ymin, ymax):
        """Crop the voxel data in the y-direction.
        """
        self.detector_construction.CropY(ymin, ymax)

    def crop_ct_z(self, zmin, zmax):
        """Crop the voxel data in the z-direction.
        """
        self.detector_construction.CropZ(zmin, zmax)

    def get_ct_origin(self):
        """Get the voxel data origin (usually reported for DICOM-CT unless otherwise set).
        """
        return self.detector_construction.GetCTOrigin()

    def hide_ct(self, hide):
        """Hide the voxel data from the viewer.
        """
        self.detector_construction.HideCT(hide)

    def use_phantom(self, use):
        """Nide the voxel data from the navigator.
        """
        self.detector_construction.UsePhantom(use)

    ## Scoring ##

    def save_histograms(self, directory, name, runid):
        """Dump the saved `numpy` array histograms, that align with the voxel data,to disk.
        """
        energy_data = self.detector_construction.GetEnergyHistogram()
        numpy.save("%s/energy_%s_%s_%s" % (directory, self.name, name, runid), energy_data)

        energy2_data = self.detector_construction.GetEnergySqHistogram()
        numpy.save("%s/energy2_%s_%s_%s" % (directory, self.name, name, runid), energy2_data)

        counts_data = self.detector_construction.GetCountsHistogram()
        numpy.save("%s/counts_%s_%s_%s" % (directory, self.name, name, runid), counts_data)

    def zero_histograms(self):
        """Zero all historams without regard for their content.
        """
        self.detector_construction.ZeroHistograms()

    ## Visualisation/Session ##

    def show(self):
        """Show the geometry using built-in visualisation. Requires a `macros/vis.mac` macros.
        """
        Geant4.gUImanager.ExecuteMacroFile("macros/vis.mac")  

    def start_session(self):
        """Start a GEANT4 command line users session (`g4py`).
        """
        Geant4.StartUISession()

    ## Geometry ##

    def build_materials(self):
        """Build the materials defined in the user `Linac` configureation.
        """
        for material in self.config["materials"]:
            self.detector_construction.AddMaterial(material["name"], material["density"], cb)

    def build_geometry(self):
        """Recursively build the user defined geometry.
        """
        def build(volume, mother): 
            for name, params in volume.daughters.iteritems():
                if params.filename != "":
                    physical = self.detector_construction.AddCADComponent(name, params.filename,
                        params.material, params.scale, params.translation_vector, params.rotation_vector,
                        params.color, params.tessellated, mother)
                if hasattr(params, "solid"):
                    if params.solid == "cylinder":
                        physical = self.detector_construction.AddTube(name,
                                0, params.radius, params.length,
                                params.translation_vector, params.rotation_vector,
                                params.material, params.color, mother)
                    if params.solid == "tube":
                        physical = self.detector_construction.AddTube(name,
                                params.inner_radius, params.outer_radius, params.length,
                                params.translation_vector, params.rotation_vector,
                                params.material, params.color, mother)
                    if params.solid == "slab":
                        physical = self.detector_construction.AddSlab(name, params.side,
                                params.thickness, params.material, params.translation_vector,
                                params.rotation_vector, params.color, mother) 

                if params.scorer == "StopKillSheild":
                    self.detector_construction.SetAsStopKillSheild(physical)

                self.geometry[name] = physical               
 
                build(params, physical.GetLogicalVolume())

        mother = self.detector_construction.GetWorld()
        build(self.config.world, mother)
 
        self.build_phasespaces()       

    def update_geometry(self):
        """Recursively update the user defined geometry.
        """
        def update(volume):
            for name, params in volume.daughters.iteritems():
                physical = self.geometry[name]

                physical.SetRotation(params.rotation_matrix)
                physical.SetTranslation(params.translation_vector)

                update(volume.daughters[name])
        
        update(self.config.world)

        self.build_phasespaces() 

    ## Phasespace files ##

    def get_phasespace_filename(self, name):
        """Generate a filename from the name of the phasespace as in the `Linac` configuration
        """
        return "%s/%s_%s_%s.phsp" % (self.phsp_dir, name, self.name, self.run_id)
  
    def enable_phasespace(self, name):
        self.phasespaces.append(name)

    def disable_phasespace(self, name):
        """Close an open phasespace file, and remove it from the geometry.
        """
        self.detector_construction.RemovePhasespace(self.get_phasespace_filename(name))
        self.phasespaces.remove(name)

    def disable_all_phasespaces(self):
        map(self.disable_phasespace, self.phasespaces)

    def build_phasespaces(self):
        """Create an empty phasespace file to write into, and insert it into the geometry.
        """ 
        for phasespace in self.phasespaces:
            ps = self.config.phasespaces[phasespace]
            self.detector_construction.AddPhasespace(self.get_phasespace_filename(phasespace),
                    ps["radius"], ps["z_position"], ps["kill"])

    ## Run ##
 
    def beam_on(self, histories, fwhm=2.0*mm, energy=6*MeV):
        """Shoot particles from the primary generator into the geometry. Here we automatically
        select between a bare source, or phasespace if one is specified.
        """
        self.update_geometry()

        if self.source is not None: 
            self.primary_generator.SetSource(self.get_phasespace_filename(self.source))

            z = self.config.phasespaces[self.source]["z_position"]
            self.primary_generator.SetGantryRotation(self.config.world.daughters["head"].rotation_vector)
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


