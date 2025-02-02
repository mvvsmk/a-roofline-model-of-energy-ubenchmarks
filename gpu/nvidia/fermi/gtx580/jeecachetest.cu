/*
MIT License
Copyright 2020 Jee W. Choi, Marat Dukhan, and Xing Liu
Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to use, 
copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the 
Software, and to permit persons to whom the Software is furnished to do so, subject 
to the following conditions:
The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* Libraries */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#include <cutil_inline.h>

#include "cache_kernel.h"

/* ======================================================================== */
/* GPU kernel error checking function */
void gpu_check_error__srcpos (FILE* fp, const char* filename, size_t line)
{
  cudaError_t C_E = cudaGetLastError ();
  if (C_E) {
    fprintf (fp, "*** [%s:%lu] CUDA ERROR %d: %s ***\n", filename, line, C_E, 
						 cudaGetErrorString (C_E));
    fflush (fp);
    exit (-1); /* abort program */
  }
}
/* ======================================================================== */


/* ======================================================================== */
/* Read program inputs */
void readConfig(int* wordsPerThread, int* nThreads, int* bSize, char** argv)
{
	*wordsPerThread = atoi (argv[1]);
	*nThreads = atoi (argv[2]);
	*bSize = atoi (argv[3]);

}
/* ======================================================================== */


/* ======================================================================== */
/* Validate the results to make sure the kernel ran correctly */
int validateResults(int nThreads, int bSize, int wordsPerThread, int* out, int* in)
{
	int i, j;
	int tmp, cnt;
	int* test;
	
	test = (int*) malloc (nThreads * sizeof (int));
	for(i = 0; i < nThreads; i++) {
		test[i] = 0;
	}

	for(i = 0; i < nThreads; i++) {
		tmp = in[i % bSize];
		for(j = 0; j < wordsPerThread; j++) {	
			tmp = in[tmp];
		}
		test[i] = tmp;
	}

	cnt = 0;
	for(i = 0; i < nThreads; i++) {
		if(test[i] != out[i]) cnt++;
	}
	
	return cnt;
}
/* ======================================================================== */


/* ======================================================================== */
/* Kernel used for clearing the cache before the main test */
__global__ void clear_cache (int *in)
{
	int tid = threadIdx.x + blockIdx.x * blockDim.x;
	in[tid] = in[tid] + 1;
}
/* ======================================================================== */


/* ======================================================================== */
/* Run the main test */
float runTest (int* h_out, int* d_out, int* d_pchase, int nThreads, int bSize, 
							 int wordsPerThread)
{
	/* CUDA timers */
  cudaEvent_t start, stop;
	float total_time_taken;

	int num_blocks = (nThreads + bSize - 1) / bSize;

  dim3 grid (num_blocks);
  dim3 threads (bSize);

	fprintf (stderr, "number of iterations is %d\n", NUM_ITER);


  /* Start timer */
	cutilSafeCall (cudaEventCreate (&start));
	cutilSafeCall (cudaEventCreate (&stop));
	cutilSafeCall (cudaEventRecord (start, 0));

	for(int iter = 0; iter < NUM_ITER; iter++) {
		switch (wordsPerThread) {
			case 1:
				cache_kernel_1 <<<grid, threads>>> (nThreads, d_out, d_pchase);	
				break;
			case 2:
				cache_kernel_2 <<<grid, threads>>> (nThreads, d_out, d_pchase);	
				break;
			case 4:
				cache_kernel_4 <<<grid, threads>>> (nThreads, d_out, d_pchase);	
				break;
			case 8:
				cache_kernel_8 <<<grid, threads>>> (nThreads, d_out, d_pchase);	
				break;
			case 16:
				cache_kernel_16 <<<grid, threads>>> (nThreads, d_out, d_pchase);	
				break;
			case 32:
				cache_kernel_32 <<<grid, threads>>> (nThreads, d_out, d_pchase);	
				break;
			case 64:
				cache_kernel_64 <<<grid, threads>>> (nThreads, d_out, d_pchase);	
				break;
			case 128:
				cache_kernel_128 <<<grid, threads>>> (nThreads, d_out, d_pchase);	
				break;
			case 256:
				cache_kernel_256 <<<grid, threads>>> (nThreads, d_out, d_pchase);	
				break;
			case 512:
				cache_kernel_512 <<<grid, threads>>> (nThreads, d_out, d_pchase);	
				break;
			case 1024:
				cache_kernel_1024 <<<grid, threads>>> (nThreads, d_out, d_pchase);	
				break;
			case 2048:
				cache_kernel_2048 <<<grid, threads>>> (nThreads, d_out, d_pchase);	
				break;
			case 4096:
				cache_kernel_4096<<<grid, threads>>> (nThreads, d_out, d_pchase);	
				break;
			default:
				fprintf(stderr, "Invalid wordsPerThread: %d\n", wordsPerThread);
				total_time_taken = -1.0f;
				break;
		}
	}

  /* End timer */
  cutilSafeCall (cudaEventRecord (stop, 0));
  cutilSafeCall (cudaThreadSynchronize ());
  cutilSafeCall (cudaEventElapsedTime (&total_time_taken, start, stop));

	/* Check for kernel errors */
  gpu_check_error__srcpos (stderr, __FILE__, __LINE__);

  /* Copy results back */
  cutilSafeCall (cudaMemcpy (h_out, d_out, nThreads * sizeof (int),
                             cudaMemcpyDeviceToHost));

	return total_time_taken; 
}
/* ======================================================================== */

int main(int argc, char** argv)
{
	int i;
	long double total_dram;

	/* Timer */
	float total_time_taken;

	/* Execution parameters */
	int wordsPerThread;
	int nThreads;
	int bSize;

	/* Memory data structures */
  int* d_out;
  int* h_out;
	int* h_pchase;
	int* d_pchase;

  if(argc < 4) {
    fprintf(stderr, "usage: %s <word/thread> <# threads> <threads/block>\n",
						argv[0]);
    exit (0);
  } else {
		readConfig (&wordsPerThread, &nThreads, &bSize, argv);
	}


	/* Find the best GPU in the system */
  cudaSetDevice(cutGetMaxGflopsDeviceId ());


	/* This is done to clear the cache */
	{
		int cache_clear = 4 * 1024 * 1024; // 4 MB
		int* h_temp = (int*) malloc (cache_clear);
		for(int i = 0; i < cache_clear / sizeof (int); i++) h_temp[i] = 1;
		int* d_temp;
		cutilSafeCall (cudaMalloc ((void**) &d_temp, cache_clear));
		cutilSafeCall (cudaMemcpy (d_temp, h_temp, cache_clear,
															 cudaMemcpyHostToDevice));

		int nt = cache_clear / sizeof (int);
		int nb = nt / 512;
		clear_cache <<<nb, 512>>> (d_temp);
  	cutilSafeCall (cudaThreadSynchronize ());

		free (h_temp);
		cutilSafeCall (cudaFree (d_temp));
	}


	/* Allocate host memory */
	h_out = (int*) malloc (nThreads * sizeof (int));
	h_pchase = (int*) malloc (bSize * sizeof (int));
	cutilSafeCall (cudaMalloc ((void**) &d_out, nThreads * sizeof (int)));
	cutilSafeCall (cudaMalloc ((void**) &d_pchase, bSize * sizeof (int)));

	/* Initialize memory */
	for(i = 0; i < bSize - 1; i++) {
		h_pchase[i] = i;
	}
	h_pchase[bSize - 1] = bSize - 1;
	cutilSafeCall (cudaMemcpy (d_pchase, h_pchase, bSize * sizeof (int), 
														 cudaMemcpyHostToDevice));

	/* Run test */
	total_time_taken = runTest (h_out, d_out, d_pchase, nThreads, bSize, 
															wordsPerThread);

	/* Validate results */
	fprintf(stderr, "Results validated: %d errors\n", validateResults (nThreads, 
																															bSize, 
																															wordsPerThread, 
																															h_out, h_pchase));

	/* Compute performance metrics */
	total_time_taken = total_time_taken / NUM_ITER;
	total_dram = ((wordsPerThread + 1) * (1.0 * nThreads/1e9)) * sizeof (int);
  fprintf (stderr, "Time taken to load %Lg GBs: %f (ms)\n", total_dram, 
					 total_time_taken);
	fprintf (stderr, "Effective bandwidth: %Lg (GB/s)\n", 
					 (total_dram/total_time_taken * 1e3));

	/* Free memory */
	free (h_out);
	cutilSafeCall (cudaFree (d_out));
	free (h_pchase);
	cutilSafeCall (cudaFree (d_pchase));

	return 0;
}
