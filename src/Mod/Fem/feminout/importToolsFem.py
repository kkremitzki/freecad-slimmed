# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Bernd Hahnebach <bernd@bimstatik.org>            *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD FEM import tools"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package importToolsFem
#  \ingroup FEM
#  \brief FreeCAD FEM import tools

import FreeCAD
from math import pow, sqrt
import numpy as np


def get_FemMeshObjectMeshGroups(fem_mesh_obj):
    """
        Get mesh groups from mesh. This also throws no exception if there
        is no Groups property at all (e.g. Netgen meshes).
    """
    fem_mesh = fem_mesh_obj.FemMesh
    try:
        gmshgroups = fem_mesh.Groups
    except:
        gmshgroups = ()

    return gmshgroups


def get_FemMeshObjectOrder(fem_mesh_obj):
    """
        Gets element order. Element order counting based on number of nodes on
        edges. Edge with 2 nodes -> linear elements, Edge with 3 nodes ->
        quadratic elements, and so on. No edges in mesh -> not determined.
        (Is this possible? Seems to be a very degenerate case.)
        If there are edges with different number of nodes appearing, return
        list of orders.
    """
    presumable_order = None

    edges = fem_mesh_obj.FemMesh.Edges

    if edges != ():
        edges_length_set = list({len(fem_mesh_obj.FemMesh.getElementNodes(e)) for e in edges})
        # only need set to eliminate double entries

        if len(edges_length_set) == 1:
            presumable_order = edges_length_set[0] - 1
        else:
            presumable_order = [el - 1 for el in edges_length_set]
    else:
        print("Found no edges in mesh: Element order determination does not work without them.")

    return presumable_order


def get_FemMeshObjectDimension(fem_mesh_obj):
    """ Count all entities in an abstract sense, to distinguish which dimension the mesh is
        (i.e. linemesh, facemesh, volumemesh)
    """
    dim = None

    if fem_mesh_obj.FemMesh.Nodes != ():
        dim = 0
    if fem_mesh_obj.FemMesh.Edges != ():
        dim = 1
    if fem_mesh_obj.FemMesh.Faces != ():
        dim = 2
    if fem_mesh_obj.FemMesh.Volumes != ():
        dim = 3

    return dim


def get_FemMeshObjectElementTypes(fem_mesh_obj, remove_zero_element_entries=True):
    """
        Spit out all elements in the mesh with their appropriate dimension.
    """
    FreeCAD_element_names_dims = {
        "Node": 0, "Edge": 1, "Hexa": 3, "Polygon": 2, "Polyhedron": 3,
        "Prism": 3, "Pyramid": 3, "Quadrangle": 2, "Tetra": 3, "Triangle": 2}

    eval_dict = locals()  # to access local variables from eval
    elements_list_with_zero = [(eval("fem_mesh_obj.FemMesh." + s + "Count", eval_dict), s, d) for (s, d) in FreeCAD_element_names_dims.items()]
    # ugly but necessary
    if remove_zero_element_entries:
        elements_list = [(num, s, d) for (num, s, d) in elements_list_with_zero if num > 0]
    else:
        elements_list = elements_list_with_zero

    return elements_list


def get_MaxDimElementFromList(elem_list):
    """
        Gets element with the maximal dimension in the mesh to determine cells.
    """
    elem_list.sort(key=lambda t: t[2])
    return elem_list[-1]


def make_femmesh(mesh_data):
    ''' makes an FreeCAD FEM Mesh object from FEM Mesh data
    '''
    import Fem
    mesh = Fem.FemMesh()
    m = mesh_data
    if ('Nodes' in m) and (len(m['Nodes']) > 0):
        FreeCAD.Console.PrintLog("Found: nodes\n")
        if (
            ('Seg2Elem' in m)
            or ('Seg3Elem' in m)
            or ('Tria3Elem' in m)
            or ('Tria6Elem' in m)
            or ('Quad4Elem' in m)
            or ('Quad8Elem' in m)
            or ('Tetra4Elem' in m)
            or ('Tetra10Elem' in m)
            or ('Penta6Elem' in m)
            or ('Penta15Elem' in m)
            or ('Hexa8Elem' in m)
            or ('Hexa20Elem' in m)
        ):

            nds = m['Nodes']
            FreeCAD.Console.PrintLog("Found: elements\n")
            for i in nds:
                n = nds[i]
                mesh.addNode(n[0], n[1], n[2], i)
            elms_hexa8 = m['Hexa8Elem']
            for i in elms_hexa8:
                e = elms_hexa8[i]
                mesh.addVolume([e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7]], i)
            elms_penta6 = m['Penta6Elem']
            for i in elms_penta6:
                e = elms_penta6[i]
                mesh.addVolume([e[0], e[1], e[2], e[3], e[4], e[5]], i)
            elms_tetra4 = m['Tetra4Elem']
            for i in elms_tetra4:
                e = elms_tetra4[i]
                mesh.addVolume([e[0], e[1], e[2], e[3]], i)
            elms_tetra10 = m['Tetra10Elem']
            for i in elms_tetra10:
                e = elms_tetra10[i]
                mesh.addVolume([e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7], e[8], e[9]], i)
            elms_penta15 = m['Penta15Elem']
            for i in elms_penta15:
                e = elms_penta15[i]
                mesh.addVolume([e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7], e[8], e[9],
                                e[10], e[11], e[12], e[13], e[14]], i)
            elms_hexa20 = m['Hexa20Elem']
            for i in elms_hexa20:
                e = elms_hexa20[i]
                mesh.addVolume([e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7], e[8], e[9],
                                e[10], e[11], e[12], e[13], e[14], e[15], e[16], e[17], e[18], e[19]], i)
            elms_tria3 = m['Tria3Elem']
            for i in elms_tria3:
                e = elms_tria3[i]
                mesh.addFace([e[0], e[1], e[2]], i)
            elms_tria6 = m['Tria6Elem']
            for i in elms_tria6:
                e = elms_tria6[i]
                mesh.addFace([e[0], e[1], e[2], e[3], e[4], e[5]], i)
            elms_quad4 = m['Quad4Elem']
            for i in elms_quad4:
                e = elms_quad4[i]
                mesh.addFace([e[0], e[1], e[2], e[3]], i)
            elms_quad8 = m['Quad8Elem']
            for i in elms_quad8:
                e = elms_quad8[i]
                mesh.addFace([e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7]], i)
            elms_seg2 = m['Seg2Elem']
            for i in elms_seg2:
                e = elms_seg2[i]
                mesh.addEdge([e[0], e[1]], i)
            elms_seg3 = m['Seg3Elem']
            for i in elms_seg3:
                e = elms_seg3[i]
                mesh.addEdge([e[0], e[1], e[2]], i)
            FreeCAD.Console.PrintLog("imported mesh: {} nodes, {} HEXA8, {} PENTA6, {} TETRA4, {} TETRA10, {} PENTA15".format(
                len(nds), len(elms_hexa8), len(elms_penta6), len(elms_tetra4), len(elms_tetra10), len(elms_penta15)
            ))
            FreeCAD.Console.PrintLog("imported mesh: {} HEXA20, {} TRIA3, {} TRIA6, {} QUAD4, {} QUAD8, {} SEG2, {} SEG3".format(
                len(elms_hexa20), len(elms_tria3), len(elms_tria6), len(elms_quad4), len(elms_quad8), len(elms_seg2), len(elms_seg3)
            ))
        else:
            FreeCAD.Console.PrintError("No Elements found!\n")
    else:
        FreeCAD.Console.PrintError("No Nodes found!\n")
    return mesh


def fill_femresult_mechanical(res_obj, result_set):
    ''' fills a FreeCAD FEM mechanical result object with result data
    '''
    if 'number' in result_set:
        eigenmode_number = result_set['number']
    else:
        eigenmode_number = 0
    if 'time' in result_set:
        step_time = result_set['time']
        step_time = round(step_time, 2)

    # if disp exists, fill res_obj.NodeNumbers and res_obj.DisplacementVectors as well as stress and strain
    if 'disp' in result_set:
        disp = result_set['disp']
        displacement = []
        for k, v in disp.items():
            displacement.append(v)

        x_max, y_max, z_max = map(max, zip(*displacement))
        if eigenmode_number > 0:
            span = get_span(res_obj.Mesh.FemMesh.Nodes.items())
            max_disp = max(x_max, y_max, z_max)
            # Allow for max displacement to be 0.1% of the span
            # FIXME - add to Preferences
            max_allowed_disp = 0.001 * span
            scale = max_allowed_disp / max_disp
        else:
            scale = 1.0

        res_obj.DisplacementVectors = list(map((lambda x: x * scale), disp.values()))
        res_obj.NodeNumbers = list(disp.keys())

        # fill res_obj.StressVectors if they exist
        if 'stress' in result_set:
            stress = result_set['stress']
            stressv1 = {}
            for i, stval in enumerate(stress.values()):  # i .. stresstuple .. (Sxx, Syy, Szz, Sxy, Syz, Szx)
                stressv1[i] = (FreeCAD.Vector(stval[0], stval[1], stval[2]))  # Sxx, Syy, Szz
            res_obj.StressVectors = list(map((lambda x: x * scale), stressv1.values()))
            stress_keys = list(stress.keys())
            if (res_obj.NodeNumbers != 0 and res_obj.NodeNumbers != stress_keys):
                print("Inconsistent FEM results: element number for Stress doesn't equal element number for Displacement {} != {}"
                      .format(res_obj.NodeNumbers, len(res_obj.StressValues)))

        # fill res_obj.StrainVectors if they exist
        if 'strainv' in result_set:
            strainv = result_set['strainv']
            res_obj.StrainVectors = list(map((lambda x: x * scale), strainv.values()))

        # calculate von Mises, principal and max Shear and fill them in res_obj
        if 'stress' in result_set:
            stress = result_set['stress']
            if len(stress) > 0:
                mstress = []
                prinstress1 = []
                prinstress2 = []
                prinstress3 = []
                shearstress = []
                for i in stress.values():
                    mstress.append(calculate_von_mises(i))
                    prin1, prin2, prin3, shear = calculate_principal_stress(i)
                    prinstress1.append(prin1)
                    prinstress2.append(prin2)
                    prinstress3.append(prin3)
                    shearstress.append(shear)
                if eigenmode_number > 0:
                    res_obj.StressValues = list(map((lambda x: x * scale), mstress))
                    res_obj.PrincipalMax = list(map((lambda x: x * scale), prinstress1))
                    res_obj.PrincipalMed = list(map((lambda x: x * scale), prinstress2))
                    res_obj.PrincipalMin = list(map((lambda x: x * scale), prinstress3))
                    res_obj.MaxShear = list(map((lambda x: x * scale), shearstress))
                    res_obj.Eigenmode = eigenmode_number
                else:
                    res_obj.StressValues = mstress
                    res_obj.PrincipalMax = prinstress1
                    res_obj.PrincipalMed = prinstress2
                    res_obj.PrincipalMin = prinstress3
                    res_obj.MaxShear = shearstress

        # fill Equivalent Plastic strain if they exist
        if 'peeq' in result_set:
            Peeq = result_set['peeq']
            if len(Peeq) > 0:
                if len(Peeq.values()) != len(disp.values()):
                    Pe = []
                    Pe_extra_nodes = Peeq.values()
                    nodes = len(disp.values())
                    for i in range(nodes):
                        Pe_value = Pe_extra_nodes[i]
                        Pe.append(Pe_value)
                    res_obj.Peeq = Pe
                else:
                    res_obj.Peeq = Peeq.values()

    # fill res_obj.Temperature if they exist
    # TODO, check if it is possible to have Temperature without disp, we would need to set NodeNumbers than
    if 'temp' in result_set:
        Temperature = result_set['temp']
        if len(Temperature) > 0:
            if len(Temperature.values()) != len(disp.values()):
                Temp = []
                Temp_extra_nodes = Temperature.values()
                nodes = len(disp.values())
                for i in range(nodes):
                    Temp_value = Temp_extra_nodes[i]
                    Temp.append(Temp_value)
                res_obj.Temperature = list(map((lambda x: x), Temp))
            else:
                res_obj.Temperature = list(map((lambda x: x), Temperature.values()))
            res_obj.Time = step_time

    # fill res_obj.MassFlow
    if 'mflow' in result_set:
        MassFlow = result_set['mflow']
        if len(MassFlow) > 0:
            res_obj.MassFlowRate = list(map((lambda x: x), MassFlow.values()))
            res_obj.Time = step_time
            res_obj.NodeNumbers = list(MassFlow.keys())  # disp does not exist, res_obj.NodeNumbers needs to be set

    # fill res_obj.NetworkPressure, disp does not exist, see MassFlow
    if 'npressure' in result_set:
        NetworkPressure = result_set['npressure']
        if len(NetworkPressure) > 0:
            res_obj.NetworkPressure = list(map((lambda x: x), NetworkPressure.values()))
            res_obj.Time = step_time

    return res_obj


# helper
def calculate_von_mises(i):
    # Von mises stress (http://en.wikipedia.org/wiki/Von_Mises_yield_criterion)
    s11 = i[0]
    s22 = i[1]
    s33 = i[2]
    s12 = i[3]
    s23 = i[4]
    s31 = i[5]
    s11s22 = pow(s11 - s22, 2)
    s22s33 = pow(s22 - s33, 2)
    s33s11 = pow(s33 - s11, 2)
    s12s23s31 = 6 * (pow(s12, 2) + pow(s23, 2) + pow(s31, 2))
    vm_stress = sqrt(0.5 * (s11s22 + s22s33 + s33s11 + s12s23s31))
    return vm_stress


def calculate_principal_stress(i):
    sigma = np.array([[i[0], i[3], i[5]],
                      [i[3], i[1], i[4]],
                      [i[5], i[4], i[2]]])  # https://forum.freecadweb.org/viewtopic.php?f=18&t=24637&start=10#p240408

    try:  # it will fail if NaN is inside the array,
        # compute principal stresses
        eigvals = list(np.linalg.eigvalsh(sigma))
        eigvals.sort()
        eigvals.reverse()
        maxshear = (eigvals[0] - eigvals[2]) / 2.0
        return (eigvals[0], eigvals[1], eigvals[2], maxshear)
    except:
        return (float('NaN'), float('NaN'), float('NaN'), float('NaN'))
    # TODO might be possible without a try except for NaN, https://forum.freecadweb.org/viewtopic.php?f=22&t=33911&start=10#p284229

def get_span(node_items):
    positions = []  # list of node vectors
    for k, v in node_items:
        positions.append(v)
    p_x_max, p_y_max, p_z_max = map(max, zip(*positions))
    p_x_min, p_y_min, p_z_min = map(min, zip(*positions))
    x_span = abs(p_x_max - p_x_min)
    y_span = abs(p_y_max - p_y_min)
    z_span = abs(p_z_max - p_z_min)
    span = max(x_span, y_span, z_span)
    return span
