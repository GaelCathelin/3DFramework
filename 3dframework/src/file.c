#include <file.h>
#include <vector.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "private_log.h"

bool fileExists(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

File loadFile(const char *filename) {
    File f;
    FILE *file;
    int c, i;

    file = fopen(filename, "rb");
    if (!file) {
        logError("Can't read from file \"%s\"\n", filename);
        return (File){};
    }

    fseek(file, 0, SEEK_END);
    f.size = (ftell(file) + 3) / 4 * 4;
    rewind(file);

    f.data = aligned_malloc(f.size + 1);

    i = 0;
    while ((c = fgetc(file)) != EOF)
        f.data[i++] = c;
    f.data[i] = 0;

    fclose(file);
    return f;
}

void writeFile(const char *filename, const char *str, ...) {
    va_list args;
    va_start(args, str);
    FILE *file = fopen(filename, "w");

    if (!file) {
        logError("Can't write to file \"%s\"\n", filename);
        return;
    }

    vfprintf(file, str, args);
    fclose(file);
    va_end(args);
}

void appendFile(const char *filename, const char *str, ...) {
    va_list args;
    va_start(args, str);
    FILE *file = fopen(filename, "a+");

    if (!file) {
        logError("Can't write to file \"%s\"\n", filename);
        return;
    }

    vfprintf(file, str, args);
    fclose(file);
    va_end(args);
}

void deleteFile(File *file) {
    if (!file) return;
    aligned_free(file->data);
    memset(file, 0, sizeof(File));
}
