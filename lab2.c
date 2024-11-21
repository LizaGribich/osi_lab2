#include "lab2.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <windows.h>

CachePage cache[CACHE_SIZE];

void cache_init() {
  for (int i = 0; i < CACHE_SIZE; i++) {
    cache[i].fd = -1;
    cache[i].page_offset = 0;
    cache[i].data = (char *)_aligned_malloc(PAGE_SIZE, PAGE_SIZE);

    if (cache[i].data == NULL) {
      fprintf(stderr,
              "cache_init: Failed to allocate memory for cache page %d\n", i);
      for (int j = 0; j < i; j++) {
        _aligned_free(cache[j].data);
        cache[j].data = NULL;
      }
      exit(EXIT_FAILURE);
    }
    memset(cache[i].data, 0, PAGE_SIZE);
    cache[i].is_dirty = 0;
    cache[i].is_valid = 0;
  }
}

void cache_flush() {
  for (int i = 0; i < CACHE_SIZE; i++) {
    if (cache[i].is_dirty) {
      lseek(cache[i].fd, cache[i].page_offset, SEEK_SET);
      write(cache[i].fd, cache[i].data, PAGE_SIZE);
    }
    cache[i].fd = -1;
    cache[i].is_valid = 0;
    cache[i].is_dirty = 0;
  }
}

void cache_destroy() {
  for (int i = 0; i < CACHE_SIZE; i++) {
    if (cache[i].data) {
      _aligned_free(cache[i].data);
    }
  }
}

int get_evict_index() { return rand() % CACHE_SIZE; }

int lab2_open(const char *path) {
  HANDLE hFile =
      CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                 FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH, NULL);

  if (hFile == INVALID_HANDLE_VALUE) {
    perror("lab2_open: CreateFile failed");
    return -1;
  }
  return _open_osfhandle((intptr_t)hFile, 0);
}

int lab2_close(int fd) {
  cache_flush();
  return close(fd);
}

int lab2_fsync(int fd) {
  HANDLE hFile = (HANDLE)_get_osfhandle(fd);
  if (hFile == INVALID_HANDLE_VALUE) {
    perror("lab2_fsync: invalid file handle");
    return -1;
  }
  if (!FlushFileBuffers(hFile)) {
    perror("lab2_fsync: FlushFileBuffers failed");
    return -1;
  }
  return 0;
}
ssize_t lab2_read(int fd, void *buf, size_t count) {
  if ((uintptr_t)buf % PAGE_SIZE != 0) {
    fprintf(stderr, "lab2_read: Buffer must be aligned to PAGE_SIZE.\n");
    return -1;
  }

  off_t file_offset = lseek(fd, 0, SEEK_CUR);
  if (file_offset == -1) {
    perror("lab2_read: lseek failed to get current offset");
    return -1;
  }

  struct stat file_stat;
  if (fstat(fd, &file_stat) == -1) {
    perror("lab2_read: Failed to get file size");
    return -1;
  }

  if (file_offset >= file_stat.st_size) {
    return 0;
  }

  size_t max_readable = file_stat.st_size - file_offset;
  if (count > max_readable) {
    count = max_readable;
  }

  size_t total_read = 0;

  while (count > 0) {
    off_t page_offset = (file_offset / PAGE_SIZE) * PAGE_SIZE;
    size_t page_pos = file_offset % PAGE_SIZE;
    size_t to_read = PAGE_SIZE - page_pos;
    if (to_read > count)
      to_read = count;

    int found = 0;
    for (int i = 0; i < CACHE_SIZE; i++) {
      if (cache[i].fd == fd && cache[i].page_offset == page_offset &&
          cache[i].is_valid) {
        memcpy(buf, cache[i].data + page_pos, to_read);
        found = 1;
        break;
      }
    }

    if (!found) {
      int evict_index = get_evict_index();

      if (cache[evict_index].is_dirty) {
        if (lseek(cache[evict_index].fd, cache[evict_index].page_offset,
                  SEEK_SET) == -1) {
          perror("lab2_read: Failed to set file offset for eviction");
          return -1;
        }
        if (write(cache[evict_index].fd, cache[evict_index].data, PAGE_SIZE) !=
            PAGE_SIZE) {
          perror("lab2_read: Failed to write evicted page to disk");
          return -1;
        }
      }

      cache[evict_index].fd = fd;
      cache[evict_index].page_offset = page_offset;
      cache[evict_index].is_dirty = 0;
      cache[evict_index].is_valid = 1;

      if (lseek(fd, page_offset, SEEK_SET) == -1) {
        perror("lab2_read: lseek failed before disk read");
        return -1;
      }

      ssize_t disk_read = read(fd, cache[evict_index].data, PAGE_SIZE);
      if (disk_read == -1) {
        perror("lab2_read: Failed to read page from disk");
        return -1;
      }

      memcpy(buf, cache[evict_index].data + page_pos, to_read);
    }

    file_offset += to_read;
    buf = (char *)buf + to_read;
    count -= to_read;
    total_read += to_read;
  }

  if (lseek(fd, file_offset, SEEK_SET) == -1) {
    perror("lab2_read: lseek failed after read");
    return -1;
  }

  return total_read;
}

ssize_t lab2_write(int fd, const void *buf, size_t count) {
  if ((uintptr_t)buf % PAGE_SIZE != 0) {
    fprintf(stderr, "lab2_write: Buffer must be aligned to PAGE_SIZE.\n");
    return -1;
  }

  off_t file_offset = lseek(fd, 0, SEEK_CUR);
  if (file_offset == -1) {
    perror("lab2_write: lseek failed");
    return -1;
  }

  size_t total_written = 0;
  while (count > 0) {
    off_t page_offset = file_offset / PAGE_SIZE * PAGE_SIZE;
    size_t page_pos = file_offset % PAGE_SIZE;
    size_t to_write = PAGE_SIZE - page_pos;
    if (to_write > count)
      to_write = count;

    int found = 0;
    for (int i = 0; i < CACHE_SIZE; i++) {
      if (cache[i].fd == fd && cache[i].page_offset == page_offset &&
          cache[i].is_valid) {
        memcpy(cache[i].data + page_pos, buf, to_write);
        cache[i].is_dirty = 1;
        found = 1;
        break;
      }
    }

    if (!found) {
      int evict_index = get_evict_index();
      if (cache[evict_index].is_dirty) {
        lseek(cache[evict_index].fd, cache[evict_index].page_offset, SEEK_SET);
        write(cache[evict_index].fd, cache[evict_index].data, PAGE_SIZE);
      }
      cache[evict_index].fd = fd;
      cache[evict_index].page_offset = page_offset;
      cache[evict_index].is_dirty = 1;
      cache[evict_index].is_valid = 1;
      lseek(fd, page_offset, SEEK_SET);
      read(fd, cache[evict_index].data, PAGE_SIZE);
      memcpy(cache[evict_index].data + page_pos, buf, to_write);
    }

    file_offset += to_write;
    buf = (const char *)buf + to_write;
    count -= to_write;
    total_written += to_write;
  }

  lseek(fd, file_offset, SEEK_SET);
  return total_written;
}

off_t lab2_lseek(int fd, off_t offset, int whence) {
  HANDLE hFile = (HANDLE)_get_osfhandle(fd);
  if (hFile == INVALID_HANDLE_VALUE) {
    perror("lab2_lseek: invalid file handle");
    return -1;
  }

  LARGE_INTEGER liOffset;
  liOffset.QuadPart = offset;

  LARGE_INTEGER newPos;
  DWORD moveMethod;

  switch (whence) {
  case SEEK_SET:
    moveMethod = FILE_BEGIN;
    break;
  case SEEK_CUR:
    moveMethod = FILE_CURRENT;
    break;
  case SEEK_END:
    moveMethod = FILE_END;
    break;
  default:
    fprintf(stderr, "lab2_lseek: invalid whence value\n");
    return -1;
  }

  if (!SetFilePointerEx(hFile, liOffset, &newPos, moveMethod)) {
    perror("lab2_lseek: SetFilePointerEx failed");
    return -1;
  }

  return (off_t)newPos.QuadPart;
}