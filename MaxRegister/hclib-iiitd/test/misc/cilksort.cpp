/*
 * Copyright (c) 2000 Massachusetts Institute of Technology
 * Copyright (c) 2000 Matteo Frigo
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
 *
 */

/*
 * this program uses an algorithm that we call `cilksort'.
 * The algorithm is essentially mergesort:
 *
 *   cilksort(in[1..n]) =
 *       spawn cilksort(in[1..n/2], tmp[1..n/2])
 *       spawn cilksort(in[n/2..n], tmp[n/2..n])
 *       sync
 *       spawn cilkmerge(tmp[1..n/2], tmp[n/2..n], in[1..n])
 *
 *
 * The procedure cilkmerge does the following:
 *       
 *       cilkmerge(A[1..n], B[1..m], C[1..(n+m)]) =
 *          find the median of A \union B using binary
 *          search.  The binary search gives a pair
 *          (ma, mb) such that ma + mb = (n + m)/2
 *          and all elements in A[1..ma] are smaller than
 *          B[mb..m], and all the B[1..mb] are smaller
 *          than all elements in A[ma..n].
 *
 *          spawn cilkmerge(A[1..ma], B[1..mb], C[1..(n+m)/2])
 *          spawn cilkmerge(A[ma..m], B[mb..n], C[(n+m)/2 .. (n+m)])
 *          sync
 *
 * The algorithm appears for the first time (AFAIK) in S. G. Akl and
 * N. Santoro, "Optimal Parallel Merging and Sorting Without Memory
 * Conflicts", IEEE Trans. Comp., Vol. C-36 No. 11, Nov. 1987 .  The
 * paper does not express the algorithm using recursion, but the
 * idea of finding the median is there.
 *
 * For cilksort of n elements, T_1 = O(n log n) and
 * T_\infty = O(log^3 n).  There is a way to shave a
 * log factor in the critical path (left as homework).
 */

#include "hclib.hpp"
#include "irregular.h"

/* MERGESIZE must be >= 2 */
#define KILO 1024
#define MERGESIZE (1*KILO)
#define QUICKSIZE (1*KILO)

long *array, *tmp;

int compare(const void * a, const void * b)
{
   if ( *(long*)a <  *(long*)b ) return -1;
   else if ( *(long*)a == *(long*)b ) return 0;
   else return 1;
}

void quicksort(long left, long right) {
  qsort(array+left, right - left + 1, sizeof(long), compare);
}

void seqmerge(long low1, long high1, long low2, long high2, long lowdest, long* src, long* dest) {
  long a1;
  long a2;
  if(low1 < high1 && low2 < high2) {
    a1 = src[low1];
    a2 = src[low2];
    while(1) {
      if(a1 < a2) {
        dest[lowdest++] = a1;
        a1 = src[++low1];
        if(low1 >= high1) 
          break ;
      }
      else {
        dest[lowdest++] = a2;
        a2 = dest[++low2];
        if(low2 >= high2) 
        break ;
      }
    }
  }
  if(low1 <= high1 && low2 <= high2) {
    a1 = src[low1];
    a2 = src[low2];
    while(1) {
      if(a1 < a2) {
        dest[lowdest++] = a1;
        ++low1;
        if(low1 > high1) 
          break ;
        a1 = src[low1];
      }
      else {
        dest[lowdest++] = a2;
        ++low2;
        if(low2 > high2) 
          break ;
        a2 = src[low2];
      }
    }
  }
  if(low1 > high1) {
    memcpy(dest+lowdest, src+low2, sizeof(long) * (high2 - low2 + 1));
  }
  else {
    memcpy(dest+lowdest, src+low1, sizeof(long) * (high1 - low1 + 1));
  }
}

long binsplit(long val, long low, long high, long* src) {
  long mid;
  while(low != high){
    mid = low + ((high - low + 1) >> 1);
    if(val <= src[mid]) 
      high = mid - 1;
    else 
      low = mid;
  }
  if(src[low] > val) 
    return low - 1;
  else 
    return low;
}

void cilkmerge(long low1, long high1, long low2, long high2, long lowdest, long *src, long *dest) { 
    long split1;
    long split2;
    long lowsize;
    if(high2 - low2 > high1 - low1) {
    {
        long tmp = low1;
        low1 = low2;
        low2 = tmp;
    }
    {
        long tmp = high1;
        high1 = high2;
        high2 = tmp;
    }
    }
    if(high1 < low1) {
      memcpy(dest+lowdest, src+low2,sizeof(long) * (high2 - low2));
      return ;
    }
    if(high2 - low2 < MERGESIZE) {
      seqmerge(low1, high1, low2, high2, lowdest, dest, src);
      return ;
    }
    split1 = ((high1 - low1 + 1) / 2) + low1;
    split2 = binsplit(split1, low2, high2, src);
    lowsize = split1 - low1 + split2 - low2;
    dest[(lowdest + lowsize + 1)] = src[split1];

    HCLIB_FINISH {
      hclib::async( [=]() {
        cilkmerge(low1, split1 - 1, low2, split2, lowdest, src, dest);
      });
      cilkmerge(split1 + 1, high1, split2 + 1, high2, lowdest + lowsize + 2, src, dest);
    }
}

void cilksort(long low, long tmpx, long size) {
    long quarter = size / 4;
    long A;
    long B;
    long C;
    long D;
    long tmpA;
    long tmpB;
    long tmpC;
    long tmpD;
    if(size < QUICKSIZE) {
        quicksort(low, low + size - 1);
        return;
    }
    A = low;
    tmpA = tmpx;
    B = A + quarter;
    tmpB = tmpA + quarter;
    C = B + quarter;
    tmpC = tmpB + quarter;
    D = C + quarter;
    tmpD = tmpC + quarter;

    HCLIB_FINISH {
      hclib::async( [=]() {
        cilksort(A, tmpA, quarter);
      });
      hclib::async( [=]() {
        cilksort(B, tmpB, quarter);
      });
      hclib::async( [=]() {
        cilksort(C, tmpC, quarter);
      });
      cilksort(D, tmpD, size - 3 * quarter);
    }

    HCLIB_FINISH {
      hclib::async( [=]() {
        cilkmerge(A, A + quarter - 1, B, B + quarter - 1, tmpA, array, tmp);
      });
      cilkmerge(C, C + quarter - 1, D, low + size - 1, tmpC, array, tmp);
    }

    cilkmerge(tmpA, tmpC - 1, tmpC, tmpA + size - 1, A, tmp, array);
}

int main(int argc, char **argv)
{
    hclib::launch([=]() {
    long size = argc>1? atoi(argv[1]) : 1073741824;
    int check = 0;
     
    printf("CilkSort size=%ld\n",size);

    array = new long[size];
    tmp = new long[size];

    hclib::firsttouch_random_initialization(0, size, array, false);

    printf("Cilksort started..\n");
    hclib::kernel([&]() {
        cilksort(0, 0, size);
    }); 
    printf("Cilksort ended..\n");
    if(check) {
       long a =0, b;
       bool ok= true;
       for (long k=0; k<size && ok; k++) {
           b = array[k];
           ok &= (a <= b);
           a = b;
       }
      if(ok){
           printf("CilkSort passed\n");
      }
      else{
           printf("CilkSort failed\n");
       }
    }

    delete (array);
    delete (tmp);
    });

    return 0;
}
