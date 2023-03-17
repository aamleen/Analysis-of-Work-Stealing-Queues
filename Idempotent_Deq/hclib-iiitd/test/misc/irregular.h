#include "hclib.hpp"
#include <iostream>

namespace hclib {

template<typename T>
void __divide2_1D(int low, int high, int threshold, T && lambda);

template<typename T>
void __divide4_1D(int low, int high, int threshold, T && lambda){
    if(high-low > threshold && high-low>=4) {
	int chunk = (high - low) / 4;
        HCLIB_FINISH {
            async([=]() {
                __divide2_1D(low, low+chunk, threshold, lambda);
	    });
            async([=]() {
                __divide2_1D(low+chunk, low + 2 * chunk, threshold, lambda);
	    });
            async([=]() {
                __divide4_1D(low + 2 * chunk, low + 3 * chunk, threshold, lambda);
	    });
            __divide2_1D(low + 3 * chunk, high, threshold, lambda);
	}
    } else {
        for(int i=low; i<high; i++) {
            lambda(i);
	}
    }
}

template<typename T>
void __divide2_1D(int low, int high, int threshold, T && lambda){
    if(high-low > threshold) {
        HCLIB_FINISH {
            async([=]() {
	        __divide4_1D(low, (low+high)/2, threshold, lambda);		    
	    });
	    __divide2_1D((low+high)/2, high, threshold, lambda);		    
	}
    } else {
        for(int i=low; i<high; i++) {
            lambda(i);
	}
    }
}

template<typename T>
void irregular_recursion1D(loop_domain_t* loop, T && lambda) {
    assert(loop->stride == 1);
    __divide2_1D(loop->low, loop->high, loop->tile, lambda);
}

template<typename T>
void __divide_1D(int low, int high, int threshold, T && lambda){
    if(high-low > threshold) {
        HCLIB_FINISH {
            async([=]() {
	        __divide_1D(low, (low+high)/2, threshold, lambda);		    
	    });
	    __divide_1D((low+high)/2, high, threshold, lambda);		    
	}
    } else {
        for(int i=low; i<high; i++) {
            lambda(i);
	}
    }
}

template<typename T>
void recursion1D(loop_domain_t* loop, T && lambda) {
    assert(loop->stride == 1);
    __divide_1D(loop->low, loop->high, loop->tile, lambda);
}

template<typename T>
void firsttouch_random_initialization(int low, int high, T* array1d, bool decimal) {
    int numWorkers= hclib::num_workers();
    assert(high%numWorkers == 0);
    int batchSize = high / numWorkers;
    int chunk=0;
    HCLIB_FINISH {
    for(int wid=0; wid<numWorkers; wid++) { 
        int start = wid * batchSize;
        int end = start + batchSize;
        hclib::async([=]() {
	    unsigned int seed = wid+1;
            for(int j=start; j<end; j++) {
	        int num = rand_r(&seed);
	        array1d[j] = decimal? T(num/RAND_MAX) : T(num);
            }
        });
    }
    }
}

template<typename T>
T* _numa_malloc(size_t count) {
    return (T*) malloc(sizeof(T) * count);
}
template<typename T>
T** _numa_malloc(size_t row, size_t col) {
    T** var = (T**) malloc(sizeof(T*) * row);
    for(int j=0; j<row; j++) {
        var[j] = (T*) malloc(sizeof(T) * col);
    }
    return var;
}
template<typename T>
void _numa_free(T* mem, size_t row) {
    for(int j=0; j<row; j++) {
        free(mem[j]);
    }
    free(mem);
}
template<typename T>
void _numa_free(T* mem) {
        free(mem);
}
#define numa_malloc _numa_malloc
#define numa_interleave_malloc _numa_malloc
#define numa_free _numa_free
}
