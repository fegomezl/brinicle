#include "header.h"

double r_f(const Vector &x);
double r_inv_f(const Vector &x);
void zero_f(const Vector &x, Vector &f);
void r_inv_hat_f(const Vector &x, Vector &f);
void rot_f(const Vector &x, DenseMatrix &f);

double FusionPoint(const double S);
double Phase(const double T, const double S);
double HeatInertia(const double T, const double S);
double HeatDiffusivity(const double T, const double S);
double SaltDiffusivity(const double T, const double S);
double Impermeability(const double T, const double S);
double ExpansivityTemperature(const double T, const double S);
double ExpansivitySalinity(const double T, const double S);
double Buoyancy(const double T, const double S);

double T_bounded(const double T);
double S_bounded(const double S);

void Artic_sea::assemble_system(){

    //Define solution x
    if (!config.restart){
        temperature = new ParGridFunction(fespace_H1);
        salinity = new ParGridFunction(fespace_H1);
    } else {
        std::ifstream in;
        std::ostringstream oss;
        oss << std::setw(10) << std::setfill('0') << config.pid;
        std::string n_temperature = "results/restart/temperature_"+oss.str()+".gf";
        std::string n_salinity = "results/restart/salinity_"+oss.str()+".gf";

        in.open(n_temperature.c_str(),std::ios::in);
        temperature = new ParGridFunction(pmesh, in);
        in.close();

        in.open(n_salinity.c_str(),std::ios::in);
        salinity = new ParGridFunction(pmesh, in);
        in.close();
    }

    phase = new ParGridFunction(fespace_H1);
    vorticity = new ParGridFunction(fespace_H1);
    stream = new ParGridFunction(fespace_H1);
    velocity = new ParGridFunction(fespace_ND);
    rvelocity = new ParGridFunction(fespace_ND);

    Temperature = new HypreParVector(fespace_H1);
    Salinity = new HypreParVector(fespace_H1);
    rVelocity = new HypreParVector(fespace_ND);
    Velocity = new HypreParVector(fespace_ND);

    //Initialize operators
    transport_oper = new Transport_Operator(config, *fespace_H1, *fespace_ND, dim, pmesh->bdr_attributes.Max(), block_offsets_H1, *Temperature, *Salinity);
    flow_oper = new Flow_Operator(config, *fespace_H1, *fespace_ND, dim, pmesh->bdr_attributes.Max(), block_offsets_H1);

    //Solve initial velocity field
    flow_oper->SetParameters(*Temperature, *Salinity);
    flow_oper->Solve(Y, *Velocity, *rVelocity);

    //Set initial state
    temperature->Distribute(Temperature);
    salinity->Distribute(Salinity);
    vorticity->Distribute(Y.GetBlock(0));
    stream->Distribute(Y.GetBlock(1));
    velocity->Distribute(Velocity);
    rvelocity->Distribute(rVelocity);
    
    //Calculate phases
    for (int ii = 0; ii < phase->Size(); ii++)
        (*phase)(ii) = Phase((*temperature)(ii), (*salinity)(ii));

    //Normalize stream
    if (config.rescale){
        double stream_local_max = stream->Max(), stream_max;
        double stream_local_min = stream->Min(), stream_min;
        MPI_Allreduce(&stream_local_max, &stream_max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
        MPI_Allreduce(&stream_local_min, &stream_min, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
        for (int ii = 0; ii < stream->Size(); ii++)
                (*stream)(ii) = ((*stream)(ii)-stream_min)/(stream_max-stream_min);
    }

    //Set the ODE solver type
    arkode = new ARKStepSolver(MPI_COMM_WORLD, ARKStepSolver::IMPLICIT);
    arkode->Init(*transport_oper);
    arkode->SetSStolerances(config.reltol_sundials, config.abstol_sundials);
    arkode->SetMaxStep(dt);
    arkode->SetStepMode(ARK_ONE_STEP);
    ode_solver = arkode;

    //Open the paraview output and print initial state
    string folder = "results/graph"; 
    paraview_out = new ParaViewDataCollection(folder, pmesh);
    paraview_out->SetDataFormat(VTKFormat::BINARY);
    paraview_out->SetLevelsOfDetail(config.order);
    paraview_out->RegisterField("Temperature", temperature);
    paraview_out->RegisterField("Salinity", salinity);
    paraview_out->RegisterField("Phase", phase);
    paraview_out->RegisterField("Vorticity", vorticity);
    paraview_out->RegisterField("Stream", stream);
    paraview_out->RegisterField("Velocity", velocity);
    paraview_out->RegisterField("rVelocity", rvelocity);
    paraview_out->SetCycle(vis_print);
    paraview_out->SetTime(t);
    paraview_out->Save();

    //Start program check
    if (config.master){
        cout << left << setw(12)
             << "--------------------------------------------------\n"
             << left << setw(12)
             << "Step" << setw(12)
             << "Dt" << setw(12)
             << "Time" << setw(12)
             << "Progress"
             << left << setw(12)
             << "\n--------------------------------------------------\n";

        ofstream out;
        out.open("results/progress.txt", std::ios::trunc);
        out << left << setw(12) 
            << "Step" << setw(12)
            << "Dt" << setw(12)
            << "Time" << setw(12)
            << "Progress" << "\n";
    }
}

double r_f(const Vector &x){
    return Epsilon + x(0);
}

double r_inv_f(const Vector &x){
    return pow(Epsilon + x(0), -1);
}

void zero_f(const Vector &x, Vector &f){
    f(0) = 0.;
    f(1) = 0.;
}

void r_inv_hat_f(const Vector &x, Vector &f){
    f(0) = pow(Epsilon + x(0), -1);
    f(1) = 0.;
}

void rot_f(const Vector &x, DenseMatrix &f){
    f(0,0) = 0.;  f(0,1) = 1.;
    f(1,0) = -1.; f(1,1) = 0.;
}

double FusionPoint(const double S){
    double S_ = S;//S_bounded(S);
    return -(constants.FusionPoint_a*S_ + constants.FusionPoint_b*pow(S_, 3));
}

double Phase(const double T, const double S){
    //Dimentionless
    return 0.5*(1+tanh(5*EpsilonInv*(T-FusionPoint(S))));
}

double HeatInertia(const double T, const double S){
    return constants.m_s + (constants.m_l-constants.m_s)*Phase(T, S);
}

double HeatDiffusivity(const double T, const double S){ 
    return constants.d_temperature_s + (constants.d_temperature_l-constants.d_temperature_s)*Phase(T, S);
}

double SaltDiffusivity(const double T, const double S){
    return constants.d_salinity_s + (constants.d_salinity_l-constants.d_salinity_s)*Phase(T, S);
}

double Impermeability(const double T, const double S){
    //Dimentionless (Actually has 1/Lenght^2 but its not scale dependent)
    return Epsilon + pow(1-Phase(T, S), 2)/(pow(Phase(T, S), 3) + Epsilon);
} 

double ExpansivityTemperature(const double T, const double S){
    double T_ = T;//T_bounded(T);
    double S_ = S;//S_bounded(S);
    return Phase(T, S)*constants.Buoyancy_k*(
          (constants.Buoyancy_a1  + 
           constants.Buoyancy_a2*2*(T_)  + 
           constants.Buoyancy_a3*3*pow(T_, 2)    + 
           constants.Buoyancy_a4*4*pow(T_, 3))*(S_) + 
          (constants.Buoyancy_b1  + 
           constants.Buoyancy_b2*2*(T_))*pow(abs(S_), 1.5)
          );
} 

double ExpansivitySalinity(const double T, const double S){
    double T_ = T;//T_bounded(T);
    double S_ = S;//S_bounded(S);
    return Phase(T, S)*constants.Buoyancy_k*(
          (constants.Buoyancy_a0  + 
           constants.Buoyancy_a1*(T_)  + 
           constants.Buoyancy_a2*pow(T_, 2)    + 
           constants.Buoyancy_a3*pow(T_, 3)    +
           constants.Buoyancy_a4*pow(T_, 4))   + 
          (constants.Buoyancy_b0  + 
           constants.Buoyancy_b1*(T_)  +
           constants.Buoyancy_b2*pow(T_, 2))*1.5*pow(abs(S_), 0.5)  +
          (constants.Buoyancy_c0)*2*(S_)
          );
}

double Buoyancy(const double T, const double S){
    double T_ = T;//T_bounded(T);
    double S_ = S;//S_bounded(S);
    return Phase(T, S)*constants.Buoyancy_k*(
          (constants.Buoyancy_a0  + 
           constants.Buoyancy_a1*(T_)  + 
           constants.Buoyancy_a2*pow(T_, 2)      + 
           constants.Buoyancy_a3*pow(T_, 3)      +
           constants.Buoyancy_a4*pow(T_, 4))*(S_) + 
          (constants.Buoyancy_b0  + 
           constants.Buoyancy_b1*(T_)  +
           constants.Buoyancy_b2*pow(T_, 2))*pow(abs(S_), 1.5)  +
          (constants.Buoyancy_c0)*pow(S_, 2)
          );
}

double T_bounded(const double T){
    return 0.5*(TemperatureMin+TemperatureMax)
         + 0.5*(T-TemperatureMin)*tanh(5*EpsilonInv*(T-TemperatureMin))
         + 0.5*(TemperatureMax-T)*tanh(5*EpsilonInv*(T-TemperatureMax));
}

double S_bounded(const double S){
    return 0.5*(SalinityMin+SalinityMax)
           + 0.5*(S-SalinityMin)*tanh(5*EpsilonInv*(S-SalinityMin))
           + 0.5*(SalinityMax-S)*tanh(5*EpsilonInv*(S-SalinityMax));
}