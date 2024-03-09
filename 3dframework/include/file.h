#pragma once

#include <global_defs.h>

typedef struct {
    char *data;
    int size;
} File;

#ifdef __cplusplus
extern "C" {
#endif

bool fileExists(const char *filename);
File loadFile  (const char *filename) WARN_UNUSED_RESULT;
void writeFile (const char *filename, const char *str, ...);
void appendFile(const char *filename, const char *str, ...);
#define removeFile(filename) remove(filename)
void deleteFile(File *file);

#ifdef __cplusplus
}
#endif
