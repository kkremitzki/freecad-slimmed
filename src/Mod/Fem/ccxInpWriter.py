import FemGui
import FreeCAD
import os
import time


class inp_writer:
    def __init__(self, dir_name, mesh_obj, mat_obj, fixed_obj, force_obj):
        self.mesh_object = mesh_obj
        self.material_objects = mat_obj
        self.fixed_objects = fixed_obj
        self.force_objects = force_obj
        self.base_name = dir_name + '/' + self.mesh_object.Name
        self.file_name = self.base_name + '.inp'
        print 'CalculiX .inp file will be written to: ', self.file_name

    def write_calculix_input_file(self):
        print 'write_calculix_input_file'
        self.mesh_object.FemMesh.writeABAQUS(self.file_name)

        # reopen file with "append" and add the analysis definition
        inpfile = open(self.file_name, 'a')
        inpfile.write('\n\n')
        self.write_material_element_sets(inpfile)
        self.write_fixed_node_sets(inpfile)
        self.write_load_node_sets(inpfile)
        self.write_materials(inpfile)
        self.write_step_begin(inpfile)
        self.write_constraints_fixed(inpfile)
        self.write_constraints_force(inpfile)
        self.write_outputs_types(inpfile)
        self.write_step_end(inpfile)
        self.write_footer(inpfile)
        inpfile.close()
        return self.base_name

    def write_material_element_sets(self, f):
        f.write('\n\n***********************************************************\n')
        f.write('** element sets for materials\n')
        for m in self.material_objects:
            mat_obj = m['Object']
            mat_obj_name = mat_obj.Name
            mat_name = mat_obj.Material['Name'][:80]

            print mat_obj_name, ':  ', mat_name
            f.write('*ELSET,ELSET=' + mat_obj_name + '\n')
            if len(self.material_objects) == 1:
                f.write('Eall\n')
            else:
                if mat_obj_name == 'MechanicalMaterial':
                    f.write('Eall\n')
            f.write('\n\n')

    def write_fixed_node_sets(self, f):
        f.write('\n\n***********************************************************\n')
        f.write('** node set for fixed constraint\n')
        for fobj in self.fixed_objects:
            fix_obj = fobj['Object']
            print fix_obj.Name
            f.write('*NSET,NSET=' + fix_obj.Name + '\n')
            for o, elem in fix_obj.References:
                fo = o.Shape.getElement(elem)
                n = []
                if fo.ShapeType == 'Face':
                    print '  Face Support (fixed face) on: ', elem
                    n = self.mesh_object.FemMesh.getNodesByFace(fo)
                elif fo.ShapeType == 'Edge':
                    print '  Line Support (fixed edge) on: ', elem
                    n = self.mesh_object.FemMesh.getNodesByEdge(fo)
                elif fo.ShapeType == 'Vertex':
                    print '  Point Support (fixed vertex) on: ', elem
                    n = self.mesh_object.FemMesh.getNodesByVertex(fo)
                for i in n:
                    f.write(str(i) + ',\n')
            f.write('\n\n')

    def write_load_node_sets(self, f):
        f.write('\n\n***********************************************************\n')
        f.write('** node sets for loads\n')
        for fobj in self.force_objects:
            frc_obj = fobj['Object']
            print frc_obj.Name
            f.write('*NSET,NSET=' + frc_obj.Name + '\n')
            NbrForceNodes = 0
            for o, elem in frc_obj.References:
                fo = o.Shape.getElement(elem)
                n = []
                if fo.ShapeType == 'Face':
                    print '  AreaLoad (face load) on: ', elem
                    n = self.mesh_object.FemMesh.getNodesByFace(fo)
                elif fo.ShapeType == 'Edge':
                    print '  Line Load (edge load) on: ', elem
                    n = self.mesh_object.FemMesh.getNodesByEdge(fo)
                elif fo.ShapeType == 'Vertex':
                    print '  Point Load (vertex load) on: ', elem
                    n = self.mesh_object.FemMesh.getNodesByVertex(fo)
                for i in n:
                    f.write(str(i) + ',\n')
                    NbrForceNodes = NbrForceNodes + 1   # NodeSum of mesh-nodes of ALL reference shapes from force_object
            # calculate node load
            if NbrForceNodes == 0:
                print '  Warning --> no FEM-Mesh-node to apply the load to was found?'
            else:
                fobj['NodeLoad'] = (frc_obj.Force) / NbrForceNodes
                #  FIXME this method is incorrect, but we don't have anything else right now
                #  Please refer to thread "CLOAD and DLOAD for the detailed description
                #  http://forum.freecadweb.org/viewtopic.php?f=18&t=10692
                f.write('** concentrated load [N] distributed on all mesh nodes of the given shapes\n')
                f.write('** ' + str(frc_obj.Force) + ' N / ' + str(NbrForceNodes) + ' Nodes = ' + str(fobj['NodeLoad']) + ' N on each node\n')
            if frc_obj.Force == 0:
                print '  Warning --> Force = 0'
            f.write('\n\n')

    def write_materials(self, f):
        f.write('\n\n***********************************************************\n')
        f.write('** materials\n')
        f.write('** youngs modulus unit is MPa = N/mm2\n')
        for m in self.material_objects:
            mat_obj = m['Object']
            # get material properties
            YM = FreeCAD.Units.Quantity(mat_obj.Material['YoungsModulus'])
            YM_in_MPa = YM.getValueAs('MPa')
            PR = float(mat_obj.Material['PoissonRatio'])
            mat_obj_name = mat_obj.Name
            mat_name = mat_obj.Material['Name'][:80]
            # write material properties
            f.write('*MATERIAL, NAME=' + mat_name + '\n')
            f.write('*ELASTIC \n')
            f.write('{}, '.format(YM_in_MPa))
            f.write('{0:.3f}\n'.format(PR))
            # write element properties
            if len(self.material_objects) == 1:
                f.write('*SOLID SECTION, ELSET=' + mat_obj_name + ', MATERIAL=' + mat_name + '\n\n')
            else:
                if mat_obj_name == 'MechanicalMaterial':
                    f.write('*SOLID SECTION, ELSET=' + mat_obj_name + ', MATERIAL=' + mat_name + '\n\n')

    def write_step_begin(self, f):
        f.write('\n\n\n\n***********************************************************\n')
        f.write('** one step is needed to calculate the mechanical analysis of FreeCAD\n')
        f.write('** loads are applied quasi-static, means without involving the time dimension\n')
        f.write('*STEP\n')
        f.write('*STATIC\n\n')

    def write_constraints_fixed(self, f):
        f.write('\n** constaints\n')
        for fixed_object in self.fixed_objects:
            fix_obj_name = fixed_object['Object'].Name
            f.write('*BOUNDARY\n')
            f.write(fix_obj_name + ',1\n')
            f.write(fix_obj_name + ',2\n')
            f.write(fix_obj_name + ',3\n\n')

    def write_constraints_force(self, f):
        f.write('\n** loads\n')
        f.write('** node loads, see load node sets for how the value is calculated!\n')
        for fobj in self.force_objects:
            if 'NodeLoad' in fobj:
                frc_obj = fobj['Object']
                node_load = fobj['NodeLoad']
                frc_obj_name = frc_obj.Name
                vec = frc_obj.DirectionVector
                f.write('*CLOAD\n')
                f.write('** force: ' + str(node_load) + ' N,  direction: ' + str(vec) + '\n')
                v1 = "{:.15}".format(repr(vec.x * node_load))
                v2 = "{:.15}".format(repr(vec.y * node_load))
                v3 = "{:.15}".format(repr(vec.z * node_load))
                f.write(frc_obj_name + ',1,' + v1 + '\n')
                f.write(frc_obj_name + ',2,' + v2 + '\n')
                f.write(frc_obj_name + ',3,' + v3 + '\n\n')

    def write_outputs_types(self, f):
        f.write('\n** outputs --> frd file\n')
        f.write('*NODE FILE\n')
        f.write('U\n')
        f.write('*EL FILE\n')
        f.write('S, E\n')
        f.write('** outputs --> dat file\n')
        f.write('*NODE PRINT , NSET=Nall \n')
        f.write('U \n')
        f.write('*EL PRINT , ELSET=Eall \n')
        f.write('S \n')
        f.write('\n\n')

    def write_step_end(self, f):
        f.write('*END STEP \n')

    def write_footer(self, f):
        FcVersionInfo = FreeCAD.Version()
        f.write('\n\n\n\n***********************************************************\n')
        f.write('**\n')
        f.write('**   CalculiX Inputfile\n')
        f.write('**\n')
        f.write('**   written by    --> FreeCAD ' + FcVersionInfo[0] + '.' + FcVersionInfo[1] + '.' + FcVersionInfo[2] + '\n')
        f.write('**   written on    --> ' + time.ctime() + '\n')
        f.write('**   file name     --> ' + os.path.basename(FreeCAD.ActiveDocument.FileName) + '\n')
        f.write('**   analysis name --> ' + FemGui.getActiveAnalysis().Name + '\n')
        f.write('**\n')
        f.write('**\n')
        f.write('**   Units\n')
        f.write('**\n')
        f.write('**   Geometry (mesh data)        --> mm\n')
        f.write("**   Materials (Young's modulus) --> N/mm2 = MPa\n")
        f.write('**   Loads (nodal loads)         --> N\n')
        f.write('**\n')
        f.write('**\n')
