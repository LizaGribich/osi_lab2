#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 8192
#define FILE_SIZE (1024 * 1024 * 100)
#define FILENAME "testfile.bin"

int main() {
  FILE *file = fopen(FILENAME, "wb");
  if (file == NULL) {
    perror("Failed to create file");
    return 1;
  }

  char *buffer = (char *)malloc(BUFFER_SIZE);
  if (buffer == NULL) {
    perror("Failed to allocate memory");
    fclose(file);
    return 1;
  }

  for (size_t i = 0; i < FILE_SIZE / BUFFER_SIZE; i++) {
    fwrite(buffer, BUFFER_SIZE, 1, file);
  }

  free(buffer);
  fclose(file);
  printf("Test file created: %s\n", FILENAME);
  return 0;
}
