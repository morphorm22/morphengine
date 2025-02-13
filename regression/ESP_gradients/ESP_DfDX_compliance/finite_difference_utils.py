import exodus

def forward_difference(platoAnalyze, fval, originalMeshName, meshName, dim, perturbNode, step):

    # forward step
    perturb_mesh(originalMeshName, meshName, dim, step, perturbNode)
    platoAnalyze.compute("Reinitialize")
    platoAnalyze.compute("Compute Objective Value")
    f_plus = platoAnalyze.exportData("Objective Value", "SCALAR")

    # compute fd value
    DfDX = (f_plus - fval) / step

    return DfDX

def save_original_mesh(meshName):
    mesh = exodus.ExodusDB()
    mesh.read(meshName)

    savedName = meshName + ".orig"
    mesh.write(savedName)
    return savedName

def perturb_mesh(originalMeshName, meshName, dim, step, perturbNode):

    # read in mesh
    mesh = exodus.ExodusDB()
    mesh.read(originalMeshName)

    # perturb coords
    coords = mesh.elementBlocks[0].coordinates[dim]
    coords[perturbNode] += step

    # write mesh with new coords
    mesh.write(meshName)
