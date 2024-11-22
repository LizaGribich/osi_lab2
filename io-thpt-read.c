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

  HANDLE file;
  char buffer[BLOCK_SIZE];
  DWORD bytesRead;
  LARGE_INTEGER start, end, frequency;

  QueryPerformanceFrequency(&frequency);

  QueryPerformanceCounter(&start);

  size_t totalBytesRead = 0;

  for (int i = 0; i < ITERATION_COUNT; i++) {
    file = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                      OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
    if (file == INVALID_HANDLE_VALUE) {
      printf("Failed to open file: %s\n", filePath);
      return 1;
    }
    size_t bytesReadInBlock = 0;
    while (ReadFile(file, buffer, BLOCK_SIZE, &bytesRead, NULL) &&
           bytesRead > 0) {
      bytesReadInBlock += bytesRead;
    }
    CloseHandle(file);
    totalBytesRead += bytesReadInBlock;
  }

  QueryPerformanceCounter(&end);

  double total_time_taken =
      (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
  double total_data_MB = (double)(totalBytesRead) / (1024 * 1024);

  double throughput_MBps = total_data_MB / total_time_taken;

  printf("Total time taken for %d repetitions: %.4f seconds\n", ITERATION_COUNT,
         total_time_taken);
  printf("Total data read: %.2f MB\n", total_data_MB);
  printf("Throughput: %.2f MB/s\n", throughput_MBps);

  return 0;
}
