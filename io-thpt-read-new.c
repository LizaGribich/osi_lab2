#include "lab2.h"
#include <stdio.h>
#include <windows.h>

#define BLOCK_SIZE (8 * 8 * 1024)
#define ITERATION_COUNT 10

int main() {
  char *filePath = getenv("TESTFILE_PATH");
  if (filePath == NULL) {
    printf("TESTFILE_PATH is not set.\n");
    return 1;
  }

  printf("Path: %s\n", filePath);

  cache_init();

  void *buffer = _aligned_malloc(BLOCK_SIZE, PAGE_SIZE);
  if (!buffer) {
    fprintf(stderr, "Failed to allocate aligned buffer.\n");
    cache_destroy();
    return 1;
  }

  LARGE_INTEGER start, end, frequency;
  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&start);

  size_t totalBytesRead = 0;

  for (int i = 0; i < ITERATION_COUNT; i++) {
    int file = lab2_open(filePath);
    if (file < 0) {
      printf("Failed to open file: %s\n", filePath);
      _aligned_free(buffer);
      cache_destroy();
      return 1;
    }

    ssize_t bytesRead;
    while ((bytesRead = lab2_read(file, buffer, BLOCK_SIZE)) > 0) {
      totalBytesRead += bytesRead;
    }

    if (bytesRead < 0) {
      fprintf(stderr, "Error reading file during iteration %d.\n", i + 1);
      lab2_close(file);
      _aligned_free(buffer);
      cache_destroy();
      return 1;
    }

    if (lab2_close(file) < 0) {
      printf("Error closing file.\n");
      _aligned_free(buffer);
      cache_destroy();
      return 1;
    }
  }

  QueryPerformanceCounter(&end);

  double total_time_taken =
      (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
  double total_data_MB = (double)totalBytesRead / (1024 * 1024);
  double throughput_MBps = total_data_MB / total_time_taken;

  printf("Total time taken for %d repetitions: %.4f seconds\n", ITERATION_COUNT,
         total_time_taken);
  printf("Total data read: %.2f MB\n", total_data_MB);
  printf("Throughput: %.2f MB/s\n", throughput_MBps);

  _aligned_free(buffer);
  cache_destroy();

  return 0;
}
