#include "header.h"

//Rotational functions
double r_f(const Vector &x);
void zero_f(const Vector &x, Vector &f);

void Artic_sea::assemble_system(){
    //Initialize the system
    t = 0;
    dt = config.dt_init;
    last = false;
    vis_steps = config.vis_steps_max;
    vis_impressions = 0;

    //Define solution x
    x = new ParGridFunction(fespace);
    X = new HypreParVector(fespace);

    //Initialize operators
    oper = new Conduction_Operator(config, *fespace, dim, pmesh->bdr_attributes.Max(), *X);

    //Read initial global state
    x->Distribute(X);

    //Set the ODE solver type
    arkode = new ARKStepSolver(MPI_COMM_WORLD, ARKStepSolver::IMPLICIT);
    arkode->Init(*oper);
    arkode->SetSStolerances(config.reltol_sundials, config.abstol_sundials);
    arkode->SetMaxStep(dt);
    arkode->SetStepMode(ARK_ONE_STEP);
    ode_solver = arkode;

    //Open the paraview output and print initial state
    string folder = "results/graph"; 
    paraview_out = new ParaViewDataCollection(folder, pmesh);
    paraview_out->SetDataFormat(VTKFormat::BINARY);
    paraview_out->SetLevelsOfDetail(config.order);
    paraview_out->RegisterField("Temperature", x);
    paraview_out->SetCycle(vis_impressions);
    paraview_out->SetTime(t);
    paraview_out->Save();

    //Start program check
    if (config.master)
        cout << left << setw(12)
             << "--------------------------------------------------\n"
             << left << setw(12)
             << "Step" << setw(12)
             << "Dt" << setw(12)
             << "Time" << setw(12)
             << "Progress"
             << left << setw(12)
             << "\n--------------------------------------------------\n";

}

Conduction_Operator::Conduction_Operator(Config config, ParFiniteElementSpace &fespace, int dim, int attributes, Vector &X):
    TimeDependentOperator(fespace.GetTrueVSize(), 0.),
    config(config),
    fespace(fespace),
    m(NULL), k(NULL), 
    M(NULL), M_e(NULL), M_0(NULL),
    K_0(NULL), 
    T(NULL), T_e(NULL),
    Z(&fespace),
    aux(&fespace), aux_C(&fespace), aux_K(&fespace), aux_L(&fespace),
    coeff_r(r_f), zero(dim, zero_f),
    coeff_rC(coeff_r, coeff_r),
    coeff_rK(coeff_r, coeff_r), 
    dHdT(zero, zero), dT_2(zero, zero),
    M_solver(fespace.GetComm()), T_solver(fespace.GetComm())
{
    //Set boundary conditions
    //
    //                  1
    //            /------------\
    //            |            |
    //    (0)    2|            |3
    //            |            |
    //            \------------/
    //                  0

    Array<int> ess_bdr(attributes);
    ess_bdr[0] = 1;  ess_bdr[1] = 1;
    ess_bdr[2] = 0;  ess_bdr[3] = 0;
    fespace.GetEssentialTrueDofs(ess_bdr, ess_tdof_list);

    Array<int> ess_bdr_l(attributes);
    ess_bdr_l = 0; ess_bdr_l[1] = 1;

    Array<int> ess_bdr_s(attributes);
    ess_bdr_s = 0; ess_bdr_s[0] = 1;

    //Define initial condition
    ConstantCoefficient bdr_s(T_s), bdr_l(T_l);
    aux.ProjectCoefficient(bdr_l);
    aux.ProjectBdrCoefficient(bdr_l, ess_bdr_l);
    aux.ProjectBdrCoefficient(bdr_s, ess_bdr_s);
    aux.GetTrueDofs(X);

    //Configure M solver
    M_solver.SetTol(config.reltol_conduction);
    M_solver.SetAbsTol(config.abstol_conduction);
    M_solver.SetMaxIter(config.iter_conduction);
    M_solver.SetPrintLevel(0);
    M_prec.SetPrintLevel(0);
    M_solver.SetPreconditioner(M_prec);

    //Configure T solver
    T_solver.SetTol(config.reltol_conduction);
    T_solver.SetAbsTol(config.abstol_conduction);
    T_solver.SetMaxIter(config.iter_conduction);
    T_solver.SetPrintLevel(0);
    T_prec.SetPrintLevel(0);
    T_solver.SetPreconditioner(T_prec);

    SetParameters(X);
}

double r_f(const Vector &x){
    return x(0);
}

void zero_f(const Vector &x, Vector &f){
    f(0) = 0.;
    f(1) = 0.;
}
