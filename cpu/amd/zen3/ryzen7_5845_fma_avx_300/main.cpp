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
// #include <linux/powercap.h>

#ifndef CORE
#define CORE
#endif 

#ifdef UNCORE
#undef CORE
#endif

#ifdef CORE
#define ENERGY_UJ_PATH "/sys/class/powercap/intel-rapl:0:0/energy_uj"
#define get_energy_before \
        energy_before_core = get_energy_core(); 
#define get_energy_after \
        energy_after_core = get_energy_core(); 
#endif

#ifdef UNCORE
#define ENERGY_UJ_PATH "/sys/class/powercap/intel-rapl:0:1/energy_uj"
#define get_energy_before \
        energy_before_core = get_energy_core(); \
        energy_before_zone = get_energy_zone(); 
#define get_energy_after \
        energy_after_core = get_energy_core(); \
        energy_after_zone = get_energy_zone(); 
#endif

// #define ITRS 100000
// #define ITRS 10000

#ifndef MAD_PER_ELEMENT
#define MAD_PER_ELEMENT 100
#endif

template <typename T> T getmax(T arr[], int size) {
  T max_element = arr[0];
  T element;
  for (int i = 0; i < size; i++) {
    element = arr[i];
    max_element = std::max(element, max_element);
  }
  return max_element;
}

template <typename T> double get_mean(T arr[], int size) {
  T sum = 0;
  for (T element : arr) {
    sum += element;
  }
  return sum / size;
}

template <typename T> T get_sum(T arr[], int size) {
  T sum = 0;
  T element;
  for (int i = 0; i < size; i++) {
    element = arr[i];
    sum += element;
  }
  return sum;
}

unsigned long get_thread_id() {
  return static_cast<unsigned long>(omp_get_thread_num());
}

/* ======================================================== */
template <typename T> T getMedian(T arr[], int size) {
  std::sort(arr, arr + size);

  if (size % 2 != 0) {
    return arr[size / 2];
  } else {
    return (arr[(size / 2) - 1] + arr[size / 2]) / 2.0;
  }
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

unsigned long long get_energy_core() {
    FILE *fp;
    unsigned long long energy_uj;

    // Open the sysfs entry for energy in microjoules
    fp = fopen("/sys/class/powercap/intel-rapl:0:0/energy_uj", "r");
    if (fp == NULL) {
        perror("Failed to open energy_uj file");
        return EXIT_FAILURE;
    }

    // Read the energy value
    if (fscanf(fp, "%llu", &energy_uj) != 1) {
        perror("Failed to read energy");
        fclose(fp);
        return EXIT_FAILURE;
    }

    printf("Energy core : %llu uJ\n", energy_uj);

    fclose(fp);
    return energy_uj;
}

unsigned long long get_energy_zone() {
    FILE *fp;
    unsigned long long energy_uj;

    // Open the sysfs entry for energy in microjoules
    fp = fopen("/sys/class/powercap/intel-rapl:0/energy_uj", "r");
    if (fp == NULL) {
        perror("Failed to open energy_uj file");
        return EXIT_FAILURE;
    }

    // Read the energy value
    if (fscanf(fp, "%llu", &energy_uj) != 1) {
        perror("Failed to read energy");
        fclose(fp);
        return EXIT_FAILURE;
    }

    printf("Energy zone : %llu uJ\n", energy_uj);

    fclose(fp);
    return energy_uj;
}

/* Single and double precision sum squared functions */
extern "C" void sumsq(const double *data, size_t length);
extern "C" void sumsqf(const float *data, size_t length);

int main(int argc, char **argv) {
  omp_set_num_threads(8);

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
  double *data6 = (double *)memalign(64, array_length * sizeof(double));
  double *data7 = (double *)memalign(64, array_length * sizeof(double));
  memset(data0, 0, array_length * sizeof(double));
  memset(data1, 0, array_length * sizeof(double));
  memset(data2, 0, array_length * sizeof(double));
  memset(data3, 0, array_length * sizeof(double));
  memset(data4, 0, array_length * sizeof(double));
  memset(data5, 0, array_length * sizeof(double));
  memset(data6, 0, array_length * sizeof(double));
  memset(data7, 0, array_length * sizeof(double));

  double *data[8] = {data0, data1, data2, data3, data4, data5, data6, data7};
  long long counts[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  long long energies[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  /* Timers */
  double execTimes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  bool overflow[8] = {false,false,false,false,false,false,false,false};

  int retval_init = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval_init != PAPI_VER_CURRENT) {
    printf("PAPI library init error !\n");
    exit(1);
  }
  if (PAPI_thread_init(get_thread_id) != PAPI_OK) {
    printf("PAPI library init error !\n");
    exit(1);
  }
  int eventset = PAPI_NULL;
  int retval;
  int id = PAPI_thread_id();
  long long itrs = 0;
  long long start = 0;
  long long end = 0;
  long long count;
  long long unsigned energy_after_core = 0;
  long long unsigned energy_before_core = 0;
  long long unsigned energy_after_zone = 0;
  long long unsigned energy_before_zone = 0;


#pragma omp parallel private(eventset, count, itrs, retval, id, start, end,    \
                                 energy_after_zone, energy_before_zone, \
                                 energy_after_core, energy_before_core \
                                 )
  {
    eventset = PAPI_NULL;

    id = PAPI_thread_id();
    // papi creating event set
    retval = PAPI_create_eventset(&eventset);
    if (retval != PAPI_OK) {
      fprintf(stderr, "Error creating eventset! %s\n", PAPI_strerror(retval));
    }

    // papi adding event set
    retval = PAPI_add_named_event(eventset, papi_event_name);
    if (retval != PAPI_OK) {
      fprintf(stderr, "Error adding %s: %s\n", papi_event_name,
              PAPI_strerror(retval));
    }

    // starting count
    PAPI_reset(eventset);
    retval = PAPI_start(eventset);
    if (retval != PAPI_OK) {
      fprintf(stderr, "Error PAPI: %s\n", PAPI_strerror(retval));
    }

    /* start measure */
    fprintf(stderr, "Double precision...\n");
    uint64_t check0;
    fprintf(stderr, "reached benchmark\n");
#pragma omp barrier
    get_energy_before;
    start = cpu::get_ticks_acquire();
    while (itrs < ITRS) {
      sumsq(data[id], array_length);
      itrs++;
    }

    get_energy_after;
    end = cpu::get_ticks_release();
#pragma omp barrier
    fprintf(stderr, "after benchmark\n");
    /* end measure */

    // ending count
    if (energy_after_core < energy_before_core) {
      energies[id] = 0;
    }
    else{
      energies[id] = energy_after_core - energy_before_core;
    }
#ifdef UNCORE
    if(energy_after_zone < energy_before_zone){
      energies[id] = 0;
    }
    else if(energy_after_zone - energy_before_zone > energies[id]) {
      energies[id] = energy_after_zone - energy_before_zone - energies[id];
    }
    else{
      energies[id] = 0;
    }
#endif

    retval = PAPI_stop(eventset, &count);
    if (retval != PAPI_OK) {
      fprintf(stderr, "Error stopping:  %s\n", PAPI_strerror(retval));
    } else {
      fprintf(stderr, "Measured %lld hw cache misses (thread %d)\n", count, id);
    }
    counts[id] = count;

    if (energies[id] == 0) {
      overflow[id] = true;
    }
    execTimes[id] = double(end - start) / FREQ;
    fprintf(stderr, "Measured %llu energy (thread %d)\n", energies[id], id);
    fprintf(stderr, "Measured %lf time (thread %d)\n", execTimes[id], id);
  }

#if (TYPE)
  double flops = 2.0 * array_length * MAD_PER_ELEMENT * ITRS * 4.0 *
                 (1.0 / 64.0); // MAD_PER_ELEMENT = flops per inner loop itr
#else
  double flops = 2 * array_length * MAD_PER_ELEMENT * 4.0 * (1.0 / 8.0);
  // double flops = array_length * MAD_PER_ELEMENT / 64 ;
#endif
  double bytes = 2 * array_length * sizeof(double);
  double threads_measured = 2.0;
  double total_miss = get_sum(counts, 8) * 64.0; // 64 is the line size
  double total_flops = flops * 8;

  long long unsigned energy_consumed = getMedian(energies, 8);
  for(auto of : overflow){
    if(of){
      fprintf(stderr, "some overflow");
      energy_consumed = 0;
    }
  }

  /* take the maximum of the execution times of the two cores */
  // double execTime = getmax(execTimes, 6);
  double execTime = getMedian(execTimes, 8);
  /* Print performance info */
  fprintf(stderr, "Execution time (max): %lf\n", execTime);
  fprintf(stderr, "Bandwidth: %lf GB/s\n", total_miss / execTime / 1.0e+9);
  // fprintf(stderr, "Gig count per sec: %lf GB/s\n", get_sum(counts, 6) * 4.0 /
  // execTime / 1.0e+9);
  fprintf(stderr, "Performance: %lf GFLOPS\n", total_flops / execTime / 1.0e+9);
  fprintf(stderr, "Energy: %lld \n", energy_consumed);
  fprintf(stderr, "\n\n\n");

  /*printing to std out*/
  // fprintf(stdout, "%lld\n", count2);
  fprintf(stdout, "%lf\n", execTime);
  fprintf(stdout, "%5.03lf\n%5.03lf\n", bytes / 1.0e+9, flops / 1.0e+9);
  fprintf(stdout, "%lf\n", total_miss / execTime / 1.0e+9);
  fprintf(stdout, "%lf\n", total_flops / execTime / 1.0e+9);
  fprintf(stdout, "%llu\n", energy_consumed);
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
  free(data6);
  free(data7);
  return 0;
}
