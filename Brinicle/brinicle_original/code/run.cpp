#include "header.h"

Config::Config(int pid, int nproc):
    pid(pid),
    master(pid == 0),
    nproc(nproc)
{}

Artic_sea::Artic_sea(Config config):
    config(config),
    t(config.t_init), dt(config.dt_init), last(false),
    vis_steps(config.vis_steps_max), vis_print(0),
    pmesh(NULL), 
    fec_H1(NULL), fec_ND(NULL), 
    fespace_H1(NULL), fespace_ND(NULL),
    block_offsets_H1(3),
    temperature(NULL), salinity(NULL), phase(NULL), 
    vorticity(NULL), stream(NULL), 
    velocity(NULL), rvelocity(NULL), 
    Velocity(NULL), rVelocity(NULL),
    transport_oper(NULL), flow_oper(NULL),
    ode_solver(NULL), arkode(NULL),
    paraview_out(NULL)
{}

void Artic_sea::run(const char *mesh_file){
    //Run the program
    make_grid(mesh_file);
    assemble_system();
    for (iteration = 1, vis_iteration = 1; !last; iteration++, vis_iteration++)
        time_step();
    total_time = toc();
    output_results();
}

Transport_Operator::~Transport_Operator(){
    //Delete used memory
    delete m_theta; 
    delete m_phi;
    delete k_theta;
    delete k_phi;
    delete M_theta; 
    delete M_e_theta; 
    delete M_0_theta; 
    delete M_phi; 
    delete M_e_phi; 
    delete M_0_phi; 
    delete K_0_theta; 
    delete K_0_phi; 
    delete T_theta; 
    delete T_e_theta; 
    delete T_phi; 
    delete T_e_phi; 
}

Flow_Operator::~Flow_Operator(){
    delete f;
    delete g;
    delete m;
    delete d;
    delete c;
    delete ct;
    delete M;
    delete D;
    delete C;
    delete Ct;
}

Artic_sea::~Artic_sea(){
    delete pmesh;
    delete fec_H1;
    delete fec_ND;
    delete fespace_H1;
    delete fespace_ND;
    delete temperature;
    delete salinity;
    delete phase;
    delete vorticity;
    delete stream;
    delete velocity;
    delete rvelocity;
    delete Velocity;
    delete rVelocity;
    delete transport_oper;
    delete flow_oper;
    delete ode_solver;
    delete paraview_out;
    if (config.master) cout << "Memory deleted \n";
}
