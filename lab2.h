#ifndef LAB2_CACHE_H
#define LAB2_CACHE_H

#include <stddef.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifdef _WIN32
#ifdef LAB2_CACHE_EXPORTS
#define LAB2_API __declspec(dllexport)
#else
#define LAB2_API __declspec(dllimport)
#endif
#else
#define LAB2_API
#endif

#define CACHE_SIZE 10
#define PAGE_SIZE 4096

typedef struct {
    int fd;
    off_t page_offset;
    char *data;
    int is_dirty;
    int is_valid;
} CachePage;

LAB2_API void cache_init();

LAB2_API void cache_flush();

LAB2_API void cache_destroy();

LAB2_API int lab2_open(const char *path);
LAB2_API int lab2_close(int fd);
LAB2_API int lab2_fsync(int fd);
LAB2_API ssize_t lab2_read(int fd, void *buf, size_t count);
LAB2_API ssize_t lab2_write(int fd, const void *buf, size_t count);
LAB2_API off_t lab2_lseek(int fd, off_t offset, int whence);

#endif