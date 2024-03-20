#include "private_log.h"
#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

#ifdef NDEBUG
static bool logActive = false;
#else
static bool logActive = true;
#endif

#define printLog(color) { \
    if (!logActive) return; \
    va_list args; \
    va_start(args, message); \
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color); \
    vprintf(message, args); \
    putchar('\n'); \
    va_end(args); \
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); \
}

void logInfo       (const char *message, ...) printLog(FOREGROUND_INTENSITY)
void logPerfWarning(const char *message, ...) printLog(FOREGROUND_GREEN | FOREGROUND_BLUE)
void logWarning    (const char *message, ...) printLog(FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN)
void logError      (const char *message, ...) printLog(FOREGROUND_INTENSITY | FOREGROUND_RED)

void setLogActive(const bool active) {logActive = active;}
bool isLogActive() {return logActive;}
