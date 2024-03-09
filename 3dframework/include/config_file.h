#pragma once

#include <stdbool.h>

#define getItem(name, defaultVal) _Generic(defaultVal, \
    bool        : getItemAsBool  , \
    int         : getItemAsInt   , \
    double      : getItemAsDouble, \
    char*       : getItemAsString, \
    const char* : getItemAsString)(name, defaultVal)

#define getClampedItem(name, defaultVal, minVal, maxVal) ({ \
    __typeof__(defaultVal) val = getItem(name, defaultVal); \
    val = MAX(minVal, val); \
    MIN(maxVal, val); })

#ifdef __cplusplus
extern "C" {
#endif

void loadConfiguration();

const char* getItemAsString(const char *itemName, const char  *defaultValue);
int         getItemAsInt   (const char *itemName, const int    defaultValue);
double      getItemAsDouble(const char *itemName, const double defaultValue);
bool        getItemAsBool  (const char *itemName, const bool   defaultValue);

#ifdef __cplusplus
}
#endif
