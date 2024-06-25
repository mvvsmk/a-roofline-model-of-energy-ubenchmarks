/*
MIT License
Copyright 2020 Jee W. Choi, Marat Dukhan, and Xing Liu
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions: The above copyright notice and this
permission notice shall be included in all copies or substantial portions of the
Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <malloc.h>
#include <omp.h>
#include <papi.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ======================================================== */
/* Timer */
namespace cpu {

inline static uint64_t get_ticks_acquire() {
  uint32_t low, high;
  __asm__ __volatile__("xor %%eax, %%eax;"
                       "cpuid;"
                       "rdtsc;"
                       : "=a"(low), "=d"(high)
                       :
                       : "%rbx", "%rcx");
  return (uint64_t(high) << 32) | uint64_t(low);
}

inline static uint64_t get_ticks_release() {
  uint32_t low, high;
  __asm__ __volatile__("rdtscp;" : "=a"(low), "=d"(high) : : "%rcx");
  return (uint64_t(high) << 32) | uint64_t(low);
}

} // namespace cpu
/* ======================================================== */

long long get_energy() {
  FILE *fp;
  char path[1035];
  long long value;

  /* Open the command for reading. */
  fp =
      popen("echo nilesh | sudo -S rdmsr -u 1553 | xargs -0 -I{} echo {}", "r");
  if (fp == NULL) {
    fprintf(stderr, "Failed to run command\n");
    exit(1);
  }

  /* Read the output a line at a time - it should be just one line. */
  if (fgets(path, sizeof(path) - 1, fp) != NULL) {
    fprintf(stderr, "Output: %s", path);
    value = atoll(path); // Convert the output to an integer
  }

  /* Close the pipe and print the value. */
  pclose(fp);
  fprintf(stderr, "Value: %lld\n", value);

  return value;
}

/* Single and double precision sum squared functions */
extern "C" void sumsq(const double *data, size_t length);
extern "C" void sumsqf(const float *data, size_t length);

int main(int argc, char **argv) {

  /* Number of elements in the array */
  const size_t array_length = 1024 * 1024 * 300;

  /* Create and initialize arrays */
  double *data0 = (double *)memalign(64, array_length * sizeof(double));
  double *data1 = (double *)memalign(64, array_length * sizeof(double));
  double *data2 = (double *)memalign(64, array_length * sizeof(double));
  double *data3 = (double *)memalign(64, array_length * sizeof(double));
  double *data4 = (double *)memalign(64, array_length * sizeof(double));
  double *data5 = (double *)memalign(64, array_length * sizeof(double));
  memset(data0, 0, array_length * sizeof(double));
  memset(data1, 0, array_length * sizeof(double));
  memset(data2, 0, array_length * sizeof(double));
  memset(data3, 0, array_length * sizeof(double));
  memset(data4, 0, array_length * sizeof(double));
  memset(data5, 0, array_length * sizeof(double));

  /*oi counter*/
  long long count0, count1, count2, count3, count4, count5;

  /* Timers */
  double execTime0, execTime1, execTime2, execTime3, execTime4, execTime5;

  /* Since the Intel(R) Xeon(R) CPU E5-1650 v4 @ 3.60GHz has 6 cores, we use
     OpenMP to run computation on all cores. If the target processor has more
     cores, increase the number of data structures, timers, OpenMP threads, etc.
     accordingly.
   */
  long long energy_before = get_energy();
#pragma omp parallel num_threads(6)
  {
#pragma omp sections
    {
/* OpenMP thread for first core */
#pragma omp section
      {
        fprintf(stderr, "Starting first computation thread...\n");
#if (TYPE)
        /* For double	precision */
        {
          int retval;
          retval = PAPI_library_init(PAPI_VER_CURRENT);
          if (retval != PAPI_VER_CURRENT) {
            fprintf(stderr, "Error initializing PAPI! %s \n",
                    PAPI_strerror(retval));
          }
          int eventset = PAPI_NULL;
          // papi creating event set
          retval = PAPI_create_eventset(&eventset);
          if (retval != PAPI_OK) {
            fprintf(stderr, "Error creating eventset! %s\n",
                    PAPI_strerror(retval));
          }

          // papi adding event set
          retval = PAPI_add_named_event(eventset,
                                        "perf::PERF_COUNT_HW_CACHE_LL:MISS");
          if (retval != PAPI_OK) {
            fprintf(stderr,
                    "Error adding perf::PERF_COUNT_HW_CACHE_LL:MISS: %s\n",
                    PAPI_strerror(retval));
          }

          // starting count
          long long count;
          PAPI_reset(eventset);
          retval = PAPI_start(eventset);
          if (retval != PAPI_OK) {
            fprintf(stderr, "Error starting CUDA: %s\n", PAPI_strerror(retval));
          }

          /* start measure */
          fprintf(stderr, "reached benchmark\n");
          fprintf(stderr, "Double precision...\n");
          const uint64_t start0 = cpu::get_ticks_acquire();
          sumsq(data0, array_length);
          const uint64_t end0 = cpu::get_ticks_release();
          fprintf(stderr, "after benchmark\n");
          /* end measure */

          // ending count
          retval = PAPI_stop(eventset, &count);
          if (retval != PAPI_OK) {
            fprintf(stderr, "Error stopping:  %s\n", PAPI_strerror(retval));
          } else {
            fprintf(stderr, "Measured %lld hw cache misses (thread 1)\n",
                    count);
          }
          count0 = count;

          execTime0 = double(end0 - start0) / 1.7e+9;
        }
#else
        /* For single precision */
        {
          const uint64_t start0 = cpu::get_ticks_acquire();
          sumsqf((const float *)data0, array_length * 2);
          const uint64_t end0 = cpu::get_ticks_release();

          execTime0 = double(end0 - start0) / 1.7e+9;
        }
#endif
      }
/* OpenMP thread for second core */
#pragma omp section
      {
        fprintf(stderr, "Starting second computation thread...\n");
#if (TYPE)
        /* For double	precision */
        {
          int retval;
          retval = PAPI_library_init(PAPI_VER_CURRENT);
          if (retval != PAPI_VER_CURRENT) {
            fprintf(stderr, "Error initializing PAPI! %s \n",
                    PAPI_strerror(retval));
          }
          int eventset = PAPI_NULL;
          // papi creating event set
          retval = PAPI_create_eventset(&eventset);
          if (retval != PAPI_OK) {
            fprintf(stderr, "Error creating eventset! %s\n",
                    PAPI_strerror(retval));
          }

          // papi adding event set
          retval = PAPI_add_named_event(eventset,
                                        "perf::PERF_COUNT_HW_CACHE_LL:MISS");
          if (retval != PAPI_OK) {
            fprintf(stderr,
                    "Error adding perf::PERF_COUNT_HW_CACHE_LL:MISS : %s\n",
                    PAPI_strerror(retval));
          }

          // starting count
          long long count;
          PAPI_reset(eventset);
          retval = PAPI_start(eventset);
          if (retval != PAPI_OK) {
            fprintf(stderr, "Error starting CUDA: %s\n", PAPI_strerror(retval));
          }

          /*start measure */
          fprintf(stderr, "reached benchmark\n");
          const uint64_t start1 = cpu::get_ticks_acquire();
          sumsq(data1, array_length);
          const uint64_t end1 = cpu::get_ticks_release();
          fprintf(stderr, "after benchmark\n");
          /* end measure */

          // ending count
          retval = PAPI_stop(eventset, &count);
          if (retval != PAPI_OK) {
            fprintf(stderr, "Error stopping:  %s\n", PAPI_strerror(retval));
          } else {
            fprintf(stderr, "Measured %lld hw cache misses (thread 2)\n",
                    count);
          }
          count1 = count;

          execTime1 = double(end1 - start1) / 1.7e+9;
        }
#else
        /* For single precision */
        {
          const uint64_t start1 = cpu::get_ticks_acquire();
          sumsqf((const float *)data1, array_length * 2);
          const uint64_t end1 = cpu::get_ticks_release();

          execTime1 = double(end1 - start1) / 1.7e+9;
        }
#endif

        // sleep (2);
      }
/* OpenMP thread for third core */
#pragma omp section
      {
        fprintf(stderr, "Starting third computation thread...\n");
#if (TYPE)
        /* For double	precision */
        {
          // int retval;
          // retval = PAPI_library_init(PAPI_VER_CURRENT);
          // if (retval != PAPI_VER_CURRENT) {
          //   fprintf(stderr, "Error initializing PAPI! %s \n",
          //           PAPI_strerror(retval));
          // }
          // int eventset = PAPI_NULL;
          // // papi creating event set
          // retval = PAPI_create_eventset(&eventset);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error creating eventset! %s\n",
          //           PAPI_strerror(retval));
          // }

          // // papi adding event set
          // retval = PAPI_add_named_event(eventset,
          //                               "perf::PERF_COUNT_HW_CACHE_LL:MISS");
          // if (retval != PAPI_OK) {
          //   fprintf(stderr,
          //           "Error adding perf::PERF_COUNT_HW_CACHE_LL:MISS : %s\n",
          //           PAPI_strerror(retval));
          // }

          // // starting count
          // long long count;
          // PAPI_reset(eventset);
          // retval = PAPI_start(eventset);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error starting CUDA: %s\n",
          //   PAPI_strerror(retval));
          // }

          /*start measure */
          fprintf(stderr, "reached benchmark\n");
          const uint64_t start1 = cpu::get_ticks_acquire();
          sumsq(data2, array_length);
          const uint64_t end1 = cpu::get_ticks_release();
          fprintf(stderr, "after benchmark\n");
          // /* end measure */

          // ending count
          // retval = PAPI_stop(eventset, &count);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error stopping:  %s\n", PAPI_strerror(retval));
          // } else {
          //   printf("Measured %lld hw cache misses(thread3)\n", count);
          // }
          // count2 = count;

          execTime2 = double(end1 - start1) / 1.7e+9;
        }
#else
        /* For single precision */
        {
          const uint64_t start1 = cpu::get_ticks_acquire();
          sumsqf((const float *)data2, array_length * 2);
          const uint64_t end1 = cpu::get_ticks_release();

          execTime2 = double(end1 - start1) / 1.7e+9;
        }
#endif

        // sleep (4);
      }
/* OpenMP thread for fourth core */
#pragma omp section
      {
        fprintf(stderr, "Starting fourth computation thread...\n");
#if (TYPE)
        /* For double	precision */
        {
          // int retval;
          // retval = PAPI_library_init(PAPI_VER_CURRENT);
          // if (retval != PAPI_VER_CURRENT) {
          //   fprintf(stderr, "Error initializing PAPI! %s \n",
          //           PAPI_strerror(retval));
          // }
          // int eventset = PAPI_NULL;
          // // papi creating event set
          // retval = PAPI_create_eventset(&eventset);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error creating eventset! %s\n",
          //           PAPI_strerror(retval));
          // }

          // // papi adding event set
          // retval = PAPI_add_named_event(eventset,
          //                               "perf::PERF_COUNT_HW_CACHE_LL:MISS");
          // if (retval != PAPI_OK) {
          //   fprintf(stderr,
          //           "Error adding perf::PERF_COUNT_HW_CACHE_LL:MISS: %s\n",
          //           PAPI_strerror(retval));
          // }

          // // starting count
          // long long count;
          // PAPI_reset(eventset);
          // retval = PAPI_start(eventset);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error starting CUDA: %s\n",
          //   PAPI_strerror(retval));
          // }

          /* start measure */
          fprintf(stderr, "reached benchmark\n");
          const uint64_t start1 = cpu::get_ticks_acquire();
          sumsq(data3, array_length);
          const uint64_t end1 = cpu::get_ticks_release();
          fprintf(stderr, "after benchmark\n");
          /* end measure */

          // // ending count
          // retval = PAPI_stop(eventset, &count);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error stopping:  %s\n", PAPI_strerror(retval));
          // } else {
          //   printf("Measured %lld hw cache misses (thread 4)\n", count);
          // }
          // count3 = count;

          execTime3 = double(end1 - start1) / 1.7e+9;
        }
#else
        /* For single precision */
        {
          const uint64_t start1 = cpu::get_ticks_acquire();
          sumsqf((const float *)data3, array_length * 2);
          const uint64_t end1 = cpu::get_ticks_release();

          execTime3 = double(end1 - start1) / 1.7e+9;
        }
#endif

        // sleep (6);
      }
/* OpenMP thread for fifth core */
#pragma omp section
      {
        fprintf(stderr, "Starting fifth computation thread...\n");
#if (TYPE)
        /* For double	precision */
        {
          // int retval;
          // retval = PAPI_library_init(PAPI_VER_CURRENT);
          // if (retval != PAPI_VER_CURRENT) {
          //   fprintf(stderr, "Error initializing PAPI! %s \n",
          //           PAPI_strerror(retval));
          // }
          // int eventset = PAPI_NULL;
          // // papi creating event set
          // retval = PAPI_create_eventset(&eventset);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error creating eventset! %s\n",
          //           PAPI_strerror(retval));
          // }

          // // papi adding event set
          // retval = PAPI_add_named_event(eventset,
          //                               "perf::PERF_COUNT_HW_CACHE_LL:MISS");
          // if (retval != PAPI_OK) {
          //   fprintf(stderr,
          //           "Error adding perf::PERF_COUNT_HW_CACHE_LL:MISS: %s\n",
          //           PAPI_strerror(retval));
          // }

          // // starting count
          // long long count;
          // PAPI_reset(eventset);
          // retval = PAPI_start(eventset);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error starting CUDA: %s\n",
          //   PAPI_strerror(retval));
          // }

          /* start measure */
          fprintf(stderr, "reached benchmark\n");
          const uint64_t start1 = cpu::get_ticks_acquire();
          sumsq(data4, array_length);
          const uint64_t end1 = cpu::get_ticks_release();
          fprintf(stderr, "after benchmark\n");
          /* end measure */

          // // ending count
          // retval = PAPI_stop(eventset, &count);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error stopping:  %s\n", PAPI_strerror(retval));
          // } else {
          //   printf("Measured %lld hw cache misses (thread 5)\n", count);
          // }
          // count4 = count;

          execTime4 = double(end1 - start1) / 1.7e+9;
        }
#else
        /* For single precision */
        {
          const uint64_t start1 = cpu::get_ticks_acquire();
          sumsqf((const float *)data4, array_length * 2);
          const uint64_t end1 = cpu::get_ticks_release();

          execTime4 = double(end1 - start1) / 1.7e+9;
        }
#endif

        // sleep (8);
      }
/* OpenMP thread for sixth core */
#pragma omp section
      {
        fprintf(stderr, "Starting sixth computation thread...\n");
#if (TYPE)
        /* For double	precision */
        {
          // int retval;
          // retval = PAPI_library_init(PAPI_VER_CURRENT);
          // if (retval != PAPI_VER_CURRENT) {
          //   fprintf(stderr, "Error initializing PAPI! %s \n",
          //           PAPI_strerror(retval));
          // }
          // int eventset = PAPI_NULL;
          // // papi creating event set
          // retval = PAPI_create_eventset(&eventset);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error creating eventset! %s\n",
          //           PAPI_strerror(retval));
          // }

          // // papi adding event set
          // retval = PAPI_add_named_event(eventset,
          //                               "perf::PERF_COUNT_HW_CACHE_LL:MISS");
          // if (retval != PAPI_OK) {
          //   fprintf(stderr,
          //           "Error adding perf::PERF_COUNT_HW_CACHE_LL:MISS: %s\n",
          //           PAPI_strerror(retval));
          // }

          // // starting count
          // long long count;
          // PAPI_reset(eventset);
          // retval = PAPI_start(eventset);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error starting CUDA: %s\n",
          //   PAPI_strerror(retval));
          // }

          /* start measure */
          fprintf(stderr, "reached benchmark\n");
          const uint64_t start1 = cpu::get_ticks_acquire();
          sumsq(data5, array_length);
          const uint64_t end1 = cpu::get_ticks_release();
          fprintf(stderr, "after benchmark\n");
          /* end measure */

          // ending count
          // retval = PAPI_stop(eventset, &count);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error stopping:  %s\n", PAPI_strerror(retval));
          // } else {
          //   printf("Measured %lld hw cache misses(thread 6)\n", count);
          // }
          // count5 = count;

          execTime5 = double(end1 - start1) / 1.7e+9;
        }
#else
        /* For single precision */
        {
          const uint64_t start1 = cpu::get_ticks_acquire();
          sumsqf((const float *)data5, array_length * 2);
          const uint64_t end1 = cpu::get_ticks_release();

          execTime5 = double(end1 - start1) / 1.7e+9;
        }
#endif
        // sleep (10);
      }
      // removed
      /* OpenMP thread for power measurement, if any */
      // #pragma omp section
      // {
      // 	fprintf (stderr, "Starting powermon thread...\n");
      // 	/* Power measurement code start here */

      // 	/* Power measurement code end here */
      // }
    }
  }
  long long energy_after = get_energy();
/* Number of flops and bytes executed */
#if (TYPE)
  double flops = 2 * array_length * MAD_PER_ELEMENT * 2.0;
  fprintf(stderr, "if type\n");
#else
  double flops = 2 * array_length * MAD_PER_ELEMENT * 4.0;
#endif
  double bytes = 2 * array_length * sizeof(double);
  int threads_measured = 2;
  double miss = 64 * (count0 + count1 + count2 + count3 + count4 + count5);
  double miss_per_thread =
      64 * (count0 + count1 + count2 + count3 + count4 + count5) /
      threads_measured;
  double total_miss = miss_per_thread * 6;
  double total_flops = flops * 6;
  long long int energy_consumed;
  if (energy_after > energy_before)
    energy_consumed = energy_after - energy_before;
  else
    energy_consumed = -1000000;

  /* take the maximum of the execution times of the two cores */
  double execTime;
  if (execTime0 > execTime1)
    execTime = execTime0;
  else
    execTime = execTime1;
  if (execTime > execTime2)
    execTime = execTime;
  else
    execTime = execTime2;
  if (execTime > execTime3)
    execTime = execTime;
  else
    execTime = execTime3;
  if (execTime > execTime4)
    execTime = execTime;
  else
    execTime = execTime4;
  if (execTime > execTime5)
    execTime = execTime;
  else
    execTime = execTime5;

  /* Print performance info */
  fprintf(stderr,
          "Execution time 0: %lf\tExecution time 1: %lf\tExecution time 2: "
          "%lf\tExecution time 3: %lf\tExecution time 4: %lf\tExecution time "
          "5: %lf\n",
          execTime0, execTime1, execTime2, execTime3, execTime4, execTime5);
  fprintf(stderr, "Execution time: %lf\n", execTime);
  fprintf(stderr, "GBytes: %5.03lf GFlops: %5.03lf\n", bytes / 1.0e+9,
          flops / 1.0e+9);
  fprintf(stderr, "Bandwidth: %lf GB/s\n", bytes / execTime / 1.0e+9);
  fprintf(stderr, "Performance: %lf GFLOPS\n", flops / execTime / 1.0e+9);
  fprintf(stderr, "HW miss in %d threads : %lf GB\n", threads_measured,
          miss / 1.0e+9);
  // fprintf(stderr, "Performance: %lf GFLOPS\n", (flops * 6) /
  // execTime / 1.0e+9);
  fprintf(stderr, "OI: %lf \n", (flops) / miss);
  fprintf(stderr, "Energy: %lld \n", energy_consumed);
  fprintf(stderr, "\n\n\n");

  /*printing to std out*/
  fprintf(stdout, "%lf\n", execTime);
  fprintf(stdout, "%5.03lf\n%5.03lf\n", bytes / 1.0e+9, flops / 1.0e+9);
  fprintf(stdout, "%lf\n", bytes / execTime / 1.0e+9);
  fprintf(stdout, "%lf\n", flops / execTime / 1.0e+9);
  fprintf(stdout, "%lld\n", energy_consumed);
  fprintf(stdout, "%lf\n", total_flops / total_miss);
  fprintf(stdout, "%lf\n", total_flops);
  fprintf(stdout, "%lf\n", total_miss);
  /* Free memory */
  free(data0);
  free(data1);
  free(data2);
  free(data3);
  free(data4);
  free(data5);
}
