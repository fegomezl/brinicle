#include "header.h"

void Artic_sea::time_step(){
    //Update iteration parameters
    last = (t >= config.t_final - 1e-8*config.dt_init);
    dt = min(dt, config.t_final - t);

    //Perform the time_step
    oper->SetParameters(*X);
    ode_solver->Step(*X, t, dt);

    //Update visualization steps
    vis_steps = (dt == config.dt_init) ? config.vis_steps_max : int((config.dt_init/dt)*config.vis_steps_max);

    if (last || vis_steps <= vis_iteration){
        //Update parameters
        vis_iteration = 0;
        vis_impressions++;

        //Update information
        x->SetFromTrueDofs(*X);

        //Graph
        paraview_out->SetCycle(vis_impressions);
        paraview_out->SetTime(t);
        paraview_out->Save();
    }

    //Print the system state
    double percentage = 100*t/config.t_final;
    string progress = to_string((int)percentage)+"%";
    if (config.master){
        cout.precision(4);
        cout << left << setw(12)
             << iteration << setw(12)
             << dt << setw(12)
             << t  << setw(12)
             << progress << "\r";
        cout.flush();
    }
}

void Conduction_Operator::SetParameters(const Vector &X){
    //Create the auxiliar grid functions
    aux.SetFromTrueDofs(X);

    //Associate the values of each auxiliar function
    for (int ii = 0; ii < aux.Size(); ii++){
        aux(ii) -= config.T_f;

        if (aux(ii) > 0){
            aux_C(ii) = config.c_l;
            aux_K(ii) = config.k_l;
        } else {
            aux_C(ii) = config.c_s;
            aux_K(ii) = config.k_s;
        }

        aux_L(ii) = 0.5*config.L*(1 + tanh(5*config.invDeltaT*aux(ii)));
    }

    //Set the grid coefficients
    GridFunctionCoefficient coeff_C(&aux_C);
    GridFunctionCoefficient coeff_K(&aux_K);

    //Construct latent heat term
    GradientGridFunctionCoefficient dT(&aux);
    GradientGridFunctionCoefficient dH(&aux_L);
    
    dHdT.SetACoef(dH);  dT_2.SetACoef(dT);
    dHdT.SetBCoef(dT);  dT_2.SetBCoef(dT);

    SumCoefficient dT_2e(config.EpsilonT, dT_2);
    
    PowerCoefficient inv_dT_2(dT_2e, -1.);
    ProductCoefficient LDeltaT(dHdT, inv_dT_2);

    SumCoefficient coeff_CL(coeff_C, LDeltaT);

    //Construct final coefficients
    coeff_rC.SetBCoef(coeff_CL);
    coeff_rK.SetBCoef(coeff_K); 

    //Create corresponding bilinear forms
    if (m) delete m;
    if (M) delete M;
    if (M_e) delete M_e;
    if (M_0) delete M_0;
    m = new ParBilinearForm(&fespace);
    m->AddDomainIntegrator(new MassIntegrator(coeff_rC));
    m->Assemble();
    m->Finalize();
    M = m->ParallelAssemble();
    M_e = M->EliminateRowsCols(ess_tdof_list);
    M_0 = m->ParallelAssemble();

    M_prec.SetOperator(*M);
    M_solver.SetOperator(*M);

    if (k) delete k;
    if (K_0) delete K_0;
    k = new ParBilinearForm(&fespace);
    k->AddDomainIntegrator(new DiffusionIntegrator(coeff_rK));
    k->Assemble();
    k->Finalize();
    K_0 = k->ParallelAssemble();    
}

void Conduction_Operator::Mult(const Vector &X, Vector &dX_dt) const{
    //From  M(dX_dt) + K(X) = F
    //Solve M(dX_dt) + K(X) = F for dX_dt
    
    Z = 0.;
    dX_dt = 0.;
    
    K_0->Mult(-1., X, 1., Z);
    EliminateBC(*M, *M_e, ess_tdof_list, dX_dt, Z);

    M_solver.Mult(Z, dX_dt);
}

int Conduction_Operator::SUNImplicitSetup(const Vector &X, const Vector &B, int j_update, int *j_status, double scaled_dt){
    //Setup the ODE Jacobian T = M + dt*K

    if (T) delete T;
    if (T_e) delete T_e;
    T = Add(1., *M_0, scaled_dt, *K_0);
    T_e = T->EliminateRowsCols(ess_tdof_list);
    T_prec.SetOperator(*T);
    T_solver.SetOperator(*T);

    *j_status = 1;
    return 0;
}

int Conduction_Operator::SUNImplicitSolve(const Vector &X, Vector &X_new, double tol){
    //From  M(dX_dt) + K(X) = F
    //Solve M(X_new - X) + dt*K(X_new) = dt*F for X_new

    Z = 0.;
    X_new = X;

    M_0->Mult(X, Z);
    EliminateBC(*T, *T_e, ess_tdof_list, X_new, Z);

    T_solver.Mult(Z, X_new);
    return 0;
}
