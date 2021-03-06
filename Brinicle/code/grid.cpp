#include "header.h"

//Create the mesh and the FES
void Artic_sea::make_grid(const char *mesh_file){

    //Read mesh (serial)
    Mesh *mesh = new Mesh(mesh_file, 1, 1);
    dim = mesh->Dimension();
    
    //Calculate how many serial refinements are needed
    //More than 1000 cells per processor
    int elements = mesh->GetNE();
    int min_elements = 1000.*config.nproc;
    if (min_elements > elements)
        serial_refinements = min(config.refinements, (int)floor(log(min_elements/elements)/(dim*log(2.))));
    else
        serial_refinements = 0;
    
    if (!config.restart){
        //Refine mesh (serial)
        for (int ii = 0; ii < serial_refinements; ii++)
            mesh->UniformRefinement();
    }
    
    //Make mesh (parallel), delete the serial
    pmesh = new ParMesh(MPI_COMM_WORLD, *mesh);
    delete mesh;
    
    if (!config.restart){
        //Refine mesh (parallel)
        for (int ii = 0; ii < config.refinements - serial_refinements; ii++)
            pmesh->UniformRefinement();
    } else {
        //Read the input mesh
        std::ifstream in;
        std::ostringstream oss;
        oss << std::setw(10) << std::setfill('0') << config.pid;
        std::string n_mesh = "results/restart/pmesh_"+oss.str()+".msh";

        in.open(n_mesh.c_str(),std::ios::in);
        pmesh->Load(in,1,0); 
        in.close();
    }

    //Calculate minimum size of elements
    double null;
    pmesh->GetCharacteristics(h_min, null, null, null);

    //Create the FEM spaces associated with the mesh
    fec_H1 = new H1_FECollection(config.order, dim);
    fespace_H1 = new ParFiniteElementSpace(pmesh, fec_H1);
    size_H1 = fespace_H1->GlobalTrueVSize();

    fec_ND = new ND_FECollection(config.order, dim);
    fespace_ND = new ParFiniteElementSpace(pmesh, fec_ND);
    size_ND = fespace_ND->GlobalTrueVSize();

    //Create the block offsets
    block_offsets_H1[0] = 0;
    block_offsets_H1[1] = fespace_H1->TrueVSize();
    block_offsets_H1[2] = fespace_H1->TrueVSize();
    block_offsets_H1.PartialSum();

    X.Update(block_offsets_H1); X = 0.;
    Y.Update(block_offsets_H1); Y = 0.;
}
