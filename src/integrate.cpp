#include "integrate.hpp"
#include "data.hpp"
#include <vector>

using namespace std;

/**
 * Euler integration:
 * Given dy/dt = f(t, y(t)) and y(t0) = y0. With a timestep h.
 * y_{n+1} = y_{n} + h*f(t_{n}, y_{n})
 * Example 1: (Exact result is exp(t))
 * With dy/dt = y, and y(0) = 1, with a timestep of 0.5, find y(4) (exact: 54.59815)
 * y0 = 1,       t0 = 0,   y1 = 1.5
 * y1 = 1.5,     t1 = 0.5, y2 = 2.25
 * y2 = 2.25,    t2 = 1.0, y3 = 3.375
 * y3 = 3.375,   t3 = 1.5, y4 = 5.0625
 * y4 = 5.0625,  t4 = 2.0, y5 = 7.5938
 * y5 = 7.5938,  t5 = 2.5, y6 = 11.3906
 * y6 = 11.3906, t6 = 3.0, y7 = 17.0859
 * y7 = 17.0859, t7 = 3.5, y8 = 25.6289
 * => y(4) = 25.6289
 **/

void Euler(ParticleState &state, double time, double deltaTime,
           DeltaState (*evaluateFunc)(const ParticleState&, double, double, const DeltaState&))
{
    DeltaState k1 = evaluateFunc(state, time, deltaTime, DeltaState());
    
    state.pos = state.pos + deltaTime * k1.vel;
    state.vel = state.vel + deltaTime * k1.accel;
}

/**
 * Mid-point integration
 * Given dy/dt = f(t, y(t)) and y(t0) = y0. With a timestep h.
 * y_{n+1} = y_{n} + h*f(t_{n}+h/2, y_{n}+h/2*f(t_{n},y_{n}))
 * Example 1: (Exact result is exp(t)
 * With dy/dt = y, and y(0) = 1, with a timestep of 0.5, find y(4) (exact: 54.59815)
 * y0 = 1,       t0 = 0,   y0.5 = 1.25,    y1 = 1.625
 * y1 = 1.625,   t1 = 0.5, y1.5 = 2.0313,  y2 = 2.6406
 * y2 = 2.6406,  t2 = 1.0, y2.5 = 3.3008,  y3 = 4.2910
 * y3 = 4.3910,  t3 = 1.5, y3.5 = 5.3638,  y4 = 6.9729
 * y4 = 6.9729,  t4 = 2.0, y4.5 = 8.7161,  y5 = 11.3310
 * y5 = 11.3310, t5 = 2.5, y5.5 = 14.1637, y6 = 18.4128
 * y6 = 18.4128, t6 = 3.0, y6.5 = 23.0160, y7 = 29.9208
 * y7 = 29.9208, t7 = 3.5, y7.5 = 37.4010, y8 = 48.6213
 * => y(4) = 48.6213
 **/

void MidPoint(ParticleState &state, double time, double deltaTime,
              DeltaState (*evaluateFunc)(const ParticleState&, double, double, const DeltaState&))
{
    DeltaState k1 = evaluateFunc(state, time, 0.5*deltaTime, DeltaState());
    
    state.pos = state.pos + deltaTime * (k1.vel + (deltaTime/2) * k1.vel);
    state.vel = state.vel + deltaTime * (k1.accel * 1.5);
}

void Ralston(ParticleState &state,double time, double deltaTime,
             DeltaState (*evaluateFunc)(const ParticleState&, double, double, const DeltaState&))
{
    DeltaState k1, k2;
    k1 = evaluateFunc(state, time, 0., DeltaState());
    k2 = evaluateFunc(state, time, (2.0/3.0)*deltaTime, k1);
    
    state.pos = state.pos + deltaTime * (0.25 * k1.vel + 0.75 * k2.vel);
    state.vel = state.vel + deltaTime * (0.25 * k1.accel + 0.75 * k2.accel);
}

void RK4(ParticleState &state, double time, double deltaTime,
         DeltaState (*evaluateFunc)(const ParticleState&, double, double, const DeltaState&))
{
    DeltaState k1, k2, k3, k4;
    k1 = evaluateFunc(state, time, 0., DeltaState());
    k2 = evaluateFunc(state, time, 0.5*deltaTime, k1);
    k3 = evaluateFunc(state, time, 0.5*deltaTime, k2);
    k4 = evaluateFunc(state, time, deltaTime, k3);
    
    state.pos = state.pos + (deltaTime)/(6.0) * (k1.vel + 2 * (k2.vel + k3.vel) + k4.vel);
    state.vel = state.vel + (deltaTime)/(6.0) * (k1.accel + 2 * (k2.accel + k3.accel) + k4.accel);
}


/**
 * Explicit Runge-Kutta integration
 * Give dy/dt = f(t, y), and an initial condition y(t0) = y0
 * solutions take the form:
 * y_{n+1} = y_{n} + h * SUM[i=1 -> s](b_{i} * k_{i})
 * k_{i} = f(t_{n} + c_{i} * h, y_{n} + h * SUM[j=1 -> s](a_{ij} * k_{j}))
 * To solve: provide a vector c of length s, a vector b of length s, and an sXs matrix a, in a butcher tableau
 **/
void ExplicitRungeKutta(ParticleState &state, double time, double deltaTime, const ButcherTableau &tableau,
                        DeltaState (*evaluateFunc)(const ParticleState&, double, double, const vector<DeltaState>&,
                                                   const ButcherTableau&, int))
{
    vector<DeltaState> ks(tableau.b.size());
    for (int i = 0; i < ks.size(); i++)
    {
        ks[i] = evaluateFunc(state, time, deltaTime, ks, tableau, i);
    }
    
    double deltaPos = 0., deltaVel = 0.;
    for (int i = 0; i < tableau.b.size(); i++) {
        deltaPos += tableau.b[i] * ks[i].vel;
        deltaVel += tableau.b[i] * ks[i].accel;
    }
    
    state.pos = state.pos + deltaTime * deltaPos;
    state.vel = state.vel + deltaTime * deltaVel;
}
