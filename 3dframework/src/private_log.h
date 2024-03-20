#pragma once

#include <stdbool.h>

#define INHIBIT_LOG \
    struct _ResetLog_ {bool state = isLogActive(); ~_ResetLog_(){setLogActive(state);}} _resetLog_; \
    setLogActive(false);

#ifdef __cplusplus
extern "C" {
#endif

void logInfo       (const char *message, ...);
void logPerfWarning(const char *message, ...);
void logWarning    (const char *message, ...);
void logError      (const char *message, ...);

void setLogActive(const bool active);
bool isLogActive();

#ifdef __cplusplus
}
#endif
