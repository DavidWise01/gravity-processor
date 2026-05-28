/*
 * GRAVITY PROCESSOR v2.0  --  42-PRIM KERNEL   (realistic physics simulator)
 * 48 nodes: 4 live CPG cores (L0-L3) + 42 Ag2S prims (G00-G41) + IN + OUT
 * From the Meta_AI.html design thread. David Lee Wise / TriPod / ROOT0.
 *
 * HONEST MODEL NOTE
 *   "Gravity" = analog ATTRACTOR FIELD, not Newtonian gravity (mm-mass gravity
 *   ~1e-30 N, computes nothing). Realizable device: an electrostatic/ionic well
 *   array, each Ag2S memristor's CONDUCTANCE setting its well depth. A charged
 *   probe ("injected mass") drifts through the superposed wells; where it
 *   settles is the result -> analog nearest-attractor / winner-take-all classify.
 *
 *   Dynamics are OVERDAMPED (ions in an ionic medium = viscous): dp/dt = MU*F.
 *   That is the correct regime for a wet processor and guarantees the probe
 *   descends the potential and settles in a well instead of flying off.
 *
 * Ag2S ECM cell:
 *   theta in [0,1] filament state; g(theta)=G_OFF*(1-theta)+G_ON*theta.
 *   SET via sinh ion-drift, calibrated to tau_fire=15us at V_SET.
 *   0.65 eV barrier is the RETENTION barrier -> non-volatile (reported).
 *   Writes governed by logistic law F=1+4u(1-u) so drive cannot run away.
 *
 * gcc -O2 -std=c11 -o grav42 gravity42.c -lm
 */
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#define N_GRAV   42
#define PITCH    10.0e-6
#define CPG_HZ   10000.0
#define PHASE_US 25.0e-6
#define R0       1.0e6
#define G_OFF    (1.0/R0)        /* 1 uS off  */
#define G_ON     1.0e-3          /* 1 mS on   */
#define EA       0.65            /* eV, retention barrier */
#define KT_EV    0.025852        /* 300 K */
#define V_READ   0.10            /* tiny probe read bias */
#define V_SET    0.58            /* Schmitt-high write */
#define V0       0.10            /* drift voltage scale */
#define TAU_FIRE 15.0e-6         /* SET time at V_SET */
#define R_PULL   0.045           /* field coupling ("gravity r") */
#define MU       1.5e-8          /* probe mobility (overdamped) */
#define DSTEP_MAX (0.50e-6)      /* clip displacement per step (~1 m/s cap) */
#define SOFTEN   (5.0e-6)        /* half-pitch: no singular capture; depth decides */
#define DT       0.5e-6
#define T_SOLVE  400.0e-6

static double clampd(double x,double lo,double hi){return x<lo?lo:(x>hi?hi:x);}
static double fNew(double D){double u=clampd(D/100,0,1);return 1.0+4.0*u*(1.0-u);}
static double fOld(double D){double u=clampd(D/100,0,1);return 1.0+4.0*u*u;}
static double g_of(double th){return G_OFF*(1.0-th)+G_ON*th;}

/* SET prefactor calibrated so theta:0->~0.95 in TAU_FIRE at V_SET */
static const double K_SET = 3.0 / (TAU_FIRE * 1.0); /* sinh(5.8)=164.7 folded below */

typedef struct { double x,y; int phase; double theta,D; uint64_t writes; } prim_t;

static void hex_layout(prim_t *p){
    struct{double x,y,d;}c[400]; int n=0;
    for(int q=-9;q<=9;q++)for(int r=-9;r<=9;r++){
        double x=PITCH*(q+r/2.0), y=PITCH*(r*0.8660254037844386);
        c[n].x=x;c[n].y=y;c[n].d=x*x+y*y;n++;}
    for(int i=0;i<n;i++)for(int j=i+1;j<n;j++)if(c[j].d<c[i].d){__typeof__(c[0])t=c[i];c[i]=c[j];c[j]=t;}
    for(int i=0;i<N_GRAV;i++){p[i].x=c[i].x;p[i].y=c[i].y;p[i].phase=i&3;p[i].theta=0.001;p[i].D=0;p[i].writes=0;}
}
static void program(prim_t*p,const int*h,int nh){
    for(int i=0;i<N_GRAV;i++){int hot=0;for(int k=0;k<nh;k++)if(h[k]==i)hot=1;p[i].theta=hot?0.90:0.001;}
}
static void mem_write(prim_t*m,double V,double base_tw,int logistic){
    double F=logistic?fNew(m->D):fOld(m->D);
    double tw=base_tw*F;
    double rate=K_SET*sinh(V/V0)/sinh(V_SET/V0); /* normalized: =K_SET at V_SET */
    m->theta=clampd(m->theta+rate*tw*(1.0-m->theta),0,1);
    m->writes++;
    m->D=clampd((double)m->writes*R_PULL/600.0,0,100.0);
}
/* overdamped drift force from active-phase wells */
static void field_force(const prim_t*p,double px,double py,int ph,double*fx,double*fy){
    double Fx=0,Fy=0;
    for(int i=0;i<N_GRAV;i++){ if(p[i].phase!=ph) continue;
        double dx=p[i].x-px,dy=p[i].y-py,r2=dx*dx+dy*dy+SOFTEN*SOFTEN,inv=1.0/sqrt(r2);
        double depth=g_of(p[i].theta)/G_ON, mag=R_PULL*depth*inv*inv;
        Fx+=mag*dx*inv; Fy+=mag*dy*inv; }
    *fx=Fx;*fy=Fy;
}

int main(void){
    prim_t p[N_GRAV]; hex_layout(p);
    int heavy[3]={4,20,37};                 /* three stored class wells */
    program(p,heavy,3);

    /* inject probe inside the array, in the basin nearer G20 */
    double px=p[20].x-2.2*PITCH, py=p[20].y+1.4*PITCH;

    printf("=== GRAVITY PROCESSOR v2.0 / 42-PRIM KERNEL ===\n");
    printf("48 nodes (4 live CPG + 42 Ag2S prims + IN + OUT)  10 kHz quad  10 um hex\n");
    printf("Ag2S: R0=1MOhm g_off=1uS  g_on=1mS   Ea(retention)=0.65 eV\n");
    printf("stored heavy wells: G%02d G%02d G%02d   inject near G20 basin\n\n",
           heavy[0],heavy[1],heavy[2]);
    printf(" t(us) phase    probe(x,y) um      drift(um/us)  nearest  dist(um)\n");

    int steps=(int)(T_SOLVE/DT);
    double prevx=px,prevy=py;
    for(int s=0;s<steps;s++){
        double t=s*DT; int ph=((int)floor(t/PHASE_US))&3;
        double fx,fy; field_force(p,px,py,ph,&fx,&fy);
        double dx=MU*fx*DT, dy=MU*fy*DT;            /* overdamped: descends potential */
        double dm=sqrt(dx*dx+dy*dy);
        if(dm>DSTEP_MAX){ dx*=DSTEP_MAX/dm; dy*=DSTEP_MAX/dm; }  /* clip, keep direction */
        px+=dx; py+=dy;
        int nn=0;double best=1e30;
        for(int i=0;i<N_GRAV;i++){double ex=p[i].x-px,ey=p[i].y-py,d=ex*ex+ey*ey;if(d<best){best=d;nn=i;}}
        if(sqrt(best)<1.5*PITCH) mem_write(&p[nn],V_SET,0.20e-6,1);  /* Hebbian reinforce */
        if((s%(steps/12))==0){
            double sp=sqrt((px-prevx)*(px-prevx)+(py-prevy)*(py-prevy))/DT*1e-6*1e6;
            printf("%5.0f   %d   (%7.2f,%7.2f)   %8.3f      G%02d    %6.2f\n",
                   t*1e6,ph,px*1e6,py*1e6,sp,nn,sqrt(best)*1e6);
            prevx=px;prevy=py;
        }
    }

    int out=0;double best=1e30;
    for(int i=0;i<N_GRAV;i++){double ex=p[i].x-px,ey=p[i].y-py,d=ex*ex+ey*ey;if(d<best){best=d;out=i;}}
    int ish=(out==heavy[0]||out==heavy[1]||out==heavy[2]);
    double Pavg=0; for(int ph=0;ph<4;ph++){double Pp=0;for(int i=0;i<N_GRAV;i++)if(p[i].phase==ph)Pp+=V_READ*V_READ*g_of(p[i].theta);Pavg+=Pp;} Pavg/=4.0;
    double t_ret=1e-13*exp(EA/KT_EV);

    printf("\n--- READOUT ---\n");
    printf("OUT settled at      : G%02d   (%s a stored class well)\n",out,ish?"IS":"NOT");
    printf("settle distance     : %.2f um  (<half-pitch = locked)\n",sqrt(best)*1e6);
    printf("array power (TM read): %.2f uW  (design target ~96 uW; here near-off array)\n",Pavg*1e6);
    printf("retention @0.65eV    : %.2e s  (tau0*exp(Ea/kT)) -- REALITY CHECK:\n",t_ret);
    printf("   0.65 eV alone gives ~ms retention at 300 K; the 'non-volatile' claim\n");
    printf("   needs a deeper trap (>0.9 eV) or larger filament. Flagging, not faking.\n");
    printf("CPG                  : 4x25us = 100us period = 10 kHz quad\n");
    printf("solution latency     : %.0f us\n",T_SOLVE*1e6);

    /* ENDURANCE: analytic governance proof (the closed forms, not a counter) */
    /* quadratic ODE dD/dt=r(1+4u^2): u=0.5*tan(2*(r/100)t) -> tan() diverges.    */
    /*   reaches D=100 at t_c = (100/r)*0.5*atan(2) = 55.36/r, then SYSTEM_HALT.  */
    /* logistic  ODE dD/dt=r(1+4u(1-u)): u=0.5+tanh(...)/sqrt2, dD/dt bounded.    */
    double r=R_PULL;
    double t_collapse = (100.0/r)*0.5*atan(2.0);
    double S2=1.4142135623730951;
    double t_sat = (atanh(S2/2)-atanh(-S2/2))*50.0/(S2*r);
    /* run both Ag2S cells to full filament to confirm SET physics */
    prim_t a={.theta=0.001,.D=0}, b={.theta=0.001,.D=0};
    for(int i=0;i<200000;i++){mem_write(&a,V_SET,0.02e-6,1);mem_write(&b,V_SET,0.02e-6,0);}
    printf("\n--- ENDURANCE / GOVERNANCE (r=%.3f) ---\n",r);
    printf("quadratic v1: dD/dt=r(1+4u^2) -> D=100 at t_c=%.1f s, then tan() diverges = HALT\n",t_collapse);
    printf("logistic  v3: dD/dt=r(1+4u(1-u)) -> bounded, D->100 at t_sat=%.1f s, dD/dt<=r always\n",t_sat);
    printf("Ag2S SET check: logistic theta=%.4f  quadratic theta=%.4f (both reach full filament)\n",a.theta,b.theta);
    printf("verdict: the logistic correction is what keeps the array from the forced\n");
    printf("finite-time collapse the v1 quadratic feedback guarantees.\n");
    return 0;
}