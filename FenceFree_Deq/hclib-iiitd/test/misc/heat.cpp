/*
 * Heat diffusion (Jacobi-type iteration)
 *
 * Usage: see function usage();
 *
 * Volker Strumpen, Boston                                 August 1996
 *
 * Copyright (c) 1996 Massachusetts Institute of Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "hclib.hpp"
#include <math.h>
#include <unistd.h>

/* Define ERROR_SUMMARY if you want to check your numerical results */
//#define ERROR_SUMMARY

#define IRREGULAR 

#define ELEMENT_t double
#define DATA_PRINTF_MODIFIER " %f\n "
int leafmaxcol = 4;

#define f(x,y)     (sin(x)*sin(y))
#define randa(x,t) (0.0)
#define randb(x,t) (exp(-2*(t))*sin(x))
#define randc(y,t) (0.0)
#define randd(y,t) (exp(-2*(t))*sin(y))
#define solu(x,y,t) (exp(-2*(t))*sin(x)*sin(y))

int nx, ny, nt;
ELEMENT_t xu, xo, yu, yo, tu, to;
ELEMENT_t dx, dy, dt;

ELEMENT_t dtdxsq, dtdysq;
ELEMENT_t t;

#ifdef IRREGULAR
int divide4(int lb, int ub, ELEMENT_t *neww, ELEMENT_t *old, int mode, int timestep);
#endif

#define ALLC       0
#define INIT       1
#define COMP       2

/*****************   Initialization of grid partition  NEW VERSION ********************/

void initgrid(ELEMENT_t *old, int lb, int ub) {

  int a, b, llb, lub;

  llb = (lb == 0) ? 1 : lb;
  lub = (ub == nx) ? nx - 1 : ub;

  for (a=llb, b=0; a < lub; a++){		/* boundary nodes */
    old[a * ny + b] = randa(xu + a * dx, 0);
  }

  for (a=llb, b=ny-1; a < lub; a++){
    old[a * ny + b] = randb(xu + a * dx, 0);
  }

  if (lb == 0) {
    for (a=0, b=0; b < ny; b++){
      old[a * ny + b] = randc(yu + b * dy, 0);
    }
  }
  if (ub == nx) {
    for (a=nx-1, b=0; b < ny; b++){
      old[a * ny + b] = randd(yu + b * dy, 0);
    }
  }
  for (a=llb; a < lub; a++) { /* inner nodes */
    for (b=1; b < ny-1; b++) {
      old[a * ny + b] = f(xu + a * dx, yu + b * dy);
    }
  }
}

/***************** Five-Point-Stencil Computation NEW VERSION ********************/

void compstripe(ELEMENT_t *neww, ELEMENT_t *old, int lb, int ub) {
  int a, b, llb, lub;

  llb = (lb == 0) ? 1 : lb;
  lub = (ub == nx) ? nx - 1 : ub;

  for (a=llb; a < lub; a++) {
    for (b=1; b < ny-1; b++) {
        neww[a * ny + b] =   dtdxsq * (old[(a+1) * ny + b] - 2 * old[a * ny +b] + old[(a-1) * ny + b])
        + dtdysq * (old[a * ny + (b+1)] - 2 * old[a * ny + b] + old[a * ny + (b-1)])
        + old[a * ny + b];
    }
  }

  for (a=llb, b=ny-1; a < lub; a++)
    neww[a * ny + b] = randb(xu + a * dx, t);

  for (a=llb, b=0; a < lub; a++)
    neww[a * ny + b] = randa(xu + a * dx, t);

  if (lb == 0) {
    for (a=0, b=0; b < ny; b++)
      neww[a * ny + b] = randc(yu + b * dy, t);
  }
  if (ub == nx) {
    for (a=nx-1, b=0; b < ny; b++)
      neww[a * ny + b] = randd(yu + b * dy, t);
  }
}

/***************** Decomposition of 2D grids in stripes ********************/

int divide2(int lb, int ub, ELEMENT_t *neww,
           ELEMENT_t *old, int mode, int timestep){
  int l = 0, r = 0;

  if (ub - lb > leafmaxcol) {

    HCLIB_FINISH {
        hclib::async([&]() { 
    #ifdef IRREGULAR
	    l = divide4(lb, (ub + lb) / 2, neww, old, mode, timestep); 
    #else
	    l = divide2(lb, (ub + lb) / 2, neww, old, mode, timestep); 
    #endif
	});
        hclib::async([&]() { 
            r = divide2((ub + lb) / 2, ub, neww, old, mode, timestep);
	});
    }

    return(l + r);

  } else {
    switch (mode) {
      case COMP:
        if (timestep % 2) //this sqitches back and forth between the two arrays by exchanging neww and old
          compstripe(neww, old, lb, ub);
        else
          compstripe(old, neww, lb, ub);
        return 1;

      case INIT:
        initgrid(old, lb, ub);
        return 1;
    }

    return 0;
  }
}

#ifdef IRREGULAR
int divide4(int lb, int ub, ELEMENT_t *neww,
           ELEMENT_t *old, int mode, int timestep){
  int c1 = 0, c2 = 0, c3 = 0, c4 = 0;

  if (ub - lb > leafmaxcol) {
    int chunk = (ub - lb) / 4;
    HCLIB_FINISH {
        hclib::async([&]() { 
	    c1 = divide2(lb, lb+chunk, neww, old, mode, timestep);
	});
        hclib::async([&]() { 
	    c2 = divide2(lb+chunk, lb + 2 * chunk, neww, old, mode, timestep);
	});
        hclib::async([&]() { 
	    c3 = divide4(lb + 2 * chunk, lb + 3 * chunk, neww, old, mode, timestep);
	});
        hclib::async([&]() { 
	    c4 = divide2(lb + 3 * chunk, ub, neww, old, mode, timestep);
	});
    }

    return(c1 + c2 + c3 + c4);

  } else {
    switch (mode) {
      case COMP:
        if (timestep % 2) //this sqitches back and forth between the two arrays by exchanging neww and old
          compstripe(neww, old, lb, ub);
        else
          compstripe(old, neww, lb, ub);
        return 1;

      case INIT:
        initgrid(old, lb, ub);
        return 1;
    }

    return 0;
  }
}
#endif

int heat(ELEMENT_t *old_v, ELEMENT_t *new_v, int run) {
  int  c;

#ifdef ERROR_SUMMARY
  ELEMENT_t tmp, *mat;
  ELEMENT_t mae = 0.0;
  ELEMENT_t mre = 0.0;
  ELEMENT_t me = 0.0;
  int a, b;
#endif

  /* Jacobi Iteration (divide x-dimension of 2D grid into stripes) */
  /* Timing. "Start" timers */
  divide2(0, nx, new_v, old_v, INIT, 0);
  printf("Heat started...\n");
  hclib::kernel([&]() {
      for (c = 1; c <= nt; c++) {
          t = tu + c * dt;
          divide2(0, nx, new_v, old_v, COMP, c);
      }
  });
  printf("Heat ended...\n");	

#ifdef ERROR_SUMMARY
  /* Error summary computation: Not parallelized! */
  mat = (c % 2) ? old_v : new_v;
  printf("\n Error summary of last time frame comparing with exact solution:");
  for (a=0; a<nx; a++)
    for (b=0; b<ny; b++) {
      tmp = fabs(mat[a * nx + b] - solu(xu + a * dx, yu + b * dy, to));
      //printf("error: %10e\n", tmp);
      if (tmp > mae)
        mae = tmp;
    }

  printf("\n   Local maximal absolute error  %10e ", mae);

  for (a=0; a<nx; a++)
    for (b=0; b<ny; b++) {
      tmp = fabs(mat[a * nx + b] - solu(xu + a * dx, yu + b * dy, to));
      if (mat[a * nx + b] != 0.0)
        tmp = tmp / mat[a * nx + b];
      if (tmp > mre)
        mre = tmp;
    }

  printf("\n   Local maximal relative error  %10e %s ", mre * 100, "%");

  me = 0.0;
  for (a=0; a<nx; a++)
    for (b=0; b<ny; b++) {
      me += fabs(mat[a * nx + b] - solu(xu + a * dx, yu + b * dy, to));
    }

  me = me / (nx * ny);
  printf("\n   Global Mean absolute error    %10e\n\n", me);
#endif
  return 0;
}

int main(int argc, char *argv[]) {

  hclib::launch([&]() {
  
  int ret, benchmark, help;

  nx = 512;
  ny = 512;
  nt = 100;
  xu = 0.0;
  xo = 1.570796326794896558;
  yu = 0.0;
  yo = 1.570796326794896558;
  tu = 0.0;
  to = 0.0000001;

  // use the math related function before parallel region;
  // there is some benigh race in initalization code for the math functions.
  fprintf(stderr, "Testing exp: %f\n", randb(nx, nt));

  benchmark = argc>1?atoi(argv[1]) : 3;

  if (benchmark) {
    switch (benchmark) {
      case 1:      /* short benchmark options -- a little work*/
        nx = 512;
        ny = 512;
        nt = 1;
        xu = 0.0;
        xo = 1.570796326794896558;
        yu = 0.0;
        yo = 1.570796326794896558;
        tu = 0.0;
        to = 0.0000001;
        break;
      case 2:      /* standard benchmark options*/
        nx = 4096;
        ny = 512;
        nt = 40;
        xu = 0.0;
        xo = 1.570796326794896558;
        yu = 0.0;
        yo = 1.570796326794896558;
        tu = 0.0;
        to = 0.0000001;
        break;
      case 3:      /* long benchmark options -- a lot of work*/
        nx = 4096;
        ny = 1024;
        nt = 100;
        xu = 0.0;
        xo = 1.570796326794896558;
        yu = 0.0;
        yo = 1.570796326794896558;
        tu = 0.0;
        to = 0.0000001;
        break;
      case 4:      /* long benchmark options -- a lot of work*/
        nx = 8192;
        ny = 8192;
        nt = 50;
        xu = 0.0;
        xo = 1.570796326794896558;
        yu = 0.0;
        yo = 1.570796326794896558;
        tu = 0.0;
        to = 0.0000001;
        break;
      case 5:      /* long benchmark options -- a lot of work*/
        nx = 16384;
        ny = 16384;
        nt = 50;
        xu = 0.0;
        xo = 1.570796326794896558;
        yu = 0.0;
        yo = 1.570796326794896558;
        tu = 0.0;
        to = 0.0000001;
        break;
      case 6:      /* long benchmark options -- a lot of work*/
        nx = 32768;
        ny = 32768;
        nt = 200;
        xu = 0.0;
        xo = 1.570796326794896558;
        yu = 0.0;
        yo = 1.570796326794896558;
        tu = 0.0;
        to = 0.0000001;
        break;
    }
  }
  printf("Input(case %d): nx=%d, ny=%d, nt=%d, leafmaxcol=%d\n",benchmark,nx,ny,nt,leafmaxcol);

  dx = (xo - xu) / (nx - 1);
  dy = (yo - yu) / (ny - 1);
  dt = (to - tu) / nt;	/* nt effective time steps! */

  dtdxsq = dt / (dx * dx);
  dtdysq = dt / (dy * dy);

   /* Memory Allocation */

  ELEMENT_t *old_v, *new_v;
  old_v = (double*) malloc(sizeof(double) * nx * ny);
  new_v = (double*) malloc(sizeof(double) * nx * ny);

  ret = heat(old_v, new_v, 0);

  printf("\nHeat");
  printf("\n dx = ");printf(DATA_PRINTF_MODIFIER, dx);
  printf(" dy = ");printf(DATA_PRINTF_MODIFIER, dy);
  printf(" dt = ");printf(DATA_PRINTF_MODIFIER, dt);
  printf(" Stability Value for explicit method must be > 0:"); 
  printf(DATA_PRINTF_MODIFIER,(0.5 - (dt / (dx * dx) + dt / (dy * dy))));
  printf("Options: granularity = %d\n", leafmaxcol);
  printf("         nx          = %d\n", nx);
  printf("         ny          = %d\n", ny);
  printf("         nt          = %d\n", nt);
  
  free(new_v);
  free(old_v);

  });
  return 0;
}
