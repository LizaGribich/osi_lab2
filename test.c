#include "lab2.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void generate_large_test_file(const char *filename) {
  int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
    perror("Failed to create test file");
    return;
  }

  const size_t file_size = 16384;
  char buffer[PAGE_SIZE];
  for (int i = 0; i < PAGE_SIZE; i++) {
    buffer[i] = 'A' + (i % 26);
  }

  for (size_t written = 0; written < file_size; written += PAGE_SIZE) {
    if (write(fd, buffer, PAGE_SIZE) != PAGE_SIZE) {
      perror("Failed to write to test file ");
      close(fd);
      return;
    }
  }

  close(fd);
  printf("File created !!!!\n");
}

int main() {
  cache_init();

  const char *filename = "test_file.txt";
  generate_large_test_file(filename);

  printf("=== Testing lab2_open ===\n");
  int fd = lab2_open(filename);
  if (fd < 0) {
    fprintf(stderr, "Failed to open file\n");
    return -1;
  }
  printf("File opened successfully\n\n");

  printf("=== Testing lab2_read ===\n");
  void *readBuffer = _aligned_malloc(PAGE_SIZE, PAGE_SIZE);
  if (!readBuffer) {
    fprintf(stderr, "Failed to allocate aligned buffer for reading\n");
    lab2_close(fd);
    return -1;
  }
  memset(readBuffer, 0, PAGE_SIZE);

  ssize_t bytesRead = lab2_read(fd, readBuffer, PAGE_SIZE);
  if (bytesRead == -1) {
    fprintf(stderr, "Failed to read from file\n");
  } else {
    printf("Bytes read: %zd\n", bytesRead);
    printf("Data: %.*s\n", (int)bytesRead, (char *)readBuffer);
  }
  printf("\n");

  printf("=== Testing lab2_lseek ===\n");

  off_t target_offset = 0;
  printf("Attempting to seek to offset %lld (SEEK_SET)\n",
         (long long)target_offset);
  off_t current_offset = lab2_lseek(fd, target_offset, SEEK_SET);
  if (current_offset == -1) {
    fprintf(stderr, "Failed to seek to the start of file\n");
  } else {
    printf("Seeked to offset %lld (SEEK_SET). Current offset: %lld\n",
           (long long)target_offset, (long long)current_offset);
  }

  target_offset = 4096;
  printf("Attempting to seek forward by %lld bytes (SEEK_CUR)\n",
         (long long)target_offset);
  current_offset = lab2_lseek(fd, target_offset, SEEK_CUR);
  if (current_offset == -1) {
    fprintf(stderr, "Failed to seek 4096 bytes forward\n");
  } else {
    printf("Seeked forward by %lld bytes (SEEK_CUR). Current offset: %lld\n",
           (long long)target_offset, (long long)current_offset);
  }

  target_offset = -4096;
  printf("Attempting to seek backward by %lld bytes from the end (SEEK_END)\n",
         (long long)target_offset);
  current_offset = lab2_lseek(fd, target_offset, SEEK_END);
  if (current_offset == -1) {
    fprintf(stderr, "Failed to seek -4096 bytes from the end of file\n");
  } else {
    printf("Seeked backward by %lld bytes from the end (SEEK_END). Current "
           "offset: %lld\n",
           (long long)target_offset, (long long)current_offset);
  }
  printf("\n");

  printf("=== Testing lab2_write ===\n");
  void *writeBuffer = _aligned_malloc(PAGE_SIZE, PAGE_SIZE);
  if (!writeBuffer) {
    fprintf(stderr, "Failed to allocate aligned buffer for writing\n");
    _aligned_free(readBuffer);
    lab2_close(fd);
    return -1;
  }
  memset(writeBuffer, 0, PAGE_SIZE);
  strcpy((char *)writeBuffer, "New data written to the file!");

  if (lab2_lseek(fd, 0, SEEK_SET) == -1) {
    fprintf(stderr, "Failed to seek to the start of file before writing.\n");
  }
  ssize_t bytesWritten =
      lab2_write(fd, writeBuffer, strlen("New data written to the file!"));
  if (bytesWritten == -1) {
    fprintf(stderr, "Failed to write to file\n");
  } else {
    printf("Bytes written: %zd\n", bytesWritten);
  }
  printf("\n");

  printf("=== Testing lab2_fsync ===\n");
  if (lab2_fsync(fd) == -1) {
    fprintf(stderr, "Failed to sync data with disk\n");
  } else {
    printf("Data successfully synchronized with disk\n");
  }
  printf("\n");

  printf("=== Verifying lab2_write and lab2_read ===\n");
  memset(readBuffer, 0, PAGE_SIZE);
  if (lab2_lseek(fd, 0, SEEK_SET) == -1) {
    fprintf(stderr, "Failed to seek to the start of file before reading\n");
  }
  bytesRead = lab2_read(fd, readBuffer, PAGE_SIZE);
  if (bytesRead == -1) {
    fprintf(stderr, "Failed to read from file after write\n");
  } else {
    printf("After write - Bytes read: %zd\n", bytesRead);
    printf("Data: %.*s\n", (int)bytesRead, (char *)readBuffer);
  }
  printf("\n");

  printf("=== Testing lab2_close ===\n");
  if (lab2_close(fd) == -1) {
    fprintf(stderr, "Failed to close the file\n");
  } else {
    printf("File closed successfully :3\n");
  }

  _aligned_free(readBuffer);
  _aligned_free(writeBuffer);
  cache_destroy();

  printf("\nAll tests ok!\n");
  return 0;
}