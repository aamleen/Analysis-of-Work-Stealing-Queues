#include "hclib.hpp"
#include <sys/time.h>

/*
 * Multiplication two square matrices in parallel using forasync2D
 */

double **a, **b, **c;

/*
 * Intialize input matrices and the output matrix
 */
void init(int n) {
  a = new double*[n];
  b = new double*[n];
  c = new double*[n];

  /* Demonstration of forasync1D */
  hclib::loop_domain_t loop1 = {0, n, 1, 8};
  hclib::finish([&]() {
    hclib::forasync1D(&loop1, [=](int i) {
      a[i] = new double[n];
      b[i] = new double[n];
      c[i] = new double[n];
    }, FORASYNC_MODE_RECURSIVE);
  });
  
  /* Demonstration of forasync2D */
  hclib::loop_domain_t loop2[2] = {{0, n, 1, 8}, {0, 8, 1, 8}};
  hclib::finish([&]() {
    hclib::forasync2D(loop2, [=](int i, int j) {
      a[i][j] = 1.0;  
      b[i][j] = 1.0;  
      c[i][j] = 0;
    }, FORASYNC_MODE_RECURSIVE);    
  });
}

/*
 * release memory
 */
void freeall(int n) {
  for(int i=0; i<n; i++) {
    delete(a[i]);
    delete(b[i]);
    delete(c[i]);
  }
  delete (a);
  delete (b);
  delete (c);
}

/*
 * Validate the result of matrix multiplication
 */
int verify(int n) {
  for(int i=0; i<n; i++) {
    for(int j=0; j<n; j++) {
      if(c[i][j] != n) {
        printf("result = %.3f\n",c[i][j]);
        return false;
      }
    }
  }
  return true;
}

void multiply(int n) {
  /* Demonstration of forasync2D */
  hclib::loop_domain_t loop[2] = {{0, n, 1, 8}, {0, 8, 1, 8}};
  hclib::finish([&]() {
    hclib::forasync2D(loop, [=](int i, int j) {
      for(int k=0; k<n; k++) {
        c[i][j] += a[i][k] * b[k][j];
      }
    }, FORASYNC_MODE_RECURSIVE);    
  });
}

int main(int argc, char** argv) {
  int n = argc>1 ? atoi(argv[1]) : 1024;
  printf("Size = %d\n",n);
  hclib::launch([&]() {
    /* initialize */
    init(n);
    /* multiply matrices */
    hclib::kernel([&]() {
    multiply(n);
    });
    /* validate result */
    int result = verify(n);
    printf("MatrixMultiplication result = %d\n",result);
    /* release memory */
    freeall(n);
  });
  return 0;
}
