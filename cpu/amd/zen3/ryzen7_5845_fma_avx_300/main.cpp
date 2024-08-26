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

#include <algorithm>
#include <malloc.h>
#include <omp.h>
#include <papi.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <unordered_map>

// #define ITRS 100000
// #define ITRS 10000

/* ======================================================== */

double get_median(long long itr0, long long itr1, long long itr2,
                  long long itr3, long long itr4, long long itr5) {
  // Store all variables in an array
  long long arr[6] = {itr0, itr1, itr2, itr3, itr4, itr5};

  std::sort(arr, arr + 6);

  return (arr[2] + arr[3]) / 2;
}

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
  fp = popen("rdmsr -u 1553 | xargs -0 -I{} echo {}", "r");
  if (fp == NULL) {
    fprintf(stderr, "Failed to run command\n");
    exit(1);
  }

  /* Read the output a line at a time - it should be just one line. */
  if (fgets(path, sizeof(path) - 1, fp) != NULL) {
    // fprintf(stderr, "Output: %s", path);
    value = atoll(path); // Convert the output to an integer
  }

  /* Close the pipe and print the value. */
  pclose(fp);
  // fprintf(stderr, "Value: %lld\n", value);

  return value;
}

/* Single and double precision sum squared functions */
extern "C" void sumsq(const double *data, size_t length);
extern "C" void sumsqf(const float *data, size_t length);

int main(int argc, char **argv) {

  const char *duration = "ITRS";
  const long long ITRS = atoll(getenv(duration));

  const char *freq = "FREQ";
  const long long FREQ = atoll(getenv(freq));
  // long long unsigned CLCK = int(time_per_benchmark * FREQ);
  // CLCK = 20;
  // fprintf(stderr, "CLCK %llu \n", CLCK);

  const char *env_var_name = "PAPI_EVENT_NAME";
  const char *papi_event_name = getenv(env_var_name);

  const char *env_var_cache_size = "SIZE_ARR";
  long long unsigned array_size = atoll(getenv(env_var_cache_size));
  /* Number of elements in the array */
  // const size_t array_length = 64 * 4;
  const size_t array_length = array_size;

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

  // printf("data,address\n");
  // for(int i = 0; i < array_size; i++){
  //   printf("%lf,%d\n",data0[i],&data1[i]);
  //   // printf("data %lf address : %d \n",data0[i],&data1[i]);
  // }
  // return 0;

  /*oi counter*/
  long long count0, count1, count2, count3, count4, count5;
  long long e0, e1, e2, e3, e4, e5;
  e0 = 0;
  e1 = 0;
  e2 = 0;
  e3 = 0;
  e4 = 0;
  e5 = 0;

  count0 = 0;
  count1 = 0;
  count2 = 0;
  count3 = 0;
  count4 = 0;
  count5 = 0;

  long long itr0 = 0;
  long long itr1 = 0;
  long long itr2 = 0;
  long long itr3 = 0;
  long long itr4 = 0;
  long long itr5 = 0;

  uint64_t temp;

  /* Timers */
  double execTime0, execTime1, execTime2, execTime3, execTime4, execTime5;

  /* Since the Intel(R) Xeon(R) CPU E5-1650 v4 @ 3.60GHz has 6 cores, we use
     OpenMP to run computation on all cores. If the target processor has more
     cores, increase the number of data structures, timers, OpenMP threads, etc.
     accordingly.
   */

  // long long energy_before = get_energy();
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
          retval = PAPI_add_named_event(eventset, papi_event_name);
          if (retval != PAPI_OK) {
            fprintf(stderr, "Error adding %s: %s\n", papi_event_name,
                    PAPI_strerror(retval));
          }

          // starting count
          long long count;
          PAPI_reset(eventset);
          retval = PAPI_start(eventset);
          if (retval != PAPI_OK) {
            fprintf(stderr, "Error PAPI: %s\n", PAPI_strerror(retval));
          }

          /* start measure */
          fprintf(stderr, "Double precision...\n");
          uint64_t check0;
          fprintf(stderr, "reached benchmark\n");
          long long energy_before0 = get_energy();
          const uint64_t start0 = cpu::get_ticks_acquire();
          while (itr0 < ITRS) {
            sumsq(data0, array_length);
            itr0++;
          }
          const uint64_t end0 = cpu::get_ticks_release();
          long long energy_after0 = get_energy();
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
          temp = end0 - start0;
          e0 = energy_after0 - energy_before0;
          execTime0 = double(end0 - start0) / FREQ;
        }
#else
        /* For single precision */
        {
          const uint64_t start0 = cpu::get_ticks_acquire();
          sumsqf((const float *)data0, array_length * 2);
          const uint64_t end0 = cpu::get_ticks_release();

          execTime0 = double(end0 - start0) / FREQ;
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
          retval = PAPI_add_named_event(eventset, papi_event_name);
          if (retval != PAPI_OK) {
            fprintf(stderr, "Error adding %s: %s\n", papi_event_name,
                    PAPI_strerror(retval));
          }

          // starting count
          long long count;
          PAPI_reset(eventset);
          retval = PAPI_start(eventset);
          if (retval != PAPI_OK) {
            fprintf(stderr, "Error PAPI: %s\n", PAPI_strerror(retval));
          }

          /*start measure */
          fprintf(stderr, "reached benchmark\n");
          uint64_t check1;
          long long energy_before1 = get_energy();
          const uint64_t start1 = cpu::get_ticks_acquire();
          while (itr1 < ITRS) {
            sumsq(data1, array_length);
            itr1++;
          }
          const uint64_t end1 = cpu::get_ticks_release();
          long long energy_after1 = get_energy();
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
          e1 = energy_after1 - energy_before1;
          execTime1 = double(end1 - start1) / FREQ;
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
          // "perf::PERF_COUNT_HW_CPU_CYCLES"); if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error adding perf::PERF_COUNT_HW_CPU_CYCLES:
          //   %s\n",
          //           PAPI_strerror(retval));
          // }

          // // starting count
          // long long count;
          // PAPI_reset(eventset);
          // retval = PAPI_start(eventset);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error PAPI: %s\n", PAPI_strerror(retval));
          // }

          /*start measure */
          uint64_t check2;
          fprintf(stderr, "reached benchmark\n");
          long long energy_before2 = get_energy();
          const uint64_t start2 = cpu::get_ticks_acquire();
          while (itr2 < ITRS) {
            sumsq(data2, array_length);
            itr2++;
          }
          const uint64_t end2 = cpu::get_ticks_release();
          long long energy_after2 = get_energy();
          fprintf(stderr, "after benchmark\n");
          // /* end measure */

          // ending count
          // retval = PAPI_stop(eventset, &count);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error stopping:  %s\n", PAPI_strerror(retval));
          // } else {
          //   printf("Measured %lld perf::PERF_COUNT_HW_CPU_CYCLES \n", count);
          // }
          // count2 = count;
          e2 = energy_after2 - energy_before2;
          execTime2 = double(end2 - start2) / 1.7e+9;
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
          // retval = PAPI_add_named_event(eventset, papi_event_name);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error adding %s: %s\n", papi_event_name,
          //           PAPI_strerror(retval));
          // }

          // // starting count
          // long long count;
          // PAPI_reset(eventset);
          // retval = PAPI_start(eventset);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error PAPI: %s\n", PAPI_strerror(retval));
          // }

          /* start measure */
          fprintf(stderr, "reached benchmark\n");
          uint64_t check3;
          long long energy_before3 = get_energy();
          const uint64_t start3 = cpu::get_ticks_acquire();
          while (itr3 < ITRS) {
            sumsq(data3, array_length);
            itr3++;
          }
          const uint64_t end3 = cpu::get_ticks_release();
          long long energy_after3 = get_energy();
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
          e3 = energy_after3 - energy_before3;
          execTime3 = double(end3 - start3) / FREQ;
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
          // retval = PAPI_add_named_event(eventset, papi_event_name);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error adding %s: %s\n", papi_event_name,
          //           PAPI_strerror(retval));
          // }

          // // starting count
          // long long count;
          // PAPI_reset(eventset);
          // retval = PAPI_start(eventset);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error PAPI: %s\n", PAPI_strerror(retval));
          // }

          /* start measure */
          fprintf(stderr, "reached benchmark\n");
          uint64_t check4;
          long long energy_before4 = get_energy();
          const uint64_t start4 = cpu::get_ticks_acquire();
          while (itr4 < ITRS) {
            sumsq(data4, array_length);
            itr4++;
          }
          const uint64_t end4 = cpu::get_ticks_release();
          long long energy_after4 = get_energy();
          /* end measure */

          // // ending count
          // retval = PAPI_stop(eventset, &count);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error stopping:  %s\n", PAPI_strerror(retval));
          // } else {
          //   printf("Measured %lld hw cache misses (thread 5)\n", count);
          // }
          // count4 = count;
          // e4 = energy_after - energy_before;
          execTime4 = double(end4 - start4) / FREQ;
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
          // retval = PAPI_add_named_event(eventset, papi_event_name);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error adding %s: %s\n", papi_event_name,
          //           PAPI_strerror(retval));
          // }

          // // starting count
          // long long count;
          // PAPI_reset(eventset);
          // retval = PAPI_start(eventset);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error PAPI: %s\n", PAPI_strerror(retval));
          // }

          /* start measure */
          fprintf(stderr, "reached benchmark\n");
          uint64_t check5;
          long long energy_before5 = get_energy();
          const uint64_t start5 = cpu::get_ticks_acquire();
          while (itr5 < ITRS) {
            sumsq(data5, array_length);
            itr5++;
          }
          const uint64_t end5 = cpu::get_ticks_release();
          long long energy_after5 = get_energy();
          /* end measure */

          // ending count
          // retval = PAPI_stop(eventset, &count);
          // if (retval != PAPI_OK) {
          //   fprintf(stderr, "Error stopping:  %s\n", PAPI_strerror(retval));
          // } else {
          //   printf("Measured %lld hw cache misses(thread 6)\n", count);
          // }
          // count5 = count;
          // e5 = energy_after - energy_after;
          execTime5 = double(end5 - start5) / FREQ;
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
  // long long energy_after = get_energy();
  /* Number of flops and bytes executed */

#if (TYPE)
  double flops = 2 * array_length * MAD_PER_ELEMENT * ITRS *
                 (1.0 / 64.0); // MAD_PER_ELEMENT = flops per inner loop itr
#else
  double flops = 2 * array_length * MAD_PER_ELEMENT * 4.0 * (1.0 / 8.0);
  // double flops = array_length * MAD_PER_ELEMENT / 64 ;
#endif
  double bytes = 2 * array_length * sizeof(double);
  double threads_measured = 2.0;
  double count_per_thread =
      (count0 + count1 + count2 + count3 + count4 + count5) / threads_measured;
  double miss_per_thread =
      64.0 * (count0 + count1 + count2 + count3 + count4 + count5) /
      threads_measured;
  double total_miss = miss_per_thread * 6;
  double total_flops = flops * 6;

  long long int energy_consumed;
  if (e0 > e1)
    energy_consumed = e0;
  else
    energy_consumed = e1;
  if (energy_consumed > e2)
    energy_consumed = energy_consumed;
  else
    energy_consumed = e2;
  if (energy_consumed > e3)
    energy_consumed = energy_consumed;
  else
    energy_consumed = e3;
  if (energy_consumed > e4)
    energy_consumed = energy_consumed;
  else
    energy_consumed = e4;
  if (energy_consumed > e5)
    energy_consumed = energy_consumed;
  else
    energy_consumed = e5;

  if (energy_consumed < 0)
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
  fprintf(stderr, "Execution cycle: %ld\n", temp);
  fprintf(stderr, "GBytes: %5.03lf GFlops: %5.03lf\n", bytes / 1.0e+9,
          flops / 1.0e+9);
  fprintf(stderr, "Bandwidth: %lf GB/s\n", bytes / execTime / 1.0e+9);
  fprintf(stderr, "Performance: %lf GFLOPS\n", flops / execTime / 1.0e+9);
  fprintf(stderr, "Average count of %s event in %lf threads : %lf \n",
          papi_event_name, threads_measured, count_per_thread);
  fprintf(stderr, "Average count of %s event in 6 threads : %lf \n",
          papi_event_name, count_per_thread * 6);
  fprintf(stderr, "Energy: %lld \n", energy_consumed);
  fprintf(stderr, "\n\n\n");

  /*printing to std out*/
  // fprintf(stdout, "%lld\n", count2);
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
