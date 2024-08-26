#include <stdio.h>
#include <stdlib.h>

long long get_energy() {
  FILE *fp;
  char path[1035];
  long long value;

  /* Open the command for reading. */
  fp =
      popen("echo nilesh | sudo -S rdmsr -u 1553 | xargs -0 -I{} echo {}", "r");
  if (fp == NULL) {
    printf("Failed to run command\n");
    exit(1);
  }

  /* Read the output a line at a time - it should be just one line. */
  if (fgets(path, sizeof(path) - 1, fp) != NULL) {
    printf("Output: %s", path);
    value = atoll(path); // Convert the output to an integer
  }

  /* Close the pipe and print the value. */
  pclose(fp);
  printf("Value: %lld\n", value);

  return value;
}