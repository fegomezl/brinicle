#include "header.h"

//Dimensions of the mesh
double Rmin;
double Zmin;
double Rmax;
double Zmax;

int main(int argc, char *argv[]){
    //Define MPI parameters
    int nproc = 0, pid = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);

    //Define program paramenters
    const char *mesh_file;
    Config config((pid == 0), nproc);
    int nDeltaT = 0;
    int nEpsilonT = 0;

    OptionsParser args(argc, argv);
    args.AddOption(&mesh_file, "-m", "--mesh",
                   "Mesh file to use.");
    args.AddOption(&Rmin, "-Rmin", "--Rmin",
                   "Minimum R border.");
    args.AddOption(&Rmax, "-Rmax", "--Rmax",
                   "Maximum R border.");
    args.AddOption(&Zmin, "-Zmin", "--Zmin",
                   "Minimum Z border.");
    args.AddOption(&Zmax, "-Zmax", "--Zmax",
                   "Maximum Z border.");

    args.AddOption(&config.dt_init, "-dt", "--time_step",
                   "Initial time step.");
    args.AddOption(&config.t_final, "-t_f", "--t_final",
                   "Final time.");
    args.AddOption(&config.vis_steps_max, "-v_s", "--visualization_steps",
                   "Visualize every n-th timestep.");

    args.AddOption(&config.refinements, "-ref", "--refinements",
                   "Number of total uniform refinements.");
    args.AddOption(&config.order, "-o", "--order",
                   "Finite element order (polynomial degree) or -1 for isoparametric space.");
    args.AddOption(&config.abstol_conduction, "-abstol_c", "--tolabsoluteConduction",
                   "Absolute tolerance of Conduction.");
    args.AddOption(&config.reltol_conduction, "-reltol_c", "--tolrelativeConduction",
                   "Relative tolerance of Conduction.");
    args.AddOption(&config.iter_conduction, "-iter_c", "--iterationsConduction",
                   "Iterations of Conduction.");
    args.AddOption(&config.abstol_sundials, "-abstol_s", "--tolabsoluteSUNDIALS",
                   "Absolute tolerance of SUNDIALS.");
    args.AddOption(&config.reltol_sundials, "-reltol_s", "--tolrelativeSUNDIALS",
                   "Relative tolerance of SUNDIALS.");

    args.AddOption(&config.a_l, "-a_l", "--a_l",
                   "Liquid thermal diffusivity.");
    args.AddOption(&config.a_s, "-a_s", "--a_s",
                   "Solid thermal diffusivity.");
    args.AddOption(&config.d_l, "-d_l", "--d_l",
                   "Liquid mass diffusivity.");
    args.AddOption(&config.d_s, "-d_s", "--d_s",
                   "Solid mass diffusivity.");
    args.AddOption(&config.L_l, "-L_l", "--L_l",
                   "Liquid latent term (L/c).");
    args.AddOption(&config.L_s, "-L_s", "--L_s",
                   "Solid latent term (L/c).");
    args.AddOption(&nDeltaT, "-DT", "--DeltaT",
                   "Temperature interface interval (10^(-n)).");
    args.AddOption(&nEpsilonT, "-ET", "--EpsilonT",
                   "Epsilon constant for temperature (1/(x+e)) (10^(-n)).");

    //Check if parameters were read correctly
    args.Parse();
    if (!args.Good()){
        if (config.master) args.PrintUsage(cout);
        MPI_Finalize();
        return 1;
    }
    if (config.master) args.PrintOptions(cout);

    //Run the program
    {
        tic();
        config.invDeltaT = pow(10, nDeltaT);
        config.EpsilonT = pow(10, -nEpsilonT);
        Artic_sea artic_sea(config);
        artic_sea.run(mesh_file); 
    }

    MPI_Finalize();

    return 0;
}
