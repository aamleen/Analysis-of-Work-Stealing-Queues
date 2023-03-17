// srad.cpp : Defines the entry point for the console application.
//

#include "irregular.h"
#include <math.h>
#include <unistd.h>

#define	ITERATION
#define ELEMENT_t float

void random_matrix(ELEMENT_t *I, int rows, int cols);

void usage(int argc, char **argv)
{
    fprintf(stderr, "Usage: %s <rows> <cols> <y1> <y2> <x1> <x2> <lamda> <no. of iter>\n", argv[0]);
    fprintf(stderr, "\t<rows>   - number of rows\n");
    fprintf(stderr, "\t<cols>    - number of cols\n");
    fprintf(stderr, "\t<y1> 	 - y1 value of the speckle\n");
    fprintf(stderr, "\t<y2>      - y2 value of the speckle\n");
    fprintf(stderr, "\t<x1>       - x1 value of the speckle\n");
    fprintf(stderr, "\t<x2>       - x2 value of the speckle\n");
    fprintf(stderr, "\t<lamda>   - lambda (0,1)\n");
    fprintf(stderr, "\t<no. of iter>   - number of iterations\n");

    exit(1);
}

int main(int argc, char* argv[])
{
	hclib::launch([&]() {	
	    int rows, cols, size_I, size_R, niter = 10, iter;
	    ELEMENT_t *I, *J, q0sqr, sum, sum2, tmp, meanROI,varROI ;
	    ELEMENT_t *dN,*dS,*dW,*dE;
	    int r1, r2, c1, c2;
	    ELEMENT_t *c, D;
	    ELEMENT_t lambda;
	    int i, j;

	    // 16384 16384 0 127 0 127 0.5 1
	    rows = argc>1?atoi(argv[1]) : 16384; //number of rows in the domain
	    cols = argc>2?atoi(argv[2]) : 16384; //number of cols in the domain
	    if ((rows%16!=0) || (cols%16!=0)){
	    fprintf(stderr, "rows and cols must be multiples of 16\n");
	    exit(1);
	    }
	    r1   = 0; //y1 position of the speckle
	    r2   = 127; //y2 position of the speckle
	    c1   = 0; //x1 position of the speckle
	    c2   = 127; //x2 position of the speckle
	    lambda = 0.5; //Lambda value
	    niter = 10; //number of iterations
	    //usage(argc, argv);

	    size_I = cols * rows;
	    size_R = (r2-r1+1)*(c2-c1+1);   

	    I = hclib::numa_malloc<ELEMENT_t>(size_I);
	    J = hclib::numa_malloc<ELEMENT_t>( size_I );
	    c  = hclib::numa_malloc<ELEMENT_t>( size_I) ;
	    dN = hclib::numa_malloc<ELEMENT_t>(size_I) ;
	    dS = hclib::numa_malloc<ELEMENT_t>(size_I) ;
	    dW = hclib::numa_malloc<ELEMENT_t>(size_I) ;
	    dE = hclib::numa_malloc<ELEMENT_t>(size_I) ;    

	    int iN[rows], iS[rows], jW[cols], jE[cols];

	    for (int i=0; i< rows; i++) {
		iN[i] = i-1;
		iS[i] = i+1;
	    }    
	    for (int j=0; j< cols; j++) {
		jW[j] = j-1;
		jE[j] = j+1;
	    }
	    iN[0]    = 0;
	    iS[rows-1] = rows-1;
	    jW[0]    = 0;
	    jE[cols-1] = cols-1;

	    printf("Randomizing the input matrix\n");

	    random_matrix(I, rows, cols);

	    for (int k = 0;  k < size_I; k++ ) {
		J[k] = (ELEMENT_t)exp(I[k]) ;
	    }

	    printf("Start the SRAD main loop\n");

	    hclib::kernel([&]() {
#ifdef ITERATION
		for (iter=0; iter< niter; iter++) {
#endif        
		    sum=0; sum2=0;     
		    for (i=r1; i<=r2; i++) {
		    	for (j=c1; j<=c2; j++) {
		    		tmp   = J[i * cols + j];
		    		sum  += tmp ;
		    		sum2 += tmp*tmp;
		    	}
		    }
		    meanROI = sum / size_R;
		    varROI  = (sum2 / size_R) - meanROI*meanROI;
		    q0sqr   = varROI / (meanROI*meanROI);
		    hclib::loop_domain_t loop = {0, rows, 1, 2};
		    auto start = [=](int i) { return i*cols;};
		    auto end = [=](int i) { return i*cols-1;};
		    hclib::irregular_recursion1D(&loop, [&] (int i) {
			    int k;
			    ELEMENT_t Jc, G2, L, num, den, qsqr;
			    const int iN_i = iN[i];
			    const int iS_i = iS[i];
			    for (int j = 0; j < cols; j++) {
				    k = i * cols + j;
				    Jc = J[k];
				    // directional derivates
				    dN[k] = J[iN_i * cols + j] - Jc;
				    dS[k] = J[iS_i * cols + j] - Jc;
				    dW[k] = J[i * cols + jW[j]] - Jc;
				    dE[k] = J[i * cols + jE[j]] - Jc;

				    G2 = (dN[k]*dN[k] + dS[k]*dS[k] 
					    + dW[k]*dW[k] + dE[k]*dE[k]) / (Jc*Jc);

				    L = (dN[k] + dS[k] + dW[k] + dE[k]) / Jc;

				    num  = (0.5*G2) - ((1.0/16.0)*(L*L)) ;
				    den  = 1 + (.25*L);
				    qsqr = num/(den*den);

				    // diffusion coefficent (equ 33)
				    den = (qsqr-q0sqr) / (q0sqr * (1+q0sqr)) ;
				    c[k] = 1.0 / (1.0+den) ;

				    // saturate diffusion coefficent
				    if (c[k] < 0) {c[k] = 0;}
				    else if (c[k] > 1) {c[k] = 1;}

			   }
		    });

		    hclib::irregular_recursion1D(&loop, [&] (int i) {
			    int k;
			    ELEMENT_t cN,cS,cW,cE,D;
			    const int iS_i = iS[i];
			    for (int j = 0; j < cols; j++) {
				    // current index
				    k = i * cols + j;
				    // diffusion coefficent
				    cN = c[k];
				    cS = c[iS_i * cols + j];
				    cW = c[k];
				    cE = c[i * cols + jE[j]];
				    // divergence (equ 58)
				    D = cN * dN[k] + cS * dS[k] + cW * dW[k] + cE * dE[k];
				    // image update (equ 61)
				    J[k] = J[k] + 0.25*lambda*D;
			    }
		    });
#ifdef ITERATION
		 }
#endif
	    });
	    printf("Computation Done\n");

	    hclib::numa_free(J);
	    hclib::numa_free(c) ;
	    hclib::numa_free(dN) ;
	    hclib::numa_free(dS) ;
	    hclib::numa_free(dW) ;
	    hclib::numa_free(dE) ;    
	    hclib::numa_free(I);
    });
    return 0;
}

void random_matrix(ELEMENT_t *I, int rows, int cols){
    hclib::firsttouch_random_initialization(0, rows*cols, I, true);
}

