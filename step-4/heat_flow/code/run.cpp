#include "header.h"

Config::Config(bool master, int nproc):
    master(master),
    nproc(nproc)
{}

Artic_sea::Artic_sea(Config config):
    config(config),
    pmesh(NULL), fec(NULL), fec_v(NULL), fespace(NULL), fespace_v(NULL),
    theta(NULL), Theta(NULL), Psi(NULL),
    cond_oper(NULL), flow_oper(NULL),
    ode_solver(NULL), cvode(NULL), arkode(NULL),
    paraview_out(NULL)
{}

void Artic_sea::run(const char *mesh_file){
    //Run the program
    make_grid(mesh_file);
    assemble_system();
    for (iteration = 1, vis_iteration = 1; !last; iteration++, vis_iteration++)
        time_step();
    output_results();
}

Conduction_Operator::~Conduction_Operator(){
    //Delete used memory
    delete m, k, t;
}

Flow_Operator::~Flow_Operator(){
    delete f, g;
    delete m, d, c, ct;
    delete M, D, C;
    delete psi, w, v;
    delete psi_aux, w_aux, theta_aux;
    delete theta_eta, theta_rho;
}

Artic_sea::~Artic_sea(){
    delete pmesh, fec, fespace;
    delete fec_v, fespace_v;
    delete theta, Theta, Psi;
    delete cond_oper, flow_oper;
    delete ode_solver;
    delete paraview_out;
    if (config.master) cout << "Memory deleted \n";
}
